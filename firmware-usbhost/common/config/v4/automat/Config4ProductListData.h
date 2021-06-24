#ifndef COMMON_CONFIG_V4_PRODUCTLISTDATA_H_
#define COMMON_CONFIG_V4_PRODUCTLISTDATA_H_

#include "memory/include/Memory.h"

#pragma pack(push,1)
struct Config4ProductListDataStruct {
	uint16_t productNum;
	uint16_t priceListNum;
	uint8_t  crc[1];
};
#pragma pack(pop)

class Config4ProductListData {
public:
	Config4ProductListDataStruct data;

	MemoryResult init(uint16_t productNum, uint16_t priceListNum, Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult save();

	Memory *getMemory() { return memory; }
	uint32_t getAddress() { return address; }
	static uint32_t getDataSize();

private:
	Memory *memory;
	uint32_t address;
};

#endif
