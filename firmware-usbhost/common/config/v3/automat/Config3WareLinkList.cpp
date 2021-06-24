#include "Config3WareLinkList.h"

#include "memory/include/MemoryCrc.h"
#include "utils/include/NetworkProtocol.h"
#include "logger/include/Logger.h"

namespace Config3 {

#pragma pack(push,1)
struct WareLinkListData {
	uint16_t num;
	uint8_t  crc[1];
};

struct WareLinkData {
	uint32_t groupId;
	uint32_t wareId;
	uint8_t  crc[1];
};
#pragma pack(pop)

MemoryResult WareLinkList::init(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	list.clearAndFree();
	return saveData();
}

MemoryResult WareLinkList::load(Memory *memory) {
	MemoryResult result = loadData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Group header load failed");
		return result;
	}
	for(uint8_t i = 0; i < num; i++) {
		WareLink *entry = new WareLink;
		result = loadLinkData(entry);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Group " << i << " read failed");
			return result;
		}
		list.add(entry);
		LOG_INFO(LOG_CFG, "load group " << i << "/" << entry->groupId << "/" << entry->wareId);
	}
	return MemoryResult_Ok;
}

MemoryResult WareLinkList::save() {
	MemoryResult result = saveData();
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Group header load failed");
		return result;
	}
	for(uint8_t i = 0; i < list.getSize(); i++) {
		WareLink *entry = list.get(i);
		if(entry == NULL) {
			return MemoryResult_ProgError;
		}
		result = saveLinkData(entry);
		if(result != MemoryResult_Ok) {
			return result;
		}
		LOG_INFO(LOG_CFG, "save link " << i << "/" << entry->groupId << "/" << entry->wareId);
	}
	return MemoryResult_Ok;
}

void WareLinkList::add(uint32_t groupId, uint32_t wareId) {
	WareLink *entry = new WareLink;
	entry->groupId = groupId;
	entry->wareId = wareId;
	list.add(entry);
}

WareLink *WareLinkList::get(uint32_t index) {
	return list.get(index);
}

uint32_t WareLinkList::getLen() const {
	return list.getSize();
}

uint32_t WareLinkList::getGroupIdByWareId(uint32_t wareId) {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		WareLink *entry = list.get(i);
		if(entry == NULL) {
			return 0;
		}
		if(entry->wareId == wareId) {
			return entry->groupId;
		}
	}
	return 0;
}

void WareLinkList::clear() {
	list.clearAndFree();
}

MemoryResult WareLinkList::loadData(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	WareLinkListData data;
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	num = data.num;
	LOG_DEBUG(LOG_CFG, "GroupNum=" << num);
	return MemoryResult_Ok;
}

MemoryResult WareLinkList::saveData() {
	LOG_DEBUG(LOG_CFG, "saveData " << address << "," << list.getSize());
	WareLinkListData data;
	data.num = list.getSize();
	MemoryCrc crc(memory);
	MemoryResult result = crc.writeDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult WareLinkList::loadLinkData(WareLink *entry) {
	LOG_DEBUG(LOG_CFG, "loadLinkData");
	WareLinkData data;
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	entry->groupId = data.groupId;
	entry->wareId = data.wareId;
	return MemoryResult_Ok;
}

MemoryResult WareLinkList::saveLinkData(WareLink *entry) {
	LOG_DEBUG(LOG_CFG, "saveLinkData");
	WareLinkData data;
	data.groupId = entry->groupId;
	data.wareId = entry->wareId;
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

}
