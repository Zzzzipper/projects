#include "EcpServerPacketLayer.h"
#include "EcpProtocol.h"
#include "logger/include/Logger.h"

namespace Ecp {

ServerPacketLayer::ServerPacketLayer(TimerEngine *timers, AbstractUart *uart) :
	uart(uart),
	timers(timers),
	observer(NULL),
	state(State_Idle)
{
	this->recvBuf = new Buffer(ECP_PACKET_MAX_SIZE);
	this->timer = timers->addTimer<ServerPacketLayer, &ServerPacketLayer::procTimer>(this);
}

ServerPacketLayer::~ServerPacketLayer() {
	timers->deleteTimer(this->timer);
	delete this->recvBuf;
}

void ServerPacketLayer::setObserver(Observer *observer) {
	this->observer = observer;
}

void ServerPacketLayer::reset() {
	LOG_DEBUG(LOG_ECP, "reset");
	uart->setReceiveHandler(this);
	command = Command_None;
	state = State_Connecting;
}

void ServerPacketLayer::shutdown() {
	LOG_DEBUG(LOG_ECP, "shutdown");
	uart->setReceiveHandler(NULL);
	state = State_Idle;
}

void ServerPacketLayer::disconnect() {
	LOG_DEBUG(LOG_ECP, "disconnect");
	if(state == State_Idle) {
		observer->procDisconnect();
		return;
	}
	command = Command_Disconnect;
}

bool ServerPacketLayer::sendData(const Buffer *sendBuf) {
	const uint8_t *data = sendBuf->getData();
	uint8_t dataLen = sendBuf->getLen();
	LOG_DEBUG(LOG_ECP, "sendData " << dataLen);
	LOG_TRACE_HEX(LOG_ECP, data, dataLen);

	uart->send(Control_ACK);
	uart->send(dataLen);
	for(uint8_t i = 0; i < dataLen; i++) {
		uart->send(data[i]);
	}
	uart->send(Crc::calc(sendBuf));
	return true;
}

void ServerPacketLayer::handle() {
	LOG_TRACE(LOG_ECP, "handle");
	while(uart->isEmptyReceiveBuffer() == false) {
		LOG_TRACE(LOG_ECP, "have data");
		switch(state) {
		case State_Connecting: stateConnectingRecv(); break;
		case State_STX: stateSTXRecv(); break;
		case State_Length: stateLengthRecv(); break;
		case State_Data: stateDataRecv(); break;
		case State_CRC: stateCRCRecv(); break;
		case State_Disconnecting: stateDisconnectingRecv(); break;
		default: LOG_ERROR(LOG_ECP, "Unwaited data state=" << state << ", byte=" << uart->receive());
		}
	}
}

void ServerPacketLayer::procTimer() {
	LOG_DEBUG(LOG_ECP, "procTimer " << state);
	switch(state) {
		case State_STX: gotoStateConnecting(); break;
		case State_Length: gotoStateSTX(); break;
		case State_Data: gotoStateSTX(); break;
		case State_CRC: gotoStateSTX(); break;
		case State_Answer: stateAnswerTimeout(); break;
		case State_Disconnecting: gotoStateConnecting(); break;
		default: LOG_ERROR(LOG_ECP, "Unwaited timeout state=" << state);
	}
}

void ServerPacketLayer::gotoStateConnecting() {
	LOG_DEBUG(LOG_ECP, "gotoStateConnecting");
	timer->stop();
	command = Command_None;
	state = State_Connecting;
	observer->procDisconnect();
}

void ServerPacketLayer::stateConnectingRecv() {
	LOG_DEBUG(LOG_ECP, "stateConnectingRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_ENQ) {
		LOG_INFO(LOG_ECP, "ENQ");
		uart->send(Control_ACK);
		gotoStateSTX();
		observer->procConnect();
		return;
	}
}

void ServerPacketLayer::gotoStateSTX() {
	LOG_DEBUG(LOG_ECP, "gotoStateSTX");
	if(command == Command_Disconnect) {
		gotoStateConnecting();
		return;
	}
	timer->start(ECP_KEEP_ALIVE_TIMEOUT);
	state = State_STX;
}

void ServerPacketLayer::stateSTXRecv() {
	LOG_DEBUG(LOG_ECP, "stateSTXRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_ENQ) {
		LOG_INFO(LOG_ECP, "ENQ");
		if(command == Command_Disconnect) {
			uart->send(Control_EOT);
			timer->start(ECP_DISCONNECT_TIMEOUT);
			state = State_Disconnecting;
			return;
		}
		uart->send(Control_ACK);
		timer->start(ECP_KEEP_ALIVE_TIMEOUT);
		return;
	} else if(b1 == Control_EOT) {
		LOG_INFO(LOG_ECP, "EOT");
		uart->send(Control_ACK);
		gotoStateConnecting();
		return;
	} else if(b1 == Control_STX) {
		gotoStateLenght();
		return;
	} else {
		LOG_ERROR(LOG_ECP, "Unwaited answer " << b1);
		return;
	}
}

void ServerPacketLayer::gotoStateLenght() {
	LOG_DEBUG(LOG_ECP, "gotoStateLenght");
	timer->start(ECP_PACKET_TIMEOUT);
	state = State_Length;
}

void ServerPacketLayer::stateLengthRecv() {
	LOG_DEBUG(LOG_ECP, "stateLengthRecv");
	recvLength = uart->receive();
	recvBuf->clear();
	LOG_DEBUG(LOG_ECP, "responseLength=" << recvLength);
	state = State_Data;
}

void ServerPacketLayer::stateDataRecv() {
	LOG_DEBUG(LOG_ECP, "stateDataRecv");
	uint8_t b = uart->receive();
	LOG_TRACE(LOG_ECP, "read[" << recvBuf->getLen() << "]=" << b);
	recvBuf->addUint8(b);
	if(recvBuf->getLen() >= recvLength) {
		state = State_CRC;
	}
}

void ServerPacketLayer::stateCRCRecv() {
	LOG_DEBUG(LOG_ECP, "stateCRCRecv");
	if(command == Command_Disconnect) {
		uart->send(Control_EOT);
		timer->start(ECP_DISCONNECT_TIMEOUT);
		state = State_Disconnecting;
		return;
	}

	uint8_t recvCrc = uart->receive();
	uint8_t calcCrc = Crc::calc(recvBuf);
	if(recvCrc == calcCrc) {
		gotoStateAnswer();
		return;
	} else {
		LOG_ERROR(LOG_ECP, "Wrong CRC: recv=" << recvCrc << ", calc=" << calcCrc);
		uart->send(Control_NAK);
		gotoStateSTX();
		return;
	}
}

void ServerPacketLayer::gotoStateAnswer() {
	LOG_DEBUG(LOG_ECP, "gotoStateAnswer");
	timer->start(ECP_DELIVER_TIMEOUT);
	state = State_Answer;
}

void ServerPacketLayer::stateAnswerTimeout() {
	LOG_DEBUG(LOG_ECP, "stateAnswerTimeout");
	gotoStateSTX();
	observer->procRecvData(recvBuf->getData(), recvBuf->getLen());
}

void ServerPacketLayer::stateDisconnectingRecv() {
	LOG_DEBUG(LOG_ECP, "stateDisconnectingRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_EOT) {
		gotoStateConnecting();
		return;
	}
}

}
