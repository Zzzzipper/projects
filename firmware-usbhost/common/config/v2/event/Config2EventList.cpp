#include "Config2EventList.h"
#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

#include <string.h>

#define CONFIG_EVENTLIST_VERSION2 2
#define CONFIG_EVENT_UNSET 0xFFFF

Config2EventList::Config2EventList() {
	data.version = CONFIG_EVENTLIST_VERSION2;
	data.size = 0;
	data.first = CONFIG_EVENT_UNSET;
	data.last = CONFIG_EVENT_UNSET;
	data.sync = CONFIG_EVENT_UNSET;
}

MemoryResult Config2EventList::load(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "load");
	MemoryResult result = loadHeadData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Event head header load failed");
		return result;
	}

	for(uint16_t i = 0; i < data.size; i++) {
		Config2Event *event = new Config2Event;
		result = event->load(memory);
		if(result != MemoryResult_Ok) {
//			event->setDefault();
		}
		list.add(event);
	}

	result = loadTailData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Event tail header load failed");
		return result;
	}

	LOG_INFO(LOG_CFG, "Events config OK");
	return MemoryResult_Ok;
}

MemoryResult Config2EventList::save(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "save");
	MemoryResult result = saveHeadData(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}

	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config2Event *event = list.get(i);
		result = event->save(memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}

	result = saveTailData(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

void Config2EventList::add(uint16_t code) {
	Config2Event *event = new Config2Event;
	event->data.code = code;
	list.add(event);
	data.size++;
}

Config2Event *Config2EventList::getByIndex(uint16_t index) {
	if(index >= list.getSize()) {
		LOG_ERROR(LOG_CFG, "Wrong index " << index << "," << data.size);
		return NULL;
	}
	return list.get(index);
}

MemoryResult Config2EventList::loadHeadData(Memory *memory) {
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Wrong event list CRC");
		return result;
	}
	if(data.version != CONFIG_EVENTLIST_VERSION) {
		LOG_ERROR(LOG_CFG, "Wrong event list version " << data.version);
		return MemoryResult_WrongVersion;
	}
	LOG_DEBUG(LOG_CFG, "loadHeadData " << data.first << "," << data.last << "," << data.sync);
	return MemoryResult_Ok;
}

MemoryResult Config2EventList::saveHeadData(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "saveHeadData");
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

MemoryResult Config2EventList::loadTailData(Memory *memory) {
	MemoryCrc crc(memory);
	Config3EventListData data2;
	MemoryResult result = crc.readDataWithCrc(&data2, sizeof(data2));
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Wrong event list CRC");
		return result;
	}
	if(data2.version != CONFIG_EVENTLIST_VERSION) {
		LOG_ERROR(LOG_CFG, "Wrong event list version " << data2.version);
		return MemoryResult_WrongVersion;
	}
	if(data.first != data2.first || data.last != data2.last || data.size != data2.size || data.sync != data2.sync) {
		LOG_ERROR(LOG_CFG, "Data not equal");
		return MemoryResult_WrongData;
	}
	LOG_DEBUG(LOG_CFG, "loadTailData " << data2.first << "," << data2.last << "," << data2.sync);
	return MemoryResult_Ok;
}

MemoryResult Config2EventList::saveTailData(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "saveTailData");
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

void Config2EventList::incrementPosition() {
	if(data.last == CONFIG_EVENT_UNSET) {
		data.last = 0;
	} else {
		data.last++;
		if(data.last >= data.size) {
			data.last = 0;
		}
	}

	if(data.first == CONFIG_EVENT_UNSET) {
		data.first = 0;
	} else {
		if(data.last == data.first) {
			data.first++;
			if(data.first >= data.size) {
				data.first = 0;
			}
		}
	}

	if(data.last == data.sync) {
		data.sync = CONFIG_EVENT_UNSET;
	}

	LOG_DEBUG(LOG_CFG, "incrementPosition " << data.first << "," << data.last << "," << data.sync);
}
