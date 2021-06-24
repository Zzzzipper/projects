#include "Config4EventIterator.h"
#include "logger/include/Logger.h"

Config4EventIterator::Config4EventIterator(Config4EventList *events) :
	events(events),
	index(CONFIG4_EVENT_UNSET),
	loadResult(false)
{
}

bool Config4EventIterator::findByIndex(uint16_t index) {
	return events->get(index, &event);
}

bool Config4EventIterator::first() {
	index = events->getFirst();
	if(index == CONFIG4_EVENT_UNSET) {
		LOG_ERROR(LOG_CFG, "first failed" << index);
		return false;
	}
	loadResult = events->get(index, &event);
	return true;
}

bool Config4EventIterator::next() {
	if(index == CONFIG4_EVENT_UNSET) {
		return false;
	}
	if(index == events->getLast()) {
		return false;
	}
	index++;
	if(index >= events->getSize()) {
		index = 0;
	}
	loadResult = events->get(index, &event);
	return true;
}

bool Config4EventIterator::unsync() {
	index = events->getUnsync();
	if(index == CONFIG4_EVENT_UNSET) {
		return false;
	}
	loadResult = events->get(index, &event);
	return true;
}

bool Config4EventIterator::hasLoadError() {
	return (loadResult == false);
}

uint16_t Config4EventIterator::getIndex() {
	return index;
}

DateTime *Config4EventIterator::getDate() {
	return event.getDate();
}

uint16_t Config4EventIterator::getCode() {
	return event.getCode();
}

const char *Config4EventIterator::getString() {
	return event.getString();
}

Config4EventSale *Config4EventIterator::getSale() {
	return event.getSale();
}

Config4EventStruct *Config4EventIterator::getData() {
	return event.getData();
}

Config4Event *Config4EventIterator::getEvent() {
	return &event;
}
