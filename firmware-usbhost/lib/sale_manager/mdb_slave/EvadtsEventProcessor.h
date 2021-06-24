#ifndef LIB_SALEMANAGER_EVADTSEVENTPROCESSOR
#define LIB_SALEMANAGER_EVADTSEVENTPROCESSOR

#include "common/timer/include/DateTime.h"
#include "common/config/include/ConfigModem.h"

class EvadtsEventProcessor {
public:
	static void proc(const char *name, uint8_t activity, uint8_t duration, DateTime *datetime, ConfigModem* config);

private:
	static bool isError(const char *name);
	static ConfigEvent::Code nameToCode(const char *name, uint8_t activity);
};

#endif
