#if 1
#include "AtolPacketLayer.h"

#include "fiscal_register/atol/AtolProtocol.h"
#include "logger/include/Logger.h"

namespace Atol {

PacketParser::PacketParser() :
	data(ATOL_PACKET_MAX_SIZE)
{

}

void PacketParser::start() {
	gotoStateSTX();
}

void PacketParser::procData(Buffer *data) {
	LOG_DEBUG(LOG_FRP, "procData " << data->getLen());
	for(uint16_t i = 0; i < data->getLen(); i++) {
		uint8_t b = (*data)[i];
		switch(state) {
		case State_Stx: stateStxSymbol(b); break;
		case State_Header: stateHeaderSymbol(b); break;
		case State_Data: stateDataSymbol(b); break;
		case State_Crc: stateCrcSymbol(b); break;
		default:;
		}
	}
}

bool PacketParser::isComplete() {
	return state == State_Complete;
}

uint8_t PacketParser::getPackeId() {
	return packetId;
}

uint8_t *PacketParser::getData() {
	return data.getData();
}

uint16_t PacketParser::getDataLen() {
	return data.getLen();
}

void PacketParser::gotoStateSTX() {
	data.clear();
	state = State_Stx;
}

void PacketParser::stateStxSymbol(uint8_t b) {
	if(b != Control_STX) {
		return;
	}
	data.addUint8(b);
	state = State_Header;
}

//todo: проверка на допустимые значения len и packetId
void PacketParser::stateHeaderSymbol(uint8_t b) {
	if(b == Control_STX) {
		LOG_WARN(LOG_FRP, "Unwaited STX");
		procUnwaitedSTX();
		return;
	}
	if(b == Control_ESC) {
		LOG_WARN(LOG_FRP, "Unwaited ESC");
		gotoStateSTX();
		return;
	}

	data.addUint8(b);
	if(data.getLen() < sizeof(PacketHeader)) {
		return;
	}

	PacketHeader *header = (PacketHeader*)data.getData();
	dataLen = header->getDataLen();
	packetId = header->id;
	data.clear();
	flagESC = false;
	crc.start();
	crc.add(packetId);
	if(dataLen == 0) {
		state = State_Crc;
	} else {
		state = State_Data;
	}
}

void PacketParser::stateDataSymbol(uint8_t b) {
	if(b == Control_STX) {
		LOG_ERROR(LOG_FRP, "Unwaited STX");
		procUnwaitedSTX();
		return;
	}

	crc.add(b);
	if(b == Control_ESC) {
		if(flagESC == false) {
			flagESC = true;
			return;
		} else {
			LOG_ERROR(LOG_FRP, "Unwaited ESC");
			gotoStateSTX();
			return;
		}
	}
	if(flagESC == true) {
		if(b == Control_TSTX) {
			data.addUint8(Control_STX);
			flagESC = false;
		} else if(b == Control_TESC) {
			data.addUint8(Control_ESC);
			flagESC = false;
		} else {
			LOG_ERROR(LOG_FRP, "Unwaited symbol");
			gotoStateSTX();
			return;
		}
	} else {
		data.addUint8(b);
	}
	if(data.getLen() >= dataLen) {
		state = State_Crc;
	}
}

void PacketParser::stateCrcSymbol(uint8_t b) {
	LOG_DEBUG(LOG_FRP, "stateCrcSymbol");
	if(b == Control_STX) {
		LOG_ERROR(LOG_FRP, "Unwaited STX");
		procUnwaitedSTX();
		return;
	}
	if(b == Control_ESC) {
		if(flagESC == false) {
			flagESC = true;
			return;
		} else {
			LOG_ERROR(LOG_FRP, "Unwaited ESC");
			gotoStateSTX();
			return;
		}
	}
	if(flagESC == true) {
		if(b == Control_TSTX) {
			b = Control_STX;
			flagESC = false;
		} else if(b == Control_TESC) {
			b = Control_ESC;
			flagESC = false;
		} else {
			LOG_ERROR(LOG_FRP, "Unwaited symbol");
			gotoStateSTX();
			return;
		}
	}
	if(crc.getCrc() != b) {
		LOG_ERROR(LOG_FRP, "Wrong CRC " << crc.getCrc() << "<>" << b);
		gotoStateSTX();
		return;
	}

	state = State_Complete;
}

void PacketParser::procUnwaitedSTX() {
	LOG_DEBUG(LOG_FRP, "procUnwaitedSTX");
	data.clear();
	data.addUint8(Control_STX);
	state = State_Header;
}

PacketLayer::PacketLayer(TimerEngine *timers, TcpIp *conn) :
	timers(timers),
	conn(conn),
	observer(NULL),
	state(State_Idle),
	sendPacketId(ATOL_PACKET_ID_MIN_NUMBER),
	sendBuf(ATOL_PACKET_MAX_SIZE),
	recvBuf(ATOL_PACKET_MAX_SIZE)
{
	this->recvTimer = timers->addTimer<PacketLayer, &PacketLayer::procRecvTimer>(this);
	this->conn->setObserver(this);
}

PacketLayer::~PacketLayer() {
	timers->deleteTimer(this->recvTimer);
}

void PacketLayer::setObserver(PacketLayerObserver *observer) {
	this->observer = observer;
}

bool PacketLayer::connect(const char *domainname, uint16_t port, TcpIp::Mode mode) {
	LOG_INFO(LOG_FRP, "connect");
	if(state != State_Idle) {
		LOG_ERROR(LOG_FRP, "Wrong state " << state);
		return false;
	}
	if(conn->connect(domainname, port, mode) == false) {
		LOG_ERROR(LOG_FRP, "Connect failed");
		return false;
	}
	state = State_Connect;
	return true;
}

bool PacketLayer::disconnect() {
	LOG_INFO(LOG_FRP, "disconnect");
	conn->close();
	state = State_Disconnect;
	return true;
}

bool PacketLayer::sendPacket(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FRP, "sendPacket");
	if(state != State_Wait) {
		LOG_ERROR(LOG_FRP, "Wrong state " << state);
		return false;
	}

	incSendPacketId();
	PacketHeader header;
	header.set(sendPacketId, dataLen);
	sendBuf.clear();
	sendBuf.addUint8(header.stx);
	sendBuf.addUint8(header.len[0]);
	sendBuf.addUint8(header.len[1]);
	sendBuf.addUint8(header.id);
	Crc crc;
	crc.start();
	crc.add(header.id);
	for(uint16_t i = 0; i < dataLen; i++) {
		uint8_t b1 = data[i];
		if(b1 == Control_STX) {
			crc.add(Control_ESC);
			crc.add(Control_TSTX);
			sendBuf.addUint8(Control_ESC);
			sendBuf.addUint8(Control_TSTX);
		} else if(b1 == Control_ESC) {
			crc.add(Control_ESC);
			crc.add(Control_TESC);
			sendBuf.addUint8(Control_ESC);
			sendBuf.addUint8(Control_TESC);
		} else {
			crc.add(b1);
			sendBuf.addUint8(b1);
		}
	}
	uint8_t b2 = crc.getCrc();
	if(b2 == Control_STX) {
		sendBuf.addUint8(Control_ESC);
		sendBuf.addUint8(Control_TSTX);
	} else if(b2 == Control_ESC) {
		sendBuf.addUint8(Control_ESC);
		sendBuf.addUint8(Control_TESC);
	} else {
		sendBuf.addUint8(b2);
	}

	if(conn->send(sendBuf.getData(), sendBuf.getLen()) == false) {
		LOG_ERROR(LOG_FRP, "Send failed");
		return false;
	}

	LOG_DEBUG(LOG_FRP, "sended " << sendPacketId);
	state = State_Send;
	return true;
}

void PacketLayer::incSendPacketId() {
	sendPacketId++;
	if(sendPacketId > ATOL_PACKET_ID_MAX_NUMBER) {
		sendPacketId = ATOL_PACKET_ID_MIN_NUMBER;
	}
}

void PacketLayer::proc(Event *event) {
	LOG_DEBUG(LOG_FRP, "proc " << state);
	switch(state) {
	case State_Connect: stateConnectEvent(event); break;
	case State_Wait: stateWaitEvent(event); break;
	case State_Send: stateSendEvent(event); break;
	case State_Recv: stateRecvEvent(event); break;
	case State_Disconnect: stateDisconnectEvent(event); break;
	default: LOG_ERROR(LOG_FRP, "Unwaited data state=" << state << "," << event->getType());
	}
}

void PacketLayer::procRecvTimer() {
	LOG_DEBUG(LOG_FRP, "procRecvTimer");
	observer->procError(PacketLayerObserver::Error_RecvTimeout);
}

void PacketLayer::stateConnectEvent(Event *event) {
	LOG_DEBUG(LOG_FRP, "stateConnectEvent" << event->getType());
	switch(event->getType()) {
	case TcpIp::Event_ConnectOk: {
		state = State_Wait;
		observer->procError(PacketLayerObserver::Error_OK);
		return;
	}
	case TcpIp::Event_ConnectError:
	case TcpIp::Event_Close: {
		state = State_Idle;
		observer->procError(PacketLayerObserver::Error_ConnectFailed);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

void PacketLayer::stateWaitEvent(Event *event) {
	LOG_DEBUG(LOG_FRP, "stateWaitEvent");
	switch(event->getType()) {
	case TcpIp::Event_IncomingData: {
		gotoStateRecv();
		return;
	}
	case TcpIp::Event_Close: {
		state = State_Idle;
		observer->procError(PacketLayerObserver::Error_RemoteClose);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

void PacketLayer::stateSendEvent(Event *event) {
	LOG_DEBUG(LOG_FRP, "stateSendEvent " << event->getType());
	switch(event->getType()) {
	case TcpIp::Event_SendDataOk: {
		gotoStateRecv();
		return;
	}
	case TcpIp::Event_SendDataError: { //TODO: disconnect
		state = State_Idle;
		observer->procError(PacketLayerObserver::Error_SendFailed);
		return;
	}
	case TcpIp::Event_Close: {
		state = State_Idle;
		observer->procError(PacketLayerObserver::Error_RemoteClose);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

void PacketLayer::gotoStateRecv() {
	LOG_DEBUG(LOG_FRP, "gotoStateRecv");
	recvBuf.clear();
	if(conn->recv(recvBuf.getData(), recvBuf.getSize()) == false) {
		recvTimer->stop();
		state = State_Idle;
		observer->procError(PacketLayerObserver::Error_RecvFailed);
		return;
	}
	parser.start();
	recvTimer->start(ATOL_PACKET_TIMEOUT);
	state = State_Recv;
}

void PacketLayer::stateRecvEvent(Event *event) {
	LOG_DEBUG(LOG_FRP, "stateRecvEvent" << event->getType());
	switch(event->getType()) {
	case TcpIp::Event_RecvDataOk: stateRecvEventRecvData(event); break;
	case TcpIp::Event_RecvDataError: {
		recvTimer->stop();
		state = State_Idle;
		observer->procError(PacketLayerObserver::Error_RecvFailed);
		return;
	}
	case TcpIp::Event_Close: {
		recvTimer->stop();
		state = State_Idle;
		observer->procError(PacketLayerObserver::Error_RemoteClose);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

void PacketLayer::stateRecvEventRecvData(Event *event) {
	LOG_DEBUG(LOG_FRP, "stateRecvEventRecvData");
	recvBuf.setLen(event->getUint16());
	parser.procData(&recvBuf);
	if(parser.isComplete() == false) {
		recvBuf.clear();
		if(conn->recv(recvBuf.getData(), recvBuf.getSize()) == false) {
			recvTimer->stop();
			state = State_Idle;
			observer->procError(PacketLayerObserver::Error_RecvFailed);
			return;
		}
	} else {
		recvTimer->stop();
		state = State_Wait;
		LOG_DEBUG_HEX(LOG_FRP, parser.getData(), parser.getDataLen());
		observer->procRecvData(parser.getPackeId(), parser.getData(), parser.getDataLen());
	}
}

void PacketLayer::stateDisconnectEvent(Event *event) {
	LOG_DEBUG(LOG_FRP, "stateDisconnectEvent");
	switch(event->getType()) {
	case TcpIp::Event_Close: {
		state = State_Idle;
		observer->procError(PacketLayerObserver::Error_OK);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

}
#else
#include "AtolPacketLayer.h"

#include "fiscal_register/atol/AtolProtocol.h"
#include "logger/include/Logger.h"

#define RECV_TIMEOUT 20000

namespace Atol {

PacketLayer::PacketLayer(TimerEngine *timers, TcpIp *conn) :
	timers(timers),
	conn(conn),
	observer(NULL),
	state(State_STX),
	sendPacketId(0),
	flagESC(false)
{
	this->recvBuf = new Buffer(ATOL_PACKET_MAX_SIZE);
	this->recvTimer = timers->addTimer<PacketLayer, &PacketLayer::procRecvTimer>(this);
	this->uart->setReceiveHandler(this);
}

PacketLayer::~PacketLayer() {
	timers->deleteTimer(this->recvTimer);
	delete this->recvBuf;
}

void PacketLayer::setObserver(PacketLayerObserver *observer) {
	this->observer = observer;
}

uint8_t PacketLayer::sendPacket(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FRP, "sendPacket");
	incSendPacketId();
	PacketHeader header;
	header.set(sendPacketId, dataLen);
	this->uart->send(header.stx);
	this->uart->send(header.len[0]);
	this->uart->send(header.len[1]);
	Crc crc;
	crc.start();
	crc.add(header.id);
	this->uart->send(header.id);
	for(uint16_t i = 0; i < dataLen; i++) {
		uint8_t b1 = data[i];
		if(b1 == Control_STX) {
			crc.add(Control_ESC);
			this->uart->send(Control_ESC);
			crc.add(Control_TSTX);
			this->uart->send(Control_TSTX);
		} else if(b1 == Control_ESC) {
			crc.add(Control_ESC);
			this->uart->send(Control_ESC);
			crc.add(Control_TESC);
			this->uart->send(Control_TESC);
		} else {
			crc.add(b1);
			this->uart->send(b1);
		}
	}
	uint8_t b2 = crc.getCrc();
	if(b2 == Control_STX) {
		this->uart->send(Control_ESC);
		this->uart->send(Control_TSTX);
	} else if(b2 == Control_ESC) {
		this->uart->send(Control_ESC);
		this->uart->send(Control_TESC);
	} else {
		this->uart->send(b2);
	}
	recvTimer->start(RECV_TIMEOUT);
	LOG_DEBUG(LOG_FRP, "sended #" << sendPacketId);
	return sendPacketId;
}

void PacketLayer::incSendPacketId() {
	sendPacketId++;
	if(sendPacketId > ATOL_PACKET_ID_MAX_NUMBER) {
		sendPacketId = 0;
	}
}

void PacketLayer::handle() {
	LOG_TRACE(LOG_FRP, "handle");
	while(uart->isEmptyReceiveBuffer() == false) {
		LOG_TRACE(LOG_FRP, "have data");
		switch(state) {
		case State_STX: stateSTXHandle(); break;
		case State_Len0: stateLen0Handle(); break;
		case State_Len1: stateLen1Handle(); break;
		case State_Id: stateIdHandle(); break;
		case State_Data: stateDataHandle(); break;
		case State_CRC: stateCRCHandle(); break;
		default: LOG_ERROR(LOG_FRP, "Unwaited data state=" << state << ", byte=" << uart->receive());
		}
	}
}

void PacketLayer::procRecvTimer() {
	LOG_DEBUG(LOG_FRP, "procRecvTimer");
	gotoStateSTX();
	observer->procRecvError(PacketLayerObserver::Error_ResponseTimeout);
}

void PacketLayer::gotoStateSTX() {
	LOG_DEBUG(LOG_FRP, "gotoStateSTX");
	recvTimer->stop();
	state = State_STX;
}

void PacketLayer::stateSTXHandle() {
	LOG_DEBUG(LOG_FRP, "stateSTXHandle");
	uint8_t b1 = uart->receive();
	if(b1 == Control_STX) {
		recvBuf->clear();
		recvBuf->addUint8(b1);
		state = State_Len0;
		return;
	}
}

void PacketLayer::procUnwaitedSTX() {
	LOG_DEBUG(LOG_FRP, "procUnwaitedSTX");
	recvBuf->clear();
	recvBuf->addUint8(Control_STX);
	state = State_Len0;
}

void PacketLayer::stateLen0Handle() {
	LOG_DEBUG(LOG_FRP, "stateLen0Handle");
	uint8_t b1 = uart->receive();
	if(b1 == Control_STX) {
		LOG_WARN(LOG_FRP, "Unwaited STX");
		procUnwaitedSTX();
		return;
	}
	if(b1 == Control_ESC) {
		LOG_WARN(LOG_FRP, "Unwaited ESC");
		gotoStateSTX();
		return;
	}
	recvBuf->addUint8(b1);
	state = State_Len1;
}

void PacketLayer::stateLen1Handle() {
	LOG_DEBUG(LOG_FRP, "stateLen1Handle");
	uint8_t b1 = uart->receive();
	if(b1 == Control_STX) {
		LOG_WARN(LOG_FRP, "Unwaited STX");
		procUnwaitedSTX();
		return;
	}
	if(b1 == Control_ESC) {
		LOG_WARN(LOG_FRP, "Unwaited ESC");
		gotoStateSTX();
		return;
	}
	recvBuf->addUint8(b1);
	state = State_Id;
}

void PacketLayer::stateIdHandle() {
	LOG_DEBUG(LOG_FRP, "stateIdHandle");
	uint8_t b1 = uart->receive();
	if(b1 == Control_STX) {
		LOG_WARN(LOG_FRP, "Unwaited STX");
		procUnwaitedSTX();
		return;
	}
	if(b1 == Control_ESC) {
		LOG_WARN(LOG_FRP, "Unwaited ESC");
		gotoStateSTX();
		return;
	}
	recvBuf->addUint8(b1);
	PacketHeader *header = (PacketHeader *)recvBuf->getData();
	recvPacketId = header->id;
	recvSize = header->getDataLen();
	recvCrc.start();
	recvCrc.add(recvPacketId);
	LOG_DEBUG(LOG_FRP, "received #" << recvPacketId << " " << recvBuf->getLen());
	LOG_TRACE_HEX(LOG_FRP, recvBuf->getData(), recvBuf->getLen());
	recvBuf->clear();
	flagESC = false;
	if(recvSize == 0) {
		state = State_CRC;
	} else {
		state = State_Data;
	}
}

void PacketLayer::stateDataHandle() {
	LOG_DEBUG(LOG_FRP, "stateDataHandle");
	uint8_t b1 = uart->receive();
	if(b1 == Control_STX) {
		LOG_WARN(LOG_FRP, "Unwaited STX");
		procUnwaitedSTX();
		return;
	}
	recvCrc.add(b1);
	if(b1 == Control_ESC) {
		if(flagESC == false) {
			flagESC = true;
			return;
		} else {
			LOG_WARN(LOG_FRP, "Unwaited ESC");
			gotoStateSTX();
			return;
		}
	}
	if(flagESC == true) {
		if(b1 == Control_TSTX) {
			recvBuf->addUint8(Control_STX);
			flagESC = false;
		} else if(b1 == Control_TESC) {
			recvBuf->addUint8(Control_ESC);
			flagESC = false;
		} else {
			LOG_WARN(LOG_FRP, "Unwaited symbol");
			gotoStateSTX();
			return;
		}
	} else {
		recvBuf->addUint8(b1);
	}
	if(recvBuf->getLen() >= recvSize) {
		state = State_CRC;
	}
}

void PacketLayer::stateCRCHandle() {
	LOG_DEBUG(LOG_FRP, "stateCRCHandle");
	uint8_t b1 = uart->receive();
	if(b1 == Control_STX) {
		LOG_WARN(LOG_FRP, "Unwaited STX");
		procUnwaitedSTX();
		return;
	}
	if(b1 == Control_ESC) {
		if(flagESC == false) {
			flagESC = true;
			return;
		} else {
			LOG_WARN(LOG_FRP, "Unwaited ESC");
			gotoStateSTX();
			return;
		}
	}
	if(flagESC == true) {
		if(b1 == Control_TSTX) {
			b1 = Control_STX;
			flagESC = false;
		} else if(b1 == Control_TESC) {
			b1 = Control_ESC;
			flagESC = false;
		} else {
			LOG_WARN(LOG_FRP, "Unwaited symbol");
			gotoStateSTX();
			return;
		}
	}
	if(recvCrc.getCrc() != b1) {
		LOG_WARN(LOG_FRP, "Wrong CRC");
		return;
	}
	gotoStateSTX();
	observer->procRecvData(recvPacketId, recvBuf->getData(), recvBuf->getLen());
}

}
#endif
