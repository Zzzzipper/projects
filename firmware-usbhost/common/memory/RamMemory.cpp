#include "include/RamMemory.h"

#include "logger/include/Logger.h"

MemoryResult RamMemory::write(const void *pData, uint32_t len) {
	if((address + len) >= buf.getSize()) {
		LOG_ERROR(LOG_CFG, "try write out of buffer");
		return MemoryResult_WriteError;
	}
	buf.setLen(address);
	buf.add(pData, len);
	address += len;
	return MemoryResult_Ok;
}

MemoryResult RamMemory::read(void *pData, uint32_t size) {
	if((address + size) >= buf.getSize()) {
		LOG_ERROR(LOG_CFG, "try read out of buffer");
		return MemoryResult_ReadError;
	}
	uint32_t len = buf.getSize() - address;
	uint8_t *src = buf.getData() + address;
	if(len > size) {
		len = size;
	}
	uint8_t *dst = (uint8_t*)pData;
	for(uint32_t i = 0; i < len; i++) {
		dst[i] = src[i];
	}
	address += len;
	return MemoryResult_Ok;
}

uint32_t RamMemory::getMaxSize() const {
	return buf.getSize();
}

uint32_t RamMemory::getPageSize() const {
	return 0;
}

void RamMemory::clear() {
	fill(0x00);
}

void RamMemory::fill(uint8_t b) {
	uint8_t *data = buf.getData();
	for(uint32_t i = 0; i < buf.getSize(); i++) {
		data[i] = b;
	}
}

void RamMemory::printAllData() {
	for(uint32_t i = 0; i < buf.getSize(); i += 20) {
		uint32_t len = buf.getSize() - i;
		if(len > 20) { len = 20; }
		LOG_HEX(buf.getData() + i, len);
	}
}
