#include "CciCsiPacketLayer.h"

#include "CciCsiProtocol.h"
#include "logger/RemoteLogger.h"
#include "logger/include/Logger.h"

namespace CciCsi {

PacketLayer::PacketLayer(TimerEngine *timers, AbstractUart *uart) :
	timers(timers),
	uart(uart),
	observer(NULL),
	state(State_Idle),
	recvBuf(CCICSI_PACKET_MAX_SIZE),
	crcBuf(CCICSI_CRC_SIZE)
{
	this->timer = timers->addTimer<PacketLayer, &PacketLayer::procTimer>(this);
	this->uart->setReceiveHandler(this);
}

PacketLayer::~PacketLayer() {
	this->timers->deleteTimer(this->timer);
}

void PacketLayer::setObserver(PacketLayerObserver *observer) {
	this->observer = observer;
}

void PacketLayer::reset() {
	LOG_DEBUG(LOG_CCIP, "reset");
	gotoStateStx();
}

bool PacketLayer::sendPacket(Buffer *data) {
	Crc crc;
	crc.start();
	crc.add(data->getData(), data->getLen());
	crc.add(Control_ETX);

	uart->send(Control_STX);
	sendData(data->getData(), data->getLen());
	REMOTE_LOG_IN(RLOG_CCICSI, data->getData(), data->getLen());
	uart->send(Control_ETX);
	uart->send(crc.getHighByte());
	uart->send(crc.getLowByte());
	uart->send(Control_ETB);
	return true;
}

bool PacketLayer::sendControl(uint8_t byte) {
	LOG_DEBUG(LOG_CCIP, "sendControl " << byte);
	uart->send(byte);
	return true;
}

void PacketLayer::sendData(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_CCIP, "sendData");
	LOG_DEBUG_HEX(LOG_CCIP, data, dataLen);
	for(uint16_t i = 0; i < dataLen; i++) {
		uart->send(data[i]);
	}
}

void PacketLayer::skipData() {
	while(uart->isEmptyReceiveBuffer() == false) {
		uart->receive();
	}
}

void PacketLayer::handle() {
	while(uart->isEmptyReceiveBuffer() == false) {
//		LOG_TRACE(LOG_CCIP, "have data " << state);
		switch(state) {
		case State_STX: stateStxRecv(); break;
		case State_ETX: stateEtxRecv(); break;
		case State_CRC: stateCrcRecv(); break;
		default: LOG_ERROR(LOG_CCIP, "Unwaited data state=" << state << ", byte=" << uart->receive());
		}
	}
}

void PacketLayer::procTimer() {
	LOG_DEBUG(LOG_CCIP, "procTimer " << state);
	switch(state) {
	case State_ETX:
	case State_CRC: stateRecvTimeout(); break;
	default: LOG_ERROR(LOG_CCIP, "Unwaited timeout state=" << state);
	}
}

void PacketLayer::gotoStateStx() {
	LOG_DEBUG(LOG_CCIP, "gotoStateStx");
	timer->stop();
	state = State_STX;
}

void PacketLayer::stateStxRecv() {
	LOG_DEBUG(LOG_CCIP, "stateStxRecv");
	uint8_t b1 = uart->receive();
	LOG_TRACE_HEX(LOG_CCIP, b1);
	if(b1 == Control_STX) {
		gotoStateEtx();
		return;
	} else if(b1 == Control_ACK) {
		gotoStateStx();
		if(observer == NULL) {
			LOG_ERROR(LOG_CCIP, "Observer not defined");
			return;
		}
		REMOTE_LOG_OUT(RLOG_CCICSI, b1);
		observer->procControl(b1);
		return;
	} else {
		LOG_ERROR(LOG_CCIP, "Unwaited answer " << b1);
		return;
	}
}

void PacketLayer::gotoStateEtx() {
	LOG_DEBUG(LOG_CCIP, "gotoStateEtx");
	timer->start(CCICSI_RECV_TIMEOUT);
	recvBuf.clear();
	state = State_ETX;
}

void PacketLayer::stateEtxRecv() {
	LOG_DEBUG(LOG_CCIP, "stateEtxRecv");
	uint8_t b1 = uart->receive();
	LOG_TRACE_HEX(LOG_CCIP, b1);
	if(b1 == Control_STX) {
		LOG_ERROR(LOG_CCIP, "Unwaited STX");
		gotoStateEtx();
		return;
	}
	if(b1 == Control_ETX) {
		gotoStateCrc();
		return;
	}
	recvBuf.addUint8(b1);
}

void PacketLayer::gotoStateCrc() {
	LOG_DEBUG(LOG_CCIP, "gotoStateCrc");
	crcBuf.clear();
	state = State_CRC;
}

void PacketLayer::stateCrcRecv() {
	LOG_DEBUG(LOG_CCIP, "stateCrcRecv");
	uint8_t b1 = uart->receive();
	LOG_TRACE_HEX(LOG_CCIP, b1);
	if(b1 == Control_STX) {
		LOG_ERROR(LOG_CCIP, "Unwaited STX");
		gotoStateEtx();
		return;
	}
	if(crcBuf.getLen() < CCICSI_CRC_SIZE) {
		crcBuf.addUint8(b1);
		return;
	}
	if(b1 != Control_ETB) {
		LOG_ERROR(LOG_CCIP, "ETB not received");
		gotoStateStx();
		return;
	}

	Crc crc;
	crc.start();
	crc.add(recvBuf.getData(), recvBuf.getLen());
	crc.add(Control_ETX);
	if(crc.getHighByte() != crcBuf[0] || crc.getLowByte() != crcBuf[1]) {
		LOG_ERROR(LOG_CCIP, "Crc not equal" << crc.getHighByte() << "," << crc.getLowByte() << "<>" <<  crcBuf[0] << "," << crcBuf[1]);
		LOG_ERROR_HEX(LOG_CCIP, recvBuf.getData(), recvBuf.getLen());
		gotoStateStx();
		return;
	}

	gotoStateStx();
	if(observer == NULL) {
		LOG_ERROR(LOG_CCIP, "Observer not defined");
		return;
	}
	REMOTE_LOG_OUT(RLOG_CCICSI, recvBuf.getData(), recvBuf.getLen());
	observer->procPacket(recvBuf.getData(), recvBuf.getLen());
}

void PacketLayer::stateRecvTimeout() {
	LOG_DEBUG(LOG_CCIP, "stateRecvTimeout");
	gotoStateStx();
}

#if 0
void PacketLayer::gotoStateSkip(Error error) {
	LOG_DEBUG(LOG_CCIP, "gotoStateSkip " << error);
	recvError = error;
	state = State_SkipData;
}

void PacketLayer::stateSkipRecv() {
	LOG_DEBUG(LOG_CCIP, "stateSkipRecv");
	uart->receive();
}

void PacketLayer::stateSkipTimeout() {
	LOG_DEBUG(LOG_CCIP, "stateSkipTimeout");
	procRecvError(recvError);
}

void PacketLayer::procRecvError(Error error) {
	state = State_Idle;
	if(observer == NULL) {
		LOG_ERROR(LOG_CCIP, "Observer not defined");
		return;
	}
	observer->procRecvError(error);
}
#endif

}
