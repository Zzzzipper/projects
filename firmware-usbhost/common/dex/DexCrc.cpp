#include "DexCrc.h"

namespace Dex {
	
Crc::Crc() : crc(0) {}

void Crc::start() {
	this->crc = 0;
}

void Crc::addUint8(const uint8_t value) {
	uint16_t BCC_0, BCC_1, BCC_14, DATA_0, X2, X15, X16;
	for(int j = 0; j < 8; j++) {
		DATA_0 = (value >> j) & 0x0001;
		BCC_0 = (this->crc & 0x0001);
		BCC_1 = (this->crc >> 1) & 0x0001;
		BCC_14 = (this->crc >> 14) & 0x0001;
		X16 = (BCC_0 ^ DATA_0) & 0x0001; // bit15 of BCC after shift
		X15 = (BCC_1 ^ X16) & 0x0001; // bit0 of BCC after shift
		X2 = (BCC_14 ^ X16) & 0x0001; // bit13 of BCC after shift
		this->crc = this->crc >> 1;
		this->crc = this->crc & 0x5FFE;
		this->crc = this->crc | (X15);
		this->crc = this->crc | (X2 << 13);
		this->crc = this->crc | (X16 << 15);
	}
}

void Crc::add(const char *string) {
	const char *s = string;
	while(*s) {
		this->addUint8(*s++);
	}
}

void Crc::add(const void *data, const uint16_t len) {
	const uint8_t *p = (uint8_t*)data;
	for(uint16_t i = 0; i < len; i++) {
		addUint8(p[i]);
	}
}

uint8_t Crc::getHighByte() {
	return 0xff & (this->crc >> 8);
}

uint8_t Crc::getLowByte() {
	return 0xff & this->crc;
}

}
