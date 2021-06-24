#include "MdbSlave.h"
#include "logger/include/Logger.h"
#include "common.h"

MdbSlave::Sender::Sender() : buf(MDB_PACKET_MAX_SIZE) {}

void MdbSlave::Sender::sendData(const uint8_t *data, uint16_t dataLen) {
	startData();
	addData(data, dataLen);
	sendData();
}

void MdbSlave::Sender::startData() {
	this->buf.clear();
}

void MdbSlave::Sender::addUint8(uint8_t data) {
	this->buf.addUint8(data);
}

void MdbSlave::Sender::addUint16(uint16_t data) {
	this->buf.addUint8(data >> 8);
	this->buf.addUint8(data & 0xFF);
}

void MdbSlave::Sender::addData(const uint8_t *data, uint16_t dataLen) {
	for(uint16_t i = 0; i < dataLen; i++) {
		this->buf.addUint8(data[i]);
	}
}
