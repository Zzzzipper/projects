#ifndef COMMON_CONFIG_CONFIGPRODUCTLIST1_H_
#define COMMON_CONFIG_CONFIGPRODUCTLIST1_H_

#include "Evadts1Protocol.h"
#include "Config1PriceList.h"
#include "memory/include/Memory.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/List.h"

#pragma pack(push,1)
struct Config1ProductData {
	uint8_t enable;
	StrParam<Evadts1ProductNameSize> name;
	uint16_t taxRate;
	uint32_t totalCount;
	uint32_t totalMoney;
	uint32_t count;
	uint32_t money;
};
#pragma pack(pop)

class Config1Product {
public:
	Config1ProductData data;
	Config1PriceList prices;

	void setDefault();
	MemoryResult load(uint16_t priceListNum, Memory *memory);
	MemoryResult save(Memory *memory);

private:
	MemoryResult loadData(Memory *memory);
	MemoryResult saveData(Memory *memory);
};

class Config1ProductList {
public:
	void set(uint16_t priceListNum, uint16_t productNum);
	void add(Config1Product *product);
	MemoryResult load(Memory *memory);
	MemoryResult save(Memory *memory);

	uint32_t getTotalCount() { return data.totalCount; }
	uint32_t getTotalMoney() { return data.totalMoney; }
	uint32_t getCount() { return data.count; }
	uint32_t getMoney() { return data.money; }
	uint16_t getProductNum() { return data.productNum; }
	uint16_t getPriceListNum() { return data.priceListNum; }

	Config1Product *getByIndex(uint16_t index);
	uint16_t getLen() { return list.getSize(); }
	List<Config1Product> *getList() { return &list; }

private:
	uint16_t num;
	struct {
		uint32_t totalCount;
		uint32_t totalMoney;
		uint32_t count;
		uint32_t money;
		uint16_t productNum;
		uint16_t priceListNum;
	} data;
	List<Config1Product> list;

	MemoryResult loadData(Memory *memory);
	MemoryResult saveData(Memory *memory);
};

#endif
