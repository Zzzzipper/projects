#include "Config3ProductIndex.h"
#include "memory/include/MemoryCrc.h"
#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"

MemoryResult Config3ProductIndexList::init(Memory *memory) {
	return save(memory);
}

MemoryResult Config3ProductIndexList::load(uint16_t productNum, Memory *memory) {
	list.clearAndFree();
	MemoryCrc crc(memory);
	crc.startCrc();
	for(uint16_t i = 0; i < productNum; i++) {
		Config3ProductIndex *item = new Config3ProductIndex;
		MemoryResult result = crc.read(item, sizeof(Config3ProductIndex));
		if(result != MemoryResult_Ok) {
			delete item;
			return result;
		}
		list.add(item);
	}
	return crc.readCrc();
}

MemoryResult Config3ProductIndexList::save(Memory *memory) {
	MemoryCrc crc(memory);
	crc.startCrc();
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config3ProductIndex *item = list.get(i);
		MemoryResult result = crc.write(item, sizeof(Config3ProductIndex));
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return crc.writeCrc();
}

void Config3ProductIndexList::clear() {
	list.clearAndFree();
}

bool Config3ProductIndexList::add(const char *selectId, uint16_t cashlessId) {
	uint16_t index = this->getIndex(selectId);
	if(index < list.getSize()) {
		LOG_ERROR(LOG_CFG, "SelectId " << selectId << " already registered");
		return false;
	}
	if(cashlessId != UndefinedIndex) {
		index = this->getIndex(cashlessId);
		if(index < list.getSize()) {
			LOG_ERROR(LOG_CFG, "CashlessId " << cashlessId << " already registered");
			return false;
		}
	}
	Config3ProductIndex *item = new Config3ProductIndex;
	item->selectId.set(selectId);
	item->cashlessId = cashlessId;
	list.add(item);
	LOG_INFO(LOG_CFG, "Added product " << selectId << "/" << cashlessId);
	return true;
}

uint16_t Config3ProductIndexList::getIndex(const char *selectId) {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config3ProductIndex *index = list.get(i);
		if(index->selectId.equal(selectId)) {
			return i;
		}
	}
	return UndefinedIndex;
}

uint16_t Config3ProductIndexList::getIndex(uint16_t cashlessId) {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config3ProductIndex *index = list.get(i);
		if(index->cashlessId == cashlessId) {
			return i;
		}
	}
	return UndefinedIndex;
}

Config3ProductIndex* Config3ProductIndexList::get(uint16_t index) {
	return list.get(index);
}
