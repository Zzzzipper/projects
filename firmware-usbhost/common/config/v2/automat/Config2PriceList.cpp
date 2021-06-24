#include "Config2PriceList.h"
#include "logger/include/Logger.h"

void Config2Price::setDefault() {
	data.price = 1000;
	data.totalCount = 0;
	data.totalMoney = 0;
	data.count = 0;
	data.money = 0;
}

MemoryResult Config2Price::load(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "load");
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config2Price::save(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "save");
	MemoryCrc crc(memory);
	MemoryResult result = crc.writeDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config2PriceList::load(uint16_t priceListNum, Memory *memory) {
	for(uint8_t i = 0; i < priceListNum; i++) {
		Config2Price *price = new Config2Price;
		MemoryResult result = price->load(memory);
		if(result != MemoryResult_Ok) {
			price->setDefault();
		}
		list.add(price);
	}
	return MemoryResult_Ok;
}

MemoryResult Config2PriceList::save(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "save " << list.getSize());
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config2Price *price = list.get(i);
		MemoryResult result = price->save(memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return MemoryResult_Ok;
}

void Config2PriceList::add(Config2Price *price) {
	list.add(price);
}
