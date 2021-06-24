#ifndef COMMON_CONFIG3_PRICELIST_H_
#define COMMON_CONFIG3_PRICELIST_H_

#include "evadts/EvadtsProtocol.h"
#include "memory/include/MemoryCrc.h"
#include "utils/include/NetworkProtocol.h"
#include "Config3AutomatData.h"

class Config3Price {
public:
	Config3PriceData data;

	MemoryResult init(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult save();
	MemoryResult reset();
	MemoryResult resetTotal();
	MemoryResult sale(uint32_t value);

	uint32_t getPrice() { return data.price; }
	uint32_t getTotalCount() { return data.totalCount; }
	uint32_t getTotalMoney() { return data.totalMoney; }
	uint32_t getCount() { return data.count; }
	uint32_t getMoney() { return data.money; }

	static uint32_t getDataSize();

private:
	Memory *memory;
	uint32_t address;
};

class Config3PriceList {
public:
	MemoryResult init(uint16_t priceListNum, Memory *memory);
	MemoryResult load(uint16_t priceListNum, Memory *memory);
	MemoryResult save();
	MemoryResult reset();
	MemoryResult resetTotal();

	MemoryResult getByIndex(uint16_t priceIndex, Config3Price *price);
	MemoryResult sale(uint16_t priceIndex);

	static uint32_t getDataSize(uint16_t priceListNum);

private:
	Memory *memory;
	uint32_t address;
	uint16_t priceListNum;
};

#endif
