#include "RpSystem1FaPacketLayerCom.h"

#include "fiscal_register/rp_system_1fa/include/RpSystem1Fa.h"
#include "fiscal_register/rp_system_1fa/RpSystem1FaProtocol.h"
#include "fiscal_register/shtrih-m/ShtrihmProtocol.h" //todo: подумать куда венести ShtrihmCrc
#include "logger/include/Logger.h"

namespace RpSystem1Fa {

PacketLayerCom::PacketLayerCom(TimerEngine *timers, AbstractUart *uart) : timers(timers), uart(uart), state(State_Idle), sendBuf(NULL) {
	this->recvBuf = new Buffer(RPSYSTEM1FA_PACKET_MAX_SIZE);
	this->timer = timers->addTimer<PacketLayerCom, &PacketLayerCom::procTimer>(this);
	this->uart->setReceiveHandler(this);
}

PacketLayerCom::~PacketLayerCom() {
	timers->deleteTimer(this->timer);
	delete this->recvBuf;
}

bool PacketLayerCom::sendPacket(const Buffer *data) {
	if(state != State_Idle) {
		LOG_ERROR(LOG_FR, "Wrong state " << state);
		return false;
	}
	this->sendBuf = data;
	gotoStateConfirm();
	return true;
}

void PacketLayerCom::handle() {
	LOG_TRACE(LOG_FR, "handle");
	while(uart->isEmptyReceiveBuffer() == false) {
		LOG_TRACE(LOG_FR, "have data");
		switch(state) {
		case State_Confirm: stateConfirmRecv(); break;
		case State_STX: stateSTXRecv(); break;
		case State_Length: stateLengthRecv(); break;
		case State_Data: stateDataRecv(); break;
		case State_CRC: stateCRCRecv(); break;
		default: LOG_ERROR(LOG_FR, "Unwaited data state=" << state << ", byte=" << uart->receive());
		}
	}
}

void PacketLayerCom::procTimer() {
	LOG_DEBUG(LOG_FR, "procTimer " << state);
	switch(state) {
		case State_Confirm: gotoStateNoConnection(); break;
		case State_STX: gotoStateNoConnection(); break;
		case State_Length: gotoStateNoConnection(); break;
		case State_Data: gotoStateNoConnection(); break;
		case State_CRC: gotoStateNoConnection(); break;
		case State_Answer: stateAnswerTimeout(); break;
		default: LOG_ERROR(LOG_FR, "Unwaited timeout state=" << state);
	}
}

void PacketLayerCom::gotoStateConfirm() {
	const uint8_t *data = sendBuf->getData();
	uint8_t dataLen = sendBuf->getLen();
	LOG_DEBUG(LOG_FR, "gotoStateConfirm " << dataLen);
	LOG_TRACE_HEX(LOG_FR, data, dataLen);

	uart->send(Control_STX);
	uart->send(dataLen);
	for(uint8_t i = 0; i < dataLen; i++) {
		uart->send(data[i]);
	}
	uart->send(ShtrihmCrc::calc(sendBuf));

	timer->start(RPSYSTEM1FA_CONFIRM_TIMEOUT);
	state = State_Confirm;
}

void PacketLayerCom::stateConfirmRecv() {
	LOG_DEBUG(LOG_FR, "stateConfirmRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_ACK) {
		gotoStateSTX();
		return;
	} else if(b1 == Control_NAK) {
		LOG_ERROR(LOG_FR, "Negative confirm " << b1);
		gotoStateNoConnection(); //todo: тут рекомендуют 10 попыток перепосылки запроса
		return;
	} else {
		LOG_ERROR(LOG_FR, "Unwaited answer " << b1);
		gotoStateNoConnection();
		return;
	}
}

void PacketLayerCom::gotoStateSTX() {
	LOG_DEBUG(LOG_FR, "gotoStateSTX");
	timer->start(RPSYSTEM1FA_RESPONSE_TIMEOUT);
	state = State_STX;
}

void PacketLayerCom::stateSTXRecv() {
	LOG_DEBUG(LOG_FR, "stateSTXRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_STX) {
		gotoStateLenght();
	} else {
		LOG_ERROR(LOG_FR, "Unwaited answer " << b1);
		gotoStateNoConnection();
		return;
	}
}

void PacketLayerCom::gotoStateLenght() {
	LOG_DEBUG(LOG_FR, "gotoStateLenght");
	recvBuf->clear();
	state = State_Length;
}

void PacketLayerCom::stateLengthRecv() {
	LOG_DEBUG(LOG_FR, "stateLengthRecv");
	recvLength = uart->receive();
	LOG_DEBUG(LOG_FR, "responseLength=" << recvLength);
	state = State_Data;
}

void PacketLayerCom::stateDataRecv() {
	LOG_DEBUG(LOG_FR, "stateDataRecv");
	recvBuf->addUint8(uart->receive());
	if(recvBuf->getLen() >= recvLength) {
		state = State_CRC;
	}
}

void PacketLayerCom::stateCRCRecv() {
	LOG_DEBUG(LOG_FR, "stateCRCRecv");
	uint8_t recvCrc = uart->receive();
	uint8_t calcCrc = ShtrihmCrc::calc(recvBuf);
	if(recvCrc == calcCrc) {
		uart->send(Control_ACK);
		gotoStateAnswer();
		return;
	} else {
		uart->send(Control_NAK);
		LOG_ERROR(LOG_FR, "Wrong CRC: recv=" << recvCrc << ", calc=" << calcCrc);
		gotoStateNoConnection();
		return;
	}
}

void PacketLayerCom::gotoStateAnswer() {
	LOG_DEBUG(LOG_FR, "gotoStateAnswer");
	timer->start(RPSYSTEM1FA_DELIVER_TIMEOUT);
	state = State_Answer;
}

void PacketLayerCom::stateAnswerTimeout() {
	LOG_DEBUG(LOG_FR, "stateAnswerTimeout");
	state = State_Idle;
	LOG_DEBUG(LOG_FR, "Recv answer");
	observer->procRecvData(recvBuf->getData(), recvBuf->getLen());
}

void PacketLayerCom::gotoStateNoConnection() {
	LOG_DEBUG(LOG_FR, "gotoStateNoConnection");
	timer->stop();
	state = State_NoConnection;
	observer->procRecvError();
}

}
