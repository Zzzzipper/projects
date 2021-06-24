#ifndef COMMON_CONFIG_PRICEINDEX1_H_
#define COMMON_CONFIG_PRICEINDEX1_H_

#include "Evadts1Protocol.h"
#include "memory/include/Memory.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/List.h"

#pragma pack(push,1)
struct Config1PriceIndex {
	StrParam<Evadts1PaymentDeviceSize> device;
	uint8_t number;
};
#pragma pack(pop)

class Config1PriceIndexList {
public:
	void init(Memory *memory);
	MemoryResult save(Memory *memory);
	MemoryResult load(uint16_t priceListNum, Memory *memory);
	void clear();
	bool add(const char *device, uint8_t number);
	uint16_t getIndex(const char *device, uint8_t number);
	uint16_t getSize() { return list.getSize(); }
	Config1PriceIndex* get(uint16_t index);
private:
	List<Config1PriceIndex> list;
};

#endif
