#include "config/include/ConfigErrorList.h"
#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

#include <string.h>

ConfigErrorList::ConfigErrorList(RealTimeInterface *realtime) :
	realtime(realtime)
{

}

bool ConfigErrorList::add(uint16_t code, const char *str) {
	DateTime datetime;
	realtime->getDateTime(&datetime);

	ConfigEvent *error = getByCode(code);
	if(error != NULL) {
		error->set(&datetime, code, str);
		return false;
	}

	error = new ConfigEvent;
	error->set(&datetime, code, str);
	list.add(error);
	return true;
}

void ConfigErrorList::remove(uint16_t code) {
	ConfigEvent *error = getByCode(code);
	if(error == NULL) {
		return;
	}

	list.remove(error);
}

void ConfigErrorList::removeAll() {
	list.clearAndFree();
}

ConfigEvent *ConfigErrorList::get(uint16_t index) {
	return list.get(index);
}

ConfigEvent *ConfigErrorList::getByCode(uint16_t code) {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		ConfigEvent *event = list.get(i);
		if(event->getCode() == code) {
			return event;
		}
	}
	return NULL;
}

uint16_t ConfigErrorList::getSize() {
	return list.getSize();
}
