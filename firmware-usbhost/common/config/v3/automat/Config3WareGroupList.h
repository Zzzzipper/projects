#ifndef COMMON_CONFIG_WAREGROUPLIST_H_
#define COMMON_CONFIG_WAREGROUPLIST_H_

#include "memory/include/Memory.h"
#include "utils/include/StringBuilder.h"
#include "utils/include/List.h"

namespace Config3 {

class WareGroup {
public:
	uint32_t id;
	uint32_t parentId;
	StringBuilder name;
	StringBuilder color;

	WareGroup();
};

class WareGroupList {
public:
	MemoryResult init(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult save();

	void add(uint32_t id, uint32_t parentId, const char *name, const char *color);
	WareGroup *get(uint32_t index);
	uint32_t getLen();
	void clear();

private:
	Memory *memory;
	uint32_t address;
	uint32_t num;
	List<WareGroup> list;

	MemoryResult loadData(Memory *memory);
	MemoryResult saveData();
	MemoryResult loadGroupData(WareGroup *entry);
	MemoryResult saveGroupData(WareGroup *entry);
};

}

#endif
