#ifndef COMMON_CONFIG_V2_EVENTLIST_H_
#define COMMON_CONFIG_V2_EVENTLIST_H_

#include "Config2Event.h"

#include "config/v3/event/Config3Event.h"
#include "utils/include/List.h"

class Config2EventList {
public:
	Config2EventList();
	MemoryResult load(Memory *memory);
	MemoryResult save(Memory *memory);

	void add(uint16_t code);
	Config2Event *getByIndex(uint16_t index);

	uint16_t getSize() { return data.size; }
	uint16_t getFirst() { return data.first; }
	uint16_t getLast() { return data.last; }
	uint16_t getSync() { return data.sync; }

private:
	Config3EventListData data;
	List<Config2Event> list;

	bool bind(uint16_t index, Config3Event *event);
	bool initHeadData(uint16_t size, Memory *memory);
	MemoryResult loadHeadData(Memory *memory);
	MemoryResult saveHeadData(Memory *memory);
	MemoryResult loadTailData(Memory *memory);
	MemoryResult saveTailData(Memory *memory);
	void incrementPosition();
};

#endif
