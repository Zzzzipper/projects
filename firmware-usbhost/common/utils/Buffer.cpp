#include "utils/include/Buffer.h"
#include "platform/include/platform.h"
#include "logger/include/Logger.h"

Buffer::Buffer(uint32_t size) : size(size), len(0) {
	this->buf = new uint8_t[this->size];
}

Buffer::~Buffer() {
	delete this->buf;
}

void Buffer::addUint8(uint8_t data) {
	if(len >= size) {
		return;
	}
	buf[len] = data;
	len++;
}

void Buffer::add(const void *data, const uint32_t dataLen) {
	uint8_t *d = (uint8_t*)data;
	for(uint32_t i = 0; i < dataLen; i++) {
		addUint8(d[i]);
	}
}

void Buffer::setLen(uint32_t newLen) {
	len = newLen >= size ? size : newLen;
}

void Buffer::remove(uint32_t pos, uint32_t num) {
	if(pos > len) { return; }
	uint32_t to = pos;
	uint32_t from = pos + num;
	for(; from < len; to++, from++) {
		buf[to] = buf[from];
	}
	uint32_t tail = 0;
	if(pos < len) { tail = len - pos; }
	if(num < tail) { tail = tail - num; } else { tail = 0; }
	len = pos + tail;
}

void Buffer::clear() {
	len = 0;
}

const uint8_t &Buffer::operator [](uint32_t index) const {
	if(index >= size) {
		index = size - 1;
	}
	return buf[index];
}

uint8_t &Buffer::operator [](uint32_t index) {
	if(index >= size) {
		index = size - 1;
	}
	return buf[index];
}

uint8_t *Buffer::getData() {
	return buf;
}

const uint8_t *Buffer::getData() const {
	return buf;
}

uint32_t Buffer::getLen() const {
	return len;
}

uint32_t Buffer::getSize() const {
	return size;
}
