#include "include/TestEventObserver.h"

TestEventObserver::TestEventObserver(StringBuilder *result) : result(result) {}

void TestEventObserver::proc(Event *event) {
	*result << "<event=" << event->getType() << ">";
}
