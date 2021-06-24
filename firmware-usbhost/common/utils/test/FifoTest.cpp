#include "utils/include/Fifo.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class FifoTest : public TestSet {
public:
	FifoTest();
	bool test();
	bool testStaticBuffer();
};

TEST_SET_REGISTER(FifoTest);

FifoTest::FifoTest() {
	TEST_CASE_REGISTER(FifoTest, test);
	TEST_CASE_REGISTER(FifoTest, testStaticBuffer);
}

bool FifoTest::test() {
	const int max = 128;
	Fifo<char> fifo(max);

	int i = 0;
	for(i = 0; i < max; i++) {
		TEST_NUMBER_EQUAL(true, fifo.push(i));
	}

	for(; i < 256; i++) {
		TEST_NUMBER_EQUAL(false, fifo.push(i));
	}

	for(int j = 0; j < max; j++) {
		TEST_NUMBER_EQUAL(j, fifo.pop());
	}

	return true;
}

bool FifoTest::testStaticBuffer() {
	uint8_t buf[32];
	Fifo<char> fifo(sizeof(buf), buf);
	for(uint16_t i = 0; i < 32; i++) {
		TEST_NUMBER_EQUAL(true, fifo.push(i));
	}
	TEST_NUMBER_EQUAL(32, fifo.getSize());
	return true;
}
