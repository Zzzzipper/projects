#include "IngenicoPacketLayer.h"

#include "IngenicoProtocol.h"
#include "logger/include/Logger.h"

namespace Ingenico {

PacketLayer::PacketLayer(TimerEngine *timers, AbstractUart *uart) :
	timers(timers),
	uart(uart),
	observer(NULL),
	state(State_Idle),
	recvBuf(INGENICO_PACKET_SIZE)
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
	LOG_TRACE(LOG_ECLP, "reset");
//	packetLayer->sendControl(Control_EXT);
//	packetLayer->sendControl(Control_EOT);
//	packetLayer->sendControl(Control_EOT);
	gotoStateWait();
}

bool PacketLayer::sendPacket(Buffer *data) {
	uint16_t dataLen = data->getLen();
	uart->send(Control_SOH);
	uart->send(dataLen >> 8);
	uart->send(dataLen);
	sendData(data->getData(), dataLen);
//	LOG_DEBUG_HEX(LOG_ECLP, (uint8_t)Control_SOH);
//	LOG_DEBUG_HEX(LOG_ECLP, (uint8_t)(dataLen >> 8));
//	LOG_DEBUG_HEX(LOG_ECLP, (uint8_t)dataLen);
//	LOG_DEBUG_HEX(LOG_ECLP, data->getData(), dataLen);
	LOG_INFO(LOG_ECLP, ">>" << dataLen);
	LOG_INFO_STR(LOG_ECLP, data->getData(), dataLen);
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
	while(uart->isEmptyReceiveBuffer() == false) {
		LOG_TRACE(LOG_ECLP, "have data " << state);
		switch(state) {
		case State_SOH: stateSohRecv(); break;
		case State_Length: stateLengthRecv(); break;
		case State_Data: stateDataRecv(); break;
		default: LOG_ERROR(LOG_ECLP, "Unwaited data state=" << state << ", byte=" << uart->receive());
		}
	}
}

void PacketLayer::procTimer() {
	LOG_DEBUG(LOG_ECLP, "procTimer " << state);
	switch(state) {
	case State_Length:
	case State_Data: stateRecvTimeout(); break;
	default: LOG_ERROR(LOG_ECLP, "Unwaited timeout state=" << state);
	}
}

void PacketLayer::gotoStateWait() {
	LOG_DEBUG(LOG_ECLP, "gotoStateWait");
	timer->stop();
	state = State_SOH;
}

void PacketLayer::stateSohRecv() {
	LOG_DEBUG(LOG_ECLP, "stateStxRecv");
	uint8_t b1 = uart->receive();
	LOG_DEBUG_HEX(LOG_ECLP, b1);
	if(b1 == Control_SOH) {
		gotoStateLength();
		return;
	} else {
		LOG_ERROR(LOG_ECLP, "Unwaited answer " << b1);
		return;
	}
}

void PacketLayer::gotoStateLength() {
	LOG_DEBUG(LOG_ECLP, "gotoStateLength");
	timer->start(INGENICO_RECV_TIMEOUT);
	recvBuf.clear();
	state = State_Length;
}

void PacketLayer::stateLengthRecv() {
	LOG_DEBUG(LOG_ECLP, "stateLengthRecv");
	uint8_t b1 = uart->receive();
	LOG_DEBUG_HEX(LOG_ECLP, b1);
	recvBuf.addUint8(b1);
	if(recvBuf.getLen() < 2) {
		return;
	}

	recvLength = (recvBuf[0] << 8) | recvBuf[1];
	LOG_DEBUG(LOG_ECLP, "responseLength=" << recvLength);
	gotoStateData();
}

void PacketLayer::gotoStateData() {
	LOG_DEBUG(LOG_ECLP, "gotoStateData");
	recvBuf.clear();
	state = State_Data;
}

void PacketLayer::stateDataRecv() {
	LOG_DEBUG(LOG_ECLP, "stateDataRecv");
	recvBuf.addUint8(uart->receive());
	if(recvBuf.getLen() >= recvLength) {
//		LOG_DEBUG_HEX(LOG_ECLP, recvBuf.getData(), recvBuf.getLen());
		LOG_INFO(LOG_ECLP, "<<" << recvBuf.getLen() << "/" << recvLength);
		LOG_INFO_STR(LOG_ECLP, recvBuf.getData(), recvBuf.getLen());
		recvBuf.addUint8('\0');
		gotoStateWait();
		observer->procPacket(recvBuf.getData(), recvBuf.getLen());
		return;
	}
}

void PacketLayer::stateRecvTimeout() {
	LOG_DEBUG(LOG_ECLP, "stateRecvTimeout");
	LOG_INFO(LOG_ECLP, "<<TIMEOUT " << recvBuf.getLen() << "/" << recvLength);
	LOG_INFO_STR(LOG_ECLP, recvBuf.getData(), recvBuf.getLen());
	gotoStateWait();
}

}
