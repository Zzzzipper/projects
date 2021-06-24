#include "Config3WareGroupList.h"

#include "memory/include/MemoryCrc.h"
#include "utils/include/NetworkProtocol.h"
#include "logger/include/Logger.h"

namespace Config3 {

#pragma pack(push,1)
struct WareGroupListData {
	uint16_t num;
	uint8_t  crc[1];
};

struct WareGroupData {
	uint32_t id;
	uint32_t groupId;
	StrParam<32> name;
	StrParam<6> color;
	uint8_t  crc[1];
};
#pragma pack(pop)

WareGroup::WareGroup() :
	name(32, 32),
	color(6, 6)
{

}

MemoryResult WareGroupList::init(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	list.clearAndFree();
	return saveData();
}

MemoryResult WareGroupList::load(Memory *memory) {
	MemoryResult result = loadData(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Group header load failed");
		return result;
	}
	for(uint8_t i = 0; i < num; i++) {
		WareGroup *entry = new WareGroup;
		result = loadGroupData(entry);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Group " << i << " read failed");
			return result;
		}
		list.add(entry);
		LOG_INFO(LOG_CFG, "load group " << i << "/" << entry->id << "/" << entry->name.getString());
	}
	return MemoryResult_Ok;
}

MemoryResult WareGroupList::save() {
	MemoryResult result = saveData();
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Group header load failed");
		return result;
	}
	for(uint8_t i = 0; i < list.getSize(); i++) {
		WareGroup *entry = list.get(i);
		if(entry == NULL) {
			return MemoryResult_ProgError;
		}
		result = saveGroupData(entry);
		if(result != MemoryResult_Ok) {
			return result;
		}
		LOG_INFO(LOG_CFG, "save group " << i << "/" << entry->id << "/" << entry->name.getString());
	}
	return MemoryResult_Ok;
}

void WareGroupList::add(uint32_t id, uint32_t parentId, const char *name, const char *color) {
	WareGroup *entry = new WareGroup;
	entry->id = id;
	entry->parentId = parentId;
	entry->name.set(name);
	entry->color.set(color);
	list.add(entry);
}

WareGroup *WareGroupList::get(uint32_t index) {
	return list.get(index);
}

uint32_t WareGroupList::getLen() {
	return list.getSize();
}

void WareGroupList::clear() {
	list.clearAndFree();
}

MemoryResult WareGroupList::loadData(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	WareGroupListData data;
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	num = data.num;
	LOG_DEBUG(LOG_CFG, "GroupNum=" << num);
	return MemoryResult_Ok;
}

MemoryResult WareGroupList::saveData() {
	LOG_DEBUG(LOG_CFG, "saveData " << address << "," << list.getSize());
	WareGroupListData data;
	data.num = list.getSize();
	MemoryCrc crc(memory);
	MemoryResult result = crc.writeDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult WareGroupList::loadGroupData(WareGroup *entry) {
	LOG_DEBUG(LOG_CFG, "loadGroupData");
	WareGroupData data;
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	entry->id = data.id;
	entry->parentId = data.groupId;
	entry->name.set(data.name.get());
	entry->color.set(data.color.get());
	return MemoryResult_Ok;
}

MemoryResult WareGroupList::saveGroupData(WareGroup *entry) {
	LOG_DEBUG(LOG_CFG, "saveGroupData");
	WareGroupData data;
	data.id = entry->id;
	data.groupId = entry->parentId;
	data.name.set(entry->name.getString());
	data.color.set(entry->color.getString());
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

}
