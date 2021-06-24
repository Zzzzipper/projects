#ifndef TEST_EEPROMRESOURCETEST_H_
#define TEST_EEPROMRESOURCETEST_H_

#include "common/i2c/I2C.h"
#include "common/memory/stm32/include/ExternalEeprom.h"
#include "common/timer/include/TimerEngine.h"

class EepromResourceTest {
public:
	EepromResourceTest(TimerEngine *timerEngine);
	~EepromResourceTest();
	void test();

private:
	TimerEngine *timerEngine;
	Timer *timer;
	StatStorage *stat;
	ExternalEeprom *memory;
	uint32_t address;
	uint32_t step;
	uint32_t maxTryNumber;
	uint32_t tryNumber;
	uint32_t t;
	uint8_t *data0;
	uint8_t *data1;
	uint8_t *buf;
	bool zero;

	void procTimer();
	bool writeBlock(uint8_t *data, uint32_t dataLen);
	bool checkBlock(uint8_t *data, uint32_t dataLen);
	void saveResult(uint8_t result);
	void printResult();
};

#endif
