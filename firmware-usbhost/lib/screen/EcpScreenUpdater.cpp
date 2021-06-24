#include "EcpScreenUpdater.h"

#include "timer/stm32/include/SystemTimer.h"
#include "logger/include/Logger.h"

EcpScreenUpdater::EcpScreenUpdater(Screen *screen) : screen(screen)
{
}

bool EcpScreenUpdater::getScreenStage(Screen::Reg24 &stage) {
	for(uint16_t i = 0; i < 20; i++) {
		if(screen->getApplicationStage(stage) == true) {
			return true;
		}
	}
	return false;
}

Dex::DataParser::Result EcpScreenUpdater::start(uint32_t dataSize) {
	LOG_INFO(LOG_BOOT, "start " << dataSize);
	if(screen == NULL) {
		LOG_ERROR(LOG_BOOT, "screen disabled");
		return Result_Error;
	}

	Screen::Reg24 stage;
	if(getScreenStage(stage) == false) {
		LOG_ERROR(LOG_BOOT, "getApplicationStage failed");
		return Result_Error;
	}

	if(stage == Screen::Reg24::Normal) {
		LOG_INFO(LOG_BOOT, "Normal restart");
		screen->restart();
		if(getScreenStage(stage) == false) {
			LOG_ERROR(LOG_BOOT, "getApplicationStage failed");
			return Result_Error;
		}

		if(stage == Screen::Reg24::Normal) {
			LOG_ERROR(LOG_BOOT, "wrong stage");
			return Result_Error;
		}
	}

	LOG_INFO(LOG_BOOT, "Firmware erase");
	if(screen->eraseFirmware() == false) {
		LOG_ERROR(LOG_BOOT, "eraseFirmware failed");
		return Result_Error;
	}

	LOG_INFO(LOG_BOOT, "Delay");
	for(uint16_t i = 0; i < 5; i++) {
		SystemTimer::get()->delay_ms(1000);
		IWDG_ReloadCounter();
	}

	this->delay = true;
	this->dataSize = dataSize;
	this->dataCount = 0;
	return Result_Ok;
}

Dex::DataParser::Result EcpScreenUpdater::procData(const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_BOOT, "recv " << dataLen << "/" << this->dataCount << " from " << this->dataSize);
//	if(delay == true) {
//		SystemTimer::get()->delay_ms(5000);
//		delay = false;
//	}
	if(screen->writeFirmware((uint8_t *)data, dataLen) == false) {
		LOG_ERROR(LOG_BOOT, "writeFirmware failed");
		return Result_Error;
	}

	this->dataCount += dataLen;
	return Result_Ok;
}

Dex::DataParser::Result EcpScreenUpdater::complete() {
	LOG_INFO(LOG_BOOT, "complete");
	Screen::Reg25 status;
	if(screen->getSoftwareStatus(status) == false) {
		LOG_ERROR(LOG_BOOT, "getSoftwareStatus failed");
		return Result_Error;
	}

	if(status != Screen::Reg25::Normal) {
		LOG_ERROR(LOG_BOOT, "wrong status " << (uint32_t)status);
		return Result_Error;
	}

// todo: checkFirmware
/*	memory.stop();
	config->setFirmwareState(ConfigBoot::FirmwareState_UpdateComplete);
	config->save();
	courier.deliver(DataParser::Event_AsyncOk);*/
	LOG_INFO(LOG_BOOT, "Screen firmware complete");
	return Result_Ok;
}

void EcpScreenUpdater::error() {
	LOG_INFO(LOG_BOOT, "error");
//	memory.stop();
//	courier.deliver(DataParser::Event_AsyncError);
}
