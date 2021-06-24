#ifndef COMMON_CONFIG_V3_REPAIRER_H_
#define COMMON_CONFIG_V3_REPAIRER_H_

#include "Config4Modem.h"

class Config4Repairer {
public:
	Config4Repairer(Config4Modem *config, Memory *memory);
	MemoryResult repair();

private:
	Memory *memory;
	uint32_t address;
	Config4Modem *config;
};

#endif
