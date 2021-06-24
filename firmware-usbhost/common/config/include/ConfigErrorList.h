#ifndef COMMON_CONFIG_ERRORLIST_H_
#define COMMON_CONFIG_ERRORLIST_H_

#include "ConfigEvent.h"

#include "timer/include/RealTime.h"
#include "utils/include/List.h"

class ConfigErrorList {
public:
	ConfigErrorList(RealTimeInterface *realtime);
	bool add(uint16_t code, const char *str);
	void remove(uint16_t code);
	void removeAll();
	ConfigEvent *get(uint16_t index);
	ConfigEvent *getByCode(uint16_t code);
	uint16_t getSize();

private:
	RealTimeInterface *realtime;
	List<ConfigEvent> list;
};

#endif
