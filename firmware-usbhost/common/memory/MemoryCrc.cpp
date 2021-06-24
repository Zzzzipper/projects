#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

MemoryCrc::MemoryCrc(Memory *memory) : memory(memory) {}

void MemoryCrc::startCrc() {
	crc = 0xFF;
}

MemoryResult MemoryCrc::write(const void *data, uint16_t len) {
	MemoryResult result = memory->write(data, len);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "read failed " << result);
		return result;
	}
	calcCrc((uint8_t*)data, len);
	return MemoryResult_Ok;
}

MemoryResult MemoryCrc::writeCrc() {
	LOG_DEBUG(LOG_CFG, "writeCrc");
	LOG_DEBUG_HEX(LOG_CFG, &crc, sizeof(crc));
	return memory->write(&crc, sizeof(crc));
}

MemoryResult MemoryCrc::read(void *buf, uint16_t size) {
	MemoryResult result = memory->read(buf, size);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "read failed " << result);
		return result;
	}
	calcCrc((uint8_t*)buf, size);
	return MemoryResult_Ok;
}

MemoryResult MemoryCrc::readCrc() {
	LOG_DEBUG(LOG_CFG, "readCrc");
	uint8_t crcBuf;
	MemoryResult result = memory->read(&crcBuf, sizeof(crcBuf));
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "read failed " << result);
		return result;
	}

	LOG_DEBUG_HEX(LOG_CFG, &crc, sizeof(crc));
	LOG_DEBUG_HEX(LOG_CFG, &crcBuf, sizeof(crcBuf));
	if(crc != crcBuf) {
		return MemoryResult_WrongCrc;
	}

	return MemoryResult_Ok;
}

MemoryResult MemoryCrc::writeDataWithCrc(const void *block, uint16_t blockLen) {
	uint8_t *data = (uint8_t*)block;
	uint16_t dataLen = blockLen - getCrcSize();
	startCrc();
	calcCrc(data, dataLen);
	data[dataLen] = crc;
	return memory->write(block, blockLen);
}

MemoryResult MemoryCrc::readDataWithCrc(void *block, uint16_t blockLen) {
	uint8_t *data = (uint8_t*)block;
	uint16_t dataLen = blockLen - getCrcSize();
	MemoryResult result = memory->read(block, blockLen);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "read failed " << result);
		return result;
	}

	startCrc();
	calcCrc(data, dataLen);
	if(data[dataLen] != crc) {
		LOG_ERROR(LOG_CFG, "wrong crc " << data[dataLen] << " != " << crc);
		return MemoryResult_WrongCrc;
	}

	return MemoryResult_Ok;
}

uint32_t MemoryCrc::getCrcSize() {
	return sizeof(uint8_t);
}

void MemoryCrc::calcCrc(const uint8_t *data, uint16_t len) {
	for(uint16_t i = 0; i < len; i++) {
		crc += data[i];
	}
}
