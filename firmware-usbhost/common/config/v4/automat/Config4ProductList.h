#ifndef COMMON_CONFIG_V4_PRODUCTLIST_H_
#define COMMON_CONFIG_V4_PRODUCTLIST_H_

#include "config/v3/automat/Config3Product.h"
#include "config/v4/automat/Config4ProductListData.h"
#include "config/v4/automat/Config4ProductListStat.h"

class Config4ProductList {
public:
	MemoryResult init(uint16_t productNum, uint16_t priceListNum, Memory *memory);
	void asyncInitStart(uint16_t productNum, uint16_t priceListNum, Memory *memory);
	bool asyncInitProc(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult loadAndCheck(Memory *memory);
	MemoryResult save();
	MemoryResult reset();

	MemoryResult sale(const char *device, uint16_t productIndex, uint16_t priceIndex, uint32_t value);
	uint16_t getProductNum() { return data.data.productNum; }
	uint16_t getPriceListNum() { return data.data.priceListNum; }
	Config4ProductListData* getData() { return &data; }
	Config4ProductListStat* getStat() { return &stat; }

	MemoryResult getByIndex(uint16_t productIndex, Config3Product *product);

private:
	uint16_t num;
	Config4ProductListData data;
	Config4ProductListStat stat;
};

#endif
