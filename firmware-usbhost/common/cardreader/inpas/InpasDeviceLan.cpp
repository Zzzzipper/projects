#include "InpasDeviceLan.h"

#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"

namespace Inpas {

DeviceLan::DeviceLan(
	PacketLayerInterface *packetLayer,
	TcpIp *conn
) :
	packetLayer(packetLayer),
	conn(conn),
	state(State_Idle),
	resp(INPAS_PACKET_SIZE)
{
	if(conn != NULL) { conn->setObserver(this); }
}

void DeviceLan::procRequest(TlvPacket *req) {
	LOG_DEBUG(LOG_ECL, "deviceLan");
	if(conn == NULL) {
		LOG_ERROR(LOG_ECL, "conn not inited");
		return;
	}

	uint16_t type;
	if(req->getNumber(Tlv_CommandType, &type) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		return;
	}

	uint16_t mode;
	if(req->getNumber(Tlv_CommandMode, &mode) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		return;
	}

	if(type == CommandType_Conn && mode == CommandConn_Open) {
		deviceLanOpen(req);
		return;
	}
	if(type == CommandType_Data && mode == CommandData_Send) {
		deviceLanSend(req);
		return;
	}
	if(type == CommandType_Data && mode == CommandData_Recv) {
		deviceLanRecv(req);
		return;
	}
	if(type == CommandType_Conn && mode == CommandConn_Close) {
		deviceLanClose(req);
		return;
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

void DeviceLan::deviceLanOpen(TlvPacket *req) {
	LOG_DEBUG(LOG_ECL, "deviceLanOpen");
	StringBuilder param(32, 32);
	if(req->getString(Tlv_CommandData, &param) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		return;
	}

	StringParser parser(param.getString());
	char addr[16];
	memset(addr, 0, sizeof(addr));
	if(parser.getValue(";", addr, (sizeof(addr) - 1)) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		return;
	}
	parser.skipEqual(";");
	uint16_t port;
	if(parser.getNumber(&port) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		return;
	}

	conn->connect(addr, port, TcpIp::Mode_TcpIp);
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
	resp.clear();
	resp.addNumber(Tlv_OperationCode, 2, Operation_Net);
	resp.addNumber(Tlv_CommandType, 2, CommandType_Conn);
	resp.addNumber(Tlv_CommandResult, 1, 0);
	packetLayer->sendPacket(resp.getBuf());
	state = State_Wait;
}

void DeviceLan::stateOpenEventConnectError() {
	LOG_DEBUG(LOG_ECL, "stateOpenEventConnectError");
	resp.clear();
	resp.addNumber(Tlv_OperationCode, 1, Operation_Net);
	resp.addNumber(Tlv_CommandType, 2, CommandType_Conn);
	resp.addNumber(Tlv_CommandResult, 1, 1);
	packetLayer->sendPacket(resp.getBuf());
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

void DeviceLan::deviceLanSend(TlvPacket *req) {
	LOG_DEBUG(LOG_ECL, "deviceLanSend");
	TlvHeader *param = req->find(Tlv_CommandData);
	if(param == NULL) {
		LOG_ERROR(LOG_ECL, "Bad format");
		return;
	}

	if(conn->send(param->data, param->len.get()) == false) {
		LOG_ERROR(LOG_HTTP, "send fail");
		stateSendEventError();
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
	resp.clear();
	resp.addNumber(Tlv_OperationCode, 2, Operation_Net);
	resp.addNumber(Tlv_CommandType, 2, CommandType_Data);
	resp.addNumber(Tlv_CommandResult, 1, 0);
	packetLayer->sendPacket(resp.getBuf());
	state = State_Wait;
}

void DeviceLan::stateSendEventError() {
	LOG_DEBUG(LOG_ECL, "stateSendEventError");
	resp.clear();
	resp.addNumber(Tlv_OperationCode, 2, Operation_Net);
	resp.addNumber(Tlv_CommandType, 2, CommandType_Data);
	resp.addNumber(Tlv_CommandResult, 1, 1);
	packetLayer->sendPacket(resp.getBuf());
	state = State_Wait; //State_Idle;
}

void DeviceLan::deviceLanRecv(TlvPacket *req) {
	(void)req;
	LOG_DEBUG(LOG_ECL, "deviceLanRecv");
	if(conn->hasRecvData() == true) {
		resp.clear();
		resp.addNumber(Tlv_OperationCode, 2, Operation_Net);
		resp.addNumber(Tlv_CommandType, 2, CommandType_Data);
		resp.addNumber(Tlv_CommandResult, 1, 0);
		uint8_t *b = resp.getBuf()->getData() + resp.getBuf()->getLen() + 3;
		uint16_t bsize = resp.getBuf()->getSize() - resp.getBuf()->getLen() - 3;
		if(conn->recv(b, bsize) == false) {
			LOG_ERROR(LOG_ECL, "recv fail");
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
	resp.clear();
	resp.addNumber(Tlv_OperationCode, 2, Operation_Net);
	resp.addNumber(Tlv_CommandType, 2, CommandType_Data);
	resp.addNumber(Tlv_CommandResult, 1, 0);
	uint8_t *b = resp.getBuf()->getData() + resp.getBuf()->getLen() + 3;
	uint16_t bsize = resp.getBuf()->getSize() - resp.getBuf()->getLen() - 3;
	if(conn->recv(b, bsize) == false) {
		LOG_ERROR(LOG_ECL, "recv fail");
		return;
	}
	state = State_Recv;
}

void DeviceLan::stateRecvWaitEventError() {
	LOG_DEBUG(LOG_ECL, "stateRecvWaitEventError");
	resp.clear();
	resp.addNumber(Tlv_OperationCode, 2, Operation_Net);
	resp.addNumber(Tlv_CommandType, 2, CommandType_Data);
	resp.addNumber(Tlv_CommandResult, 1, 1);
	packetLayer->sendPacket(resp.getBuf());
	state = State_Wait; //State_Idle;
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
	TlvHeader *h = (TlvHeader*)(resp.getBuf()->getData() + resp.getBuf()->getLen());
	h->type = Tlv_CommandData;
	h->len.set(event->getUint16());
	resp.getBuf()->setLen(resp.getBuf()->getLen() + 3 + event->getUint16());
	packetLayer->sendPacket(resp.getBuf());
	state = State_Wait;
}

void DeviceLan::stateRecvEventError() {
	LOG_DEBUG(LOG_ECL, "stateRecvEventError");
	resp.clear();
	resp.addNumber(Tlv_OperationCode, 2, Operation_Net);
	resp.addNumber(Tlv_CommandType, 2, CommandType_Data);
	resp.addNumber(Tlv_CommandResult, 1, 1);
	packetLayer->sendPacket(resp.getBuf());
	state = State_Idle;
}

void DeviceLan::deviceLanClose(TlvPacket *req) {
	(void)req;
	LOG_DEBUG(LOG_ECL, "deviceLanClose");
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
	resp.clear();
	resp.addNumber(Tlv_OperationCode, 2, Operation_Net);
	resp.addNumber(Tlv_CommandType, 2, CommandType_Conn);
	resp.addNumber(Tlv_CommandResult, 1, 0);
	packetLayer->sendPacket(resp.getBuf());
	state = State_Idle;
}

}
