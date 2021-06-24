#include "Config3ProductList.h"

#include "memory/include/MemoryCrc.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "logger/include/Logger.h"
#include "platform/include/platform.h"

MemoryResult Config3ProductList::init(uint16_t productNum, uint16_t priceListNum, Memory *memory) {
	LOG_DEBUG(LOG_CFG, "init " << productNum << "," << priceListNum);
	MemoryResult result = initData(productNum, priceListNum, memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	Config3Product product;
	for(uint16_t i = 0; i < productNum; i++) {
		LOG_DEBUG(LOG_CFG, "init product " << i);
		result = product.init(priceListNum, memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	LOG_DEBUG(LOG_CFG, "complete");
	return MemoryResult_Ok;
}

void Config3ProductList::asyncInitStart(uint16_t productNum, uint16_t priceListNum, Memory *memory) {
	LOG_DEBUG(LOG_CFG, "init " << productNum << "," << priceListNum);
	initData(productNum, priceListNum, memory);
	num = 0;
}

bool Config3ProductList::asyncInitProc() {
	if(num >= data.productNum) {
		LOG_DEBUG(LOG_CFG, "complete");
		return false;
	}
	LOG_DEBUG(LOG_CFG, "init product " << num);
	Config3Product product;
	product.init(data.priceListNum, memory);
	num++;
	return true;
}

MemoryResult Config3ProductList::load(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "load " << memory->getAddress());
	MemoryResult result = loadData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "ProductList header load failed");
		return result;
	}
	uint32_t address = memory->getAddress() + data.productNum * Config3Product::getDataSize(data.priceListNum);
	memory->setAddress(address);
	return MemoryResult_Ok;
}

MemoryResult Config3ProductList::loadAndCheck(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "loadAndCheck " << memory->getAddress());
	MemoryResult result = loadData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "ProductList header load failed");
		return result;
	}
	Config3Product product;
	for(uint8_t i = 0; i < data.productNum; i++) {
		result = product.load(data.priceListNum, memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return MemoryResult_Ok;
}

MemoryResult Config3ProductList::save() {
	return saveData();
}

MemoryResult Config3ProductList::reset() {
	data.count = 0;
	data.money = 0;
	MemoryResult result = saveData();
	if(result != MemoryResult_Ok) {
		return result;
	}

	Config3Product product;
	for(uint8_t i = 0; i < data.productNum; i++) {
		getByIndex(i, &product);
		result = product.reset();
		if(result != MemoryResult_Ok) {
			return result;
		}
	}

	return MemoryResult_Ok;
}

MemoryResult Config3ProductList::resetTotal() {
	data.totalCount = 0;
	data.totalMoney = 0;
	data.count = 0;
	data.money = 0;
	MemoryResult result = saveData();
	if(result != MemoryResult_Ok) {
		return result;
	}

	Config3Product product;
	for(uint8_t i = 0; i < data.productNum; i++) {
		getByIndex(i, &product);
		result = product.resetTotal();
		if(result != MemoryResult_Ok) {
			return result;
		}
	}

	return MemoryResult_Ok;
}

MemoryResult Config3ProductList::initData(uint16_t productNum, uint16_t priceListNum, Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	data.totalCount = 0;
	data.totalMoney = 0;
	data.count = 0;
	data.money = 0;
	data.productNum = productNum;
	data.priceListNum = priceListNum;
	return saveData();
}

MemoryResult Config3ProductList::loadData(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	LOG_DEBUG(LOG_CFG, "productNum=" << data.productNum);
	LOG_DEBUG(LOG_CFG, "priceListNum=" << data.priceListNum);
	LOG_DEBUG(LOG_CFG, "totalCount=" << data.totalCount);
	LOG_DEBUG(LOG_CFG, "totalMoney=" << data.totalMoney);
	LOG_DEBUG(LOG_CFG, "count=" << data.count);
	LOG_DEBUG(LOG_CFG, "money=" << data.money);
	return MemoryResult_Ok;
}

MemoryResult Config3ProductList::saveData() {
	LOG_DEBUG(LOG_CFG, "saveData " << address);
	memory->setAddress(address);
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

MemoryResult Config3ProductList::getByIndex(uint16_t index, Config3Product *product) {
	if(index >= data.productNum) {
		LOG_ERROR(LOG_CFG, "Product not found " << index << ":" << data.productNum);
		return MemoryResult_NotFound;
	}
	uint32_t entrySize = Config3Product::getDataSize(data.priceListNum);
	uint32_t entryAddress = address + sizeof(data) + index * entrySize;
	LOG_DEBUG(LOG_CFG, "getByIndex " << index << ":" << entryAddress);
	memory->setAddress(entryAddress);
	return product->load(data.priceListNum, memory);
}

MemoryResult Config3ProductList::sale(uint16_t productIndex, uint16_t priceIndex, uint32_t value) {
	Config3Product product;
	MemoryResult result = getByIndex(productIndex, &product);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Product with index " << productIndex << " not exist");
		return result;
	}
	result = product.sale(priceIndex, value);
	if(result != MemoryResult_Ok) {
		return result;
	}
	data.totalCount += 1;
	data.totalMoney += value;
	data.count += 1;
	data.money += value;
	return saveData();
}
