#include "Config1ProductList.h"

#include "memory/include/MemoryCrc.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "logger/include/Logger.h"
#include "platform/include/platform.h"

void Config1Product::setDefault() {
	data.enable = true;
	data.name.set("ίχεικΰ");
	data.taxRate = Fiscal::TaxRate_NDSNone;
	data.totalCount = 0;
	data.totalMoney = 0;
	data.count = 0;
	data.money = 0;
}

MemoryResult Config1Product::load(uint16_t priceListNum, Memory *memory) {
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

MemoryResult Config1Product::save(Memory *memory) {
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

MemoryResult Config1Product::loadData(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "loadData " << memory->getAddress());
	MemoryCrc crc(memory);
	crc.startCrc();
	MemoryResult result = crc.read(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return crc.readCrc();
}

MemoryResult Config1Product::saveData(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "saveData " << memory->getAddress());
	MemoryCrc crc(memory);
	crc.startCrc();
	MemoryResult result = crc.write(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return crc.writeCrc();
}

void Config1ProductList::set(uint16_t priceListNum, uint16_t productNum) {
	data.priceListNum = priceListNum;
	data.productNum = productNum;
}

void Config1ProductList::add(Config1Product *product) {
	list.add(product);
}

MemoryResult Config1ProductList::load(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "loadAndCheck " << memory->getAddress());
	loadData(memory);
	for(uint16_t i = 0; i < data.productNum; i++) {
		Config1Product *product = new Config1Product;
		if(product->load(data.priceListNum, memory) == false) {
			product->setDefault();
		}
		list.add(product);
	}
	return MemoryResult_Ok;
}

MemoryResult Config1ProductList::save(Memory *memory) {
	MemoryResult result = saveData(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config1Product *product = list.get(i);
		result = product->save(memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return MemoryResult_Ok;
}

Config1Product *Config1ProductList::getByIndex(uint16_t index) {
	if(index >= data.productNum) {
		LOG_ERROR(LOG_CFG, "Product not found " << index << ":" << data.productNum);
		return NULL;
	}
	return list.get(index);
}

MemoryResult Config1ProductList::loadData(Memory *memory) {
	MemoryCrc crc(memory);
	crc.startCrc();
	MemoryResult result = crc.read(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = crc.readCrc();
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config1ProductList::saveData(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "saveData " << memory->getAddress());
	MemoryCrc crc(memory);
	crc.startCrc();
	MemoryResult result = crc.write(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = crc.writeCrc();
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}
