#include "Config3PriceList.h"

#include "logger/include/Logger.h"
#include "platform/include/platform.h"

MemoryResult Config3Price::init(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	data.price = 1000;
	data.totalCount = 0;
	data.totalMoney = 0;
	data.count = 0;
	data.money = 0;
	return save();
}

MemoryResult Config3Price::load(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	LOG_DEBUG(LOG_CFG, "loadData " << this->address);
	MemoryCrc crc(memory);
	return crc.readDataWithCrc(&data, sizeof(data));
}

MemoryResult Config3Price::save() {
	LOG_DEBUG(LOG_CFG, "saveData " << address);
	memory->setAddress(address);
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

MemoryResult Config3Price::reset() {
	data.count = 0;
	data.money = 0;
	return save();
}

MemoryResult Config3Price::resetTotal() {
	data.totalCount = 0;
	data.count = 0;
	data.totalMoney = 0;
	data.money = 0;
	return save();
}

MemoryResult Config3Price::sale(uint32_t value) {
	data.totalCount += 1;
	data.totalMoney += value;
	data.count += 1;
	data.money += value;
	return save();
}

uint32_t Config3Price::getDataSize() {
	return sizeof(Config3PriceData);
}

MemoryResult Config3PriceList::init(uint16_t priceListNum, Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	this->priceListNum = priceListNum;
	Config3Price price;
	for(uint16_t i = 0; i < priceListNum; i++) {
		LOG_DEBUG(LOG_CFG, "init price " << i);
		MemoryResult result = price.init(memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return MemoryResult_Ok;
}

MemoryResult Config3PriceList::load(uint16_t priceListNum, Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	this->priceListNum = priceListNum;
	Config3Price price;
	for(uint8_t i = 0; i < priceListNum; i++) {
		MemoryResult result = price.load(memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return MemoryResult_Ok;
}

MemoryResult Config3PriceList::save() {
	LOG_DEBUG(LOG_CFG, "save " << priceListNum);
	return MemoryResult_Ok;
}

MemoryResult Config3PriceList::reset() {
	Config3Price price;
	for(uint8_t i = 0; i < priceListNum; i++) {
		getByIndex(i, &price);
		MemoryResult result = price.reset();
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return MemoryResult_Ok;
}

MemoryResult Config3PriceList::resetTotal() {
	Config3Price price;
	for(uint8_t i = 0; i < priceListNum; i++) {
		getByIndex(i, &price);
		MemoryResult result = price.resetTotal();
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return MemoryResult_Ok;
}

MemoryResult Config3PriceList::getByIndex(uint16_t priceIndex, Config3Price *price) {
	if(priceIndex >= priceListNum) {
		return MemoryResult_NotFound;
	}
	uint32_t entrySize = Config3Price::getDataSize();
	uint32_t entryAddress = address + priceIndex * entrySize;
	LOG_DEBUG(LOG_CFG, "getByIndex " << priceIndex << ":" << entryAddress);
	memory->setAddress(entryAddress);
	return price->load(memory);
}

uint32_t Config3PriceList::getDataSize(uint16_t priceListNum) {
	return (Config3Price::getDataSize() * priceListNum);
}
