#include "common/firmware/include/Firmware.h"
#include "common/logger/include/Logger.h"

#include "common/utils/include/Version.h"

#include <stdio.h>

#define STM32_FLASH_SIZEOF 0x188 // сдвиг до начала данных, установлен эксперементально
#define SHA1_SIZE 20
#define HEADER_SEQUENCE 0x12345678
#define CRC_SEQUENCE 0x87654321

Firmware::Firmware(uint8_t *startAddress) : startAddress(startAddress) {
}

uint32_t Firmware::getHeaderSequence() {
	return *((uint32_t*)getHeaderAddress());
}

uint32_t Firmware::getApplicationSize() {
	return *(((uint32_t*)getHeaderAddress()) + 1);
}

uint32_t Firmware::getHardwareVersion() {
	return *(((uint32_t*)getHeaderAddress()) + 2);
}

uint32_t Firmware::getSoftwareVersion() {
	return *(((uint32_t*)getHeaderAddress()) + 3);
}

uint32_t Firmware::getCrcSequence() {
	return *((uint32_t*)getCrcAddress());
}

uint8_t *Firmware::getCrc() {
	return (getCrcAddress() + 4);
}

uint32_t Firmware::getCrcSize() {
	return HASH_LENGTH;
}

/*
 * –ассчитывает и перезаписывает контрольную сумму прошивки.
 * –аботает только дл€ прошивки загруженной в оперативную пам€ть.
 */
void Firmware::updateCrc() {
	calcCrc();
	uint8_t *src = crc.state.b;
	uint8_t *dst = getCrc();
	for(uint32_t i = 0; i < getCrcSize(); i++) {
		dst[i] = src[i];
	}
}

bool Firmware::checkSequence() {
	if(getHeaderSequence() != HEADER_SEQUENCE) {
		return false;
	}
	if(getCrcSequence() != CRC_SEQUENCE) {
		return false;
	}
	return true;
}

bool Firmware::checkCrc() {
	calcCrc();
	uint8_t *crc1 = crc.state.b;
	uint8_t *crc2 = getCrc();
	LOG_INFO_HEX(LOG_BOOT, crc1, getCrcSize());
	LOG_INFO_HEX(LOG_BOOT, crc2, getCrcSize());
	for(uint32_t i = 0; i < getCrcSize(); i++) {
		if(crc1[i] != crc2[i]) {
			return false;
		}
	}
	return true;
}

void Firmware::print() {
#ifdef LOGGING
#if LOG_BOOT >= LOG_LEVEL_INFO
	Logger *logger = Logger::get();
	*logger << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>" << Logger::endl;
	*logger << "StartAddress=" << (uint32_t)startAddress << Logger::endl;
	*logger << "HeaderAddress=" << (uint32_t)getHeaderAddress() << Logger::endl;
	*logger << "HeaderSequence="; logger->hex(getHeaderAddress(), sizeof(uint32_t)); *logger << Logger::endl;
	*logger << "ApplicationSize=" << getApplicationSize() << Logger::endl;
	*logger << "HardwareVersion=" << LOG_VERSION(getHardwareVersion()) << Logger::endl;
	*logger << "SoftwareVersion=" << LOG_VERSION(getSoftwareVersion()) << Logger::endl;
	*logger << "CrcAddress=" << (uint32_t)getCrcAddress() << Logger::endl;
	*logger << "CrcSiquence="; logger->hex(getCrcAddress(), sizeof(uint32_t)); *logger << Logger::endl;
	*logger << "Crc="; logger->hex((uint8_t*)(getCrcAddress() + 4), getCrcSize()); *logger << Logger::endl;
	*logger << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>" << Logger::endl;
#endif
#endif
}

uint8_t *Firmware::getHeaderAddress() {
	return (startAddress + STM32_FLASH_SIZEOF);
}

uint8_t *Firmware::getCrcAddress() {
	return (startAddress + getApplicationSize());
}

void Firmware::calcCrc() {
	sha1_init(&crc);
	sha1_write(&crc, (const char*)startAddress, getApplicationSize());
	sha1_result(&crc);
}
