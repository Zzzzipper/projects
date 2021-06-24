#ifndef COMMON_CONFIG_PRICEINDEX_H_
#define COMMON_CONFIG_PRICEINDEX_H_

#include "evadts/EvadtsProtocol.h"
#include "memory/include/Memory.h"
#include "timer/include/TimeTable.h"
#include "timer/include/DateTime.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/List.h"

enum Config3PriceIndexType {
	Config3PriceIndexType_None = 0,
	Config3PriceIndexType_Base = 1,
	Config3PriceIndexType_Time = 2,
};

class Config3PriceIndex {
public:
	StrParam<EvadtsPaymentDeviceSize> device;
	uint8_t number;
	uint8_t type;
	TimeTable timeTable;
};

class Config3PriceIndexList {
public:
	MemoryResult init(Memory *memory);
	MemoryResult load(uint16_t priceListNum, Memory *memory);
	MemoryResult save();
	MemoryResult save(Memory *memory);
	void clear();
	bool add(const char *device, uint8_t number, Config3PriceIndexType type);
	bool add(const char *device, uint8_t number, TimeTable *tt);
	uint16_t getIndex(const char *device, uint8_t number);
	uint16_t getIndexByDateTime(const char *device, DateTime *datetime);
	uint16_t getSize() { return list.getSize(); }
	Config3PriceIndex* get(uint16_t index);
	Config3PriceIndex* get(const char *device, uint8_t number);
	Config3PriceIndex* get(const char *device, DateTime *datetime);

private:
	Memory *memory;
	uint32_t address;
	List<Config3PriceIndex> list;
};

#endif
