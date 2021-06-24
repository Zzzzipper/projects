#ifndef COMMON_CONFIG_V1_PRICELIST_H_
#define COMMON_CONFIG_V1_PRICELIST_H_

#include "Evadts1Protocol.h"
#include "memory/include/MemoryCrc.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/List.h"

#pragma pack(push,1)
struct Config1PriceData {
	uint32_t price;
	uint32_t totalCount;
	uint32_t totalMoney;
	uint32_t count;
	uint32_t money;
};
#pragma pack(pop)

class Config1Price {
public:
	Config1PriceData data;

	void setDefault();
	MemoryResult load(Memory *memory);
	MemoryResult save(Memory *memory);
};

class Config1PriceList {
public:
	MemoryResult load(uint16_t priceListNum, Memory *memory);
	MemoryResult save(Memory *memory);

	void add(Config1Price *price);
	Config1Price *getByIndex(uint16_t index) { return list.get(index); }
	uint16_t getSize() { return list.getSize(); }

private:
	List<Config1Price> list;
};

#endif
