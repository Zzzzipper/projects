#ifndef LIB_MODEM_CONFIGMASTER_H
#define LIB_MODEM_CONFIGMASTER_H

#include "common/config/include/ConfigModem.h"
#include "common/utils/include/StringBuilder.h"

class ConfigMaster {
public:
	ConfigMaster(ConfigModem *config);
	ConfigModem *getConfig();
	StringBuilder *getBuffer();
	bool lock();
	void unlock();

private:
	ConfigModem *config;
	StringBuilder buffer;
	bool locked;
};

#endif
