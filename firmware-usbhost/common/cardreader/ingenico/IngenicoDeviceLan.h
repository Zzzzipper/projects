#ifndef COMMON_INGENICO_DEVICELAN_H
#define COMMON_INGENICO_DEVICELAN_H

#include "IngenicoProtocol.h"

#include "tcpip/include/TcpIp.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/StringParser.h"

namespace Ingenico {

class DeviceLan : public EventObserver {
public:
	DeviceLan(PacketLayerInterface *packetLayer, TcpIp *conn, TimerEngine *timers);
	void procDeviceOpen(StringParser *parser);
	void procIoctl(StringParser *parser);
	void procConnect(StringParser *parser);
	void procRead(StringParser *parser);
	void procWrite(StringParser *parser);
	void procDisconnect(StringParser *parser);
	void procDeviceClose(StringParser *parser);

	void proc(Event *event) override;
	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Open,
		State_Wait,
		State_RecvWait,
		State_Recv,
		State_Send,
		State_Close,
	};

	PacketLayerInterface *packetLayer;
	TcpIp *conn;
	TimerEngine *timers;
	Timer *timer;
	State state;
	Buffer sendBuf;
	uint16_t recvTimeout;
	uint16_t recvMaxSize;

	void stateOpenEvent(Event *event);
	void stateOpenEventConnectOk();
	void stateOpenEventConnectError();

	void stateWaitEvent(Event *event);

	void gotoStateRecvWait();
	void stateRecvWaitEvent(Event *event);
	void stateRecvWaitEventIncomingData();
	void stateRecvWaitEventError();
	void stateRecvWaitTimeout();

	void stateRecvEvent(Event *event);
	void stateRecvEventRecvDataOK(Event *event);
	void stateRecvEventError();

	void stateSendEvent(Event *event);
	void stateSendEventSendDataOK();
	void stateSendEventError();

	void stateCloseEvent(Event *event);
	void stateCloseEventClose();
};

}

#endif
