#ifndef COMMON_CONFIG_V4_EVENTITERATOR_H_
#define COMMON_CONFIG_V4_EVENTITERATOR_H_

#include "Config4EventList.h"

class Config4EventIterator {
public:
	Config4EventIterator(Config4EventList *events);
	bool findByIndex(uint16_t index);
	bool first();
	bool next();
	bool unsync();
	bool hasLoadError();

	uint16_t getIndex();
	DateTime *getDate();
	uint16_t getCode();
	const char *getString();
	Config4EventSale *getSale();
	Config4EventStruct *getData();
	Config4Event *getEvent();

private:
	Config4EventList *events;
	Config4Event event;
	uint32_t index;
	bool loadResult;
};

#endif
