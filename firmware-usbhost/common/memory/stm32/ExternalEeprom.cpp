#include <stdio.h>
#include <string.h>

#include "cmsis_boot/stm32f4xx_conf.h"

#include "i2c/I2C.h"
#include "memory/stm32/include/ExternalEeprom.h"
#include "timer/stm32/include/SystemTimer.h"
#include "utils/include/StringBuilder.h"
#include "utils/include/Utils.h"
#include "mdb/MdbProtocol.h"
#include "logger/include/Logger.h"

#define I2C_MAX_WRITE_TIMEOUT_MS	10
#define I2C_OPERATION_TIMEOUT_MS	3000

ExternalEeprom::ExternalEeprom(I2C *i2c, uint16_t hwAddress, uint32_t maxSize, uint16_t pageSize) :
	i2c(i2c),
	hwAddress(hwAddress),
	maxSize(maxSize),
	pageSize(pageSize)
{
}

MemoryResult ExternalEeprom::read(void *pData, const uint32_t len) {
	uint8_t *tmp = (uint8_t *) pData;
	for(uint32_t offz = 0; offz < len; offz += pageSize) {
		if(!readImpl((uint8_t *) &(tmp[offz]), (len - offz >= pageSize) ? pageSize : len - offz)) {
			LOG_ERROR(LOG_EEPROM, "readImpl failed " << address << "," << len);
#ifdef PRODUCTION
			reboot();
#endif
			return MemoryResult_ReadError;
		}
	}

	return MemoryResult_Ok;
}

MemoryResult ExternalEeprom::write(const void *pData, const uint32_t len) {
	uint8_t *tmp = (uint8_t *) pData;
// TODO: ” микросхемы постранична€ запись, поэтому данные выравниваем по этим страницам и ждем 5 млс по окончании записи.
	IWDG_ReloadCounter();
	for(uint32_t offz = 0; offz < len;) {
		uint32_t step = pageSize - (address % pageSize);
		step = (len - offz >= step) ? step : len - offz;

		if(!writeImpl((const uint8_t *) &(tmp[offz]), step)) {
#ifdef PRODUCTION
			reboot();
#endif
			LOG_ERROR(LOG_EEPROM, "writeImpl failed " << address << "," << len);
			return MemoryResult_WriteError;
		}

		offz += step;
		IWDG_ReloadCounter(); // запись может длитьс€ очень долго
	}

	return MemoryResult_Ok;
}

void ExternalEeprom::logData(char *title, int startAddr, uint8_t *p, int len) {
	StringBuilder str;
	str << title << ", addr: " << startAddr << ", len: " << len << ", data: ";
	for(int i = 0; i < len; i++) {
		str << p[i] << " ";
	}
}

bool ExternalEeprom::readImpl(uint8_t *pData, const uint32_t len) {
	bool result = i2c->syncReadData(hwAddress, address, 2, pData, len, I2C_OPERATION_TIMEOUT_MS);
	if (result)
		address += len;

	return result;
}

bool ExternalEeprom::writeImpl(const uint8_t *pData, const uint32_t len) {

	bool result = i2c->syncWriteData(hwAddress, address, 2, (uint8_t *)pData, len, I2C_OPERATION_TIMEOUT_MS);
	if (result)
		address += len;

	i2c->syncWriteData(hwAddress, address, 0, 0, 0, I2C_MAX_WRITE_TIMEOUT_MS);

	return result;
}
