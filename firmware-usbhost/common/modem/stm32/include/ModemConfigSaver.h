#ifndef MODEMCONFIGSAVER_H_
#define MODEMCONFIGSAVER_H_

#include "common/modem/include/ModemConfig.h"

class ModemConfigSaver {
public:
	static void save(ModemConfig *config);
	static void load(ModemConfig *config);
private:
	static const uint8_t *getData();
	static uint32_t getDataLen();
};

#endif
