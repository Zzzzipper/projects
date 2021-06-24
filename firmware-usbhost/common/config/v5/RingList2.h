#ifndef COMMON_CONFIG_V5_RINGLIST2_H_
#define COMMON_CONFIG_V5_RINGLIST2_H_

#include "memory/include/Memory.h"
#include "memory/include/MemoryCrc.h"

#include <stdint.h>

#define RINGLIST2_UNDEFINED 0xFFFFFFFF

class RingList2 {
public:
	RingList2();
	MemoryResult init(uint32_t dataSize, uint32_t size, Memory *memory);
	MemoryResult insert(void *data, uint32_t dataLen);
	MemoryResult get(uint32_t index, void *buf, uint32_t bufSize);
	uint32_t getSize();
	uint32_t getLen();
	uint32_t getFirst();
	uint32_t getLast();
	uint32_t getTail();

private:
	Memory *memory;
	uint32_t address;
	uint32_t dataSize;
	uint32_t size;
	uint32_t id;
	uint32_t first;
	uint32_t last;

	uint32_t getEntrySize();
	MemoryResult initHeadData(uint32_t dataSize, uint32_t size, Memory *memory);
	MemoryResult saveHeadData();
	MemoryResult initEntryData(Memory *memory);
	MemoryResult saveEntryData(void *data, uint32_t dataLen);
	MemoryResult loadEntryData(void *buf, uint32_t bufSize);
	void incrementPosition();
};

#endif
