#include "Config3EventList.h"
#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

#include <string.h>

Config3EventList::Config3EventList(RealTimeInterface *realtime) :
	realtime(realtime),
	memory(NULL)
{
}

MemoryResult Config3EventList::init(uint16_t size, Memory *memory) {
	LOG_DEBUG(LOG_CFG, "init");
	MemoryResult result = initHeadData(size, memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	Config3Event event;
	for(uint16_t i = 0; i < data.size; i++) {
		result = event.init(memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return saveTailData();
}

MemoryResult Config3EventList::load(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "load");
	MemoryResult result = loadHeadData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Event head header load failed");
		return result;
	}
	result = loadTailData();
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Event tail header load failed");
		return result;
	}
	LOG_INFO(LOG_CFG, "Events config OK");
	return MemoryResult_Ok;
}

MemoryResult Config3EventList::loadAndCheck(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "loadAndCheck");
	MemoryResult result = loadHeadData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Event head header load failed");
		return result;
	}
	Config3Event event;
	for(uint16_t i = 0; i < data.size; i++) {
		result = event.load(memory);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Event " << i << " load failed");
			return result;
		}
	}
	result = loadTailData();
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Event tail header load failed");
		return result;
	}
	LOG_INFO(LOG_CFG, "Events config OK");
	return MemoryResult_Ok;
}

MemoryResult Config3EventList::save() {
	LOG_DEBUG(LOG_CFG, "save");
	MemoryResult result = saveHeadData();
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = saveTailData();
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config3EventList::clear() {
	data.first = CONFIG3_EVENT_UNSET;
	data.last = CONFIG3_EVENT_UNSET;
	data.sync = CONFIG3_EVENT_UNSET;
	MemoryResult result = saveHeadData();
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = saveTailData();
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config3EventList::repair(int16_t size, Memory *memory) {
	Config3EventListData data1;
	Config3EventListData data2;
	uint32_t address1 = memory->getAddress();
	uint32_t address2 = address1 + sizeof(data) + size * Config3Event::getDataSize();
	MemoryResult result = loadData(memory, &data1);
	if(result != MemoryResult_Ok) {
		LOG_INFO(LOG_CFG, "Head data damaged");
		memory->setAddress(address2);
		result = loadData(memory, &data2);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Head and tail damaged");
			return result;
		}
		this->memory = memory;
		this->address = address1;
		this->data.set(&data2);
		result = saveHeadData();
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Head data repair failed");
			return result;
		}
		LOG_INFO(LOG_CFG, "Head data repaired");
		return MemoryResult_Ok;
	} else {
		LOG_INFO(LOG_CFG, "Head data OK");
		memory->setAddress(address2);
		result = loadData(memory, &data2);
		if(result == MemoryResult_Ok) {
			LOG_INFO(LOG_CFG, "Head and tail data OK");
			return MemoryResult_Ok;
		}
		this->memory = memory;
		this->address = address1;
		this->data.set(&data1);
		result = saveTailData();
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Tail data repair failed");
			return result;
		}
		LOG_INFO(LOG_CFG, "Tail data repaired");
		return MemoryResult_Ok;
	}
}

void Config3EventList::add(Config3Event::Code code, const char *str) {
	DateTime datetime;
	realtime->getDateTime(&datetime);
	add(&datetime, code, str);
}

void Config3EventList::add(Fiscal::Sale *sale) {
	DateTime datetime;
	realtime->getDateTime(&datetime);
	add(&datetime, sale);
}

void Config3EventList::add(DateTime *datetime, Config3Event::Code code, const char *str) {
	LOG_DEBUG(LOG_CFG, "add");
	Config3Event event;
	if(bind(getTail(), &event) == false) {
		LOG_ERROR(LOG_CFG, "Load event failed");
		return;
	}
	event.set(datetime, code, str);
	incrementPosition();
}

void Config3EventList::add(DateTime *datetime, Fiscal::Sale *sale) {
#if 0
	LOG_DEBUG(LOG_CFG, "add");
	Config3Event event;
	if(bind(getTail(), &event) == false) {
		LOG_ERROR(LOG_CFG, "Load event failed");
		return;
	}
	event.set(datetime, sale);
	incrementPosition();
#else
	LOG_DEBUG(LOG_CFG, "add");
	Config3Event event;
	for(uint16_t i = 0; i < sale->getProductNum(); i++) {
		Fiscal::Product *product = sale->products.get(i);
		for(uint16_t q = 0; q < product->quantity; q++) {
			if(bind(getTail(), &event) == false) {
				LOG_ERROR(LOG_CFG, "Load event failed");
				return;
			}
			datetime->addSeconds(1);
			event.set(datetime, sale, i);
			incrementPosition();
		}
	}
#endif
}

bool Config3EventList::get(uint16_t index, Config3Event *event) {
	if(memory == NULL) {
		LOG_ERROR(LOG_CFG, "Event list not inited");
		return false;
	}
	if(index >= data.size) {
		LOG_ERROR(LOG_CFG, "Wrong index " << index << "," << data.size);
		return false;
	}
	uint32_t entrySize = Config3Event::getDataSize();
	uint32_t entryAddress = address + sizeof(data) + index * entrySize;
	LOG_DEBUG(LOG_CFG, "getByIndex " << index << ":" << "," << entryAddress);
	memory->setAddress(entryAddress);
	MemoryResult result = event->load(memory);
	if(result != MemoryResult_Ok) {
		return false;
	}
	return true;
}

bool Config3EventList::getFromFirst(uint16_t index, Config3Event *event) {
	if(index >= data.size || data.first == CONFIG3_EVENT_UNSET) {
		return false;
	}
	uint16_t i = data.first + index;
	if(data.last > data.first) {
		if(i > data.last) {
			return false;
		}
	} else {
		if(i >= data.size) {
			i = i - data.size;
			if(i > data.last) {
				return false;
			}
		}
	}
	return get(i, event);
}

void Config3EventList::setSync(uint16_t index) {
	data.sync = index;
	saveHeadData();
	saveTailData();
}

uint16_t Config3EventList::getLen() {
	if(data.first == CONFIG3_EVENT_UNSET) {
		return 0;
	}
	if(data.first <= data.last) {
		return (data.last - data.first + 1);
	} else {
		return data.size;
	}
}

uint16_t Config3EventList::getUnsync() {
	if(data.sync == CONFIG3_EVENT_UNSET) {
		if(data.first == CONFIG3_EVENT_UNSET) {
			return CONFIG3_EVENT_UNSET;
		} else {
			return data.first;
		}
	}
	if(data.sync == data.last) {
		return CONFIG3_EVENT_UNSET;
	}

	uint16_t unsync = data.sync + 1;
	if(unsync >= data.size) {
		unsync = 0;
	}
	if(data.first <= data.last) {
		if(unsync < data.first || unsync > data.last) {
			return CONFIG3_EVENT_UNSET;
		}
	} else {
		if(unsync < data.first && unsync > data.last) {
			return CONFIG3_EVENT_UNSET;
		}
	}
	return unsync;
}

uint16_t Config3EventList::getTail() {
	uint16_t tail = data.last + 1;
	if(tail >= data.size) {
		tail = 0;
	}
	return tail;
}

bool Config3EventList::bind(uint16_t index, Config3Event *event) {
	if(memory == NULL) {
		LOG_ERROR(LOG_CFG, "Event list not inited");
		return false;
	}
	if(index >= data.size) {
		LOG_ERROR(LOG_CFG, "Wrong index " << index << "," << data.size);
		return false;
	}
	uint32_t entrySize = Config3Event::getDataSize();
	uint32_t entryAddress = address + sizeof(data) + index * entrySize;
	LOG_DEBUG(LOG_CFG, "getByIndex " << index << ":" << "," << entryAddress);
	memory->setAddress(entryAddress);
	event->bind(memory);
	return true;
}

MemoryResult Config3EventList::initHeadData(uint16_t size, Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	data.version = CONFIG_EVENTLIST_VERSION;
	data.size = size;
	data.first = CONFIG3_EVENT_UNSET;
	data.last = CONFIG3_EVENT_UNSET;
	data.sync = CONFIG3_EVENT_UNSET;
	return saveHeadData();
}

MemoryResult Config3EventList::loadHeadData(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
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

MemoryResult Config3EventList::saveHeadData() {
	LOG_DEBUG(LOG_CFG, "saveHeadData");
	memory->setAddress(address);
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

MemoryResult Config3EventList::loadTailData() {
	uint32_t address2 = this->address + sizeof(data) + data.size * Config3Event::getDataSize();
	memory->setAddress(address2);
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

MemoryResult Config3EventList::saveTailData() {
	LOG_DEBUG(LOG_CFG, "saveTailData");
	uint32_t address2 = this->address + sizeof(data) + data.size * Config3Event::getDataSize();
	memory->setAddress(address2);
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

MemoryResult Config3EventList::incrementPosition() {
	if(data.last == CONFIG3_EVENT_UNSET) {
		data.last = 0;
	} else {
		data.last++;
		if(data.last >= data.size) {
			data.last = 0;
		}
	}

	if(data.first == CONFIG3_EVENT_UNSET) {
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
		data.sync = CONFIG3_EVENT_UNSET;
	}

	LOG_DEBUG(LOG_CFG, "incrementPosition " << data.first << "," << data.last << "," << data.sync);
	MemoryResult result = saveHeadData();
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = saveTailData();
	if(result != MemoryResult_Ok) {
		return result;
	}

	return MemoryResult_Ok;
}

MemoryResult Config3EventList::loadData(Memory *memory, Config3EventListData *data) {
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(data, sizeof(Config3EventListData));
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Wrong event list CRC");
		return result;
	}
	if(data->version != CONFIG_EVENTLIST_VERSION) {
		LOG_ERROR(LOG_CFG, "Wrong event list version " << data->version);
		return MemoryResult_WrongVersion;
	}
	return MemoryResult_Ok;
}
