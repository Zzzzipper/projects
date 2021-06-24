#include "MdbMasterSender.h"

#include "logger/include/Logger.h"
#include "common.h"

MdbMasterSender::MdbMasterSender() : uart(NULL), buf(MDB_PACKET_MAX_SIZE) {}

void MdbMasterSender::setUart(AbstractUart *uart) {
	this->uart = uart;
}

void MdbMasterSender::sendRequest(void *data, uint16_t dataLen) {
	startRequest();
	addData((uint8_t*)data, dataLen);
	sendRequest();
}

void MdbMasterSender::sendConfirm(Mdb::Control control) {
	if(this->uart == NULL) {
		return;
	}
	this->uart->sendAsync((uint8_t)0x00);
	this->uart->sendAsync(control);
	LOG_TRACE(LOG_MDBM, "control=" << control);
}

void MdbMasterSender::startRequest() {
	this->buf.clear();
}

void MdbMasterSender::addUint8(uint8_t data) {
	this->buf.addUint8(data);
}

void MdbMasterSender::addUint16(uint16_t data) {
	this->buf.addUint8(data >> 8);
	this->buf.addUint8(data & 0xFF);
}

void MdbMasterSender::addData(const uint8_t *data, uint16_t dataLen) {
	for(uint16_t i = 0; i < dataLen; i++) {
		this->buf.addUint8(data[i]);
	}
}

void MdbMasterSender::sendRequest() {
	if(this->uart == NULL) {
		return;
	}
	if(buf.getLen() > 0) {
		this->uart->sendAsync((uint8_t)0x01);
		this->uart->sendAsync(buf[0]);
	}
	for(uint16_t i = 1; i < buf.getLen(); i++) {
		this->uart->sendAsync((uint8_t)0x00);
		this->uart->sendAsync(buf[i]);
	}
	uint8_t chk = Mdb::calcCrc(buf.getData(), buf.getLen());
	this->uart->sendAsync((uint8_t)0x00);
	this->uart->sendAsync(chk);
	LOG_TRACE_HEX(LOG_MDBM, buf.getData(), buf.getLen());
}
