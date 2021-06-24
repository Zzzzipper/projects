#ifndef COMMON_CONFIG_PRICELIST2_H_
#define COMMON_CONFIG_PRICELIST2_H_

#include "config/v2/automat/Config2AutomatData.h"
#include "config/v3/automat/Config3PriceIndex.h"
#include "memory/include/MemoryCrc.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/List.h"

class Config2Price {
public:
	Config2PriceData data;

	void setDefault();
	MemoryResult load(Memory *memory);
	MemoryResult save(Memory *memory);
};

class Config2PriceList {
public:
	MemoryResult load(uint16_t priceListNum, Memory *memory);
	MemoryResult save(Memory *memory);

	void add(Config2Price *price);
	Config2Price *getByIndex(uint16_t index) { return list.get(index); }
	uint16_t getSize() { return list.getSize(); }

private:
	List<Config2Price> list;
};

#endif
