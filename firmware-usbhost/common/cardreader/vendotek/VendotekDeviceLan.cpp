#include "VendotekDeviceLan.h"

#include "logger/include/Logger.h"

namespace Vendotek {

DeviceLan::DeviceLan(
	PacketLayerInterface *packetLayer,
	TcpIp *conn
) :
	packetLayer(packetLayer),
	conn(conn),
	state(State_Idle),
	resp(VENDOTEK_PACKET_SIZE),
	sendBuf(VENDOTEK_DATA_SIZE),
	recvBuf(VENDOTEK_DATA_SIZE)
{
	if(conn != NULL) { conn->setObserver(this); }
}

void DeviceLan::procRequestCon(Tlv::Packet *req) {
	LOG_DEBUG(LOG_ECLT, "procRequestCon");
	deviceLanOpen(req);
}

void DeviceLan::procRequestDat(Tlv::Packet *req) {
	LOG_DEBUG(LOG_ECLT, "procRequestDat");
	deviceLanSend(req);
}

void DeviceLan::procRequestDsc() {
	LOG_DEBUG(LOG_ECLT, "procRequestDsc");
	deviceLanClose();
}

void DeviceLan::proc(Event *event) {
	LOG_DEBUG(LOG_ECLT, "proc " << state);
	if(conn == NULL) {
		LOG_ERROR(LOG_ECLT, "conn not inited");
		return;
	}

	switch(state) {
		case State_Open: stateOpenEvent(event); return;
		case State_Wait: stateWaitEvent(event); return;
		case State_Send: stateSendEvent(event); return;
		case State_Recv: stateRecvEvent(event); return;
		case State_Close: stateCloseEvent(event); return;
		default: LOG_ERROR(LOG_ECLT, "Unwaited event " << state << "," << event->getType());
	}
}

void DeviceLan::deviceLanOpen(Tlv::Packet *req) {
	LOG_DEBUG(LOG_ECLT, "deviceLanOpen");
	uint8_t data[16];
	uint16_t destLen = req->getData(Type_TcpIpDestination, data, sizeof(data));
	if(destLen < sizeof(TcpIpDestination)) {
		LOG_ERROR(LOG_ECLT, "Bad format");
		return;
	}
	TcpIpDestination *dest2 = (TcpIpDestination*)data;
	index = dest2->index;
	addr = dest2->addr.get();
	port = dest2->port.get();
	windowSize = VENDOTEK_DATA_SIZE;

	if(conn == NULL || index > 0) {
		LOG_ERROR(LOG_ECLT, "conn not inited or wrong index " << index);
		TcpIpDestination dest;
		dest.index = index;
		dest.addr.set(addr);
		dest.port.set(port);
		dest.result = ConnStatus_NoService;
		dest.windowSize.set(windowSize);

		resp.clear();
		resp.addString(Type_MessageName, "CON", 3);
		resp.addData(Type_TcpIpDestination, &dest, sizeof(dest));
		packetLayer->sendPacket(resp.getBuf());
		return;
	}

	StringBuilder addr1;
	uint8_t *addr2 = (uint8_t*)&addr;
	addr1 << addr2[0] << "." << addr2[1] << "." << addr2[2] << "." << addr2[3];
	LOG_INFO(LOG_ECLT, "connect to " << addr1.getString() << ":" << port);
	state = State_Open;
	conn->connect(addr1.getString(), port, TcpIp::Mode_TcpIp);
}

void DeviceLan::stateOpenEvent(Event *event) {
	LOG_DEBUG(LOG_ECLT, "stateOpenEvent " << event->getType());
	switch(event->getType()) {
		case TcpIp::Event_ConnectOk: stateOpenEventConnectOk(); return;
		case TcpIp::Event_ConnectError: stateOpenEventConnectError(); return;
		case TcpIp::Event_Close: stateOpenEventConnectError(); return;
		default: LOG_ERROR(LOG_ECLT, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateOpenEventConnectOk() {
	LOG_DEBUG(LOG_ECLT, "stateOpenEventConnectOk");
	TcpIpDestination dest;
	dest.index = index;
	dest.addr.set(addr);
	dest.port.set(port);
	dest.result = ConnStatus_Established;
	dest.windowSize.set(windowSize);

	resp.clear();
	resp.addString(Type_MessageName, "CON", 3);
	resp.addData(Type_TcpIpDestination, &dest, sizeof(dest));
	packetLayer->sendPacket(resp.getBuf());
	gotoStateWait();
}

void DeviceLan::stateOpenEventConnectError() {
	LOG_DEBUG(LOG_ECLT, "stateOpenEventConnectError");
	TcpIpDestination dest;
	dest.index = index;
	dest.addr.set(addr);
	dest.port.set(port);
	dest.result = ConnStatus_LinkInactive;
	dest.windowSize.set(windowSize);

	resp.clear();
	resp.addString(Type_MessageName, "CON", 3);
	resp.addData(Type_TcpIpDestination, &dest, sizeof(dest));
	packetLayer->sendPacket(resp.getBuf());
	state = State_Idle;
}

void DeviceLan::gotoStateWait() {
	LOG_DEBUG(LOG_ECLT, "gotoStateWait");
	if(conn->hasRecvData() == true) {
		gotoStateRecv();
		return;
	} else {
		state = State_Wait;
		return;
	}
}

void DeviceLan::stateWaitEvent(Event *event) {
	LOG_DEBUG(LOG_ECLT, "stateWaitEvent");
	switch(event->getType()) {
		case TcpIp::Event_IncomingData: stateWaitEventIncomingData(); return;
		case TcpIp::Event_Close: stateCloseEventClose(); return;
		default: LOG_ERROR(LOG_ECLT, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateWaitEventIncomingData() {
	LOG_DEBUG(LOG_ECLT, "stateWaitEventIncomingData");
	gotoStateRecv();
}

void DeviceLan::deviceLanSend(Tlv::Packet *req) {
	LOG_DEBUG(LOG_ECLT, "deviceLanSend");
	uint16_t sendLen = req->getData(Type_ConfirmableDataBlock, sendBuf.getData(), sendBuf.getSize());
	if(sendLen < 1) {
		LOG_ERROR(LOG_ECLT, "Bad format");
		return;
	}

	sendBuf.setLen(sendLen);
	LOG_DEBUG_HEX(LOG_ECLT, sendBuf.getData(), sendBuf.getLen());
	if(conn->send(sendBuf.getData(), sendBuf.getLen()) == false) {
		LOG_ERROR(LOG_ECLT, "send fail");
		gotoStateClose();
		return;
	}

	state = State_Send;
}

void DeviceLan::stateSendEvent(Event *event) {
	LOG_DEBUG(LOG_ECLT, "stateSendEvent " << event->getType());
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: stateSendEventSendDataOK(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: stateSendEventError(); return;
		default: LOG_ERROR(LOG_ECLT, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateSendEventSendDataOK() {
	LOG_DEBUG(LOG_ECLT, "stateSendEventSendDataOK");
	uint8_t dest[] = { 0x00 };
	resp.clear();
	resp.addString(Type_MessageName, "DAT");
	resp.addData(Type_TcpIpDestination, dest, 1);
	resp.addNumber(Type_OutgoingByteCounter, sendBuf.getLen());
	packetLayer->sendPacket(resp.getBuf());
	gotoStateWait();
}

void DeviceLan::stateSendEventError() {
	LOG_DEBUG(LOG_ECLT, "stateSendEventError");
	gotoStateClose();
}

void DeviceLan::gotoStateRecv() {
	LOG_DEBUG(LOG_ECLT, "gotoStateRecv");
	if(conn->recv(recvBuf.getData(), recvBuf.getSize()) == false) {
		LOG_ERROR(LOG_ECLT, "recv fail");
		gotoStateWait();
		return;
	}

	state = State_Recv;
}

void DeviceLan::stateRecvEvent(Event *event) {
	LOG_DEBUG(LOG_ECLT, "stateRecvEvent " << event->getType());
	switch(event->getType()) {
		case TcpIp::Event_RecvDataOk: stateRecvEventRecvDataOK(event); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: stateRecvEventError(); return;
		default: LOG_ERROR(LOG_ECLT, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateRecvEventRecvDataOK(Event *event) {
	LOG_DEBUG(LOG_ECLT, "stateRecvEventRecvDataOK " << event->getUint16());
	recvBuf.setLen(event->getUint16());

	uint8_t dest[] = { 0x00 };
	resp.clear();
	resp.addString(Type_MessageName, "DAT");
	resp.addData(Type_TcpIpDestination, dest, 1);
//	resp.addNumber(Type_OutgoingByteCounter, recvBuf.getLen());
	resp.addData(Type_SimpleDataBlock, recvBuf.getData(), recvBuf.getLen());
	LOG_TRACE_HEX(LOG_ECLT, recvBuf.getData(), recvBuf.getLen());
	packetLayer->sendPacket(resp.getBuf());
	gotoStateWait();
}

void DeviceLan::stateRecvEventError() {
	LOG_DEBUG(LOG_ECLT, "stateRecvEventError");
	gotoStateClose();
}

void DeviceLan::gotoStateRecvResp() {
	LOG_DEBUG(LOG_ECLT, "gotoStateRecvResp");
	state = State_RecvResp;
}

void DeviceLan::deviceLanClose() {
	LOG_DEBUG(LOG_ECLT, "deviceLanClose");
	gotoStateClose();
}

void DeviceLan::gotoStateClose() {
	LOG_DEBUG(LOG_ECLT, "gotoStateClose");
	conn->close();
	state = State_Close;
}

void DeviceLan::stateCloseEvent(Event *event) {
	LOG_DEBUG(LOG_ECLT, "stateCloseEvent " << event->getType());
	switch(event->getType()) {
		case TcpIp::Event_Close: stateCloseEventClose(); return;
		default: LOG_ERROR(LOG_ECLT, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateCloseEventClose() {
	LOG_DEBUG(LOG_ECLT, "stateCloseEventClose");
	uint8_t dest[] = { 0x00 };
	resp.clear();
	resp.addString(Type_MessageName, "DSC", 3);
	resp.addData(Type_TcpIpDestination, dest, 1);
	packetLayer->sendPacket(resp.getBuf());
	state = State_Idle;
}

/*
void DeviceLan::stateWaitEvent(Event *event) {
	LOG_DEBUG(LOG_ECLT, "stateWaitEvent");
	switch(event->getType()) {
		case TcpIp::Event_IncomingData: return;
		default: LOG_ERROR(LOG_ECLT, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::deviceLanRead(MasterCallRequest *req) {
	LOG_DEBUG(LOG_ECLT, "deviceLanRead");
	DeviceLanReadParam *param = (DeviceLanReadParam*)(req->paramVal);
	recvMaxSize = sendBuf.getSize() - sizeof(MasterCallRequest);
	if(recvMaxSize > param->maxSize.get()) { recvMaxSize = param->maxSize.get(); }

	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	resp->result = 0;
	resp->num.set(req->num.get() | SBERBANK_RESPONSE_FLAG);
	resp->instruction = req->instruction;
	resp->device = req->device;
	resp->reserved = 0;

	if(conn->hasRecvData() == true) {
		if(conn->recv(resp->paramVal, recvMaxSize) == false) {
			LOG_ERROR(LOG_ECLT, "recv fail");
			//todo: recvfail
			return;
		}
		state = State_Recv;
		return;
	} else {
		state = State_RecvWait;
		return;
	}
}

void DeviceLan::stateRecvWaitEvent(Event *event) {
	LOG_DEBUG(LOG_ECLT, "stateRecvWaitEvent");
	switch(event->getType()) {
		case TcpIp::Event_IncomingData: stateRecvWaitEventIncomingData(); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: stateRecvWaitEventError(); return;
		default: LOG_ERROR(LOG_ECLT, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateRecvWaitEventIncomingData() {
	LOG_DEBUG(LOG_ECLT, "stateRecvWaitEventIncomingData");
	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	if(conn->recv(resp->paramVal, recvMaxSize) == false) {
		LOG_ERROR(LOG_ECLT, "recv fail");
		//todo: recvfail
		return;
	}
	state = State_Recv;
}

void DeviceLan::stateRecvWaitEventError() {
	LOG_DEBUG(LOG_ECLT, "stateRecvWaitEventError");
	//todo: recvfail
}

void DeviceLan::stateRecvEvent(Event *event) {
	LOG_DEBUG(LOG_ECLT, "stateRecvEvent " << event->getType());
	switch(event->getType()) {
		case TcpIp::Event_RecvDataOk: stateRecvEventRecvDataOK(event); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: stateRecvEventError(); return;
		default: LOG_ERROR(LOG_ECLT, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateRecvEventRecvDataOK(Event *event) {
	LOG_DEBUG(LOG_ECLT, "stateRecvEventRecvDataOK " << event->getUint16());
	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	resp->paramLen.set(event->getUint16());
	resp->len.set(sizeof(MasterCallRequest) - sizeof(Packet) + event->getUint16());
	sendBuf.setLen(sizeof(MasterCallRequest) + event->getUint16());
	packetLayer->sendPacket(&sendBuf);
	state = State_Wait;
}

void DeviceLan::stateRecvEventError() {
	LOG_DEBUG(LOG_ECLT, "stateRecvEventError");
	//todo: recvfail
}

void DeviceLan::deviceLanWrite(MasterCallRequest *req) {
	LOG_DEBUG(LOG_ECLT, "deviceLanWrite");
	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	resp->result = 0;
	resp->num.set(req->num.get() | SBERBANK_RESPONSE_FLAG);
	resp->instruction = req->instruction;
	resp->device = req->device;
	resp->reserved = 0;
	DeviceLanSendResponse *param = (DeviceLanSendResponse*)(resp->paramVal);
	param->sendLen.set(req->paramLen.get());
	resp->paramLen.set(sizeof(DeviceLanSendResponse));
	resp->len.set(sizeof(MasterCallRequest) - sizeof(Packet) + sizeof(DeviceLanSendResponse));
	sendBuf.setLen(sizeof(MasterCallRequest) + sizeof(DeviceLanSendResponse));

	if(conn->send(req->paramVal, req->paramLen.get()) == false) {
		LOG_ERROR(LOG_ECLT, "send fail");
		//todo: sendfail
		return;
	}
	state = State_Send;
}

void DeviceLan::stateSendEvent(Event *event) {
	LOG_DEBUG(LOG_ECLT, "stateSendEvent " << event->getType());
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: stateSendEventSendDataOK(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: stateSendEventError(); return;
		default: LOG_ERROR(LOG_ECLT, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateSendEventSendDataOK() {
	LOG_DEBUG(LOG_ECLT, "stateSendEventSendDataOK");
	packetLayer->sendPacket(&sendBuf);
	state = State_Wait;
}

void DeviceLan::stateSendEventError() {
	LOG_DEBUG(LOG_ECLT, "stateSendEventError");
	//todo: recvfail
}
*/
}
