#include "MdbSlaveSender.h"
#include "logger/include/Logger.h"
#include "common.h"

MdbSlaveSender::MdbSlaveSender() : uart(NULL) {}

void MdbSlaveSender::setUart(AbstractUart *uart) {
	this->uart = uart;
}

void MdbSlaveSender::sendAnswer(Mdb::Control answer) {
	if(this->uart == NULL) {
		return;
	}
	this->uart->sendAsync((uint8_t)0x01);
	this->uart->sendAsync(answer);
}

void MdbSlaveSender::sendData() {
	if(this->uart == NULL) {
		return;
	}
	for(uint16_t i = 0; i < buf.getLen(); i++) {
		this->uart->sendAsync((uint8_t)0x00);
		this->uart->sendAsync(buf[i]);
	}
	uint8_t chk = Mdb::calcCrc(buf.getData(), buf.getLen());
	this->uart->sendAsync((uint8_t)0x01);
	this->uart->sendAsync(chk);
}
