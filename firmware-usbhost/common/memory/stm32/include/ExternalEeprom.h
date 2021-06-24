#ifndef __EXTERNAL_EEPROM_H
#define __EXTERNAL_EEPROM_H

/*
	Модуль внешней памяти. Использует модуль I2C.
	Режим работы: синхронный.
*/

#include "common/memory/include/Memory.h"

#define EE_24LC64_ADDRESS			0xA0
#define EE_24LC64_MAX_SIZE			8192
#define EE_24LC64_PAGE_SIZE			32

#define EE_24LC256_ADDRESS 			0xA0
#define EE_24LC256_MAX_SIZE 		(32*1024)
#define EE_24LC256_PAGE_SIZE 		64

#define EE_24LC512_ADDRESS 			0xA0
#define EE_24LC512_MAX_SIZE 		(64*1024)
#define EE_24LC512_PAGE_SIZE 		128

#define EE_M24M01_ADDRESS 			0xA0
#define EE_M24M01_MAX_SIZE 			(128*1024)
#define EE_M24M01_PAGE_SIZE 		256

class I2C;

class ExternalEeprom : public Memory {
public:
	ExternalEeprom(I2C *i2c, uint16_t hwAddress, uint32_t maxSize, uint16_t pageSize);

	virtual MemoryResult write(const void *pData, const uint32_t len) override;
	virtual MemoryResult read(void *pData, const uint32_t len) override;
	virtual uint32_t getMaxSize() const override { return maxSize; }
	virtual uint32_t getPageSize() const override { return pageSize; }

private:
	I2C *i2c;
	uint16_t hwAddress;
	uint32_t maxSize;
	uint16_t pageSize;

	bool readImpl(uint8_t *pData, const uint32_t len);
	bool writeImpl(const uint8_t *pData, const uint32_t len);

	void logData(char *title, int startAddr, uint8_t *p, int len);
};

#endif
