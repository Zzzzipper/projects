#include "Config3PriceIndex.h"
#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

#pragma pack(push,1)
struct Config3PriceIndexData {
	StrParam<EvadtsPaymentDeviceSize> device;
	uint8_t number;
	uint8_t type;
	uint8_t week;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint32_t interval;

	void set(Config3PriceIndex *data);
	void get(Config3PriceIndex *data);
};
#pragma pack(pop)

void Config3PriceIndexData::set(Config3PriceIndex *data) {
	device.set(data->device.get());
	type = data->type;
	number = data->number;
	week = data->timeTable.getWeek()->getValue();
	hour = data->timeTable.getInterval()->getTime()->getHour();
	minute = data->timeTable.getInterval()->getTime()->getMinute();
	second = data->timeTable.getInterval()->getTime()->getSecond();
	interval = data->timeTable.getInterval()->getInterval();
}

void Config3PriceIndexData::get(Config3PriceIndex *data) {
	data->device.set(device.get());
	data->number = number;
	data->type = type;
	data->timeTable.setWeekTable(week);
	data->timeTable.setInterval(hour, minute, second, interval);
}

MemoryResult Config3PriceIndexList::init(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	return save(memory);
}

MemoryResult Config3PriceIndexList::load(uint16_t priceListNum, Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	list.clearAndFree();
	MemoryCrc crc(memory);
	Config3PriceIndexData data;
	crc.startCrc();
	for(uint16_t i = 0; i < priceListNum; i++) {
		Config3PriceIndex *item = new Config3PriceIndex;
		MemoryResult result = crc.read(&data, sizeof(data));
		if(result != MemoryResult_Ok) {
			delete item;
			return result;
		}
		data.get(item);
		list.add(item);
	}
	return crc.readCrc();
}

MemoryResult Config3PriceIndexList::save() {
	memory->setAddress(address);
	return save(memory);
}

MemoryResult Config3PriceIndexList::save(Memory *memory) {
	Config3PriceIndexData data;
	MemoryCrc crc(memory);
	crc.startCrc();
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config3PriceIndex *item = list.get(i);
		data.set(item);
		MemoryResult result = crc.write(&data, sizeof(data));
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return crc.writeCrc();
}

void Config3PriceIndexList::clear() {
	list.clearAndFree();
}

bool Config3PriceIndexList::add(const char *device, uint8_t number, Config3PriceIndexType type) {
	uint16_t index = this->getIndex(device, number);
	if(index < list.getSize()) {
		return false;
	}
	Config3PriceIndex *item = new Config3PriceIndex;
	item->device.set(device);
	item->number = number;
	item->type = type;
	item->timeTable.clear();
	list.add(item);
	return true;
}

bool Config3PriceIndexList::add(const char *device, uint8_t number, TimeTable *tt) {
	uint16_t index = this->getIndex(device, number);
	if(index < list.getSize()) {
		return false;
	}
	Config3PriceIndex *item = new Config3PriceIndex;
	item->device.set(device);
	item->number = number;
	item->type = Config3PriceIndexType_Time;
	item->timeTable.set(tt);
	list.add(item);
	return true;
}

uint16_t Config3PriceIndexList::getIndex(const char *device, uint8_t number) {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config3PriceIndex *index = list.get(i);
		if(index->device.equal(device) == true && index->number == number) {
			return i;
		}
	}
	return CONFIG_INDEX_UNDEFINED;
}

uint16_t Config3PriceIndexList::getIndexByDateTime(const char *device, DateTime *datetime) {
	WeekDay weekDay = datetime->getWeekDay();
	uint32_t seconds = datetime->getTimeSeconds();
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config3PriceIndex *index = list.get(i);
		if(index->type == Config3PriceIndexType_Time	&& index->device.equal(device) == true && index->timeTable.check(seconds, weekDay) == true) {
			return i;
		}
	}
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config3PriceIndex *index = list.get(i);
		if(index->type == Config3PriceIndexType_Base	&& index->device.equal(device) == true) {
			return i;
		}
	}
	return CONFIG_INDEX_UNDEFINED;
}

Config3PriceIndex* Config3PriceIndexList::get(uint16_t i) {
	return list.get(i);
}

Config3PriceIndex* Config3PriceIndexList::get(const char *device, uint8_t number) {
	uint16_t index = getIndex(device, number);
	if(index == CONFIG_INDEX_UNDEFINED) {
		return NULL;
	}
	return get(index);
}

Config3PriceIndex* Config3PriceIndexList::get(const char *device, DateTime *datetime) {
	uint16_t index = getIndexByDateTime(device, datetime);
	if(index == CONFIG_INDEX_UNDEFINED) {
		return NULL;
	}
	return get(index);
}
