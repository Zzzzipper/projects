#include "Config4ProductList.h"

#include "memory/include/MemoryCrc.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "logger/include/Logger.h"
#include "platform/include/platform.h"

MemoryResult Config4ProductList::init(uint16_t productNum, uint16_t priceListNum, Memory *memory) {
	LOG_DEBUG(LOG_CFG, "init " << productNum << "," << priceListNum);
	MemoryResult result = data.init(productNum, priceListNum, memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = stat.init(memory);
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

void Config4ProductList::asyncInitStart(uint16_t productNum, uint16_t priceListNum, Memory *memory) {
	LOG_DEBUG(LOG_CFG, "init " << productNum << "," << priceListNum);
	data.init(productNum, priceListNum, memory);
	stat.init(memory);
	num = 0;
}

bool Config4ProductList::asyncInitProc(Memory *memory) {
	if(num >= data.data.productNum) {
		LOG_DEBUG(LOG_CFG, "complete");
		return false;
	}
	LOG_DEBUG(LOG_CFG, "init product " << num);
	Config3Product product;
	product.init(data.data.priceListNum, memory);
	num++;
	return true;
}

MemoryResult Config4ProductList::load(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "load " << memory->getAddress());
	MemoryResult result = data.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "ProductList data load failed");
		return result;
	}
	result = stat.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "ProductList stat load failed");
		return result;
	}
	uint32_t address = memory->getAddress() + data.data.productNum * Config3Product::getDataSize(data.data.priceListNum);
	memory->setAddress(address);
	return MemoryResult_Ok;
}

MemoryResult Config4ProductList::loadAndCheck(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "loadAndCheck " << memory->getAddress());
	MemoryResult result = data.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "ProductList data load failed");
		return result;
	}
	result = stat.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "ProductList stat load failed");
		return result;
	}
	Config3Product product;
	for(uint8_t i = 0; i < data.data.productNum; i++) {
		result = product.load(data.data.priceListNum, memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return MemoryResult_Ok;
}

MemoryResult Config4ProductList::save() {
	return data.save();
}

MemoryResult Config4ProductList::reset() {
	MemoryResult result = stat.reset();
	if(result != MemoryResult_Ok) {
		return result;
	}
	Config3Product product;
	for(uint8_t i = 0; i < data.data.productNum; i++) {
		result = getByIndex(i, &product);
		if(result != MemoryResult_Ok) {
			return result;
		}
		result = product.reset();
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return MemoryResult_Ok;
}

MemoryResult Config4ProductList::getByIndex(uint16_t index, Config3Product *product) {
	if(index >= data.data.productNum) {
		LOG_ERROR(LOG_CFG, "Product not found " << index << ":" << data.data.productNum);
		return MemoryResult_NotFound;
	}
	uint32_t entrySize = Config3Product::getDataSize(data.data.priceListNum);
	uint32_t entryAddress = data.getAddress() + data.getDataSize() + stat.getDataSize() + index * entrySize;
	LOG_DEBUG(LOG_CFG, "getByIndex " << index << ":" << entryAddress);
	data.getMemory()->setAddress(entryAddress);
	return product->load(data.data.priceListNum, data.getMemory());
}

MemoryResult Config4ProductList::sale(const char *device, uint16_t productIndex, uint16_t priceIndex, uint32_t value) {
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
	return stat.sale(device, value);
}
