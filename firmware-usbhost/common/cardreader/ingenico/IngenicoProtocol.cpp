#include "IngenicoProtocol.h"

#include "utils/include/Number.h"

namespace Ingenico {

Packet::Packet(uint16_t size) :
	buf(size)
{

}

void Packet::clear() {
	buf.clear();
}

void Packet::addControl(uint8_t control) {
	buf.addUint8(control);
}

void Packet::addSymbol(char symbol) {
	buf.addUint8(symbol);
}

void Packet::addString(const char *str) {
	for(uint16_t i = 0; str[i] != '\0'; i++) {
		buf.addUint8(str[i]);
	}
}

void Packet::addString(const char *str, uint16_t strLen) {
	for(uint16_t i = 0; i < strLen; i++) {
		buf.addUint8(str[i]);
	}
}

void Packet::addNumber(uint32_t number) {
	uint16_t len = Sambery::numberToString<uint32_t>(number, (char*)(buf.getData() + buf.getLen()), buf.getSize() - buf.getLen());
	buf.setLen(buf.getLen() + len);
}

Buffer *Packet::getBuf() {
	return &buf;
}

uint8_t *Packet::getData() {
	return buf.getData();
}

uint32_t Packet::getDataLen() {
	return buf.getLen();
}

}
