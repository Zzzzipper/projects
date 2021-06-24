#ifndef COMMON_CONFIG_CONFIGEVENTLIST1_H_
#define COMMON_CONFIG_CONFIGEVENTLIST1_H_

#include "Config1Event.h"
#include "utils/include/List.h"

class Config1EventList {
public:
	Config1EventList();
	MemoryResult load(Memory *memory);
	MemoryResult save(Memory *memory);

	void add(uint16_t code);
	Config1Event *getByIndex(uint16_t index);

	uint16_t getSize() { return data.size; }
	uint16_t getFirst() { return data.first; }
	uint16_t getLast() { return data.last; }
	uint16_t getSync() { return data.sync; }

private:
	struct {
		uint8_t version;
		uint16_t size;
		uint16_t first;
		uint16_t last;
		uint16_t sync;
	} data;
	List<Config1Event> list;

	MemoryResult loadData(Memory *memory);
	MemoryResult saveData(Memory *memory);
};

#endif
