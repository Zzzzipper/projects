#include "Config3Repairer.h"
#include "config/v1/automat/Config1AutomatConverter.h"
#include "config/v2/automat/Config2AutomatConverter.h"
#include "config/v1/fiscal/Config1FiscalConverter.h"
#include "config/v1/event/Config1EventListConverter.h"
#include "config/v2/event/Config2EventList.h"
#include "logger/include/Logger.h"

#define EVENT_LIST_SIZE 100

Config3Repairer::Config3Repairer(
	Config3Modem *config,
	Memory *memory
) :
	config(config),
	memory(memory),
	address(0)
{

}

MemoryResult Config3Repairer::repair() {
	LOG_DEBUG(LOG_CFG, "repair");
	MemoryResult result = convert2();
	if(result == MemoryResult_Ok) {
		return config->load();
	}

	result = convert1();
	if(result == MemoryResult_Ok) {
		return config->load();
	}

	return config->reinit();
}

MemoryResult Config3Repairer::convert1() {
	LOG_DEBUG(LOG_CFG, "convert1");
	memory->setAddress(address);
	Config1Boot boot;
	MemoryResult result = boot.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Boot1 load failed");
		return result;
	}

	uint32_t start = memory->getAddress();
	ConfigFiscal1Converter fiscal;
	result = fiscal.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_INFO(LOG_CFG, "Fiscal1 load failed");
		return result;
	}

	Config1EventListConverter events;
	result = events.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Events1 load failed");
		return result;
	}

	Config1AutomatConverter automat;
	result = automat.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Automat1 load failed");
		return result;
	}

	memory->setAddress(start);
	result = fiscal.save(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Fiscal2 save failed");
		return result;
	}

	result = events.save(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Events2 save failed");
		return result;
	}

	result = automat.save(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Automat3 save failed");
		return result;
	}

	LOG_INFO(LOG_CFG, "Config converted from 1 to 3");
	return MemoryResult_Ok;
}

MemoryResult Config3Repairer::convert2() {
	LOG_DEBUG(LOG_CFG, "convert2");
	memory->setAddress(address);
	Config1Boot boot;
	MemoryResult result = boot.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Boot1 load failed");
		return result;
	}

	uint32_t start = memory->getAddress();
	ConfigFiscal1Converter fiscal;
	result = fiscal.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Fiscal1 load failed");
		return result;
	}

	Config2EventList events;
	result = events.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Events2 load failed");
		return result;
	}

	Config2AutomatConverter automat;
	result = automat.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Automat2 load failed");
		return result;
	}

	memory->setAddress(start);
	result = fiscal.save(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Fiscal2 save failed");
		return result;
	}

	result = events.save(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Events2 save failed");
		return result;
	}

	result = automat.save(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Automat3 save failed");
		return result;
	}

	LOG_INFO(LOG_CFG, "Config converted from 2 to 3");
	return MemoryResult_Ok;
}
