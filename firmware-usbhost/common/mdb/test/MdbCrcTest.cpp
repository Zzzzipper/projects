#include "mdb/MdbProtocol.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class MdbCrcTest : public TestSet {
public:
	MdbCrcTest();
	bool test();
};

TEST_SET_REGISTER(MdbCrcTest);

MdbCrcTest::MdbCrcTest() {
	TEST_CASE_REGISTER(MdbCrcTest, test);
}

bool MdbCrcTest::test() {
	uint8_t data1[] = { 0x13, 0x00, 0x00, 0x00, 0x00, 0x01 };
	TEST_NUMBER_EQUAL(0x14, Mdb::calcCrc(data1, sizeof(data1)));
	return true;
}
