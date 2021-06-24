#include "Config3ProductList.h"

#include "memory/include/MemoryCrc.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "logger/include/Logger.h"
#include "platform/include/platform.h"

void Config3Product::setDefault() {
	data.enable = true;
	data.wareId = 0;
	data.name.set("ίχεικΰ");
	data.taxRate = Fiscal::TaxRate_NDSNone;
	data.totalCount = 0;
	data.totalMoney = 0;
	data.count = 0;
	data.money = 0;
	data.testTotalCount = 0;
	data.testCount = 0;
	data.freeTotalCount = 0;
	data.freeCount = 0;
}

MemoryResult Config3Product::init(uint16_t priceListNum, Memory *memory) {
	MemoryResult result = initData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Product init failed");
		return result;
	}
	result = prices.init(priceListNum, memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Product prices init failed");
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config3Product::load(uint16_t priceListNum, Memory *memory) {
	MemoryResult result = loadData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Load product header failed");
		return result;
	}
	result = prices.load(priceListNum, memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Load product prices failed");
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config3Product::save() {
	return saveData();
}

MemoryResult Config3Product::initData(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	data.enable = true;
	data.wareId = 0;
	data.name.set("ίχεικΰ");
	data.taxRate = Fiscal::TaxRate_NDSNone;
	data.totalCount = 0;
	data.totalMoney = 0;
	data.count = 0;
	data.money = 0;
	data.testTotalCount = 0;
	data.testCount = 0;
	data.freeTotalCount = 0;
	data.freeCount = 0;
	return saveData();
}

MemoryResult Config3Product::loadData(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	LOG_DEBUG(LOG_CFG, "loadData " << this->address);
	MemoryCrc crc(memory);
	return crc.readDataWithCrc(&data, sizeof(data));
}

MemoryResult Config3Product::saveData() {
	LOG_DEBUG(LOG_CFG, "saveData " << address);
	memory->setAddress(address);
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

MemoryResult Config3Product::reset() {
	LOG_DEBUG(LOG_CFG, "reset");
	data.count = 0;
	data.money = 0;
	data.testCount = 0;
	data.freeCount = 0;
	MemoryResult result = saveData();
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = prices.reset();
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config3Product::resetTotal() {
	LOG_DEBUG(LOG_CFG, "resetTotal");
	data.totalCount = 0;
	data.count = 0;
	data.totalMoney = 0;
	data.money = 0;
	data.testTotalCount = 0;
	data.testCount = 0;
	data.freeTotalCount = 0;
	data.freeCount = 0;
	MemoryResult result = saveData();
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = prices.resetTotal();
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config3Product::sale(uint16_t index, uint32_t value) {
	Config3Price price;
	MemoryResult result = prices.getByIndex(index, &price);
	if(result != MemoryResult_Ok) {
		return result;
	}

	result = price.sale(value);
	if(result != MemoryResult_Ok) {
		return result;
	}

	data.totalCount++;
	data.totalMoney += value;
	data.count++;
	data.money += value;
	result = saveData();
	if(result != MemoryResult_Ok) {
		return result;
	}

	return MemoryResult_Ok;
}

MemoryResult Config3Product::setEnable(uint8_t enable) {
	data.enable = enable;
	return saveData();
}

MemoryResult Config3Product::setWareId(uint32_t wareId) {
	data.wareId = wareId;
	return saveData();
}

MemoryResult Config3Product::setName(const char *name) {
	data.name.set(name);
	return saveData();
}

MemoryResult Config3Product::setTaxRate(uint16_t taxRate) {
	data.taxRate = taxRate;
	return saveData();
}

MemoryResult Config3Product::setFreeCount(uint32_t freeTotalCount, uint32_t freeCount) {
	data.freeTotalCount = freeTotalCount;
	data.freeCount = freeCount;
	return saveData();
}

MemoryResult Config3Product::setCount(uint32_t totalCount, uint32_t totalMoney, uint32_t count, uint32_t money) {
	data.totalCount = totalCount;
	data.totalMoney = totalMoney;
	data.count = count;
	data.money = money;
	return saveData();
}

uint32_t Config3Product::getDataSize(uint16_t priceListNum) {
	return (sizeof(Config3ProductData) + Config3PriceList::getDataSize(priceListNum));
}
