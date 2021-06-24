#ifndef COMMON_CONFIG_V3_EVENTITERATOR_H_
#define COMMON_CONFIG_V3_EVENTITERATOR_H_

#include "Config3EventList.h"

class Config3EventIterator {
public:
	Config3EventIterator(Config3EventList *events);
	bool findByIndex(uint16_t index);
	bool first();
	bool next();
	bool unsync();
	bool hasLoadError();

	uint16_t getIndex();
	DateTime *getDate();
	uint16_t getCode();
	const char *getString();
	Config3EventSale *getSale();
	Config3EventStruct *getData();
	Config3Event *getEvent();

private:
	Config3EventList *events;
	Config3Event event;
	uint16_t index;
	bool loadResult;
};

#endif
