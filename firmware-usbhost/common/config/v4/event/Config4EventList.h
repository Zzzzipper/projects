#ifndef COMMON_CONFIG_V4_CONFIGEVENTLIST_H_
#define COMMON_CONFIG_V4_CONFIGEVENTLIST_H_

#include "Config4Event.h"
#include "RingList.h"

#include "utils/include/List.h"

#define CONFIG4_EVENT_UNSET RINGLIST_UNDEFINED

class Config4EventList {
public:
	Config4EventList(RealTimeInterface *realtime);
	MemoryResult init(uint32_t size, Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult loadAndCheck(Memory *memory);
	MemoryResult save();
	MemoryResult clear();
	MemoryResult repair(uint32_t size, Memory *memory);

	void add(Config4Event::Code code, const char *str = "");
	void add(Fiscal::Sale *sale);
	void add(DateTime *datetime, Config4Event::Code code, const char *str);
	void add(DateTime *datetime, Fiscal::Sale *sale);
	bool get(uint32_t index, Config4Event *event);
	bool getFromFirst(uint32_t index, Config4Event *event);
	void setSync(uint32_t index);

	uint32_t getSize() { return list.getSize(); }
	uint32_t getLen();
	uint32_t getFirst() { return list.getFirst(); }
	uint32_t getLast() { return list.getLast(); }
	uint32_t getUnsync();
	uint32_t getTail();

//+++ удалить
	RealTimeInterface *getRealTime() { return realtime; }
//+++

private:
	RealTimeInterface *realtime;
	Memory *memory;
	uint32_t address;
	RingList<Config4Event> list;
};

#endif
