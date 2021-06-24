#include "EepromResourceTest.h"

#include "common/logger/include/Logger.h"

#include <memory.h>

#define TEST_BLOCK_SIZE 256

EepromResourceTest::EepromResourceTest(TimerEngine *timerEngine) :
	timerEngine(timerEngine)
{
	timer = timerEngine->addTimer<EepromResourceTest, &EepromResourceTest::procTimer>(this);
	stat = new StatStorage;
	I2C *i2c = I2C::get(I2C_3);
	i2c->setStatStorage(stat);
	memory = new ExternalEeprom(i2c, EE_ADDRESS, EE_MAX_SIZE, EE_PAGE_SIZE);
	data0 = new uint8_t[TEST_BLOCK_SIZE];
	memset(data0, 0, TEST_BLOCK_SIZE);
	data1 = new uint8_t[TEST_BLOCK_SIZE];
	memset(data1, 0xFF, TEST_BLOCK_SIZE);
	buf = new uint8_t[TEST_BLOCK_SIZE];
}

EepromResourceTest::~EepromResourceTest() {
	delete buf;
	delete data1;
	delete data0;
	delete memory;
	delete stat;
	timerEngine->deleteTimer(timer);
}

void EepromResourceTest::test() {
	printResult();
	address = 31*1024;
	step = 64;
	maxTryNumber = 1000*1000*1000;
//	maxTryNumber = 1000;
	tryNumber = 0;
	t = 0;
	zero = false;
	timer->start(1);
	LOG("Warning! This test will destroy EEPROM chip " << maxTryNumber);
}

void EepromResourceTest::procTimer() {
	if(zero == true) {
		zero = false;
		if(writeBlock(data0, TEST_BLOCK_SIZE) == false) {
			LOG("error=" << t);
			saveResult(1);
			return;
		}
		if(checkBlock(data0, TEST_BLOCK_SIZE) == false) {
			LOG("error=" << t);
			saveResult(2);
			return;
		}
	} else {
		zero = true;
		if(writeBlock(data1, TEST_BLOCK_SIZE) == false) {
			LOG("error=" << t);
			saveResult(3);
			return;
		}
		if(checkBlock(data1, TEST_BLOCK_SIZE) == false) {
			LOG("error=" << t);
			saveResult(4);
			return;
		}
	}

	t++;
	if(t >= maxTryNumber) {
		LOG("complete=" << t);
		saveResult(5);
		return;
	}

	timer->start(1);
	tryNumber++;
	if(tryNumber >= 100) {
		LOG("try=" << t);
		tryNumber = 0;
	}
}

bool EepromResourceTest::writeBlock(uint8_t *data, uint32_t dataLen) {
	memory->setAddress(address);
	for(uint32_t i = 0; i < dataLen; i += step) {
//		*(Logger::get()) << "-";
		if(memory->write(data, step) != MemoryResult::MemoryResult_Ok) {
			LOG("Write error " << (address + i));
			return false;
		}
	}
	return true;
}

bool EepromResourceTest::checkBlock(uint8_t *data, uint32_t dataLen) {
//	*(Logger::get()) << "+";
	memory->setAddress(address);
	if(memory->read(buf, dataLen) != MemoryResult::MemoryResult_Ok) {
		LOG("Read error " << address);
		return false;
	}
	for(uint32_t i = 0; i < dataLen; i++) {
		if(data[i] != buf[i]) {
			LOG("Wrong byte " << i << "," << data[i] << "<>" << buf[i]);
			return false;
		}
	}
	return true;
}

#pragma pack(push,1)
struct ResultData {
	uint8_t result;
	uint32_t tryNumber;
	uint32_t address;
	uint32_t blockSize;
	uint32_t step;
};
#pragma pack(pop)

void EepromResourceTest::saveResult(uint8_t result) {
	ResultData r;
	r.result = result;
	r.tryNumber = t;
	r.address = address;
	r.blockSize = TEST_BLOCK_SIZE;
	r.step = step;
	memory->setAddress(0);
	memory->write((uint8_t*)(&r), sizeof(r));
}

void EepromResourceTest::printResult() {
	ResultData r;
	memory->setAddress(0);
	memory->read((uint8_t*)(&r), sizeof(r));
	LOG(">>>>>>>PREV:");
	LOG("result=" << r.result);
	LOG("tryNumber=" << r.tryNumber);
	LOG("address=" << r.address);
	LOG("blockSize=" << r.blockSize);
	LOG("step=" << r.step);
}
