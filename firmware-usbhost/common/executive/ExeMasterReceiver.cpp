#include "ExeMasterReceiver.h"
#include "ExeProtocol.h"

#include "logger/include/Logger.h"

ExeMasterReceiver::ExeMasterReceiver(Customer *customer) : customer(customer), uart(NULL), sendUart(NULL) {}

void ExeMasterReceiver::setUart(AbstractUart *uart, AbstractUart *sendUart) {
	this->uart = uart;
	uart->setReceiveHandler(this);
	this->sendUart = sendUart;
}

void ExeMasterReceiver::handle() {
	if(uart == NULL) {
		return;
	}
	while(uart->isEmptyReceiveBuffer() == false) {
		uint8_t b1 = uart->receive();
		LOG_TRACE(LOG_EXE, "resp: b1=" << b1);
		if(sendUart != NULL) {
			LOG_TRACE(LOG_EXE, "resend: b1=" << b1);
			sendUart->send(b1);
		}
		this->customer->recvResponse(b1);
	}
}

bool ExeMasterReceiver::isInterruptMode() {
	return true;
}
