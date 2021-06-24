#include "VendotekPacketLayer.h"

#include "VendotekProtocol.h"
#include "logger/include/Logger.h"

namespace Vendotek {

PacketLayer::PacketLayer(TimerEngine *timers, AbstractUart *uart) :
	timers(timers),
	uart(uart),
	observer(NULL),
	state(State_Idle),
	recvBuf(VENDOTEK_PACKET_SIZE),
	crcBuf(VENDOTEK_CRC_SIZE)
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
	gotoStateWait();
}

bool PacketLayer::sendPacket(Buffer *data) {
	uint16_t dataLen = data->getLen() + 2;

	Crc crc;
	crc.start();
	crc.add(Control_START);
	crc.add(dataLen >> 8);
	crc.add(dataLen);
	crc.add(Discriminator_VMC >> 8);
	crc.add(Discriminator_VMC & 0xFF);
	crc.add(data->getData(), data->getLen());

	uart->send(Control_START);
	uart->send(dataLen >> 8);
	uart->send(dataLen);
	uart->send(Discriminator_VMC >> 8);
	uart->send(Discriminator_VMC & 0xFF);
	sendData(data->getData(), data->getLen());
	uart->send(crc.getHighByte());
	uart->send(crc.getLowByte());

#if LOG_ECLP >= LOG_LEVEL_TRACE
	LOG(">>>" << data->getLen());
	LOG_HEX(data->getData(), data->getLen());
#endif
	return true;
}

bool PacketLayer::sendControl(uint8_t byte) {
	LOG_DEBUG(LOG_ECLP, "sendPacket");
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
	LOG_TRACE(LOG_ECLP, "handle");
	while(uart->isEmptyReceiveBuffer() == false) {
		LOG_TRACE(LOG_ECLP, "have data " << state);
		switch(state) {
		case State_Start: stateStartRecv(); break;
		case State_Length: stateLengthRecv(); break;
		case State_Data: stateDataRecv(); break;
		case State_CRC: stateCrcRecv(); break;
		default: LOG_ERROR(LOG_ECLP, "Unwaited data state=" << state << ", byte=" << uart->receive());
		}
	}
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
	state = State_Start;
}

void PacketLayer::stateStartRecv() {
	LOG_DEBUG(LOG_ECLP, "stateStartRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_START) {
		gotoStateLength();
		return;
	} else {
		LOG_ERROR(LOG_ECLP, "Unwaited answer " << b1);
		return;
	}
}

void PacketLayer::gotoStateLength() {
	LOG_DEBUG(LOG_ECLP, "gotoStateLength");
	timer->start(VENDOTEK_RECV_TIMEOUT);
	recvBuf.clear();
	state = State_Length;
}

void PacketLayer::stateLengthRecv() {
	LOG_DEBUG(LOG_ECLP, "stateLengthRecv");
	recvBuf.addUint8(uart->receive());
	if(recvBuf.getLen() < 2) {
		return;
	}

	recvDataLen = (recvBuf[0] << 8) | recvBuf[1];
	LOG_DEBUG(LOG_ECLP, "responseLength=" << recvDataLen);
	gotoStateData();
}

void PacketLayer::gotoStateData() {
	LOG_DEBUG(LOG_ECLP, "gotoStateData");
	recvBuf.clear();
	recvDataCnt = 0;
	state = State_Data;
}

void PacketLayer::stateDataRecv() {
	LOG_DEBUG(LOG_ECLP, "stateDataRecv");
	recvBuf.addUint8(uart->receive());
	recvDataCnt++;
	if(recvDataCnt >= recvDataLen) {
		gotoStateCrc();
	}
}

void PacketLayer::gotoStateCrc() {
	LOG_DEBUG(LOG_ECLP, "gotoStateCrc");
	crcBuf.clear();
	state = State_CRC;
}

void PacketLayer::stateCrcRecv() {
	LOG_DEBUG(LOG_ECLP, "stateCrcRecv");
	crcBuf.addUint8(uart->receive());
	if(crcBuf.getLen() < VENDOTEK_CRC_SIZE) {
		return;
	}

	if(recvBuf.getLen() < 2 || recvBuf[0] != 0x97 || recvBuf[1] != 0xFB) {
		LOG_ERROR(LOG_ECLP, "Wrong discriminator");
		gotoStateWait();
		return;
	}

	if(recvDataCnt != recvBuf.getLen()) {
		LOG_ERROR(LOG_ECLP, "Buffer too small");
		gotoStateWait();
		return;
	}

	Crc crc;
	crc.start();
	crc.add(Control_START);
	crc.add(recvDataLen >> 8);
	crc.add(recvDataLen);
	crc.add(recvBuf.getData(), recvBuf.getLen());
	if(crc.getHighByte() != crcBuf[0] || crc.getLowByte() != crcBuf[1]) {
		LOG_ERROR(LOG_ECLP, "Crc not equal");
		gotoStateWait();
		return;
	}

	gotoStateWait();
	if(observer == NULL) {
		LOG_ERROR(LOG_ECLP, "Observer not defined");
		return;
	}

#if LOG_ECLP >= LOG_LEVEL_TRACE
	LOG("<<<");
	LOG_HEX(recvBuf.getData() + 2, recvBuf.getLen() - 2);
#endif
	observer->procPacket(recvBuf.getData() + 2, recvBuf.getLen() - 2);
}

void PacketLayer::stateRecvTimeout() {
	LOG_DEBUG(LOG_ECLP, "stateRecvTimeout");
	gotoStateWait();
}

}
