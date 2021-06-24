#include "Config1PriceList.h"

#include "logger/include/Logger.h"

void Config1Price::setDefault() {
	data.price = 1000;
	data.totalCount = 0;
	data.totalMoney = 0;
	data.count = 0;
	data.money = 0;
}

MemoryResult Config1Price::load(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "load" << memory->getAddress());
	MemoryCrc crc(memory);
	crc.startCrc();
	MemoryResult result = crc.read(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return crc.readCrc();
}

MemoryResult Config1Price::save(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "save " << memory->getAddress());
	MemoryCrc crc(memory);
	crc.startCrc();
	MemoryResult result = crc.write(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return crc.writeCrc();
}

MemoryResult Config1PriceList::load(uint16_t priceListNum, Memory *memory) {
	for(uint8_t i = 0; i < priceListNum; i++) {
		Config1Price *price = new Config1Price;
		MemoryResult result = price->load(memory);
		if(result != MemoryResult_Ok) {
			price->setDefault();
		}
		list.add(price);
	}
	return MemoryResult_Ok;
}

MemoryResult Config1PriceList::save(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "save " << list.getSize());
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config1Price *price = list.get(i);
		MemoryResult result = price->save(memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return MemoryResult_Ok;
}

void Config1PriceList::add(Config1Price *price) {
	list.add(price);
}
