#include "Config1ProductIndex.h"
#include "memory/include/MemoryCrc.h"
#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"

bool Config1ProductIndex::selectId2cashlessId(const char *selectId, uint16_t *cashlessId) {
	StringParser parser(selectId);
	if(parser.getNumber(cashlessId) == false) {
		return false;
	}
	if(parser.hasUnparsed() == true) {
		return false;
	}
	return true;
}

void Config1ProductIndexList::init(Memory *memory) {
	save(memory);
}

MemoryResult Config1ProductIndexList::save(Memory *memory) {
	MemoryCrc crc(memory);
	crc.startCrc();
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config1ProductIndex *item = list.get(i);
		MemoryResult result = crc.write(item, sizeof(Config1ProductIndex));
		if(result != MemoryResult_Ok) {
			return result;
		}
	}
	return crc.writeCrc();
}

MemoryResult Config1ProductIndexList::load(uint16_t productNum, Memory *memory) {
	list.clearAndFree();
	MemoryCrc crc(memory);
	crc.startCrc();
	for(uint16_t i = 0; i < productNum; i++) {
		Config1ProductIndex *item = new Config1ProductIndex;
		MemoryResult result = crc.read(item, sizeof(Config1ProductIndex));
		if(result != MemoryResult_Ok) {
			return result;
		}
		list.add(item);
	}
	return crc.readCrc();
}

void Config1ProductIndexList::clear() {
	list.clearAndFree();
}

bool Config1ProductIndexList::add(const char *selectId) {
	uint16_t cashlessId;
	if(Config1ProductIndex::selectId2cashlessId(selectId, &cashlessId) == false) {
		LOG_ERROR(LOG_CFG, "SelectId(" << selectId << ") not equal CashlessId(" << cashlessId << ")");
		return false;
	}
	return add(selectId, cashlessId);
}

bool Config1ProductIndexList::add(const char *selectId, uint16_t cashlessId) {
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
	Config1ProductIndex *item = new Config1ProductIndex;
	item->selectId.set(selectId);
	item->cashlessId = cashlessId;
	list.add(item);
	LOG_INFO(LOG_CFG, "Added product " << selectId << "/" << cashlessId);
	return true;
}

uint16_t Config1ProductIndexList::getIndex(const char *selectId) {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config1ProductIndex *index = list.get(i);
		if(index->selectId.equal(selectId)) {
			return i;
		}
	}
	return UndefinedIndex;
}

uint16_t Config1ProductIndexList::getIndex(uint16_t cashlessId) {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config1ProductIndex *index = list.get(i);
		if(index->cashlessId == cashlessId) {
			return i;
		}
	}
	return UndefinedIndex;
}

Config1ProductIndex* Config1ProductIndexList::get(uint16_t index) {
	return list.get(index);
}
