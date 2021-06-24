#ifndef COMMON_INPAS_DEVICELAN_H
#define COMMON_INPAS_DEVICELAN_H

#include "InpasProtocol.h"

#include "tcpip/include/TcpIp.h"

namespace Inpas {

class DeviceLan : public EventObserver {
public:
	DeviceLan(PacketLayerInterface *packetLayer, TcpIp *conn);
	void procRequest(TlvPacket *req);
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
	TlvPacketMaker resp;

	void deviceLanOpen(TlvPacket *req);
	void stateOpenEvent(Event *event);
	void stateOpenEventConnectOk();
	void stateOpenEventConnectError();

	void stateWaitEvent(Event *event);

	void deviceLanSend(TlvPacket *req);
	void stateSendEvent(Event *event);
	void stateSendEventSendDataOK();
	void stateSendEventError();

	void deviceLanRecv(TlvPacket *req);
	void stateRecvWaitEvent(Event *event);
	void stateRecvWaitEventIncomingData();
	void stateRecvWaitEventError();
	void stateRecvEvent(Event *event);
	void stateRecvEventRecvDataOK(Event *event);
	void stateRecvEventError();

	void deviceLanClose(TlvPacket *req);
	void stateCloseEvent(Event *event);
	void stateCloseEventClose();
};

}

#endif
