#include "DdcmpPacketLayer.h"

#include "DdcmpProtocol.h"
#include "logger/include/Logger.h"

namespace Ddcmp {

PacketLayer::PacketLayer(TimerEngine *timers, AbstractUart *uart) :
	timers(timers),
	uart(uart),
	observer(NULL),
	state(State_Idle),
	sendBuf(DDCMP_PACKET_SIZE),
	recvBuf(DDCMP_PACKET_SIZE)
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
	LOG_DEBUG(LOG_DDCMPP, "reset");
	gotoStateHeader();
}

void PacketLayer::sendControl(uint8_t *cmd, uint16_t cmdLen) {
	LOG_DEBUG(LOG_DDCMPP, "sendControl");
	sendBuf.clear();
	sendBuf.add(cmd, cmdLen);
	Ddcmp::Crc crc;
	crc.start();
	crc.add(cmd, cmdLen);
	sendBuf.addUint8(crc.getLowByte());
	sendBuf.addUint8(crc.getHighByte());
	for(uint16_t i = 0; i < sendBuf.getLen(); i++) {
		uart->send(sendBuf[i]);
	}
}

void PacketLayer::sendData(uint8_t *cmd, uint16_t cmdLen, uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_DDCMPP, "sendControl");
	sendBuf.clear();
	sendBuf.add(cmd, cmdLen);
	Ddcmp::Crc crc;
	crc.start();
	crc.add(cmd, cmdLen);
	sendBuf.addUint8(crc.getLowByte());
	sendBuf.addUint8(crc.getHighByte());
	sendBuf.add(data, dataLen);
	crc.start();
	crc.add(data, dataLen);
	sendBuf.addUint8(crc.getLowByte());
	sendBuf.addUint8(crc.getHighByte());
	for(uint16_t i = 0; i < sendBuf.getLen(); i++) {
		uart->send(sendBuf[i]);
	}
}

void PacketLayer::handle() {
	while(uart->isEmptyReceiveBuffer() == false) {
		LOG_TRACE(LOG_DDCMPP, "have data " << state);
		switch(state) {
		case State_Header: stateHeaderRecv(); break;
		case State_Data: stateDataRecv(); break;
		default: LOG_ERROR(LOG_DDCMPP, "Unwaited data state=" << state << ", byte=" << uart->receive());
		}
	}
}

void PacketLayer::procTimer() {
	LOG_DEBUG(LOG_DDCMPP, "procTimer " << state);
	switch(state) {
	case State_Data: stateDataTimeout(); break;
	default: LOG_ERROR(LOG_DDCMPP, "Unwaited timeout state=" << state);
	}
}

void PacketLayer::gotoStateHeader() {
	LOG_DEBUG(LOG_DDCMPP, "gotoStateHeader");
	recvBuf.clear();
	timer->stop();
	state = State_Header;
}

void PacketLayer::stateHeaderRecv() {
	LOG_DEBUG(LOG_DDCMPP, "stateHeaderRecv");
	uint8_t b1 = uart->receive();
	recvBuf.addUint8(b1);
	if(recvBuf.getLen() >= sizeof(Ddcmp::Header)) {
		Ddcmp::Header *header = (Ddcmp::Header*)(recvBuf.getData());
		if(header->message == Message_Control) {
			gotoStateHeader();
			observer->recvControl(recvBuf.getData(), sizeof(Control));
			return;
		} else if(header->message == Message_Data) {
			gotoStateData();
			return;
		} else {
			LOG_ERROR(LOG_DDCMPP, "Wrong message " << header->message)
			LOG_DEBUG_HEX(LOG_DDCMPP, recvBuf.getData(), recvBuf.getLen());
			return;
		}
	}
}

void PacketLayer::gotoStateData() {
	LOG_DEBUG(LOG_DDCMPP, "gotoStateData");
	Header *header = (Header*)(recvBuf.getData());
	if(header->message != Message_Data) {
		LOG_ERROR(LOG_DDCMPP, "Wrong message " << header->message)
		gotoStateHeader();
		return;
	}
	recvDataLen = sizeof(Header) + header->type + ((header->flags & DataFlag_LengthMask) >> 8) + 2;
	timer->start(DDCMP_RECV_TIMEOUT);
	state = State_Data;
}

void PacketLayer::stateDataRecv() {
	LOG_DEBUG(LOG_DDCMPP, "stateDataRecv");
	uint8_t b1 = uart->receive();
	recvBuf.addUint8(b1);
	if(recvBuf.getLen() >= recvDataLen) {
		gotoStateHeader();
		observer->recvData(recvBuf.getData(), sizeof(Header) - 2, recvBuf.getData() + sizeof(Header), recvDataLen - (sizeof(Header) + 2));
		return;
	}
}

void PacketLayer::stateDataTimeout() {
	LOG_ERROR(LOG_DDCMPP, "stateDataTimeout");
}

}
