#ifndef COMMON_HTTP_SERVER_H
#define COMMON_HTTP_SERVER_H

#include "http/include/Http.h"
#include "timer/include/Timer.h"

class TcpIp;
class Buffer;
class TimerEngine;

namespace Http {

class RequestParser;

class Server : public EventObserver, public ServerInterface {
public:
	Server(TimerEngine *timers, TcpIp *tcpip);
	virtual ~Server();
	virtual void setObserver(EventObserver *observer);
	virtual bool accept(Request2 *req);
	virtual bool sendResponse(const Response *resp);
	virtual bool close();

private:
	enum State {
		State_Idle = 0,
		State_WaitRequest,
		State_RecvRequest,
		State_SendHeader,
		State_SendData,
		State_Close,
		State_Error,
	};

	TcpIp *conn;
	State state;
	RequestParser *parser;
	StringBuilder *header;
	const Response *resp;
	EventCourier courier;

	void gotoStateWaitRequest();
	void stateWaitRequestEvent(Event *event);
	void gotoStateRecvRequest();
	void stateRecvRequestEvent(Event *event);
	void stateRecvRequestEventRecvDataOK(Event *event);

	bool gotoStateSendHeader();
	void makeHeader();
	void stateSendHeaderEvent(Event *event);
	void stateSendHeaderEventSendDataOK();
	void gotoStateSendData();
	void stateSendDataEvent(Event *event);
	void stateSendDataEventSendDataOK();

	void gotoStateClose();
	void stateCloseEvent(Event *event);
	void stateCloseEventClose();

	void gotoStateError();
	void stateErrorEvent(Event *event);
	void stateErrorEventClose();
	void procUnwaitedClose();

public:
	virtual void proc(Event *);
	void procTimer();
};

}

#endif
