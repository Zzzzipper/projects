#include "Config4ProductIterator.h"
#include "config/v3/automat/Config3PriceIndex.h"
#include "config/v3/automat/Config3ProductIndex.h"

#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

#define INDEX_UNDEFINED_VALUE 0xFFFF

Config4ProductIterator::Config4ProductIterator(Config3PriceIndexList *priceIndexes, Config3ProductIndexList *productIndexes, Config4ProductList *products) :
	priceIndexes(priceIndexes),
	productIndexes(productIndexes),
	products(products),
	index(INDEX_UNDEFINED_VALUE)
{
}

bool Config4ProductIterator::first() {
	if(productIndexes->getSize() <= 0) {
		return false;
	}
	index = 0;
	MemoryResult result = products->getByIndex(index, &product);
	if(result != MemoryResult_Ok) {
		return false;
	}
	return true;
}

bool Config4ProductIterator::last() {
	if(productIndexes->getSize() <= 0) {
		return false;
	}
	index = productIndexes->getSize() - 1;
	MemoryResult result = products->getByIndex(index, &product);
	if(result != MemoryResult_Ok) {
		return false;
	}
	return true;
}

bool Config4ProductIterator::prev() {
	if(index <= 0) {
		return false;
	}
	index--;
	MemoryResult result = products->getByIndex(index, &product);
	if(result != MemoryResult_Ok) {
		return false;
	}
	return true;
}

bool Config4ProductIterator::next() {
	if(index >= productIndexes->getSize()) {
		return false;
	}
	index++;
	MemoryResult result = products->getByIndex(index, &product);
	if(result != MemoryResult_Ok) {
		return false;
	}
	return true;
}

bool Config4ProductIterator::findByIndex(uint16_t productIndex) {
	MemoryResult result = products->getByIndex(productIndex, &product);
	if(result != MemoryResult_Ok) {
		unset();
		return false;
	}
	index = productIndex;
	return true;
}

bool Config4ProductIterator::findBySelectId(const char *selectId) {
	uint16_t productIndex = productIndexes->getIndex(selectId);
	if(productIndex >= productIndexes->getSize()) {
		unset();
		return false;
	}
	index = productIndex;
	MemoryResult result = products->getByIndex(index, &product);
	if(result != MemoryResult_Ok) {
		return false;
	}
	return true;
}

bool Config4ProductIterator::findByCashlessId(uint16_t selectId) {
	uint16_t productIndex = productIndexes->getIndex(selectId);
	if(productIndex >= productIndexes->getSize()) {
		unset();
		return false;
	}
	index = productIndex;
	MemoryResult result = products->getByIndex(index, &product);
	if(result != MemoryResult_Ok) {
		return false;
	}
	return true;
}

bool Config4ProductIterator::findByPrice(const char *device, uint16_t number, uint32_t priceValue) {
	for(bool result = first(); result != false; result = next()) {
		Config3Price *price = getPrice(device, number);
		if(price == NULL) {
			return false;
		}
		if(price->getPrice() == priceValue) {
			return true;
		}
	}
	return false;
}

bool Config4ProductIterator::isSet() {
	return (index != INDEX_UNDEFINED_VALUE);
}

void Config4ProductIterator::unset() {
	index = INDEX_UNDEFINED_VALUE;
}

uint16_t Config4ProductIterator::getIndex() const {
	return index;
}

const char *Config4ProductIterator::getId() const {
	return productIndexes->get(index)->selectId.get();
}

uint16_t Config4ProductIterator::getCashlessId() const {
	return productIndexes->get(index)->cashlessId;
}

Config3PriceIndex *Config4ProductIterator::getPriceIdByIndex(uint16_t priceIndex) {
	return priceIndexes->get(priceIndex);
}

Config3Price *Config4ProductIterator::getPriceByIndex(uint16_t priceIndex) {
	MemoryResult result = product.prices.getByIndex(priceIndex, &price);
	if(result != MemoryResult_Ok) {
		return NULL;
	}
	return &price;
}

Config3Price *Config4ProductIterator::getPrice(const char *device, uint16_t number) {
	uint16_t priceIndex = priceIndexes->getIndex(device, number);
	if(priceIndex >= priceIndexes->getSize()) {
		return NULL;
	}
	return getPriceByIndex(priceIndex);
}

Config3Price *Config4ProductIterator::getIndexByDateTime(const char *device, DateTime *datetime) {
	uint16_t priceIndex = priceIndexes->getIndexByDateTime(device, datetime);
	if(priceIndex >= priceIndexes->getSize()) {
		return NULL;
	}
	return getPriceByIndex(priceIndex);
}

void Config4ProductIterator::setEnable(uint8_t enable) {
	product.setEnable(enable);
}

void Config4ProductIterator::setWareId(uint32_t wareId) {
	product.setWareId(wareId);
}

void Config4ProductIterator::setName(const char *name) {
	product.setName(name);
}

void Config4ProductIterator::setTaxRate(uint16_t taxRate) {
	product.setTaxRate(taxRate);
}

void Config4ProductIterator::setFreeCount(uint32_t freeTotalCount, uint32_t freeCount) {
	product.setFreeCount(freeTotalCount, freeCount);
}

void Config4ProductIterator::setCount(uint32_t totalCount, uint32_t totalMoney, uint32_t count, uint32_t money) {
	product.setCount(totalCount, totalMoney, count, money);
}

void Config4ProductIterator::setPrice(const char *device, uint16_t number, uint32_t value) {
	uint16_t priceIndex = priceIndexes->getIndex(device, number);
	if(priceIndex >= priceIndexes->getSize()) {
		return;
	}
	MemoryResult result = product.prices.getByIndex(priceIndex, &price);
	if(result != MemoryResult_Ok) {
		return;
	}
	price.data.price = value;
	price.save();
}

void Config4ProductIterator::setPriceCount(const char *device, uint32_t number, uint16_t value, uint32_t totalCount, uint32_t totalMoney, uint32_t count, uint32_t money) {
	uint16_t priceIndex = priceIndexes->getIndex(device, number);
	if(priceIndex >= priceIndexes->getSize()) {
		return;
	}
	MemoryResult result = product.prices.getByIndex(priceIndex, &price);
	if(result != MemoryResult_Ok) {
		return;
	}
	price.data.price = value;
	price.data.totalCount = totalCount;
	price.data.totalMoney = totalMoney;
	price.data.count = count;
	price.data.money = money;
	price.save();
}
