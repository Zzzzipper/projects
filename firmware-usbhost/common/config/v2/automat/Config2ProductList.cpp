#include "Config2ProductList.h"

#include "memory/include/MemoryCrc.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "logger/include/Logger.h"
#include "platform/include/platform.h"

void Config2Product::setDefault() {
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

MemoryResult Config2Product::load(uint16_t priceListNum, Memory *memory) {
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

MemoryResult Config2Product::save(Memory *memory) {
	MemoryResult result = saveData(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = prices.save(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config2Product::loadData(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "loadData");
	MemoryCrc crc(memory);
	return crc.readDataWithCrc(&data, sizeof(data));
}

MemoryResult Config2Product::saveData(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "saveData");
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

Config2ProductList::Config2ProductList() {
	data.productNum = 0;
	data.priceListNum = 0;
	data.totalCount = 0;
	data.totalMoney = 0;
	data.count = 0;
	data.money = 0;
}

void Config2ProductList::set(uint16_t priceListNum, uint16_t productNum) {
	data.priceListNum = priceListNum;
	data.productNum = productNum;
}

void Config2ProductList::add(Config2Product *product) {
	list.add(product);
}

MemoryResult Config2ProductList::load(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "load " << memory->getAddress());
	MemoryResult result = loadData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "ProductList header load failed");
		return result;
	}
	for(uint8_t i = 0; i < data.productNum; i++) {
		Config2Product *product = new Config2Product;
		result = product->load(data.priceListNum, memory);
		if(result != MemoryResult_Ok) {
			product->setDefault();
		}
		list.add(product);
	}
	return MemoryResult_Ok;
}

MemoryResult Config2ProductList::save(Memory *memory) {
	MemoryResult result = saveData(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config2Product *product = list.get(i);
		result = product->save(memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return MemoryResult_Ok;
}

Config2Product *Config2ProductList::getByIndex(uint16_t index) {
	if(index >= data.productNum) {
		LOG_ERROR(LOG_CFG, "Product not found " << index << ":" << data.productNum);
		return NULL;
	}
	return list.get(index);
}

MemoryResult Config2ProductList::loadData(Memory *memory) {
	MemoryCrc crc(memory);
	return crc.readDataWithCrc(&data, sizeof(data));
}

MemoryResult Config2ProductList::saveData(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "saveData");
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}
