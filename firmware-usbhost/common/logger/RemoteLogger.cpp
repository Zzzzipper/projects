#include <logger/RemoteLogger.h>
#include "utils/include/Hex.h"
#include "logger/include/Logger.h"

#include <stdlib.h>

FifoBuf::FifoBuf(uint32_t dataSize) {
	this->data = new char[dataSize];
	this->dataSize = dataSize;
	this->dataLen = 0;
	this->index = 0;
}

FifoBuf::~FifoBuf() {
	delete data;
}

void FifoBuf::push(char b) {
	data[index] = b;
	index++;
	if(index >= dataSize) { index = 0; }
	if(dataLen < dataSize) { dataLen++; }
}

char* FifoBuf::getData() {
	return data;
}

uint32_t FifoBuf::getDataLen() {
	return dataLen;
}

void FifoBuf::clear() {
	index = 0;
	dataLen = 0;
}

RemoteLogger* RemoteLogger::instance = NULL;

RemoteLogger *RemoteLogger::get() {
	if(instance == NULL) {
		instance = new RemoteLogger();
	}
	return instance;
}

RemoteLogger::RemoteLogger() :
	realtime(NULL),
	buf(30000)
{
}

RemoteLogger::~RemoteLogger() {
	if(instance != NULL) {
		delete instance;
	}
}

void RemoteLogger::registerRealTime(RealTimeInterface *realtime) {
	this->realtime = realtime;
}

void RemoteLogger::time() {
	if(realtime == NULL) { return; }
	DateTime d;
	realtime->getDateTime(&d);
	buf.push('0' + d.minute / 10);
	buf.push('0' + d.minute % 10);
	buf.push(':');
	buf.push('0' + d.second / 10);
	buf.push('0' + d.second % 10);
}

void RemoteLogger::state(const char *prefix, uint32_t state) {
	time();
	str(prefix);
	buf.push('#');
	buf.push(0x30 + state/10);
	buf.push(0x30 + state%10);
	buf.push('\r');
	buf.push('\n');
}

void RemoteLogger::str(const char *str) {
	if(str == NULL) { return; }
	for(const char *s = str; *s != '\0'; s++) {
		buf.push(*s);
	}
}

void RemoteLogger::hex(const uint8_t symbol) {
	buf.push(digitToHex(symbol/16));
	buf.push(digitToHex(symbol%16));
}

void RemoteLogger::hex(const char *prefix, const uint8_t b1) {
	time();
	str(prefix);
	hex(b1);
	buf.push('\r');
	buf.push('\n');
}

void RemoteLogger::hex(const char *prefix, const void *data, uint16_t len) {
	uint8_t *d = (uint8_t*)data;
	time();
	str(prefix);
	for(uint16_t i = 0; i < len; i++) {
		this->hex(d[i]);
	}
	buf.push('\r');
	buf.push('\n');
}

char *RemoteLogger::getData() {
	return buf.getData();
}

uint32_t RemoteLogger::getDataLen() {
	return buf.getDataLen();
}

void RemoteLogger::send(const uint8_t *data, const uint16_t len) {
	for(uint16_t i = 0; i < len; i++) {
		buf.push(data[i]);
	}
}
