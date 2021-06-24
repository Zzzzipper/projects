#ifndef COMMON_UTILS_TESTEVENTOBSERVER_H
#define COMMON_UTILS_TESTEVENTOBSERVER_H

#include "utils/include/Event.h"
#include "utils/include/StringBuilder.h"

class TestEventObserver : public EventObserver {
public:
	TestEventObserver(StringBuilder *result);
	virtual void proc(Event *event);

protected:
	StringBuilder *result;
};

#endif
