#include "Config3EventIterator.h"
#include "logger/include/Logger.h"

Config3EventIterator::Config3EventIterator(Config3EventList *events) :
	events(events),
	index(CONFIG3_EVENT_UNSET),
	loadResult(false)
{
}

bool Config3EventIterator::findByIndex(uint16_t index) {
	return events->get(index, &event);
}

bool Config3EventIterator::first() {
	index = events->getFirst();
	if(index == CONFIG3_EVENT_UNSET) {
		LOG_ERROR(LOG_CFG, "first failed" << index);
		return false;
	}
	loadResult = events->get(index, &event);
	return true;
}

bool Config3EventIterator::next() {
	if(index == CONFIG3_EVENT_UNSET) {
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

bool Config3EventIterator::unsync() {
	index = events->getUnsync();
	if(index == CONFIG3_EVENT_UNSET) {
		return false;
	}
	loadResult = events->get(index, &event);
	return true;
}

bool Config3EventIterator::hasLoadError() {
	return (loadResult == false);
}

uint16_t Config3EventIterator::getIndex() {
	return index;
}

DateTime *Config3EventIterator::getDate() {
	return event.getDate();
}

uint16_t Config3EventIterator::getCode() {
	return event.getCode();
}

const char *Config3EventIterator::getString() {
	return event.getString();
}

Config3EventSale *Config3EventIterator::getSale() {
	return event.getSale();
}

Config3EventStruct *Config3EventIterator::getData() {
	return event.getData();
}

Config3Event *Config3EventIterator::getEvent() {
	return &event;
}
