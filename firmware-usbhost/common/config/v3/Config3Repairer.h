#ifndef COMMON_CONFIG_V3_REPAIRER_H_
#define COMMON_CONFIG_V3_REPAIRER_H_

#include "Config3Modem.h"

class Config3Repairer {
public:
	Config3Repairer(Config3Modem *config, Memory *memory);
	MemoryResult repair();

private:
	Config3Modem *config;
	Memory *memory;
	uint32_t address;

	MemoryResult convert1();
	MemoryResult convert2();
};

#endif
