#include "include/HttpTransport.h"
#include "logger/include/Logger.h"

namespace Http {

Transport::Transport(Http::ClientInterface *http, uint8_t tryNumberMax) :
	http(http),
	tryNumberMax(tryNumberMax),
	state(State_Idle)
{
	this->http->setObserver(this);
}

Transport::~Transport() {
}

void Transport::setObserver(EventObserver *observer) {
	courier.setRecipient(observer);
}

bool Transport::sendRequest(const Http::Request *req, Http::Response *resp) {
	this->req = req;
	this->resp = resp;
	this->tryNumber = tryNumberMax;
	if(this->http->sendRequest(req, resp) == false) {
		return false;
	}
	state = State_Wait;
	return true;
}

bool Transport::close() {
	return this->http->close();
}

void Transport::proc(Event *event) {
	LOG_DEBUG(LOG_HTTP, "proc " << event->getType());
	switch(state) {
		case State_Wait: stateWaitEvent(event); break;
		case State_Reconnect: stateReconnectEvent(event); break;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event " << state);
	}
}

void Transport::stateWaitEvent(Event *event) {
	LOG_DEBUG(LOG_HTTP, "stateWaitEvent");
	switch(event->getType()) {
		case ClientInterface::Event_RequestComplete: stateWaitEventRequestComplete(); return;
		case ClientInterface::Event_RequestError: stateWaitEventRequestError(); return;
	}
}

void Transport::stateWaitEventRequestComplete() {
	if(resp->statusCode != Response::Status_OK) {
		LOG_ERROR(LOG_HTTP, "Server wrong status " << resp->statusCode);
		gotoStateReconnect();
		return;
	}
	courier.deliver(ClientInterface::Event_RequestComplete);
}

void Transport::stateWaitEventRequestError() {
	LOG_ERROR(LOG_HTTP, "Server request failed");
	state = State_Idle;
	courier.deliver(ClientInterface::Event_RequestError);
}

void Transport::gotoStateReconnect() {
	LOG_DEBUG(LOG_HTTP, "gotoStateReconnect");
	tryNumber--;
	if(tryNumber == 0) {
		LOG_ERROR(LOG_HTTP, "Too many tries");
		state = State_Idle;
		courier.deliver(ClientInterface::Event_RequestError);
		return;
	}

	if(http->close() == false) {
		stateReconnectEventRequestComplete();
		return;
	}

	state = State_Reconnect;
}

void Transport::stateReconnectEvent(Event *event) {
	LOG_DEBUG(LOG_HTTP, "stateReconnectEvent");
	switch(event->getType()) {
		case ClientInterface::Event_RequestComplete: stateReconnectEventRequestComplete(); return;
	}
}

void Transport::stateReconnectEventRequestComplete() {
	LOG_DEBUG(LOG_HTTP, "stateReconnectEventRequestComplete");
	if(http->sendRequest(req, resp) == false) {
		LOG_ERROR(LOG_HTTP, "sendRequest failed");
		courier.deliver(ClientInterface::Event_RequestError);
		return;
	}

	state = State_Wait;
}

}
