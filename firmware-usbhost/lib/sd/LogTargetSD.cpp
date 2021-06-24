#include "LogTargetSD.h"
#include "SD.h"

#include "common/logger/include/Logger.h"

#include "extern/ub_fatfs/stm32_ub_fatfs.h"

LogTargetSD::LogTargetSD() : file(NULL) {
	SD::get(); // init SD
	init();
}

LogTargetSD::~LogTargetSD() {
	shutdown();
}

bool LogTargetSD::init() {
	if(file != NULL) {
		LOG_ERROR(LOG_SD, "Already inited");
		return false;
	}

	file = new FIL;
	if(file == NULL) {
		LOG_ERROR(LOG_SD, "Alloc memory failed");
		file = NULL;
		return false;
	}

	if(UB_Fatfs_CheckMedia(MMC_0) != FATFS_OK) {
		LOG_ERROR(LOG_SD, "SD not found");
		file = NULL;
		return false;
	}

	if(UB_Fatfs_Mount(MMC_0) != FATFS_OK) {
		LOG_ERROR(LOG_SD, "Mount failed");
		file = NULL;
		return false;
	}

	if(UB_Fatfs_OpenFile((FIL*)file, "0:/test.txt", F_WR_NEW) != FATFS_OK) {
		LOG_ERROR(LOG_SD, "File 0:/test.txt open failed");
		UB_Fatfs_UnMount(MMC_0);
		return false;
	}

	c = 0;
	LOG_ERROR(LOG_SD, "SD init succeed");
	return true;
}

void LogTargetSD::shutdown() {
	if(file == NULL) { return; }
	UB_Fatfs_CloseFile((FIL*)file);
	UB_Fatfs_UnMount(MMC_0);
	delete (FIL*)file;
	file = NULL;
}

SDTransferState SD_GetStatus(void);

void LogTargetSD::send(const uint8_t *data, const uint16_t len) {
	if(file == NULL) {
		return;
	}

	uint32_t writedLen = 0;
	FATFS_t result = UB_Fatfs_WriteBlock((FIL*)file, (unsigned char*)data, len, &writedLen);
	if(result != FATFS_OK) {
		return;
	}

	c++;
	if(c == 500) {
		c = 0;
		f_sync((FIL*)file);
	}
}
