#include "mdb/MdbCoinChangerPollParser.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class MdbCoinChangerPollParserTest : public TestSet {
public:
	MdbCoinChangerPollParserTest();
	bool test();
};

TEST_SET_REGISTER(MdbCoinChangerPollParserTest);

MdbCoinChangerPollParserTest::MdbCoinChangerPollParserTest() {
	TEST_CASE_REGISTER(MdbCoinChangerPollParserTest, test);
}

bool MdbCoinChangerPollParserTest::test() {
//	uint8_t data1[] = { 0x13, 0x00, 0x00, 0x00, 0x00, 0x01 };
//	TEST_NUMBER_EQUAL(0x14, Mdb::calcCrc(data1, sizeof(data1)));
	return true;
}
