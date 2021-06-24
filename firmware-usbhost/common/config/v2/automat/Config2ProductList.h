#ifndef COMMON_CONFIG_V2_PRODUCTLIST_H_
#define COMMON_CONFIG_V2_PRODUCTLIST_H_

#include "Config2PriceList.h"
#include "evadts/EvadtsProtocol.h"
#include "memory/include/Memory.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/List.h"

class Config2Product {
public:
	Config2ProductData data;
	Config2PriceList prices;

	void setDefault();
	MemoryResult load(uint16_t priceListNum, Memory *memory);
	MemoryResult save(Memory *memory);

private:
	MemoryResult loadData(Memory *memory);
	MemoryResult saveData(Memory *memory);
};

class Config2ProductList {
public:
	Config2ProductList();
	void set(uint16_t priceListNum, uint16_t productNum);
	void add(Config2Product *product);
	MemoryResult load(Memory *memory);
	MemoryResult save(Memory *memory);

	uint32_t getTotalCount() { return data.totalCount; }
	uint32_t getTotalMoney() { return data.totalMoney; }
	uint32_t getCount() { return data.count; }
	uint32_t getMoney() { return data.money; }
	uint16_t getProductNum() { return data.productNum; }
	uint16_t getPriceListNum() { return data.priceListNum; }

	Config2Product *getByIndex(uint16_t index);
	uint16_t getLen() { return list.getSize(); }
	List<Config2Product> *getList() { return &list; }

private:
	Config2ProductListData data;
	List<Config2Product> list;

	MemoryResult loadData(Memory *memory);
	MemoryResult saveData(Memory *memory);
};

#endif
