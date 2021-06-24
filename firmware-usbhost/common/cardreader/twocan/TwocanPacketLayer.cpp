#include "TwocanPacketLayer.h"

#include "TwocanProtocol.h"
#include "logger/include/Logger.h"

namespace Twocan {

PacketLayer::PacketLayer(TimerEngine *timers, AbstractUart *uart) :
	timers(timers),
	uart(uart),
	observer(NULL),
	state(State_Idle),
	recvBuf(TWOCAN_PACKET_SIZE),
	crcBuf(TWOCAN_CRC_SIZE)
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
	LOG_DEBUG(LOG_ECLP, "reset");
	gotoStateWait();
}

bool PacketLayer::sendPacket(Buffer *data) {
	LOG_DEBUG(LOG_ECLP, "sendPacket size " << data->getLen());
	uint8_t order = 1;
	sendData(data->getData(), data->getLen());
	return true;
}

bool PacketLayer::sendControl(uint8_t byte) {
	LOG_DEBUG(LOG_ECLP, "sendPacket");
	LOG_ERROR_HEX(LOG_ECLT, byte);
	uart->send(byte);
	return true;
}

void PacketLayer::sendData(uint8_t *data, uint16_t dataLen) {
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
#if 1
	while(uart->isEmptyReceiveBuffer() == false) {
		LOG_TRACE(LOG_ECLP, "have data " << state);
		switch(state) {
		case State_STX: stateStxRecv(); break;
		case State_Data: stateDataRecv(); break;
#if 0
		case State_SkipData: stateSkipRecv(); break;
#endif
		default: LOG_ERROR(LOG_ECLP, "Unwaited data state=" << state << ", byte=" << uart->receive());
		}
	}
#else
	while(uart->isEmptyReceiveBuffer() == false) {
		uint8_t b1 = uart->receive();
		LOG_HEX(b1);
	}
#endif
}

void PacketLayer::procTimer() {
	LOG_DEBUG(LOG_ECLP, "procTimer " << state);
	switch(state) {
	case State_Length:
	case State_Data:
	case State_CRC: stateRecvTimeout(); break;
	default: LOG_ERROR(LOG_ECLP, "Unwaited timeout state=" << state);
	}
}

void PacketLayer::gotoStateWait() {
	LOG_DEBUG(LOG_ECLP, "gotoStateWait");
	timer->stop();
	state = State_STX;
}

void PacketLayer::stateStxRecv() {
	uint8_t b1 = uart->receive();
	LOG_DEBUG(LOG_ECLP, "stateStxRecv, b1 = " << b1);
	if(b1 == Control_STX) {
		gotoStateData();
		return;
	} else if(b1 == Control_ACK || b1 == Control_EOT) {
		 gotoStateWait();
		if(observer == NULL) {
			LOG_ERROR(LOG_ECLP, "Observer not defined");
			return;
		}
		LOG_ERROR_HEX(LOG_ECLT, b1);
		observer->procControl(b1);
		return;
	} else {
		LOG_ERROR(LOG_ECLP, "Unwaited answer " << b1);
		return;
	}
}

void PacketLayer::gotoStateLength() {
	LOG_DEBUG(LOG_ECLP, "gotoStateLength");
}

void PacketLayer::stateLengthRecv() {
	LOG_DEBUG(LOG_ECLP, "stateLengthRecv");
}

void PacketLayer::gotoStateData() {
	LOG_DEBUG(LOG_ECLP, "gotoStateData");
	recvBuf.clear();
	state = State_Data;
}

void PacketLayer::stateDataRecv() {
	uint8_t b = uart->receive();
	LOG_DEBUG(LOG_ECLP, "stateDataRecv, b = " << b);
	recvBuf.clear();
	recvBuf.addUint8(b);
	if(observer == NULL) {
		LOG_ERROR(LOG_ECLP, "Observer not defined");
		return;
	}
	observer->procPacket(recvBuf.getData(), recvBuf.getLen());
	gotoStateWait();
}

void PacketLayer::gotoStateCrc() {
	LOG_DEBUG(LOG_ECLP, "gotoStateCrc");
}

void PacketLayer::stateCrcRecv() {
	LOG_DEBUG(LOG_ECLP, "stateCrcRecv");
}

void PacketLayer::stateRecvTimeout() {
	LOG_DEBUG(LOG_ECLP, "stateRecvTimeout");
	gotoStateWait();
}

}
