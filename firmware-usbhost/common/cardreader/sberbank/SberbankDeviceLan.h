#if 0
#ifndef COMMON_SBERBANK_DEVICELAN_H
#define COMMON_SBERBANK_DEVICELAN_H

#include "SberbankProtocol.h"

#include "tcpip/include/TcpIp.h"

namespace Sberbank {

class DeviceLan : public EventObserver {
public:
	DeviceLan(PacketLayerInterface *packetLayer, TcpIp *conn);
	void procRequest(MasterCallRequest *req);
	void proc(Event *event) override;

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
	State state;
	Buffer sendBuf;
	uint16_t recvMaxSize;

	void deviceLanOpen(MasterCallRequest *req);
	void stateOpenEvent(Event *event);
	void stateOpenEventConnectOk();
	void stateOpenEventConnectError();

	void stateWaitEvent(Event *event);

	void deviceLanRead(MasterCallRequest *req);
	void stateRecvWaitEvent(Event *event);
	void stateRecvWaitEventIncomingData();
	void stateRecvWaitEventError();
	void stateRecvEvent(Event *event);
	void stateRecvEventRecvDataOK(Event *event);
	void stateRecvEventError();

	void deviceLanWrite(MasterCallRequest *req);
	void stateSendEvent(Event *event);
	void stateSendEventSendDataOK();
	void stateSendEventError();

	void deviceLanClose(MasterCallRequest *req);
	void stateCloseEvent(Event *event);
	void stateCloseEventClose();
};

}

#endif
#else
#ifndef COMMON_SBERBANK_DEVICELAN_H
#define COMMON_SBERBANK_DEVICELAN_H

#include "SberbankProtocol.h"

#include "tcpip/include/TcpIp.h"

namespace Sberbank {

class DeviceLan : public EventObserver {
public:
	DeviceLan(PacketLayerInterface *packetLayer, TcpIp *conn);
	void procRequest(MasterCallRequest *req);
	void proc(Event *event) override;

private:
	enum State {
		State_Idle = 0,
		State_Open,
		State_OpenClose,
		State_Wait,
		State_RecvWait,
		State_Recv,
		State_Send,
		State_Close,
	};

	PacketLayerInterface *packetLayer;
	TcpIp *conn;
	State state;
	Buffer sendBuf;
	uint32_t reqNum;
	StringBuilder connectAddr;
	uint16_t connectPort;
	uint16_t recvMaxSize;

	void sendResponse(uint8_t instruction, uint8_t result);

	void deviceLanOpen(MasterCallRequest *req);
	void stateOpenEvent(Event *event);
	void stateOpenEventConnectOk();
	void stateOpenEventConnectError();

	void gotoStateOpenClose();
	void stateOpenCloseEvent(Event *event);
	void stateOpenCloseEventClose();

	void stateWaitEvent(Event *event);

	void deviceLanRead(MasterCallRequest *req);
	void stateRecvWaitEvent(Event *event);
	void stateRecvWaitEventIncomingData();
	void stateRecvWaitEventError();
	void stateRecvEvent(Event *event);
	void stateRecvEventRecvDataOK(Event *event);
	void stateRecvEventError();

	void deviceLanWrite(MasterCallRequest *req);
	void stateSendEvent(Event *event);
	void stateSendEventSendDataOK();
	void stateSendEventError();

	void deviceLanClose(MasterCallRequest *req);
	void stateCloseEvent(Event *event);
	void stateCloseEventClose();
};

}

#endif
#endif
