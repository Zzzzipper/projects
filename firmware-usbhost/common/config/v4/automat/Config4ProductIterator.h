#ifndef COMMON_CONFIG_V4_AUTOMAT_PRODUCTITERATOR_H_
#define COMMON_CONFIG_V4_AUTOMAT_PRODUCTITERATOR_H_

#include "Config4ProductList.h"
#include "config/v3/automat/Config3ProductIndex.h"
#include "config/v3/automat/Config3PriceList.h"
#include "config/v3/automat/Config3PriceIndex.h"
#include "memory/include/Memory.h"
#include "utils/include/NetworkProtocol.h"

class Config4ProductIterator {
public:
	Config4ProductIterator(Config3PriceIndexList *priceIndexes, Config3ProductIndexList *productIndexes, Config4ProductList *products);
	bool first();
	bool last();
	bool prev();
	bool next();
	bool findByIndex(uint16_t index);
	bool findBySelectId(const char *selectId);
	bool findByCashlessId(uint16_t selectId);
	bool findByPrice(const char *device, uint16_t number, uint32_t price);
	bool isSet();
	void unset();

	uint16_t getIndex() const;
	const char *getId() const;
	uint16_t getCashlessId() const;
	uint8_t getEnable() const { return product.data.enable; }
	uint32_t getWareId() const { return product.data.wareId; }
	const char *getName() const { return product.data.name.get(); }
	uint16_t getTaxRate() const { return product.data.taxRate; }
	uint32_t getTotalCount() const { return product.data.totalCount; }
	uint32_t getTotalMoney() const { return product.data.totalMoney; }
	uint32_t getCount() const { return product.data.count; }
	uint32_t getMoney() const { return product.data.money; }
	uint32_t getFreeTotalCount() const { return product.data.freeTotalCount; }
	uint32_t getFreeCount() const { return product.data.freeCount; }
	Config3PriceIndex *getPriceIdByIndex(uint16_t index);
	Config3Price *getPriceByIndex(uint16_t index);
	Config3Price *getPrice(const char *device, uint16_t number);
	Config3Price *getIndexByDateTime(const char *device, DateTime *datetime);

	Config3Price *sale();
	void setEnable(uint8_t enable);
	void setWareId(uint32_t wareId);
	void setName(const char *name);
	void setTaxRate(uint16_t taxRate);
	void setFreeCount(uint32_t freeTotalCount, uint32_t freeCount);
	void setCount(uint32_t totalCount, uint32_t totalMoney, uint32_t count, uint32_t money);
	void setPrice(const char *device, uint16_t number, uint32_t price);
	void setPriceCount(const char *device, uint32_t number, uint16_t price, uint32_t totalCount, uint32_t totalMoney, uint32_t count, uint32_t money);

private:
	Config3PriceIndexList *priceIndexes;
	Config3ProductIndexList *productIndexes;
	Config4ProductList *products;
	uint16_t index;
	Config3Product product;
	Config3Price price;
};

#endif
