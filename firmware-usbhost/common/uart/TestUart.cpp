#include "include/TestUart.h"
#include "utils/include/Hex.h"
#include "logger/include/Logger.h"

TestUart::TestUart(uint16_t size) : recvQueue(size), sendBuffer(size) {
}

bool TestUart::addRecvData(const char *hex) {
	const char *h = hex;
	for(; *h != '\0'; h++) {
		uint8_t c = 0;
		uint8_t d = hexToDigit(*h);
		if(d > 15) {
			LOG_ERROR(LOG_TEST, "ERROR");
			return false;
		}
		c = (d << 4);
		h++;
		if(*h == '\0') {
			LOG_ERROR(LOG_TEST, "ERROR");
			return false;
		}
		d = hexToDigit(*h);
		if(d > 15) {
			LOG_ERROR(LOG_TEST, "ERROR");
			return false;
		}
		c |= d;
		recvQueue.push(c);
	}
	execute();
	return true;
}

void TestUart::addRecvData(void *data, uint16_t dataLen) {
	uint8_t *d = (uint8_t*)data;
	for(uint16_t i = 0; i < dataLen; i++) {
		recvQueue.push(d[i]);
	}
	execute();
}

void TestUart::addRecvData(uint8_t byte) {
	recvQueue.push(byte);
	execute();
}

void TestUart::addRecvString(const char *str) {
	for(const char *s = str; *s != '\0'; s++) {
		recvQueue.push(*s);
	}
	execute();
}

uint8_t *TestUart::getSendData() {
	return sendBuffer.getData();
}

uint16_t TestUart::getSendLen() {
	return sendBuffer.getLen();
}

void TestUart::clearSendBuffer() {
	sendBuffer.clear();
}

void TestUart::send(uint8_t byte) {
	sendBuffer.addUint8(byte);
}

void TestUart::sendAsync(uint8_t byte) {
	sendBuffer.addUint8(byte);
}

uint8_t TestUart::receive() {
	return recvQueue.pop();
}

bool TestUart::isEmptyReceiveBuffer() {
	return recvQueue.isEmpty();
}

bool TestUart::isFullTransmitBuffer() {
	return false;
}

void TestUart::setReceiveHandler(UartReceiveHandler *handler) {
	this->recieveHandler = handler;
}

void TestUart::setTransmitHandler(UartTransmitHandler *handler) {
	this->transmitHandler = handler;
}

void TestUart::execute() {
	if(recieveHandler == NULL) {
		LOG_ERROR(LOG_TEST, "ReceiveHandler not inited");
		return;
	}
	if(recieveHandler->getLen() > 0 && recvQueue.isEmpty() == false) {
		recieveHandler->handle();
	}
}
