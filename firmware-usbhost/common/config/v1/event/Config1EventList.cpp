#include "Config1EventList.h"
#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

#include <string.h>

#define CONFIG_EVENTLIST_VERSION1 1
#define CONFIG_EVENT_UNSET 0xFFFF

Config1EventList::Config1EventList() {
	data.version = CONFIG_EVENTLIST_VERSION1;
	data.size = 0;
	data.first = CONFIG_EVENT_UNSET;
	data.last = CONFIG_EVENT_UNSET;
	data.sync = CONFIG_EVENT_UNSET;
}

MemoryResult Config1EventList::load(Memory *memory) {
	MemoryResult result = loadData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Event header load failed");
		return result;
	}
	for(uint8_t i = 0; i < data.size; i++) {
		Config1Event *event = new Config1Event;
		if(event->load(memory) == false) {
//			event->setDefault();
		}
		list.add(event);
	}
	return MemoryResult_Ok;
}

MemoryResult Config1EventList::save(Memory *memory) {
	MemoryResult result = saveData(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config1Event *event = list.get(i);
		result = event->save(memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return MemoryResult_Ok;
}

void Config1EventList::add(uint16_t code) {
	Config1Event *event = new Config1Event;
	event->data.code = code;
	list.add(event);
	data.size++;
}

Config1Event *Config1EventList::getByIndex(uint16_t index) {
	if(index >= list.getSize()) {
		LOG_ERROR(LOG_CFG, "Wrong index " << index << "," << data.size);
		return NULL;
	}
	return list.get(index);
}


MemoryResult Config1EventList::loadData(Memory *memory) {
	MemoryCrc crc(memory);
	crc.startCrc();
	MemoryResult result = crc.read(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = crc.readCrc();
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Wrong event list CRC");
		return result;
	}
	if(data.version != CONFIG_EVENTLIST_VERSION1) {
		LOG_ERROR(LOG_CFG, "Wrong event list version " << data.version);
		return MemoryResult_WrongVersion;
	}
	return MemoryResult_Ok;
}

MemoryResult Config1EventList::saveData(Memory *memory) {
	MemoryCrc crc(memory);
	crc.startCrc();
	MemoryResult result = crc.write(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return crc.writeCrc();
}
