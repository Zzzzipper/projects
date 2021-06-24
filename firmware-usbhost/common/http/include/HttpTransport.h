#ifndef COMMON_HTTP_TRANSPORT_H
#define COMMON_HTTP_TRANSPORT_H

#include "http/include/Http.h"

namespace Http {

class Transport : public EventObserver, public ClientInterface {
public:
	Transport(ClientInterface *http, uint8_t tryNumberMax);
	~Transport();
	virtual void setObserver(EventObserver *observer);
	virtual bool sendRequest(const Request *req, Response *resp);
	virtual bool close();
	virtual void proc(Event *event);

private:
	enum State {
		State_Idle = 0,
		State_Wait,
		State_Reconnect
	};
	EventCourier courier;
	Http::ClientInterface *http;
	const Http::Request *req;
	Http::Response *resp;
	uint8_t tryNumber;
	uint8_t tryNumberMax;
	State state;

	void stateWaitEvent(Event *event);
	void stateWaitEventRequestComplete();
	void stateWaitEventRequestError();
	void gotoStateReconnect();
	void stateReconnectEvent(Event *event);
	void stateReconnectEventRequestComplete();
};

}

#endif
