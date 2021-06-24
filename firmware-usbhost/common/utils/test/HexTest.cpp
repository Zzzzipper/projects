#include "utils/include/Hex.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class HexTest : public TestSet {
public:
	HexTest();
	bool test();
	bool testStaticBuffer();
};

TEST_SET_REGISTER(HexTest);

HexTest::HexTest() {
	TEST_CASE_REGISTER(HexTest, test);
}

bool HexTest::test() {
//uint16_t hexToData(const char *hex, uint16_t hexLen, uint8_t *buf, uint16_t bufSize) {
	const char hex1[] = "2086846a9601";
	uint8_t buf1[6];
	TEST_NUMBER_EQUAL(6, hexToData(hex1, strlen(hex1), buf1, sizeof(buf1)));
	TEST_NUMBER_EQUAL(0, hexToData(hex1, strlen(hex1), buf1, 5));
	return true;
}
