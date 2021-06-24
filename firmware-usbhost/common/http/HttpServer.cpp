#include "include/HttpServer.h"
#include "HttpRequestParser.h"

#include "tcpip/include/TcpIp.h"
#include "utils/include/Buffer.h"
#include "timer/include/TimerEngine.h"
#include "logger/include/Logger.h"

namespace Http {

#define TCP_RECV_TIMEOUT 40000
#define HTTP_HEADER_MAX_SIZE 512
#define HTTP_RESPONSE_MAX_SIZE 128
#define HTTP_BODY_SIZE 256

Server::Server(TimerEngine *timers, TcpIp *conn) :
//	timers(timers),
	conn(conn),
	state(State_Idle)
{
	this->parser = new RequestParser;
	this->header = new StringBuilder(HTTP_HEADER_MAX_SIZE, HTTP_HEADER_MAX_SIZE);
	this->conn->setObserver(this);
}

Server::~Server() {
//	timers->deleteTimer(timer);
	delete header;
	delete parser;
}

void Server::setObserver(EventObserver *observer) {
	courier.setRecipient(observer);
}

bool Server::accept(Request2 *req) {
	LOG_DEBUG(LOG_HTTP, "accept");
	parser->start(req);
	gotoStateWaitRequest();
	return true;
}

bool Server::sendResponse(const Response *resp) {
	if(state != State_WaitRequest) {
		return false;
	}

	this->resp = resp;
	return gotoStateSendHeader();
}

bool Server::close() {
	gotoStateClose();
	return true;
}

void Server::proc(Event *event) {
	LOG_DEBUG(LOG_HTTP, "Event: state=" << state << ", event=" << event->getType());
	switch(state) {
		case State_WaitRequest: stateWaitRequestEvent(event); break;
		case State_RecvRequest: stateRecvRequestEvent(event); break;
		case State_SendHeader: stateSendHeaderEvent(event); break;
		case State_SendData: stateSendDataEvent(event); break;
		case State_Close: stateCloseEvent(event); break;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void Server::procTimer() {
/*	LOG_DEBUG(LOG_HTTP, "Timeout: state=" << state);
	switch(state) {
		case State_WaitResponse: gotoStateError(); return;
		default:;
	}*/
}

void Server::gotoStateWaitRequest() {
	LOG_DEBUG(LOG_HTTP, "gotoStateWaitRequest");
	state = State_WaitRequest;
}

void Server::stateWaitRequestEvent(Event *event) {
	LOG_DEBUG(LOG_HTTP, "stateWaitRequestEvent");
	switch(event->getType()) {
		case TcpIp::Event_IncomingData: gotoStateRecvRequest(); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: procUnwaitedClose(); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void Server::gotoStateRecvRequest() {
	LOG_DEBUG(LOG_HTTP, "gotoStateRecvRequest " << parser->getBufSize());
	if(conn->recv(parser->getBuf(), parser->getBufSize()) == false) {
		LOG_ERROR(LOG_HTTP, "Fatal error: SIM800 not ready.");
		gotoStateError();
		return;
	}
	state = State_RecvRequest;
}

void Server::stateRecvRequestEvent(Event *event) {
	switch(event->getType()) {
		case TcpIp::Event_RecvDataOk: stateRecvRequestEventRecvDataOK(event); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: procUnwaitedClose(); return;
		default: {
			LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
			return;
		}
	}
}

void Server::stateRecvRequestEventRecvDataOK(Event *event) {
	LOG_DEBUG(LOG_HTTP, "stateRecvRequestEventRecvDataOK");
	parser->parseData(event->getUint16());
	if(parser->isComplete() == true) {
		gotoStateWaitRequest();
		Event event(Event_RequestIncoming);
		courier.deliver(&event);
		return;
	} else {
		if(conn->hasRecvData() == true) {
			gotoStateRecvRequest();
			return;
		} else {
			gotoStateWaitRequest();
			return;
		}
	}
}

bool Server::gotoStateSendHeader() {
	makeHeader();
	if(conn->send((const uint8_t*)header->getString(), header->getLen()) == false) {
		LOG_ERROR(LOG_HTTP, "Fatal error: SIM800 not ready.");
		gotoStateError();
		return false;
	}
	state = State_SendHeader;
	return true;
}

void Server::makeHeader() {
	header->clear();
	*header << "HTTP/1.1 " << resp->statusCode << " " << statusCodeToString(resp->statusCode) << "\r\n";
	*header << "Content-Type: " << contentTypeToString(resp->contentType) << "; charset=windows-1251\r\n";
	uint16_t contentLength = (resp->data == NULL) ? 0 : resp->data->getLen();
	*header << "Content-Length: " << contentLength << "\r\n";
	*header << "Cache-Control: no-cache\r\n";
	*header << "\r\n";
	LOG_DEBUG(LOG_HTTP, "Header " << header->getLen());
	LOG_DEBUG_STR(LOG_HTTP, (const uint8_t*)header->getString(), header->getLen());
}

void Server::stateSendHeaderEvent(Event *event) {
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: stateSendHeaderEventSendDataOK(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: procUnwaitedClose(); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void Server::stateSendHeaderEventSendDataOK() {
	if(resp->data != NULL && resp->data->getLen() > 0) {
		gotoStateSendData();
		return;
	} else {
		gotoStateClose();
		return;
	}
}

void Server::gotoStateSendData() {
	if(resp->data == NULL) {
		gotoStateClose();
		return;
	}
	LOG_TRACE(LOG_HTTP, "Data " << resp->data->getLen());
	LOG_TRACE_STR(LOG_HTTP, (const uint8_t*)resp->data->getString(), resp->data->getLen());
	if(conn->send((const uint8_t*)resp->data->getString(), resp->data->getLen()) == false) {
		LOG_ERROR(LOG_HTTP, "Fatal error: SIM800 not ready.");
		gotoStateError();
		return;
	}
	state = State_SendData;
}

void Server::stateSendDataEvent(Event *event) {
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: stateSendDataEventSendDataOK(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: procUnwaitedClose(); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void Server::stateSendDataEventSendDataOK() {
	LOG_DEBUG(LOG_HTTP, "stateSendDataEventSendDataOK");
	gotoStateClose();
}

void Server::gotoStateClose() {
	LOG_DEBUG(LOG_HTTP, "gotoStateClose");
//	timer->stop();
	conn->close();
	state = State_Close;
}

void Server::stateCloseEvent(Event *event) {
	switch(event->getType()) {
		case TcpIp::Event_Close: stateCloseEventClose(); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void Server::stateCloseEventClose() {
	LOG_INFO(LOG_HTTP, "Connection closed");
	state = State_Idle;
	Event event(Event_ResponseComplete);
	courier.deliver(&event);
}

void Server::gotoStateError() {
	conn->close();
	state = State_Error;
}

void Server::stateErrorEvent(Event *event) {
	switch(event->getType()) {
		case TcpIp::Event_Close: stateErrorEventClose(); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void Server::stateErrorEventClose() {
	LOG_INFO(LOG_HTTP, "Connection closed");
	state = State_Idle;
	Event event(Event_ResponseError);
	courier.deliver(&event);
}

void Server::procUnwaitedClose() {
	LOG_ERROR(LOG_HTTP, "Unwaited connection close: state=" << state);
	state = State_Idle;
	Event event(Event_ResponseError);
	courier.deliver(&event);
}

}
