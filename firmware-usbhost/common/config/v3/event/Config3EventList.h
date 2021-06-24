#ifndef COMMON_CONFIG_V3_EVENTLIST_H_
#define COMMON_CONFIG_V3_EVENTLIST_H_

#include "Config3Event.h"

#include "timer/include/RealTime.h"
#include "utils/include/List.h"

class Config3EventList {
public:
	Config3EventList(RealTimeInterface *realtime);
	MemoryResult init(uint16_t size, Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult loadAndCheck(Memory *memory);
	MemoryResult save();
	MemoryResult clear();
	MemoryResult repair(int16_t size, Memory *memory);

	void add(Config3Event::Code code, const char *str = "");
	void add(Fiscal::Sale *sale);
	void add(DateTime *datetime, Config3Event::Code code, const char *str);
	void add(DateTime *datetime, Fiscal::Sale *sale);
	bool get(uint16_t index, Config3Event *event);
	bool getFromFirst(uint16_t index, Config3Event *event);
	void setSync(uint16_t index);

	uint16_t getSize() { return data.size; }
	uint16_t getLen();
	uint16_t getFirst() { return data.first; }
	uint16_t getLast() { return data.last; }
	uint16_t getSync() { return data.sync; }
	uint16_t getUnsync();
	uint16_t getTail();

//+++ удалить
	RealTimeInterface *getRealTime() { return realtime; }
//+++

private:
	RealTimeInterface *realtime;
	Memory *memory;
	uint32_t address;
	Config3EventListData data;

	bool bind(uint16_t index, Config3Event *event);
	MemoryResult initHeadData(uint16_t size, Memory *memory);
	MemoryResult loadHeadData(Memory *memory);
	MemoryResult saveHeadData();
	MemoryResult loadTailData();
	MemoryResult saveTailData();
	MemoryResult incrementPosition();

	static MemoryResult loadData(Memory *memory, Config3EventListData *data);
};

#endif
