#ifndef COMMON_CONFIG_CONFIGPRODUCT_H_
#define COMMON_CONFIG_CONFIGPRODUCT_H_

#include "Config3PriceList.h"
#include "Config3AutomatData.h"
#include "memory/include/Memory.h"

class Config3Product {
public:
	Config3ProductData data;
	Config3PriceList prices;

	void setDefault();
	MemoryResult init(uint16_t priceListNum, Memory *memory);
	MemoryResult load(uint16_t priceListNum, Memory *memory);
	MemoryResult save();
	MemoryResult reset();
	MemoryResult resetTotal();
	MemoryResult sale(uint16_t priceIndex, uint32_t value);

	MemoryResult setEnable(uint8_t enable);
	MemoryResult setWareId(uint32_t wareId);
	MemoryResult setName(const char *name);
	MemoryResult setTaxRate(uint16_t taxRate);
	MemoryResult setFreeCount(uint32_t freeTotalCount, uint32_t freeCount);
	MemoryResult setCount(uint32_t totalCount, uint32_t totalMoney, uint32_t count, uint32_t money);

	static uint32_t getDataSize(uint16_t priceListNum);

private:
	Memory *memory;
	uint32_t address;
	MemoryResult initData(Memory *memory);
	MemoryResult loadData(Memory *memory);
	MemoryResult saveData();
};

#endif
