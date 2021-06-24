#ifndef COMMON_CONFIG_CONFIGPRODUCTLIST_H_
#define COMMON_CONFIG_CONFIGPRODUCTLIST_H_

#include "Config3Product.h"

class Config3ProductList {
public:
	MemoryResult init(uint16_t productNum, uint16_t priceListNum, Memory *memory);
	void asyncInitStart(uint16_t productNum, uint16_t priceListNum, Memory *memory);
	bool asyncInitProc();
	MemoryResult load(Memory *memory);
	MemoryResult loadAndCheck(Memory *memory);
	MemoryResult save();
	MemoryResult reset();
	MemoryResult resetTotal();
	MemoryResult sale(uint16_t productIndex, uint16_t priceIndex, uint32_t value);

	uint32_t getTotalCount() { return data.totalCount; }
	uint32_t getTotalMoney() { return data.totalMoney; }
	uint32_t getCount() { return data.count; }
	uint32_t getMoney() { return data.money; }
	uint16_t getProductNum() { return data.productNum; }
	uint16_t getPriceListNum() { return data.priceListNum; }

	MemoryResult getByIndex(uint16_t productIndex, Config3Product *product);

//++++ TODO: вернуть в приват
	Config3ProductListData data;
//++++
private:
	Memory *memory;
	uint32_t address;
	uint16_t num;

	MemoryResult initData(uint16_t productNum, uint16_t priceListNum, Memory *memory);
	MemoryResult loadData(Memory *memory);
	MemoryResult saveData();
};

#endif
