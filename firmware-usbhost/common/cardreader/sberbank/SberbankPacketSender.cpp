#include "SberbankPacketLayer.h"
#include "BluetoothCrc.h"

#include "logger/include/Logger.h"

namespace Sberbank {

PacketSender::PacketSender(
	TimerEngine *timers,
	FrameLayerInterface *frameLayer
) :
	timers(timers),
	frameLayer(frameLayer),
	state(State_Idle),
	queue(10),
	sendBuf(SBERBANK_PACKET_SIZE)
{
	this->timer = timers->addTimer<PacketSender, &PacketSender::procTimer>(this);
}

PacketSender::~PacketSender() {
	this->timers->deleteTimer(this->timer);
}

void PacketSender::setObserver(PacketLayerObserver *observer) {
	this->observer = observer;
}

void PacketSender::reset() {
	LOG_DEBUG(LOG_ECLP, "reset");
	gotoStateWait();
}

bool PacketSender::sendPacket(Buffer *data) {
	LOG_DEBUG(LOG_ECLP, "sendPacket " << state << ", " << data->getLen());
	LOG_TRACE_HEX(LOG_ECL, data->getData(), data->getLen());
	if(state == State_Wait) {
		LOG_DEBUG(LOG_ECLP, "start");
		this->data = data;
		this->packetId = 0;
		this->procLen = 0;
		gotoStateSendLast();
		return true;
	} else {
		LOG_DEBUG(LOG_ECLP, "in queue" << queue.getSize());
		queue.push(data);
		return true;
	}
}

void PacketSender::procControl(uint8_t control) {
	LOG_DEBUG(LOG_ECLP, "procControl " << control);
	switch(state) {
		case State_SendNext: stateSendNextProcControl(control); break;
		case State_SendLast: stateSendLastProcControl(control); break;
		default:;
	}
}

void PacketSender::procTimer() {
	LOG_DEBUG(LOG_ECLP, "procTimer");
	switch(state) {
		case State_SendNext: stateSendNextTimeout(); break;
		case State_SendLast: stateSendLastTimeout(); break;
		default:;
	}
}

void PacketSender::gotoStateWait() {
	LOG_DEBUG(LOG_ECLP, "gotoStateWait");
	if(queue.getSize() > 0) {
		this->data = queue.pop();
		this->packetId = 0;
		this->procLen = 0;
		gotoStateSendLast();
		return;
	} else {
		state = State_Wait;
		return;
	}
}

void PacketSender::gotoStateSendNext() {
	LOG_DEBUG(LOG_ECLP, "gotoStateSendNext");
	if((data->getLen() - procLen) <= SBERBANK_FRAME_BODY_SIZE) {
		gotoStateSendLast();
		return;
	}
	sendLen = data->getLen() - procLen;
	if(sendLen > SBERBANK_FRAME_BODY_SIZE) { sendLen = SBERBANK_FRAME_BODY_SIZE; }

	sendBuf.clear();
	sendBuf.addUint8(packetId + 0x80);
	sendBuf.addUint8(sendLen);
	sendBuf.add(data->getData() + procLen, sendLen);

	BluetoothCrc crc;
	crc.start();
	crc.add(sendBuf.getData(), sendBuf.getLen());

	sendBuf.addUint8(crc.getLowByte());
	sendBuf.addUint8(crc.getHighByte());

	frameLayer->sendPacket(&sendBuf);
	timer->start(SBERBANK_PACKET_TIMEOUT);
	state = State_SendNext;
}

void PacketSender::stateSendNextProcControl(uint8_t control) {
	LOG_DEBUG(LOG_ECLP, "stateSendNextProcControl");
	if((packetId % 2) == 0) {
		if(control == Control_ACK) {
			packetId++;
			procLen += sendLen;
			gotoStateSendNext();
			return;
		}
		if(control == Control_NAK) {
			gotoStateSendNext();
			return;
		}
	} else {
		if(control == Control_BEL) {
			packetId++;
			procLen += sendLen;
			gotoStateSendNext();
			return;
		}
		if(control == Control_NAK) {
			gotoStateSendNext();
			return;
		}
	}
}

void PacketSender::stateSendNextTimeout() {
	LOG_DEBUG(LOG_ECLP, "stateSendNextTimeout");
	gotoStateWait();
}

void PacketSender::gotoStateSendLast() {
	LOG_DEBUG(LOG_ECLP, "gotoStateSendLast");
	if((data->getLen() - procLen) > SBERBANK_FRAME_BODY_SIZE) {
		gotoStateSendNext();
		return;
	}
	sendLen = data->getLen() - procLen;

	sendBuf.clear();
	sendBuf.addUint8(packetId);
	sendBuf.addUint8(sendLen);
	sendBuf.add(data->getData() + procLen, sendLen);

	BluetoothCrc crc;
	crc.start();
	crc.add(sendBuf.getData(), sendBuf.getLen());

	sendBuf.addUint8(crc.getLowByte());
	sendBuf.addUint8(crc.getHighByte());

	frameLayer->sendPacket(&sendBuf);
	timer->start(SBERBANK_PACKET_TIMEOUT);
	state = State_SendLast;
}

void PacketSender::stateSendLastProcControl(uint8_t control) {
	LOG_DEBUG(LOG_ECLP, "stateSendLastProcControl");
	if(control == Control_EOT) {
		gotoStateWait();
		return;
	}
	if(control == Control_NAK) {
		gotoStateSendLast();
		return;
	}
}

void PacketSender::stateSendLastTimeout() {
	LOG_DEBUG(LOG_ECLP, "stateSendLastTimeout");
	gotoStateWait();
}

}
