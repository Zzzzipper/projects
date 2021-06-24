#ifndef COMMON_CONFIG_EVENTLIST1_CONVERTER_H_
#define COMMON_CONFIG_EVENTLIST1_CONVERTER_H_

#include "memory/include/Memory.h"
#include "Config1EventList.h"

class Config1EventListConverter {
public:
	MemoryResult load(Memory *memory);
	MemoryResult save(Memory *memory);

private:
	Memory *memory;
	Config1EventList events1;

	MemoryResult saveEventList(Memory *memory);
	MemoryResult saveEvents(Memory *memory);
	MemoryResult saveEvent(Config1Event *event1, Memory *memory);
};

#endif
