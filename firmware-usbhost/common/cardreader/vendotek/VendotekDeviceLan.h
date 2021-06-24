#ifndef COMMON_SBERBANK_DEVICELAN_H
#define COMMON_SBERBANK_DEVICELAN_H

#include "VendotekProtocol.h"

#include "tcpip/include/TcpIp.h"

namespace Vendotek {

class DeviceLan : public EventObserver {
public:
	DeviceLan(PacketLayerInterface *packetLayer, TcpIp *conn);
	void procRequestCon(Tlv::Packet *req);
	void procRequestDat(Tlv::Packet *req);
	void procRequestDsc();
	void proc(Event *event) override;

private:
	enum State {
		State_Idle = 0,
		State_Open,
		State_Wait,
		State_Recv,
		State_RecvResp,
		State_Send,
		State_Close,
	};

	PacketLayerInterface *packetLayer;
	TcpIp *conn;
	State state;
	uint8_t index;
	uint32_t addr;
	uint16_t port;
	uint16_t windowSize;
	Tlv::PacketMaker resp;
	uint16_t recvMaxSize;
	Buffer sendBuf;
	Buffer recvBuf;

	void deviceLanOpen(Tlv::Packet *req);
	void stateOpenEvent(Event *event);
	void stateOpenEventConnectOk();
	void stateOpenEventConnectError();

	void gotoStateWait();
	void stateWaitEvent(Event *event);
	void stateWaitEventIncomingData();

	void deviceLanSend(Tlv::Packet *req);
	void stateSendEvent(Event *event);
	void stateSendEventSendDataOK();
	void stateSendEventError();

	void gotoStateRecv();
	void stateRecvEvent(Event *event);
	void stateRecvEventRecvDataOK(Event *event);
	void stateRecvEventError();
	void gotoStateRecvResp();

	void deviceLanClose();
	void gotoStateClose();
	void stateCloseEvent(Event *event);
	void stateCloseEventClose();


/*

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
	void stateCloseEventClose();*/
};

}

#endif
