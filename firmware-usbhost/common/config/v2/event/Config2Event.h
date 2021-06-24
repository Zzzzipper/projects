#ifndef COMMON_CONFIG_V2_EVENT_H_
#define COMMON_CONFIG_V2_EVENT_H_

#include "config/v3/event/Config3EventData.h"
#include "timer/include/RealTime.h"
#include "utils/include/StringBuilder.h"
#include "memory/include/Memory.h"

class Config2Event {
public:
	Config3EventStruct data;

	Config2Event();
	MemoryResult load(Memory *memory);
	MemoryResult save(Memory *memory);

	DateTime *getDate() { return &data.date; }
	uint16_t getCode() { return data.code; }
	uint32_t getNumber() { return data.data.number; }
	const char *getString() { return data.data.string.get(); }
	Config3EventSale *getSale() { return &(data.sale); }
	Config3EventStruct *getData() { return &data; }
};

#endif
