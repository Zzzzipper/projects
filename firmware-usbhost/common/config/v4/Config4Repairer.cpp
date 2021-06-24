#include "Config4Repairer.h"
#include "logger/include/Logger.h"

#define EVENT_LIST_SIZE 100

Config4Repairer::Config4Repairer(Config4Modem *config, Memory *memory) :
	memory(memory),
	address(0),
	config(config)
{

}

MemoryResult Config4Repairer::repair() {
	LOG_DEBUG(LOG_CFG, "repair");
#if 0
	return config->reinit();
#else
	return config->init();
#endif
}
