#include "SberbankPacketLayer.h"
#include "BluetoothCrc.h"

#include "logger/include/Logger.h"

namespace Sberbank {

PacketReceiver::PacketReceiver(TimerEngine *timers, FrameLayerInterface *frameLayer) :
	timers(timers),
	frameLayer(frameLayer),
	state(State_Idle),
	recvBuf(SBERBANK_PACKET_SIZE)
{
	this->timer = timers->addTimer<PacketReceiver, &PacketReceiver::procTimer>(this);
}

PacketReceiver::~PacketReceiver() {
	this->timers->deleteTimer(this->timer);
}

void PacketReceiver::setObserver(PacketLayerObserver *observer) {
	this->observer = observer;
}

void PacketReceiver::reset() {
	LOG_DEBUG(LOG_ECLP, "reset");
	frameLayer->reset();
	state = State_Wait;
}

void PacketReceiver::procPacket(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_ECLP, "procPacket");
	switch(state) {
	case State_Wait: stateWaitRecv(data, len); break;
	case State_Chain: stateChainRecv(data, len); break;
	default: LOG_ERROR(LOG_ECLF, "Unwaited data state=" << state);
	}
}

void PacketReceiver::procTimer() {
	LOG_DEBUG(LOG_ECLP, "procTimer");
	state = State_Wait;
}

void PacketReceiver::stateWaitRecv(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_ECLP, "stateWaitRecv");
	if(dataLen < 4) {
		LOG_ERROR(LOG_ECLP, "Wrong packet size " << dataLen);
		return;
	}

	uint8_t recvId = data[0];
	if(recvId == 0) {
		LOG_DEBUG(LOG_ECLP, "Single " << recvId);
		recvBuf.clear();
		if(recvPacket(data, dataLen) == false) {
			return;
		}

		frameLayer->sendControl(Control_EOT);
		state = State_Wait;
		observer->procPacket(recvBuf.getData(), recvBuf.getLen());
		return;
	} else if(recvId == 0x80) {
		LOG_DEBUG(LOG_ECLP, "Chain first " << recvId);
		recvBuf.clear();
		if(recvPacket(data, dataLen) == false) {
			return;
		}

		frameLayer->sendControl(Control_ACK);
		packetId = 1;
		timer->start(SBERBANK_FRAME_TIMEOUT);
		state = State_Chain;
		return;
	}
}

void PacketReceiver::stateChainRecv(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_ECLP, "stateChainRecv");
	if(dataLen < 4) {
		LOG_ERROR(LOG_ECLP, "Wrong packet size " << dataLen);
		return;
	}

	uint8_t recvId = data[0];
	if(recvId == packetId) {
		LOG_DEBUG(LOG_ECLP, "Chain last " << recvId);
		if(recvPacket(data, dataLen) == false) {
			return;
		}

		frameLayer->sendControl(Control_EOT);
		state = State_Wait;
		observer->procPacket(recvBuf.getData(), recvBuf.getLen());
		return;
	} else if(recvId == (packetId | 0x80)) {
		LOG_DEBUG(LOG_ECLP, "Chain next " << recvId);
		if(recvPacket(data, dataLen) == false) {
			return;
		}

		if(recvId == 0x82) {
			frameLayer->sendControl(Control_ACK);
		} else {
			frameLayer->sendControl(Control_BEL);
		}
		packetId++;
		timer->start(SBERBANK_FRAME_TIMEOUT);
		return;
	} else {
		LOG_DEBUG(LOG_ECLP, "Chain unwaited " << recvId);
		LOG_DEBUG_HEX(LOG_ECLP, data, dataLen);
		return;
	}
}

bool PacketReceiver::recvPacket(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_ECLP, "recvPacket");
	uint16_t bodyLen = data[1];
	if((bodyLen + 4) != dataLen) {
		LOG_ERROR(LOG_ECLP, "Wrong packet size exp=" << (bodyLen + 4) << ", act=" << dataLen);
		LOG_DEBUG_HEX(LOG_ECLP, data, dataLen);
		return false;
	}

	BluetoothCrc crc;
	crc.start();
	crc.add(data + 2, bodyLen);
	if(data[dataLen - 2] == crc.getLowByte()) {
		LOG_ERROR(LOG_ECLP, "Wrong crc");
		LOG_DEBUG_HEX(LOG_ECLP, data, dataLen);
		frameLayer->sendControl(Control_NAK);
		return false;
	}
	if(data[dataLen - 1] == crc.getHighByte()) {
		LOG_ERROR(LOG_ECLP, "Wrong crc");
		LOG_DEBUG_HEX(LOG_ECLP, data, dataLen);
		frameLayer->sendControl(Control_NAK);
		return false;
	}

	recvBuf.add(data + 2, bodyLen);
	return true;
}

}
