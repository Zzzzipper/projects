#include "common/modem/stm32/include/ModemConfigSaver.h"

#include "common/modem/include/ModemConfigParser.h"
#include "common/modem/include/ModemConfigGenerator.h"
#include "common/memory/stm32/include/FlashMemory.h"
#include "common/logger/include/Logger.h"

#include "../defines.h"

void ModemConfigSaver::save(ModemConfig *config) {
	LOG_DEBUG(LOG_MODEM, "save");
	ModemConfigGenerator generator(config);
	FlashMemory flash;
	if(flash.start(CONFIG_SECTOR_ADDRESS, CONFIG_SECTOR_SIZE) == false) {
		LOG_ERROR(LOG_MODEM, "Start write failed");
		return;
	}

	generator.reset();
	uint32_t dataLen = 0;
	while(generator.isLast() == false) {
		dataLen += generator.getLen();
		generator.next();
	}
	dataLen += generator.getLen();
	if(flash.write((uint8_t*)&dataLen, sizeof(dataLen)) == false) {
		LOG_ERROR(LOG_MODEM, "Length write failed");
		return;
	}

	generator.reset();
	while(generator.isLast() == false) {
		flash.write((uint8_t*)generator.getData(), generator.getLen());
		generator.next();
	}
	flash.write((uint8_t*)generator.getData(), generator.getLen());
	flash.stop();
}

void ModemConfigSaver::load(ModemConfig *config) {
	LOG_DEBUG(LOG_MODEM, "load " << getDataLen());
	ModemConfigParser parser(config);
	parser.start();
	parser.procData(getData(), getDataLen());
	if(parser.hasError() == true) {
		LOG_ERROR(LOG_MODEM, "Config load failed");
		config->setDefault();
	}
}

const uint8_t *ModemConfigSaver::getData() {
	uint32_t *src = (uint32_t*)(CONFIG_SECTOR_ADDRESS + 4);
	return (uint8_t*)src;
}

uint32_t ModemConfigSaver::getDataLen() {
	uint32_t *src = (uint32_t*)CONFIG_SECTOR_ADDRESS;
	uint32_t len = *src;
	return (len > CONFIG_SECTOR_SIZE) ? 0 : len;
}
