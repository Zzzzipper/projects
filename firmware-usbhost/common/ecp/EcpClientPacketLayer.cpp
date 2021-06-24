#include "EcpClientPacketLayer.h"
#include "EcpProtocol.h"
#include "logger/include/Logger.h"

namespace Ecp {

ClientPacketLayer::ClientPacketLayer(TimerEngine *timers, AbstractUart *uart) :
	uart(uart),
	timers(timers),
	state(State_Idle),
	sendBuf(NULL)
{
	this->recvBuf = new Buffer(ECP_PACKET_MAX_SIZE);
	this->timer = timers->addTimer<ClientPacketLayer, &ClientPacketLayer::procTimer>(this);
}

ClientPacketLayer::~ClientPacketLayer() {
	timers->deleteTimer(this->timer);
	delete this->recvBuf;
}

void ClientPacketLayer::setObserver(Observer *observer) {
	this->observer = observer;
}

bool ClientPacketLayer::connect() {
	LOG_INFO(LOG_ECP, "connect");
	uart->setReceiveHandler(this);
	tries = ECP_CONNECT_TRIES;
	gotoStateConnecting();
	return true;
}

void ClientPacketLayer::disconnect() {
	LOG_INFO(LOG_ECP, "disconnect");
	if(state == State_Idle) {
		LOG_INFO(LOG_ECP, "Already disconnected");
		timer->start(1);
		state = State_Disconnecting;
		return;
	}
	gotoStateDisconnecting();
}

bool ClientPacketLayer::sendData(const Buffer *sendBuf) {
	if(state != State_Ready && state != State_KeepAlive) {
		LOG_ERROR(LOG_ECP, "Wrong state " << state);
		return false;
	}

	this->sendBuf = sendBuf;
	if(state == State_Ready) {
		gotoStateConfirm();
	}

	return true;
}

void ClientPacketLayer::handle() {
	LOG_TRACE(LOG_ECP, "handle");
	while(uart->isEmptyReceiveBuffer() == false) {
		LOG_TRACE(LOG_ECP, "have data");
		switch(state) {
		case State_Connecting: stateConnectionRecv(); break;
		case State_Confirm: stateConfirmRecv(); break;
		case State_Length: stateLengthRecv(); break;
		case State_Data: stateDataRecv(); break;
		case State_CRC: stateCRCRecv(); break;
		case State_KeepAlive: stateKeepAliveRecv(); break;
		case State_Disconnecting: stateStateDisconnectingRecv(); break;
		default: LOG_ERROR(LOG_ECP, "Unwaited data state=" << state << ", byte=" << uart->receive());
		}
	}
}

void ClientPacketLayer::procTimer() {
	LOG_DEBUG(LOG_ECP, "procTimer " << state);
	switch(state) {
		case State_Connecting: gotoStateConnecting(); break;
		case State_Ready: gotoKeepAlive(); break;
		case State_Confirm: procError(); break;
		case State_Length: procError(); break;
		case State_Data: procError(); break;
		case State_CRC: procError(); break;
		case State_Answer: stateAnswerTimeout(); break;
		case State_KeepAlive: stateKeepAliveTimeout(); break;
		case State_Disconnecting: stateStateDisconnectingTimeout(); break;
		default: LOG_ERROR(LOG_ECP, "Unwaited timeout state=" << state);
	}
}

void ClientPacketLayer::gotoStateConnecting() {
	LOG_DEBUG(LOG_ECP, "gotoStateConnecting " << tries);
	if(tries <= 0) {
		state = State_Idle;
		observer->procRecvError(Error_TooManyTries);
		return;
	}
	tries--;
	uart->send(Control_ENQ);
	timer->start(ECP_CONNECT_TIMEOUT);
	state = State_Connecting;
}

void ClientPacketLayer::stateConnectionRecv() {
	LOG_DEBUG(LOG_ECP, "stateConnectionRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_ACK) {
		LOG_INFO(LOG_ECP, "ACK");
		gotoStateReady();
		observer->procConnect();
		return;
	} else {
		LOG_ERROR(LOG_ECP, "Unwaited answer " << b1);
		gotoStateConnecting();
		return;
	}
}

void ClientPacketLayer::gotoStateReady() {
	timer->start(ECP_KEEP_ALIVE_PERIOD);
	state = State_Ready;
}

void ClientPacketLayer::gotoStateConfirm() {
	const uint8_t *data = sendBuf->getData();
	uint8_t dataLen = sendBuf->getLen();
	LOG_DEBUG(LOG_ECP, "gotoStateConfirm " << dataLen);
	LOG_TRACE_HEX(LOG_ECP, data, dataLen);

	uart->send(Control_STX);
	uart->send(dataLen);
	for(uint8_t i = 0; i < dataLen; i++) {
		uart->send(data[i]);
	}
	uart->send(Crc::calc(sendBuf));

	sendBuf = NULL;
	timer->start(ECP_CONFIRM_TIMEOUT);
	state = State_Confirm;
}

void ClientPacketLayer::stateConfirmRecv() {
	LOG_DEBUG(LOG_ECP, "stateConfirmRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_ACK) {
		gotoStateLenght();
		return;
	} else if(b1 == Control_NAK) {
		LOG_ERROR(LOG_ECP, "Negative confirm " << b1);
#if 0
		procError(); //todo: тут рекомендуют 10 попыток перепосылки запроса
#else
		gotoStateConfirm();
#endif
		return;
	} else if(b1 == Control_EOT) {
		uart->send(Control_EOT);
		timer->stop();
		state = State_Idle;
		observer->procDisconnect();
		return;
	} else {
		LOG_ERROR(LOG_ECP, "Unwaited answer " << b1);
		procError();
		return;
	}
}

void ClientPacketLayer::gotoStateLenght() {
	LOG_DEBUG(LOG_ECP, "gotoStateLenght");
	state = State_Length;
}

void ClientPacketLayer::stateLengthRecv() {
	LOG_DEBUG(LOG_ECP, "stateLengthRecv");
	recvLength = uart->receive();
	recvBuf->clear();
	LOG_DEBUG(LOG_ECP, "responseLength=" << recvLength);
	state = State_Data;
}

void ClientPacketLayer::stateDataRecv() {
	LOG_DEBUG(LOG_ECP, "stateDataRecv");
	uint8_t b = uart->receive();
	LOG_TRACE(LOG_ECP, "read[" << recvBuf->getLen() << "]=" << b);
	recvBuf->addUint8(b);
	if(recvBuf->getLen() >= recvLength) {
		state = State_CRC;
	}
}

void ClientPacketLayer::stateCRCRecv() {
	LOG_DEBUG(LOG_ECP, "stateCRCRecv");
	uint8_t recvCrc = uart->receive();
	uint8_t calcCrc = Crc::calc(recvBuf);
	if(recvCrc == calcCrc) {
		gotoStateAnswer();
		return;
	} else {
		LOG_ERROR(LOG_ECP, "Wrong CRC: recv=" << recvCrc << ", calc=" << calcCrc);
		procError();
		return;
	}
}

void ClientPacketLayer::gotoStateAnswer() {
	LOG_DEBUG(LOG_ECP, "gotoStateAnswer");
	timer->start(ECP_DELIVER_TIMEOUT);
	state = State_Answer;
}

void ClientPacketLayer::stateAnswerTimeout() {
	LOG_DEBUG(LOG_ECP, "stateAnswerTimeout");
	gotoStateReady();
	observer->procRecvData(recvBuf->getData(), recvBuf->getLen());
}

void ClientPacketLayer::gotoKeepAlive() {
	LOG_DEBUG(LOG_ECP, "gotoKeepAlive");
	uart->send(Control_ENQ);
	timer->start(ECP_CONFIRM_TIMEOUT);
	state = State_KeepAlive;
}

void ClientPacketLayer::stateKeepAliveRecv() {
	LOG_DEBUG(LOG_ECP, "stateKeepAliveRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_ACK) {
		if(sendBuf == NULL) {
			gotoStateReady();
		} else {
			gotoStateConfirm();
		}
		return;
	} else if(b1 == Control_EOT) {
		uart->send(Control_EOT);
		timer->stop();
		state = State_Idle;
		observer->procDisconnect();
		return;
	} else {
		LOG_ERROR(LOG_ECP, "Unwaited answer " << b1);
		return;
	}
}

void ClientPacketLayer::stateKeepAliveTimeout() {
	LOG_DEBUG(LOG_ECP, "stateKeepAliveTimeout");
	state = State_Idle;
	observer->procDisconnect();
}

void ClientPacketLayer::gotoStateDisconnecting() {
	uart->send(Control_EOT);
	timer->start(ECP_DISCONNECT_TIMEOUT);
	state = State_Disconnecting;
}

void ClientPacketLayer::stateStateDisconnectingRecv() {
	LOG_DEBUG(LOG_ECP, "stateStateDisconnectingRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_EOT) {
		timer->stop();
		state = State_Idle;
		observer->procDisconnect();
		return;
	} else {
		LOG_ERROR(LOG_ECP, "Unwaited answer " << b1);
		return;
	}
}

void ClientPacketLayer::stateStateDisconnectingTimeout() {
	LOG_DEBUG(LOG_ECP, "stateStateDisconnectingTimeout");
	state = State_Idle;
	observer->procDisconnect();
}

void ClientPacketLayer::procError() {
	LOG_DEBUG(LOG_ECP, "gotoStateNoConnection");
	timer->stop();
	state = State_Idle;
	observer->procRecvError(Error_Timeout);
}

}
