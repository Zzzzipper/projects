#ifndef COMMON_CONFIG_WARELINKLIST_H_
#define COMMON_CONFIG_WARELINKLIST_H_

#include "memory/include/Memory.h"
#include "utils/include/StringBuilder.h"
#include "utils/include/List.h"

namespace Config3 {

class WareLink {
public:
	uint32_t groupId;
	uint32_t wareId;
};

class WareLinkList {
public:
	MemoryResult init(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult save();

	void add(uint32_t groupId, uint32_t wareId);
	WareLink *get(uint32_t index);
	uint32_t getLen() const;
	uint32_t getGroupIdByWareId(uint32_t groupId);
	void clear();

private:
	Memory *memory;
	uint32_t address;
	uint32_t num;
	List<WareLink> list;

	MemoryResult loadData(Memory *memory);
	MemoryResult saveData();
	MemoryResult loadLinkData(WareLink *entry);
	MemoryResult saveLinkData(WareLink *entry);
};

}

#endif
