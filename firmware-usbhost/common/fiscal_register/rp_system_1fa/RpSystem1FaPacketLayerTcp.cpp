#include "RpSystem1FaPacketLayerTcp.h"

#include "fiscal_register/rp_system_1fa/include/RpSystem1Fa.h"
#include "fiscal_register/rp_system_1fa/RpSystem1FaProtocol.h"
#include "fiscal_register/shtrih-m/ShtrihmProtocol.h" //todo: подумать куда венести ShtrihmCrc
#include "logger/include/Logger.h"

namespace RpSystem1Fa {

PacketLayerTcp::PacketLayerTcp(TimerEngine *timers, AbstractUart *uart) : timers(timers), uart(uart), state(State_Idle), sendBuf(NULL) {
	this->recvBuf = new Buffer(RPSYSTEM1FA_PACKET_MAX_SIZE);
	this->timer = timers->addTimer<PacketLayerTcp, &PacketLayerTcp::procTimer>(this);
	this->uart->setReceiveHandler(this);
}

PacketLayerTcp::~PacketLayerTcp() {
	timers->deleteTimer(this->timer);
	delete this->recvBuf;
}

bool PacketLayerTcp::sendPacket(const Buffer *data) {
	if(state != State_Idle) {
		LOG_ERROR(LOG_FR, "Wrong state " << state);
		return false;
	}
	this->sendBuf = data;
	gotoStateLenght();
	return true;
}

void PacketLayerTcp::handle() {
	LOG_TRACE(LOG_FR, "handle");
	while(uart->isEmptyReceiveBuffer() == false) {
		LOG_TRACE(LOG_FR, "have data");
		switch(state) {
		case State_Length: stateLengthRecv(); break;
		case State_Data: stateDataRecv(); break;
		default: LOG_ERROR(LOG_FR, "Unwaited data state=" << state << ", byte=" << uart->receive());
		}
	}
}

void PacketLayerTcp::procTimer() {
	LOG_DEBUG(LOG_FR, "procTimer " << state);
	switch(state) {
		case State_Length: gotoStateNoConnection(); break;
		case State_Data: gotoStateNoConnection(); break;
		case State_Answer: stateAnswerTimeout(); break;
		default: LOG_ERROR(LOG_FR, "Unwaited timeout state=" << state);
	}
}

void PacketLayerTcp::gotoStateLenght() {
	const uint8_t *data = sendBuf->getData();
	uint8_t dataLen = sendBuf->getLen();
	LOG_DEBUG(LOG_FR, "gotoStateLenght " << dataLen);
	LOG_TRACE_HEX(LOG_FR, data, dataLen);

	uart->send(dataLen);
	for(uint8_t i = 0; i < dataLen; i++) {
		uart->send(data[i]);
	}

	timer->start(RPSYSTEM1FA_RESPONSE_TIMEOUT);
	recvBuf->clear();
	state = State_Length;
}

void PacketLayerTcp::stateLengthRecv() {
	LOG_DEBUG(LOG_FR, "stateLengthRecv");
	recvLength = uart->receive();
	LOG_DEBUG(LOG_FR, "responseLength=" << recvLength);
	state = State_Data;
}

void PacketLayerTcp::stateDataRecv() {
	LOG_DEBUG(LOG_FR, "stateDataRecv");
	recvBuf->addUint8(uart->receive());
	if(recvBuf->getLen() >= recvLength) {
		gotoStateAnswer();
	}
}

void PacketLayerTcp::gotoStateAnswer() {
	LOG_DEBUG(LOG_FR, "gotoStateAnswer");
	timer->start(RPSYSTEM1FA_DELIVER_TIMEOUT);
	state = State_Answer;
}

void PacketLayerTcp::stateAnswerTimeout() {
	LOG_DEBUG(LOG_FR, "stateAnswerTimeout");
	state = State_Idle;
	observer->procRecvData(recvBuf->getData(), recvBuf->getLen());
}

void PacketLayerTcp::gotoStateNoConnection() {
	LOG_DEBUG(LOG_FR, "gotoStateNoConnection");
	timer->stop();
	state = State_NoConnection;
	observer->procRecvError();
}

}
