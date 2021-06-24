#include "ConfigMaster.h"

#define AUDIT_BUFFER_SIZE 40000

static uint8_t auditBuffer[AUDIT_BUFFER_SIZE] __attribute__ ((section (".ccmram")));

ConfigMaster::ConfigMaster(ConfigModem *config) :
	config(config),
	buffer(sizeof(auditBuffer), auditBuffer),
	locked(false)
{
}

ConfigModem *ConfigMaster::getConfig() {
	return config;
}

StringBuilder *ConfigMaster::getBuffer() {
	return &buffer;
}

bool ConfigMaster::lock() {
	if(locked == true) {
		return false;
	}
	locked = true;
	return true;
}

void ConfigMaster::unlock() {
	locked = false;
}
