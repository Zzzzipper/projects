#ifndef COMMON_HTTP_CLIENT_H
#define COMMON_HTTP_CLIENT_H

#include "http/include/Http.h"
#include "timer/include/Timer.h"

class TcpIp;
class Buffer;
class TimerEngine;

namespace Http {

class ResponseParser;

class Client : public EventObserver, public ClientInterface {
public:
	Client(TimerEngine *timers, TcpIp *tcpip);
	virtual ~Client();
	virtual void setObserver(EventObserver *observer);
	virtual bool sendRequest(const Request *req, Response *resp);
	virtual bool close();

private:
	enum State {
		State_Idle = 0,
		State_Connect,
		State_WaitRequest,
		State_SendHeader,
		State_SendData,
		State_WaitResponse,
		State_RecvResponse,
		State_Close,
		State_Error,
	};
	TimerEngine *timers;
	TcpIp *tcpIp;
	EventCourier courier;
	Timer *timer;
	State state;
	const Request *req;
	ResponseParser *respParser;
	StringBuilder *header;

	bool gotoStateConnect();
	void stateConnectEvent(Event *event);
	void stateWaitRequestEvent(Event *event);

	bool gotoStateSendHeader();
	void makeHeader();
	void stateSendHeaderEvent(Event *event);
	void stateSendHeaderEventSendDataOK();
	void gotoStateSendData();
	void stateSendDataEvent(Event *event);
	void stateSendDataEventSendDataOK();

	void gotoStateWaitResponse();
	void stateWaitResponseEvent(Event *event);
	void gotoStateRecvResponse();
	void stateRecvResponseEvent(Event *event);
	void stateRecvResponseEventRecvDataOK(Event *event);

	void gotoStateWaitRequest();

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
