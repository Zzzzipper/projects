#include "include/HttpClient.h"
#include "HttpResponseParser.h"

#include "tcpip/include/TcpIp.h"
#include "utils/include/Buffer.h"
#include "timer/include/TimerEngine.h"
#include "logger/include/Logger.h"

namespace Http {

#define TCP_RECV_TIMEOUT 40000
#define HTTP_HEADER_MAX_SIZE 512

Client::Client(TimerEngine *timers, TcpIp *tcpIp) :
	timers(timers),
	tcpIp(tcpIp),
	state(State_Idle)
{
	this->header = new StringBuilder(HTTP_HEADER_MAX_SIZE, HTTP_HEADER_MAX_SIZE);
	this->respParser = new ResponseParser();
	this->timer = timers->addTimer<Client, &Client::procTimer>(this);
	this->tcpIp->setObserver(this);
}

Client::~Client() {
	timers->deleteTimer(timer);
	delete respParser;
	delete header;
}

void Client::setObserver(EventObserver *observer) {
	courier.setRecipient(observer);
}

bool Client::sendRequest(const Request *req, Response *resp) {
	if(state == State_Idle) {
		this->req = req;
		this->respParser->start(resp);
		return gotoStateConnect();
	} else if(state == State_WaitRequest) {
		this->req = req;
		this->respParser->start(resp);
		return gotoStateSendHeader();
	} else {
		LOG_ERROR(LOG_HTTP, "Wrong state " << state);
		return false;
	}
}

bool Client::close() {
	gotoStateClose();
	return true;
}

void Client::proc(Event *event) {
	LOG_DEBUG(LOG_HTTP, "Event: state=" << state << ", event=" << event->getType() << "," << (intptr_t)this);
	switch(state) {
		case State_Connect: stateConnectEvent(event); break;
		case State_WaitRequest: stateWaitRequestEvent(event); break;
		case State_SendHeader: stateSendHeaderEvent(event); break;
		case State_SendData: stateSendDataEvent(event); break;
		case State_WaitResponse: stateWaitResponseEvent(event); break;
		case State_RecvResponse: stateRecvResponseEvent(event); break;
		case State_Close: stateCloseEvent(event); break;
		case State_Error: stateErrorEvent(event); break;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void Client::procTimer() {
	LOG_DEBUG(LOG_HTTP, "Timeout: state=" << state);
	switch(state) {
		case State_WaitResponse: gotoStateError(); return;
		default:;
	}
}

bool Client::gotoStateConnect() {
	LOG_DEBUG(LOG_HTTP, "gotoStateConnect " << (intptr_t)this);
	if(tcpIp->connect(req->serverName, req->serverPort, TcpIp::Mode_TcpIpOverSsl) == false) {
		LOG_ERROR(LOG_HTTP, "Fatal error: SIM800 not ready.");
		state = State_Idle;
		return false;
	}
	this->state = State_Connect;
	return true;
}

void Client::stateConnectEvent(Event *event) {
	LOG_DEBUG(LOG_HTTP, "stateConnectEvent");
	switch(event->getType()) {
		case TcpIp::Event_ConnectOk: {
			gotoStateSendHeader();
			return;
		}
		case TcpIp::Event_ConnectError: {
			state = State_Idle;
			Event event(ClientInterface::Event_RequestError);
			courier.deliver(&event);
			return;
		}
		default: {
			LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
			return;
		}
	}
}

void Client::stateWaitRequestEvent(Event *event) {
	switch(event->getType()) {
		case TcpIp::Event_Close: {
			state = State_Idle;
			return;
		}
		default: {
			LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
			return;
		}
	}
}

bool Client::gotoStateSendHeader() {
	makeHeader();
	if(tcpIp->send((const uint8_t*)header->getString(), header->getLen()) == false) {
		LOG_ERROR(LOG_HTTP, "Fatal error: SIM800 not ready.");
		gotoStateError();
		return false;
	}
	state = State_SendHeader;
	return true;
}

void Client::makeHeader() {
	header->clear();
	*header << (req->method == Request::Method_GET ? "GET" : "POST") << " " << req->serverPath << " HTTP/1.1\r\n";
	*header << "Host: " << req->serverName << ":" << req->serverPort << "\r\n";
	*header << "Content-Type: " << contentTypeToString(req->contentType) << "; charset=windows-1251\r\n";
	uint16_t contentLength = (req->data == NULL) ? 0 : req->data->getLen();
	*header << "Content-Length: " << contentLength << "\r\n";
	*header << "Cache-Control: no-cache\r\n";
	if(req->keepAlive == true) { *header << "Connection: keep-alive\r\n"; }
	if(req->phpSessionId != NULL) { *header << "Cookie: PHPSESSID=" << req->phpSessionId << ";\r\n"; }
	if(req->rangeTo > 0) { *header << "Range: bytes=" << req->rangeFrom << "-" << req->rangeTo << "\r\n"; }
	*header << "\r\n";
	LOG_DEBUG(LOG_HTTP, "Header " << header->getLen());
	LOG_DEBUG_STR(LOG_HTTP, (const uint8_t*)header->getString(), header->getLen());
}

void Client::stateSendHeaderEvent(Event *event) {
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: stateSendHeaderEventSendDataOK(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: procUnwaitedClose(); return;
		default: {
			LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
			return;
		}
	}
}

void Client::stateSendHeaderEventSendDataOK() {
	LOG_DEBUG(LOG_HTTP, "stateSendHeaderEventSendDataOK1");
	if(req->data != NULL && req->data->getLen() > 0) {
		gotoStateSendData();
		return;
	} else {
		if(tcpIp->hasRecvData() == true) {
			gotoStateRecvResponse();
			return;
		} else {
			gotoStateWaitResponse();
			return;
		}
	}
}

void Client::gotoStateSendData() {
	LOG_TRACE(LOG_HTTP, "Data " << req->data->getLen());
	LOG_TRACE_STR(LOG_HTTP, (const uint8_t*)req->data->getString(), req->data->getLen());
	if(tcpIp->send((const uint8_t*)req->data->getString(), req->data->getLen()) == false) {
		LOG_ERROR(LOG_HTTP, "Fatal error: SIM800 not ready.");
		gotoStateError();
		return;
	}
	state = State_SendData;
}

void Client::stateSendDataEvent(Event *event) {
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: stateSendDataEventSendDataOK(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: procUnwaitedClose(); return;
		default: {
			LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
			return;
		}
	}
}

void Client::stateSendDataEventSendDataOK() {
	LOG_DEBUG(LOG_HTTP, "stateSendDataEventSendDataOK");
	if(tcpIp->hasRecvData() == true) {
		gotoStateRecvResponse();
		return;
	} else {
		gotoStateWaitResponse();
		return;
	}
}

void Client::gotoStateWaitResponse() {
	LOG_DEBUG(LOG_HTTP, "gotoStateWaitResponse");
	timer->start(TCP_RECV_TIMEOUT);
	state = State_WaitResponse;
}

void Client::stateWaitResponseEvent(Event *event) {
	LOG_DEBUG(LOG_HTTP, "stateWaitResponseEvent");
	switch(event->getType()) {
		case TcpIp::Event_IncomingData: gotoStateRecvResponse(); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: procUnwaitedClose(); return;
		default: {
			LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
			return;
		}
	}
}

void Client::gotoStateRecvResponse() {
	LOG_DEBUG(LOG_HTTP, "gotoStateRecvResponse " << respParser->getBufSize());
	if(tcpIp->recv(respParser->getBuf(), respParser->getBufSize()) == false) {
		LOG_ERROR(LOG_HTTP, "Fatal error: SIM800 not ready.");
		gotoStateError();
		return;
	}
	state = State_RecvResponse;
}

void Client::stateRecvResponseEvent(Event *event) {
	LOG_DEBUG(LOG_HTTP, "stateRecvResponseEvent");
	switch(event->getType()) {
		case TcpIp::Event_RecvDataOk: stateRecvResponseEventRecvDataOK(event); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: procUnwaitedClose(); return;
		default: {
			LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
			return;
		}
	}
}

void Client::stateRecvResponseEventRecvDataOK(Event *event) {
	LOG_DEBUG(LOG_HTTP, "stateRecvResponseEventRecvDataOK");
	respParser->parseData(event->getUint16());
	if(respParser->isComplete() == true) {
		if(req->keepAlive == true) {
			gotoStateWaitRequest();
			Event event(Event_RequestComplete);
			courier.deliver(&event);
			return;
		} else {
			gotoStateClose();
			return;
		}
	} else {
		if(tcpIp->hasRecvData() == true) {
			gotoStateRecvResponse();
			return;
		} else {
			gotoStateWaitResponse();
			return;
		}
	}
}

void Client::gotoStateWaitRequest() {
	LOG_DEBUG(LOG_HTTP, "gotoStateWaitRequest");
	timer->stop();
	state = State_WaitRequest;
}

void Client::gotoStateClose() {
	LOG_DEBUG(LOG_HTTP, "gotoStateClose");
	timer->stop();
	tcpIp->close();
	state = State_Close;
}

void Client::stateCloseEvent(Event *event) {
	switch(event->getType()) {
		case TcpIp::Event_Close: stateCloseEventClose(); return;
		default: {
			LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
			return;
		}
	}
}

void Client::stateCloseEventClose() {
	LOG_INFO(LOG_HTTP, "Connection closed");
	state = State_Idle;
	Event event(Event_RequestComplete);
	courier.deliver(&event);
}

void Client::gotoStateError() {
	tcpIp->close();
	state = State_Error;
}

void Client::stateErrorEvent(Event *event) {
	switch(event->getType()) {
		case TcpIp::Event_Close: stateErrorEventClose(); return;
		default: {
			LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
			return;
		}
	}
}

void Client::stateErrorEventClose() {
	LOG_INFO(LOG_HTTP, "Connection closed");
	state = State_Idle;
	Event event(Event_RequestError);
	courier.deliver(&event);
}

void Client::procUnwaitedClose() {
	LOG_ERROR(LOG_HTTP, "Unwaited connection close: state=" << state);
	state = State_Idle;
	Event event(Event_RequestError);
	courier.deliver(&event);
}

}
