#include "include/HttpDownloader.h"

#include "logger/include/Logger.h"

namespace Http {

#define DATA_BUFFER_SIZE 1000
#define TRY_MAY_NUMBER	 1000

Downloader::Downloader(TimerEngine *timers, Http::ClientInterface *connection) :
	timers(timers),
	connection(connection),
	state(State_Idle),
	data(DATA_BUFFER_SIZE, DATA_BUFFER_SIZE)
{
	connection->setObserver(this);
}

Downloader::~Downloader() {

}

void Downloader::download(const char *serverName, uint16_t serverPort, const char *serverPath, Dex::DataParser *receiver) {
	LOG_DEBUG(LOG_HTTP, "download: " << serverName << ":" << serverPort << "/" << serverPath);
	if(state != State_Idle) {
		LOG_ERROR(LOG_HTTP, "Wrong state " << state);
		return;
	}
	this->serverName = serverName;
	this->serverPort = serverPort;
	this->serverPath = serverPath;
	this->receiver = receiver;
	this->receiver->setObserver(this);
	this->tryNumber = 0;
	gotoStateStart();
}

void Downloader::proc(Event *event) {
	LOG_DEBUG(LOG_HTTP, "proc");
	switch(state) {
	case State_Start: stateStartEvent(event); break;
	case State_Recv: stateRecvEvent(event); break;
	default: LOG_ERROR(LOG_HTTP, "Unwaited event " << state << " " << event->getType());
	}
}

void Downloader::gotoStateStart() {
	LOG_DEBUG(LOG_HTTP, "startRequest");
	dataFrom = 0;
	dataTo = dataFrom + data.getSize() - 1;

	req.method = Http::Request::Method_GET;
	req.serverName = serverName.getString();
	req.serverPort = serverPort;
	req.serverPath = serverPath.getString();
	req.keepAlive = true;
	req.rangeFrom = dataFrom;
	req.rangeTo = dataTo;
	req.data = &reqData;
	resp.data = &data;
	if(connection->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_HTTP, "sendRequest");
		procError();
		return;
	}

	state = State_Start;
}

void Downloader::stateStartEvent(Event *event) {
	LOG_DEBUG(LOG_HTTP, "stateStartEvent");
	switch(event->getType()) {
		case Http::Client::Event_RequestComplete: stateStartEventComplete(); return;
		case Http::Client::Event_RequestError: tryAgain(); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event " << state << " " << event->getType());
	}
}

void Downloader::stateStartEventComplete() {
	LOG_WARN(LOG_HTTP, "stateStartEventComplete contentLength=" << resp.contentLength << ", " << resp.rangeFrom << "-" << resp.rangeTo << "/" << resp.rangeLength << ", dataLen=" << resp.data->getLen());
	if(resp.statusCode != Response::Status_PartialContent) {
		LOG_ERROR(LOG_HTTP, "Wrong status code " << resp.statusCode);
		procError();
		return;
	}

	dataSize = resp.rangeLength;
	if(dataFrom != resp.rangeFrom) {
		LOG_ERROR(LOG_HTTP, "Wrong response from " << dataFrom << "," << resp.rangeFrom);
		procError();
		return;
	}
	uint32_t dataLen = resp.rangeTo - resp.rangeFrom + 1;
	if(dataLen != resp.data->getLen()) {
		LOG_ERROR(LOG_HTTP, "Wrong data length exp=" << dataLen << ", act=" << resp.data->getLen());
		tryAgain();
		return;
	}

	LOG_TRACE_HEX(LOG_HTTP, resp.data->getData(), resp.data->getLen());
	if(receiver->start(resp.rangeLength) != Dex::DataParser::Result_Ok) {
		LOG_INFO(LOG_HTTP, "Download stopped by user");
		state = State_Idle;
		return;
	}

	if(receiver->procData(resp.data->getData(), resp.data->getLen()) != Dex::DataParser::Result_Ok) {
		LOG_INFO(LOG_HTTP, "Download stopped by user");
		state = State_Idle;
		return;
	}

	dataFrom += resp.contentLength;
	if(dataFrom >= dataSize) {
		state = State_Idle;
		receiver->complete();
		return;
	}
	dataTo = dataFrom + data.getSize() - 1;
	if(dataTo >= dataSize) {
		dataTo = dataSize - 1;
	}

	gotoStateRecv();
}

void Downloader::gotoStateRecv() {
	LOG_DEBUG(LOG_HTTP, "gotoStateRecv");
	req.method = Http::Request::Method_GET;
	req.serverName = serverName.getString();
	req.serverPort = serverPort;
	req.serverPath = serverPath.getString();
	req.keepAlive = true;
	req.rangeFrom = dataFrom;
	req.rangeTo = dataTo;
	req.data = &reqData;
	resp.data = &data;
	if(connection->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_HTTP, "sendRequest");
		procError();
		return;
	}

	state = State_Recv;
}

void Downloader::stateRecvEvent(Event *event) {
	LOG_DEBUG(LOG_HTTP, "stateRecvEvent");
	switch(event->getType()) {
		case Http::Client::Event_RequestComplete: stateRecvEventComplete(); return;
		case Http::Client::Event_RequestError: tryAgain(); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event " << state << " " << event->getType());
	}
}

void Downloader::stateRecvEventComplete() {
	LOG_WARN(LOG_HTTP, "stateRecvEventComplete contentLength=" << resp.contentLength << ", " << resp.rangeFrom << "-" << resp.rangeTo << "/" << resp.rangeLength << ", dataLen=" << resp.data->getLen());
	if(dataSize != resp.rangeLength) {
		LOG_ERROR(LOG_HTTP, "Wrong response size " << dataSize << "," << resp.rangeLength);
		procError();
		return;
	}
	if(dataFrom != resp.rangeFrom) {
		LOG_ERROR(LOG_HTTP, "Wrong response from " << dataFrom << "," << resp.rangeFrom);
		procError();
		return;
	}
	uint32_t dataLen = resp.rangeTo - resp.rangeFrom + 1;
	if(dataLen != resp.data->getLen()) {
		LOG_ERROR(LOG_HTTP, "Wrong data length exp=" << dataLen << ", act=" << resp.data->getLen());
		tryAgain();
		return;
	}

	LOG_TRACE_HEX(LOG_HTTP, resp.data->getData(), resp.data->getLen());
	if(receiver->procData(resp.data->getData(), resp.data->getLen()) != Dex::DataParser::Result_Ok) {
		LOG_INFO(LOG_HTTP, "Download stopped by user");
		state = State_Idle;
		return;
	}

	dataFrom += resp.contentLength;
	if(dataFrom >= dataSize) {
		LOG_INFO(LOG_HTTP, "Download complete errors=" << tryNumber);
		state = State_Idle;
		receiver->complete();
		return;
	}
	dataTo = dataFrom + data.getSize() - 1;
	if(dataTo >= dataSize) {
		dataTo = dataSize - 1;
	}

	gotoStateRecv();
}

void Downloader::tryAgain() {
	LOG_DEBUG(LOG_HTTP, "tryAgain");
	tryNumber++;
	if(tryNumber > TRY_MAY_NUMBER) {
		LOG_ERROR(LOG_HTTP, "Too many tries");
		procError();
		return;
	}

	if(connection->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_HTTP, "sendRequest");
		procError();
		return;
	}
}

void Downloader::procError() {
	LOG_DEBUG(LOG_HTTP, "procError");
	state = State_Idle;
	receiver->error();
}

}
