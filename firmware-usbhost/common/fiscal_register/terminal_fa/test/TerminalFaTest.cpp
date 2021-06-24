#include "fiscal_register/terminal_fa/TerminalFaProtocol.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class TerminalFaTest : public TestSet {
public:
	TerminalFaTest();
	bool testCrc();
};

TEST_SET_REGISTER(TerminalFaTest);

TerminalFaTest::TerminalFaTest() {
	TEST_CASE_REGISTER(TerminalFaTest, testCrc);
}

bool TerminalFaTest::testCrc() {
	TerminalFa::Crc crc;
	crc.start();
	crc.add(0x00);
	crc.add(0x05);
	crc.add(0x30);
	crc.add(0x01);
	crc.add(0x00);
	crc.add(0x00);
	crc.add(0x00);
	TEST_NUMBER_EQUAL(0xC8, crc.getHighByte());
	TEST_NUMBER_EQUAL(0x95, crc.getLowByte());
	return true;
}
