#include "Config1PriceIndex.h"

#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

void Config1PriceIndexList::init(Memory *memory) {
	save(memory);
}

MemoryResult Config1PriceIndexList::save(Memory *memory) {
	MemoryCrc crc(memory);
	crc.startCrc();
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config1PriceIndex *item = list.get(i);
		MemoryResult result = crc.write(item, sizeof(Config1PriceIndex));
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return crc.writeCrc();
}

MemoryResult Config1PriceIndexList::load(uint16_t priceListNum, Memory *memory) {
	list.clearAndFree();
	MemoryCrc crc(memory);
	crc.startCrc();
	for(uint16_t i = 0; i < priceListNum; i++) {
		Config1PriceIndex *item = new Config1PriceIndex;
		MemoryResult result = crc.read(item, sizeof(Config1PriceIndex));
		if(result != MemoryResult_Ok) {
			return result;
		}
		list.add(item);
	}
	return crc.readCrc();
}

void Config1PriceIndexList::clear() {
	list.clearAndFree();
}

bool Config1PriceIndexList::add(const char *device, uint8_t number) {
	uint16_t index = this->getIndex(device, number);
	if(index < list.getSize()) {
		return false;
	}
	Config1PriceIndex *item = new Config1PriceIndex;
	item->device.set(device);
	item->number = number;
	list.add(item);
	return true;
}

uint16_t Config1PriceIndexList::getIndex(const char *device, uint8_t number) {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config1PriceIndex *index = list.get(i);
		if(index->device.equal(device) == true && index->number == number) {
			return i;
		}
	}
	return 0xFFFF;
}

Config1PriceIndex* Config1PriceIndexList::get(uint16_t i) {
	return list.get(i);
}
