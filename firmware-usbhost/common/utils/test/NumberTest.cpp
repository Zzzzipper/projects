#include "utils/include/Number.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class NumberTest : public TestSet {
public:
	NumberTest();
	bool test();
};

TEST_SET_REGISTER(NumberTest);

NumberTest::NumberTest() {
	TEST_CASE_REGISTER(NumberTest, test);
}

bool NumberTest::test() {
	const char d1[] = "20190106T1804";
	uint32_t n1;
	TEST_NUMBER_EQUAL(4, Sambery::stringToNumber<uint32_t>(d1, sizeof(d1), 4, &n1));
	TEST_NUMBER_EQUAL(2019, n1);

	const char d2[] = "06T1804";
	uint16_t n2;
	TEST_NUMBER_EQUAL(2, Sambery::stringToNumber<uint16_t>(d2, sizeof(d2), 4, &n2));
	TEST_NUMBER_EQUAL(6, n2);

	const char d3[] = "106T1804";
	uint16_t n3;
	TEST_NUMBER_EQUAL(3, Sambery::stringToNumber<uint16_t>(d3, sizeof(d3), &n3));
	TEST_NUMBER_EQUAL(106, n3);

	const char d4[] = "ABCDEF";
	uint16_t n4;
	TEST_NUMBER_EQUAL(0, Sambery::stringToNumber<uint16_t>(d4, sizeof(d4), &n4));
	TEST_NUMBER_EQUAL(0, n4);
	return true;
}
