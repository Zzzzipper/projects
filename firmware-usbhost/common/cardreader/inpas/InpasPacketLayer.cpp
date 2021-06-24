#if 1
#include "InpasPacketLayer.h"

#include "InpasProtocol.h"
#include "logger/RemoteLogger.h"
#include "logger/include/Logger.h"

namespace Inpas {

PacketCollector::PacketCollector() :
	observer(NULL),
	packet(3),
	recvBuf(INPAS_COLLECT_SIZE),
	lastNumber(0)
{
}

void PacketCollector::setObserver(PacketLayerObserver *observer) {
	this->observer = observer;
}

void PacketCollector::reset() {
	LOG_DEBUG(LOG_ECLP, "reset");
	lastNumber = 0;
	recvBuf.clear();
}

void PacketCollector::procPacket(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_ECLP, "procPacket");
	if(observer == NULL) {
		LOG_ERROR(LOG_ECLP, "Observer not defined");
		return;
	}

	SohHeader *header = (SohHeader*)data;
	uint8_t *headerData = header->data;
	uint16_t headerDataLen = header->len.get();
	if(packet.parse(headerData, headerDataLen) == false) {
		LOG_ERROR(LOG_ECLP, "Bad SOH format");
		LOG_ERROR_HEX(LOG_ECLP, headerData, headerDataLen);
		return;
	}

	uint16_t version = 0;
	if(packet.getNumber(HeaderTlv_Version, &version) == false) {
		LOG_ERROR(LOG_ECLP, "Bad SOH format");
		LOG_ERROR_HEX(LOG_ECLP, headerData, headerDataLen);
		return;
	}

	uint32_t number = 0;
	if(packet.getNumber(HeaderTlv_Number, &number) == false) {
		LOG_ERROR(LOG_ECLP, "Bad SOH format");
		LOG_ERROR_HEX(LOG_ECLP, headerData, headerDataLen);
		return;
	}

	LOG_INFO(LOG_ECLP, "SOH packet " << version << "/" << number);
	uint8_t *packetData = headerData + headerDataLen;
	uint16_t packetDataLen = dataLen - (sizeof(SohHeader) + headerDataLen);
	if(lastNumber == 0)
		if(number == 0) {
			recvBuf.clear();
			recvBuf.add(packetData, packetDataLen);
			observer->procPacket(recvBuf.getData(), recvBuf.getLen());
			return;
		} else {
			if(number != 1) {
				LOG_ERROR(LOG_ECLP, "Bad SOH order number " << lastNumber << "/" << number);
				lastNumber = 0;
				return;
			}
			lastNumber = number;
			recvBuf.clear();
			recvBuf.add(packetData, packetDataLen);
			return;
	} else {
		if(number > 0) {
			if((number - 1) != lastNumber) {
				LOG_ERROR(LOG_ECLP, "Bad SOH order number " << lastNumber << "/" << number);
				lastNumber = 0;
				return;
			}
			lastNumber = number;
			recvBuf.add(packetData, packetDataLen);
			return;
		} else {
			lastNumber = 0;
			recvBuf.add(packetData, packetDataLen);
			REMOTE_LOG(RLOG_ECL, "SOH_OK\r\n");
			observer->procPacket(recvBuf.getData(), recvBuf.getLen());
			return;
		}
	}
}

PacketLayer::PacketLayer(TimerEngine *timers, AbstractUart *uart) :
	timers(timers),
	uart(uart),
	observer(NULL),
	state(State_Idle),
	recvBuf(INPAS_PACKET_SIZE),
	crcBuf(INPAS_CRC_SIZE)
{
	this->timer = timers->addTimer<PacketLayer, &PacketLayer::procTimer>(this);
	this->uart->setReceiveHandler(this);
}

PacketLayer::~PacketLayer() {
	this->timers->deleteTimer(this->timer);
}

void PacketLayer::setObserver(PacketLayerObserver *observer) {
	this->observer = observer;
	collector.setObserver(observer);
}

void PacketLayer::reset() {
	LOG_INFO(LOG_ECLP, "reset");
	collector.reset();
	gotoStateWait();
}

bool PacketLayer::sendPacket(Buffer *data) {
	LOG_INFO(LOG_ECLP, "sendPacket");
	Crc crc;
	crc.start();
	crc.add(Control_STX);
	crc.add(data->getLen());
	crc.add(data->getLen() >> 8);
	crc.add(data->getData(), data->getLen());

	REMOTE_LOG_OUT(RLOG_ECL, Control_STX);
	REMOTE_LOG_OUT(RLOG_ECL, data->getLen());
	REMOTE_LOG_OUT(RLOG_ECL, data->getLen() >> 8);
	REMOTE_LOG_OUT(RLOG_ECL, data->getData(), data->getLen());
	REMOTE_LOG_OUT(RLOG_ECL, crc.getHighByte());
	REMOTE_LOG_OUT(RLOG_ECL, crc.getLowByte());
	if(LOG_ECLT > LOG_LEVEL_OFF) {
		Logger::get()->hex(Control_STX);
		Logger::get()->hex(data->getLen());
		Logger::get()->hex(data->getLen() >> 8);
		Logger::get()->hex(data->getData(), data->getLen());
		Logger::get()->hex(crc.getHighByte());
		Logger::get()->hex(crc.getLowByte());
		*(Logger::get()) << Logger::endl;
	}

	uart->send(Control_STX);
	uart->send(data->getLen());
	uart->send(data->getLen() >> 8);
	sendData(data->getData(), data->getLen());
	uart->send(crc.getHighByte());
	uart->send(crc.getLowByte());
	return true;
}

bool PacketLayer::sendControl(uint8_t byte) {
	LOG_INFO(LOG_ECLP, "sendControl");
	LOG_ERROR_HEX(LOG_ECLT, byte);
	REMOTE_LOG_OUT(RLOG_ECL, byte);
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
	while(uart->isEmptyReceiveBuffer() == false) {
		LOG_TRACE(LOG_ECLP, "have data " << state);
		switch(state) {
		case State_Wait: stateWaitRecv(); break;
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
	state = State_Wait;
}

void PacketLayer::stateWaitRecv() {
	LOG_INFO(LOG_ECLP, "stateWaitRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_STX || b1 == Control_SOH) {
		firstLex = b1;
		gotoStateLength();
		return;
	} else if(b1 == Control_ACK) {
		gotoStateWait();
		if(observer == NULL) {
			LOG_ERROR(LOG_ECLP, "Observer not defined");
			return;
		}
		LOG_ERROR_HEX(LOG_ECLT, b1);
		REMOTE_LOG_IN(RLOG_ECL, b1);
		observer->procControl(b1);
		return;
	} else if(b1 == Control_EOT) {
		gotoStateWait();
		if(observer == NULL) {
			LOG_ERROR(LOG_ECLP, "Observer not defined");
			return;
		}
		LOG_ERROR_HEX(LOG_ECLT, b1);
		REMOTE_LOG_IN(RLOG_ECL, b1);
		observer->procControl(b1);
		return;
	} else {
		LOG_ERROR(LOG_ECLP, "Unwaited answer " << b1);
		REMOTE_LOG_IN(RLOG_ECL, b1);
		return;
	}
}

void PacketLayer::gotoStateLength() {
	LOG_DEBUG(LOG_ECLP, "gotoStateLength");
	timer->start(INPAS_RECV_TIMEOUT);
	recvBuf.clear();
	state = State_Length;
}

void PacketLayer::stateLengthRecv() {
	LOG_DEBUG(LOG_ECLP, "stateLengthRecv");
	recvBuf.addUint8(uart->receive());
	if(recvBuf.getLen() < 2) {
		return;
	}

	recvLength = (recvBuf[1] << 8) | recvBuf[0];
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
	if(crcBuf.getLen() < INPAS_CRC_SIZE) {
		return;
	}

	REMOTE_LOG_IN(RLOG_ECL, firstLex);
	REMOTE_LOG_IN(RLOG_ECL, recvLength);
	REMOTE_LOG_IN(RLOG_ECL, recvLength >> 8);
	REMOTE_LOG_IN(RLOG_ECL, recvBuf.getData(), recvBuf.getLen());
	REMOTE_LOG_IN(RLOG_ECL, crcBuf.getData(), crcBuf.getLen());
	if(LOG_ECLT > 0) {
		Logger::get()->hex(firstLex);
		Logger::get()->hex(recvLength);
		Logger::get()->hex(recvLength >> 8);
		Logger::get()->hex(recvBuf.getData(), recvBuf.getLen());
		Logger::get()->hex(crcBuf.getData(), crcBuf.getLen());
		*(Logger::get()) << Logger::endl;
	}

	Crc crc;
	crc.start();
	crc.add(firstLex);
	crc.add(recvLength);
	crc.add(recvLength >> 8);
	crc.add(recvBuf.getData(), recvBuf.getLen());
	if(crc.getHighByte() != crcBuf[0] || crc.getLowByte() != crcBuf[1]) {
		LOG_ERROR(LOG_ECLP, "Crc not equal");
		sendControl(Control_NAK);
		gotoStateWait();
		return;
	}

	LOG_INFO(LOG_ECLP, "recvPacket");
	sendControl(Control_ACK);
	gotoStateWait();
	if(observer == NULL) {
		LOG_ERROR(LOG_ECLP, "Observer not defined");
		return;
	}

	if(firstLex == Control_STX) {
		REMOTE_LOG(RLOG_ECL, "STX\r\n");
		observer->procPacket(recvBuf.getData(), recvBuf.getLen());
		return;
	} else if(firstLex == Control_SOH) {
		REMOTE_LOG(RLOG_ECL, "SOH\r\n");
		collector.procPacket(recvBuf.getData(), recvBuf.getLen());
		return;
	} else {
		LOG_ERROR(LOG_ECLP, "Wrong first lex " << firstLex);
		return;
	}
}

void PacketLayer::stateRecvTimeout() {
	LOG_INFO(LOG_ECLP, "stateRecvTimeout");
	collector.reset();
	gotoStateWait();
}

}
#else
#include "InpasPacketLayer.h"

#include "InpasProtocol.h"
#include "logger/include/Logger.h"

namespace Inpas {

PacketLayer::PacketLayer(TimerEngine *timers, AbstractUart *uart) :
	timers(timers),
	uart(uart),
	observer(NULL),
	state(State_Idle),
	recvBuf(INPAS_PACKET_SIZE),
	crcBuf(INPAS_CRC_SIZE)
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
//	packetLayer->sendControl(Control_EXT);
//	packetLayer->sendControl(Control_EOT);
//	packetLayer->sendControl(Control_EOT);
	gotoStateWait();
}

bool PacketLayer::sendPacket(Buffer *data) {
	Crc crc;
	crc.start();
	crc.add(Control_STX);
	crc.add(data->getLen());
	crc.add(data->getLen() >> 8);
	crc.add(data->getData(), data->getLen());

	uart->send(Control_STX);
	uart->send(data->getLen());
	uart->send(data->getLen() >> 8);
	if(LOG_ECLT > LOG_LEVEL_OFF) {
		Logger::get()->hex(Control_STX);
		Logger::get()->hex(data->getLen());
		Logger::get()->hex(data->getLen() >> 8);
		Logger::get()->hex(data->getData(), data->getLen());
		Logger::get()->hex(crc.getHighByte());
		Logger::get()->hex(crc.getLowByte());
		*(Logger::get()) << Logger::endl;
	}
	sendData(data->getData(), data->getLen());
	uart->send(crc.getHighByte());
	uart->send(crc.getLowByte());
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
	while(uart->isEmptyReceiveBuffer() == false) {
		LOG_TRACE(LOG_ECLP, "have data " << state);
		switch(state) {
		case State_STX: stateStxRecv(); break;
		case State_Length: stateLengthRecv(); break;
		case State_Data: stateDataRecv(); break;
		case State_CRC: stateCrcRecv(); break;
#if 0
		case State_SkipData: stateSkipRecv(); break;
#endif
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
	state = State_STX;
}

void PacketLayer::stateStxRecv() {
	LOG_DEBUG(LOG_ECLP, "stateStxRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_STX) {
		gotoStateLength();
		return;
	} else if(b1 == Control_ACK) {
		gotoStateWait();
		if(observer == NULL) {
			LOG_ERROR(LOG_ECLP, "Observer not defined");
			return;
		}
		LOG_ERROR_HEX(LOG_ECLT, b1);
		observer->procControl(b1);
		return;
	} else if(b1 == Control_EOT) {
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
	timer->start(INPAS_RECV_TIMEOUT);
	recvBuf.clear();
	state = State_Length;
}

void PacketLayer::stateLengthRecv() {
	LOG_DEBUG(LOG_ECLP, "stateLengthRecv");
	recvBuf.addUint8(uart->receive());
	if(recvBuf.getLen() < 2) {
		return;
	}

	recvLength = (recvBuf[1] << 8) | recvBuf[0];
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
	if(crcBuf.getLen() < INPAS_CRC_SIZE) {
		return;
	}

	Crc crc;
	crc.start();
	crc.add(Control_STX);
	crc.add(recvLength >> 8);
	crc.add(recvLength);
	crc.add(recvBuf.getData(), recvBuf.getLen());
	if(crc.getHighByte() == crcBuf[0] && crc.getLowByte() == crcBuf[1]) {
		LOG_ERROR(LOG_ECLP, "Crc not equal");
		gotoStateWait();
		return;
	}

	gotoStateWait();
	if(LOG_ECLT > LOG_LEVEL_OFF) {
		Logger::get()->hex(Control_STX);
		Logger::get()->hex(recvLength >> 8);
		Logger::get()->hex(recvLength);
		Logger::get()->hex(recvBuf.getData(), recvBuf.getLen());
		Logger::get()->hex(crc.getHighByte());
		Logger::get()->hex(crc.getLowByte());
		*(Logger::get()) << Logger::endl;
	}
	if(observer == NULL) {
		LOG_ERROR(LOG_ECLP, "Observer not defined");
		return;
	}
	observer->procPacket(recvBuf.getData(), recvBuf.getLen());
}

void PacketLayer::stateRecvTimeout() {
	LOG_DEBUG(LOG_ECLP, "stateRecvTimeout");
	gotoStateWait();
}

#if 0
void PacketLayer::gotoStateSkip(Error error) {
	LOG_DEBUG(LOG_ECLP, "gotoStateSkip " << error);
	recvError = error;
	state = State_SkipData;
}

void PacketLayer::stateSkipRecv() {
	LOG_DEBUG(LOG_ECLP, "stateSkipRecv");
	uart->receive();
}

void PacketLayer::stateSkipTimeout() {
	LOG_DEBUG(LOG_ECLP, "stateSkipTimeout");
	procRecvError(recvError);
}

void PacketLayer::procRecvError(Error error) {
	state = State_Idle;
	if(observer == NULL) {
		LOG_ERROR(LOG_ECLP, "Observer not defined");
		return;
	}
	observer->procRecvError(error);
}
#endif

}
#endif
