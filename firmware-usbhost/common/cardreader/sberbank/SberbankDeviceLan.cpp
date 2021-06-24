#if 0
#include "SberbankDeviceLan.h"

#include "logger/include/Logger.h"

namespace Sberbank {

DeviceLan::DeviceLan(
	PacketLayerInterface *packetLayer,
	TcpIp *conn
) :
	packetLayer(packetLayer),
	conn(conn),
	state(State_Idle),
	sendBuf(SBERBANK_PACKET_SIZE)
{
	if(conn != NULL) { conn->setObserver(this); }
}

void DeviceLan::procRequest(MasterCallRequest *req) {
	LOG_DEBUG(LOG_ECL, "deviceLan");
	if(conn == NULL) {
		LOG_ERROR(LOG_ECL, "conn not inited");
		return;
	}

	switch(req->instruction) {
		case Instruction_Open: deviceLanOpen(req); return;
		case Instruction_Read: deviceLanRead(req); return;
		case Instruction_Write: deviceLanWrite(req); return;
		case Instruction_Close: deviceLanClose(req); return;
		default: LOG_ERROR(LOG_SM, "Unwaited intstruction " << req->instruction);
	}
}

void DeviceLan::proc(Event *event) {
	LOG_DEBUG(LOG_SM, "proc " << state);
	if(conn == NULL) {
		LOG_ERROR(LOG_ECL, "conn not inited");
		return;
	}

	switch(state) {
		case State_Open: stateOpenEvent(event); return;
		case State_Wait: stateWaitEvent(event); return;
		case State_RecvWait: stateRecvWaitEvent(event); return;
		case State_Recv: stateRecvEvent(event); return;
		case State_Send: stateSendEvent(event); return;
		case State_Close: stateCloseEvent(event); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << state << "," << event->getType());
	}
}

void DeviceLan::deviceLanOpen(MasterCallRequest *req) {
	LOG_DEBUG(LOG_ECL, "deviceLanOpen");
	DeviceLanOpenParam *param = (DeviceLanOpenParam*)(req->paramVal);
	StringBuilder ipaddr;
	ipaddr << param->ipaddr[0] << "." << param->ipaddr[1] << "." << param->ipaddr[2] << "." << param->ipaddr[3];
	conn->connect(ipaddr.getString(), param->port.get(), TcpIp::Mode_TcpIp);

	req->result = 0;
	req->num.set(req->num.get() | SBERBANK_RESPONSE_FLAG);
	sendBuf.clear();
	sendBuf.add(req, sizeof(Packet) + req->len.get());
	state = State_Open;
}

void DeviceLan::stateOpenEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateOpenEvent " << event->getType());
	switch(event->getType()) {
		case TcpIp::Event_ConnectOk: stateOpenEventConnectOk(); return;
		case TcpIp::Event_ConnectError: stateOpenEventConnectError(); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateOpenEventConnectOk() {
	LOG_DEBUG(LOG_ECL, "stateOpenEventConnectOk");
	packetLayer->sendPacket(&sendBuf);
	state = State_Wait;
}

void DeviceLan::stateOpenEventConnectError() {
	LOG_DEBUG(LOG_ECL, "stateOpenEventConnectError");
//TODO: обработка ошибки открытия
	state = State_Idle;
}

void DeviceLan::stateWaitEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateWaitEvent");
	switch(event->getType()) {
		case TcpIp::Event_IncomingData: return;
		case TcpIp::Event_Close: stateCloseEventClose(); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::deviceLanRead(MasterCallRequest *req) {
	LOG_DEBUG(LOG_ECL, "deviceLanRead");
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
			LOG_ERROR(LOG_ECL, "recv fail");
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
	LOG_DEBUG(LOG_ECL, "stateRecvWaitEvent");
	switch(event->getType()) {
		case TcpIp::Event_IncomingData: stateRecvWaitEventIncomingData(); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: stateRecvWaitEventError(); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateRecvWaitEventIncomingData() {
	LOG_DEBUG(LOG_ECL, "stateRecvWaitEventIncomingData");
	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	if(conn->recv(resp->paramVal, recvMaxSize) == false) {
		LOG_ERROR(LOG_ECL, "recv fail");
		//todo: recvfail
		return;
	}
	state = State_Recv;
}

void DeviceLan::stateRecvWaitEventError() {
	LOG_DEBUG(LOG_ECL, "stateRecvWaitEventError");
	//todo: recvfail
}

void DeviceLan::stateRecvEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateRecvEvent " << event->getType());
	switch(event->getType()) {
		case TcpIp::Event_RecvDataOk: stateRecvEventRecvDataOK(event); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: stateRecvEventError(); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateRecvEventRecvDataOK(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateRecvEventRecvDataOK " << event->getUint16());
	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	resp->paramLen.set(event->getUint16());
	resp->len.set(sizeof(MasterCallRequest) - sizeof(Packet) + event->getUint16());
	sendBuf.setLen(sizeof(MasterCallRequest) + event->getUint16());
	packetLayer->sendPacket(&sendBuf);
	state = State_Wait;
}

void DeviceLan::stateRecvEventError() {
	LOG_DEBUG(LOG_ECL, "stateRecvEventError");
	//todo: recvfail
}

void DeviceLan::deviceLanWrite(MasterCallRequest *req) {
	LOG_DEBUG(LOG_ECL, "deviceLanWrite");
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
		LOG_ERROR(LOG_HTTP, "send fail");
		//todo: sendfail
		return;
	}
	state = State_Send;
}

void DeviceLan::stateSendEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateSendEvent " << event->getType());
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: stateSendEventSendDataOK(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: stateSendEventError(); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateSendEventSendDataOK() {
	LOG_DEBUG(LOG_ECL, "stateSendEventSendDataOK");
	packetLayer->sendPacket(&sendBuf);
	state = State_Wait;
}

void DeviceLan::stateSendEventError() {
	LOG_DEBUG(LOG_ECL, "stateSendEventError");
	//todo: recvfail
}

void DeviceLan::deviceLanClose(MasterCallRequest *req) {
	LOG_DEBUG(LOG_ECL, "deviceLanClose");
	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	resp->result = 0;
	resp->num.set(req->num.get() | SBERBANK_RESPONSE_FLAG);
	resp->instruction = req->instruction;
	resp->device = req->device;
	resp->reserved = 0;
	resp->paramLen.set(0);
	resp->len.set(sizeof(MasterCallRequest) - sizeof(Packet));
	sendBuf.setLen(sizeof(MasterCallRequest));

	conn->close();
	state = State_Close;
}

void DeviceLan::stateCloseEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateCloseEvent " << event->getType());
	switch(event->getType()) {
		case TcpIp::Event_Close: stateCloseEventClose(); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateCloseEventClose() {
	LOG_DEBUG(LOG_ECL, "stateCloseEventClose");
	packetLayer->sendPacket(&sendBuf);
	state = State_Idle;
}

}
#else
#include "SberbankDeviceLan.h"

#include "logger/include/Logger.h"

namespace Sberbank {

DeviceLan::DeviceLan(
	PacketLayerInterface *packetLayer,
	TcpIp *conn
) :
	packetLayer(packetLayer),
	conn(conn),
	state(State_Idle),
	sendBuf(SBERBANK_COMMAND_SIZE)
{
	if(conn != NULL) { conn->setObserver(this); }
}

void DeviceLan::procRequest(MasterCallRequest *req) {
	LOG_DEBUG(LOG_ECL, "deviceLan");
	if(conn == NULL) {
		LOG_ERROR(LOG_ECL, "conn not inited");
		return;
	}

	switch(req->instruction) {
		case Instruction_Open: deviceLanOpen(req); return;
		case Instruction_Read: deviceLanRead(req); return;
		case Instruction_Write: deviceLanWrite(req); return;
		case Instruction_Close: deviceLanClose(req); return;
		default: LOG_ERROR(LOG_ECL, "Unwaited intstruction " << req->instruction);
	}
}

void DeviceLan::proc(Event *event) {
	LOG_DEBUG(LOG_ECL, "proc " << state);
	if(conn == NULL) {
		LOG_ERROR(LOG_ECL, "conn not inited");
		return;
	}

	switch(state) {
		case State_Open: stateOpenEvent(event); return;
		case State_Wait: stateWaitEvent(event); return;
		case State_RecvWait: stateRecvWaitEvent(event); return;
		case State_Recv: stateRecvEvent(event); return;
		case State_Send: stateSendEvent(event); return;
		case State_Close: stateCloseEvent(event); return;
		default: LOG_ERROR(LOG_ECL, "Unwaited event " << state << "," << event->getType());
	}
}

void DeviceLan::sendResponse(uint8_t instruction, uint8_t result) {
	LOG_DEBUG(LOG_ECL, "sendResponse");
	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	resp->result = result;
	resp->num.set(reqNum | SBERBANK_RESPONSE_FLAG);
	resp->instruction = instruction;
	resp->device = Device_Lan;
	resp->reserved = 0;
	resp->paramLen.set(0);
	resp->len.set(sizeof(MasterCallRequest) - sizeof(Packet));
	sendBuf.setLen(sizeof(MasterCallRequest));
	packetLayer->sendPacket(&sendBuf);
}

void DeviceLan::deviceLanOpen(MasterCallRequest *req) {
#if 0
	LOG_DEBUG(LOG_ECL, "deviceLanOpen");
	DeviceLanOpenParam *param = (DeviceLanOpenParam*)(req->paramVal);
	StringBuilder ipaddr;
	ipaddr << param->ipaddr[0] << "." << param->ipaddr[1] << "." << param->ipaddr[2] << "." << param->ipaddr[3];
	if(conn->connect(ipaddr.getString(), param->port.get(), TcpIp::Mode_TcpIp) == false) {
		LOG_ERROR(LOG_ECL, "connection already opened");
		reqNum = req->num.get();
		gotoStateOpenClose();
		return;
	}
	req->result = 0;
	req->num.set(req->num.get() | SBERBANK_RESPONSE_FLAG);
	sendBuf.clear();
	sendBuf.add(req, sizeof(Packet) + req->len.get());
	state = State_Open;
#else
	LOG_DEBUG(LOG_ECL, "deviceLanOpen");
	reqNum = req->num.get();

	DeviceLanOpenParam *param = (DeviceLanOpenParam*)(req->paramVal);
	connectAddr.clear();
	connectAddr << param->ipaddr[0] << "." << param->ipaddr[1] << "." << param->ipaddr[2] << "." << param->ipaddr[3];
	connectPort = param->port.get();
	if(conn->connect(connectAddr.getString(), connectPort, TcpIp::Mode_TcpIp) == false) {
		LOG_ERROR(LOG_ECL, "connection already opened");
		gotoStateOpenClose();
		return;
	}

	state = State_Open;
#endif
}

void DeviceLan::stateOpenEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateOpenEvent " << event->getType());
	switch(event->getType()) {
		case TcpIp::Event_ConnectOk: stateOpenEventConnectOk(); return;
		case TcpIp::Event_ConnectError: stateOpenEventConnectError(); return;
		case TcpIp::Event_Close: stateOpenEventConnectError(); return;
		default: LOG_ERROR(LOG_ECL, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateOpenEventConnectOk() {
	LOG_DEBUG(LOG_ECL, "stateOpenEventConnectOk");
	sendResponse(Instruction_Open, 0);
	state = State_Wait;
}

void DeviceLan::stateOpenEventConnectError() {
	LOG_DEBUG(LOG_ECL, "stateOpenEventConnectError");
	sendResponse(Instruction_Open, 99);
	state = State_Idle;
}

void DeviceLan::gotoStateOpenClose() {
	LOG_DEBUG(LOG_ECL, "gotoStateOpenClose");
	conn->close();
	state = State_OpenClose;
}

void DeviceLan::stateOpenCloseEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateOpenCloseEvent");
	switch(event->getType()) {
		case TcpIp::Event_Close: stateOpenCloseEventClose(); return;
		default: LOG_ERROR(LOG_ECL, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateOpenCloseEventClose() {
	LOG_DEBUG(LOG_ECL, "stateOpenCloseEventClose");
	if(conn->connect(connectAddr.getString(), connectPort, TcpIp::Mode_TcpIp) == false) {
		LOG_ERROR(LOG_ECL, "connection already opened");
		sendResponse(Instruction_Open, 99);
		state = State_Idle;
		return;
	}

	state = State_Open;
}

void DeviceLan::stateWaitEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateWaitEvent");
	switch(event->getType()) {
		case TcpIp::Event_IncomingData: return;
		case TcpIp::Event_Close: stateCloseEventClose(); return;
		default: LOG_ERROR(LOG_ECL, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::deviceLanRead(MasterCallRequest *req) {
#if 0
	LOG_DEBUG(LOG_ECL, "deviceLanRead");
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
			LOG_ERROR(LOG_ECL, "recv fail");
			//todo: recvfail
			return;
		}
		state = State_Recv;
		return;
	} else {
		state = State_RecvWait;
		return;
	}
#else
	LOG_DEBUG(LOG_ECL, "deviceLanRead");
	reqNum = req->num.get();

	DeviceLanReadParam *param = (DeviceLanReadParam*)(req->paramVal);
	recvMaxSize = sendBuf.getSize() - sizeof(MasterCallRequest);
	if(recvMaxSize > param->maxSize.get()) { recvMaxSize = param->maxSize.get(); }
	LOG_DEBUG(LOG_ECL, "recvMaxSize=" << recvMaxSize << "(" << param->maxSize.get() << ")");

	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	resp->result = 0;
	resp->num.set(req->num.get() | SBERBANK_RESPONSE_FLAG);
	resp->instruction = req->instruction;
	resp->device = req->device;
	resp->reserved = 0;
	if(conn->hasRecvData() == true) {
		if(conn->recv(resp->paramVal, recvMaxSize) == false) {
			LOG_ERROR(LOG_ECL, "recv fail");
			//todo: recvfail
			return;
		}
		state = State_Recv;
		return;
	} else {
		state = State_RecvWait;
		return;
	}
#endif
}

void DeviceLan::stateRecvWaitEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateRecvWaitEvent");
	switch(event->getType()) {
		case TcpIp::Event_IncomingData: stateRecvWaitEventIncomingData(); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: stateRecvWaitEventError(); return;
		default: LOG_ERROR(LOG_ECL, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateRecvWaitEventIncomingData() {
	LOG_DEBUG(LOG_ECL, "stateRecvWaitEventIncomingData");
	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	if(conn->recv(resp->paramVal, recvMaxSize) == false) {
		LOG_ERROR(LOG_ECL, "recv fail");
		//todo: recvfail
		return;
	}
	state = State_Recv;
}

void DeviceLan::stateRecvWaitEventError() {
	LOG_DEBUG(LOG_ECL, "stateRecvWaitEventError");
	//todo: recvfail
}

void DeviceLan::stateRecvEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateRecvEvent " << event->getType());
	switch(event->getType()) {
		case TcpIp::Event_RecvDataOk: stateRecvEventRecvDataOK(event); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: stateRecvEventError(); return;
		default: LOG_ERROR(LOG_ECL, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateRecvEventRecvDataOK(Event *event) {
#if 0
	LOG_DEBUG(LOG_ECL, "stateRecvEventRecvDataOK " << event->getUint16());
	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	resp->paramLen.set(event->getUint16());
	resp->len.set(sizeof(MasterCallRequest) - sizeof(Packet) + event->getUint16());
	sendBuf.setLen(sizeof(MasterCallRequest) + event->getUint16());
	packetLayer->sendPacket(&sendBuf);
	state = State_Wait;
#else
	LOG_DEBUG(LOG_ECL, "stateRecvEventRecvDataOK " << event->getUint16());
	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	resp->paramLen.set(event->getUint16());
	resp->len.set(sizeof(MasterCallRequest) - sizeof(Packet) + event->getUint16());
	sendBuf.setLen(sizeof(MasterCallRequest) + event->getUint16());
	packetLayer->sendPacket(&sendBuf);
	state = State_Wait;
#endif
}

void DeviceLan::stateRecvEventError() {
	LOG_DEBUG(LOG_ECL, "stateRecvEventError");
	//todo: recvfail
}

void DeviceLan::deviceLanWrite(MasterCallRequest *req) {
#if 0
	LOG_DEBUG(LOG_ECL, "deviceLanWrite");
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
		LOG_ERROR(LOG_ECL, "send fail");
		//todo: sendfail
		return;
	}
	state = State_Send;
#else
	LOG_DEBUG(LOG_ECL, "deviceLanWrite");
	reqNum = req->num.get();

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
		LOG_ERROR(LOG_ECL, "send fail");
		//todo: sendfail
		return;
	}
	state = State_Send;
#endif
}

void DeviceLan::stateSendEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateSendEvent " << event->getType());
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: stateSendEventSendDataOK(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: stateSendEventError(); return;
		default: LOG_ERROR(LOG_ECL, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateSendEventSendDataOK() {
	LOG_DEBUG(LOG_ECL, "stateSendEventSendDataOK");
	packetLayer->sendPacket(&sendBuf);
	state = State_Wait;
}

void DeviceLan::stateSendEventError() {
	LOG_DEBUG(LOG_ECL, "stateSendEventError");
	//todo: recvfail
}

void DeviceLan::deviceLanClose(MasterCallRequest *req) {
#if 0
	LOG_DEBUG(LOG_ECL, "deviceLanClose");
	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	resp->result = 0;
	resp->num.set(req->num.get() | SBERBANK_RESPONSE_FLAG);
	resp->instruction = req->instruction;
	resp->device = req->device;
	resp->reserved = 0;
	resp->paramLen.set(0);
	resp->len.set(sizeof(MasterCallRequest) - sizeof(Packet));
	sendBuf.setLen(sizeof(MasterCallRequest));

	conn->close();
	state = State_Close;
#else
	LOG_DEBUG(LOG_ECL, "deviceLanClose");
	reqNum = req->num.get();
	conn->close();
	state = State_Close;
#endif
}

void DeviceLan::stateCloseEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateCloseEvent " << event->getType());
	switch(event->getType()) {
		case TcpIp::Event_Close: stateCloseEventClose(); return;
		default: LOG_ERROR(LOG_ECL, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::stateCloseEventClose() {
	LOG_DEBUG(LOG_ECL, "stateCloseEventClose");
	sendResponse(Instruction_Close, 0);
	state = State_Idle;
}

}
#endif
