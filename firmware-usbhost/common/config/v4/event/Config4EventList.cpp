#include "Config4EventList.h"
#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

#include <string.h>

Config4EventList::Config4EventList(RealTimeInterface *realtime) :
	realtime(realtime),
	memory(NULL)
{
}

MemoryResult Config4EventList::init(uint32_t size, Memory *memory) {
	LOG_DEBUG(LOG_CFG, "init");
	return list.init(size, memory);
}

MemoryResult Config4EventList::load(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "load");
	return list.loadAndCheck(memory);
}

MemoryResult Config4EventList::loadAndCheck(Memory *memory) {
	LOG_DEBUG(LOG_CFG, "loadAndCheck");
	return list.loadAndCheck(memory);
}

MemoryResult Config4EventList::save() {
	LOG_DEBUG(LOG_CFG, "save");
	return list.save();
}

MemoryResult Config4EventList::clear() {
	LOG_DEBUG(LOG_CFG, "clear");
	return list.clear();
}

MemoryResult Config4EventList::repair(uint32_t size, Memory *memory) {
#if 0
	Config4EventListData data1;
	Config4EventListData data2;
	uint32_t address1 = memory->getAddress();
	uint32_t address2 = address1 + sizeof(data) + size * Config4Event::getDataSize();
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
#else
	LOG_ERROR(LOG_CFG, "repair error");
	return MemoryResult_ProgError;
#endif
}

void Config4EventList::add(Config4Event::Code code, const char *str) {
	DateTime datetime;
	realtime->getDateTime(&datetime);
	add(&datetime, code, str);
}

void Config4EventList::add(Fiscal::Sale *sale) {
	DateTime datetime;
	realtime->getDateTime(&datetime);
	add(&datetime, sale);
}

void Config4EventList::add(DateTime *datetime, Config4Event::Code code, const char *str) {
	LOG_DEBUG(LOG_CFG, "add");
	Config4Event event;
	event.set(datetime, code, str);
	list.insert(&event);
}

void Config4EventList::add(DateTime *datetime, Fiscal::Sale *sale) {
#if 0
	LOG_DEBUG(LOG_CFG, "add");
	Config4Event event;
	event.set(datetime, sale);
	list.insert(&event);
#else
	LOG_DEBUG(LOG_CFG, "add");
	Config4Event event;
	for(uint16_t i = 0; i < sale->getProductNum(); i++) {
		Fiscal::Product *product = sale->products.get(i);
		for(uint16_t q = 0; q < product->quantity; q++) {
			event.set(datetime, sale, i);
			list.insert(&event);
		}
	}
#endif
}

bool Config4EventList::get(uint32_t index, Config4Event *event) {
	LOG_DEBUG(LOG_CFG, "get " << index);
	MemoryResult result = list.get(index, event);
	if(result != MemoryResult_Ok) {
		return false;
	}
	return true;
}

bool Config4EventList::getFromFirst(uint32_t index, Config4Event *event) {
#if 0
	if(index >= data.size || data.first == CONFIG_EVENT_UNSET) {
		return false;
	}
	uint32_t i = data.first + index;
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
#else
	return false;
#endif
}

void Config4EventList::setSync(uint32_t index) {
	LOG_DEBUG(LOG_CFG, "setSync");
	list.remove(index);
}

uint32_t Config4EventList::getLen() {
	return list.getLen();
}

uint32_t Config4EventList::getUnsync() {
	return list.getFirst();
}

uint32_t Config4EventList::getTail() {
	return list.getTail();
}
