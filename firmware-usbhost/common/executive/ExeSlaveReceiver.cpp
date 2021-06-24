#include "ExeSlaveReceiver.h"
#include "ExeProtocol.h"

#include "logger/include/Logger.h"

ExeSlaveReceiver::ExeSlaveReceiver(Customer *customer) : customer(customer), uart(NULL), sendUart(NULL) {}

void ExeSlaveReceiver::setUart(AbstractUart *uart, AbstractUart *sendUart) {
	this->uart = uart;
	uart->setReceiveHandler(this);
	this->sendUart = sendUart;
}

void ExeSlaveReceiver::handle() {
	if(uart == NULL) {
		return;
	}
	while(uart->isEmptyReceiveBuffer() == false) {
		uint8_t b1 = uart->receive();
		LOG_TRACE(LOG_EXE, "req: b1=" << b1);
		if(sendUart != NULL) {
			LOG_TRACE(LOG_EXE, "resend: b1=" << b1);
			sendUart->send(b1);
		}
		this->customer->recvRequest(b1);
	}
}

bool ExeSlaveReceiver::isInterruptMode() {
	return true;
}
