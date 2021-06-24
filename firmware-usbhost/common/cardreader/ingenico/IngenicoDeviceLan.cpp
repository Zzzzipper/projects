#include "IngenicoDeviceLan.h"

#include "logger/include/Logger.h"

namespace Ingenico {

DeviceLan::DeviceLan(
	PacketLayerInterface *packetLayer,
	TcpIp *conn,
	TimerEngine *timers
) :
	packetLayer(packetLayer),
	conn(conn),
	timers(timers),
	state(State_Idle),
	sendBuf(INGENICO_NETWORK_SIZE)
{
	if(conn) { conn->setObserver(this); }
	this->timer = timers->addTimer<DeviceLan, &DeviceLan::procTimer>(this);
}

void DeviceLan::procDeviceOpen(StringParser *parser) {
	(void)parser;
	LOG_DEBUG(LOG_ECL, "procDeviceOpen");

	sendBuf.clear();
	sendBuf.addUint8('O');
	sendBuf.addUint8('K');
	sendBuf.addUint8(':');
	sendBuf.addUint8('1');
	packetLayer->sendPacket(&sendBuf);
}

void DeviceLan::procIoctl(StringParser *parser) {
	LOG_DEBUG(LOG_ECL, "procIoctl");
	uint16_t socket = 0;
	uint32_t val1 = 0;
	uint32_t val2 = 0;
	if(parser->getNumber(&socket) == false) {
		return;
	}
	parser->skipEqual(":");
	if(parser->getNumber(&val1) == false) {
		return;
	}
	parser->skipEqual(":");
	if(parser->getNumber(&val2) == false) {
		return;
	}
	LOG_INFO(LOG_ECL, "IOCTL " << socket << ":" << val1 << ":" << val2);
	recvTimeout = val1 * 100;

	sendBuf.clear();
	sendBuf.addUint8('O');
	sendBuf.addUint8('K');
	packetLayer->sendPacket(&sendBuf);
}

void DeviceLan::procConnect(StringParser *parser) {
	LOG_DEBUG(LOG_ECL, "procConnect");
	uint16_t socket = 0;
	StringBuilder addr(20,20);
	uint16_t port = 0;
	if(parser->getNumber(&socket) == false) {
		return;
	}
	parser->skipEqual(":");
	uint16_t len = parser->getValue(":", (char*)addr.getString(), addr.getSize());
	if(len == 0) {
		return;
	}
	addr.setLen(len);
	parser->skipEqual(":");
	if(parser->getNumber(&port) == false) {
		return;
	}
	LOG_INFO(LOG_ECL, "CONNECT " << socket << ":" << addr.getString() << ":" << port);

	if(conn->connect(addr.getString(), port, TcpIp::Mode_TcpIp) == false) {
		LOG_ERROR(LOG_ECL, "recv fail");
		return;
	}
	state = State_Open;
}

void DeviceLan::procRead(StringParser *parser) {
	LOG_DEBUG(LOG_ECL, "procRead");
	uint16_t socket = 0;
	if(parser->getNumber(&socket) == false) {
		return;
	}
	parser->skipEqual(":");
	if(parser->getNumber(&recvMaxSize) == false) {
		return;
	}
	LOG_INFO(LOG_ECL, "READ " << socket << ":" << recvMaxSize);

	if(conn->hasRecvData() == true) {
		stateRecvWaitEventIncomingData();
		return;
	} else {
		gotoStateRecvWait();
		return;
	}
}

void DeviceLan::procWrite(StringParser *parser) {
	LOG_DEBUG(LOG_ECL, "procWrite");
	uint16_t socket = 0;
	if(parser->getNumber(&socket) == false) {
		return;
	}

	uint8_t *data = (uint8_t*)parser->unparsed() + 1; // skip ':'
	uint16_t dataLen = parser->unparsedLen() - 2; // exclude '\0'
	sendBuf.clear();
	sendBuf.add(data, dataLen);

	if(conn->send(sendBuf.getData(), sendBuf.getLen()) == false) {
		LOG_ERROR(LOG_HTTP, "send fail");
		stateSendEventError();
		return;
	}

	state = State_Send;
}

void DeviceLan::procDisconnect(StringParser *parser) {
	(void)parser;
	LOG_DEBUG(LOG_ECL, "procDisconnect");
	conn->close();
	state = State_Close;
}

void DeviceLan::procDeviceClose(StringParser *parser) {
	(void)parser;
	LOG_DEBUG(LOG_ECL, "procDeviceClose");
	sendBuf.clear();
	sendBuf.addUint8('O');
	sendBuf.addUint8('K');
	packetLayer->sendPacket(&sendBuf);
}

void DeviceLan::proc(Event *event) {
	LOG_DEBUG(LOG_ECL, "proc " << state);
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

void DeviceLan::procTimer() {
	LOG_DEBUG(LOG_ECL, "procTimer");
	switch(state) {
		case State_RecvWait: stateRecvWaitTimeout(); return;
		default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << state);
	}
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

	sendBuf.clear();
	sendBuf.addUint8('O');
	sendBuf.addUint8('K');
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
		case TcpIp::Event_IncomingData: LOG_DEBUG(LOG_ECL, "Incoming"); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void DeviceLan::gotoStateRecvWait() {
	LOG_DEBUG(LOG_ECL, "gotoStateRecvWait");
	timer->start(recvTimeout);
	state = State_RecvWait;
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
	timer->stop();
	sendBuf.clear();
	sendBuf.addUint8('1');
	sendBuf.addUint8(':');
	uint16_t recvSize = sendBuf.getSize() - sendBuf.getLen();
	if(recvSize > recvMaxSize) { recvSize = recvMaxSize; }
	LOG_DEBUG(LOG_ECL, "recvMaxSize=" << recvSize);
	if(conn->recv(sendBuf.getData() + sendBuf.getLen(), recvSize) == false) {
		LOG_ERROR(LOG_ECL, "recv fail");
		return;
	}
	state = State_Recv;
}

void DeviceLan::stateRecvWaitEventError() {
	LOG_DEBUG(LOG_ECL, "stateRecvWaitEventError");
	timer->stop();
	//todo: recvfail
}

void DeviceLan::stateRecvWaitTimeout() {
	LOG_DEBUG(LOG_ECL, "stateRecvWaitTimeout");
	sendBuf.clear();
	sendBuf.addUint8('1');
	sendBuf.addUint8(':');
	packetLayer->sendPacket(&sendBuf);
	state = State_Wait;
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
	sendBuf.setLen(sendBuf.getLen() + event->getUint16());
	LOG_HEX(sendBuf.getData(), sendBuf.getLen());
	packetLayer->sendPacket(&sendBuf);
	state = State_Wait;
}

void DeviceLan::stateRecvEventError() {
	LOG_DEBUG(LOG_ECL, "stateRecvEventError");
	//todo: recvfail
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
	sendBuf.clear();
	sendBuf.addUint8('O');
	sendBuf.addUint8('K');
	packetLayer->sendPacket(&sendBuf);
	state = State_Wait;
}

void DeviceLan::stateSendEventError() {
	LOG_DEBUG(LOG_ECL, "stateSendEventError");
	//todo: recvfail
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
	sendBuf.clear();
	sendBuf.addUint8('O');
	sendBuf.addUint8('K');
	packetLayer->sendPacket(&sendBuf);
	state = State_Idle;
}

}
