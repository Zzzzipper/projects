#include "utils/include/NetworkProtocol.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class NetworkProtocolTest : public TestSet {
public:
	NetworkProtocolTest();
	bool testBinParam();
	bool testLEUint();
	bool testLEUnum();
};

TEST_SET_REGISTER(NetworkProtocolTest);

NetworkProtocolTest::NetworkProtocolTest() {
	TEST_CASE_REGISTER(NetworkProtocolTest, testBinParam);
	TEST_CASE_REGISTER(NetworkProtocolTest, testLEUint);
	TEST_CASE_REGISTER(NetworkProtocolTest, testLEUnum);
}

bool NetworkProtocolTest::testBinParam() {
	BinParam<uint8_t, 10> bin1;

	uint8_t data1[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C' };
	bin1.set(data1, sizeof(data1));
	TEST_NUMBER_EQUAL(10, bin1.getSize());
	TEST_NUMBER_EQUAL(10, bin1.getLen());
	TEST_HEXDATA_EQUAL("30313233343536373839", bin1.getData(), bin1.getLen());

	uint8_t data2[] = { 'A', 'B', 'C', 'D', 'E' };
	bin1.set(data2, sizeof(data2));
	TEST_NUMBER_EQUAL(10, bin1.getSize());
	TEST_NUMBER_EQUAL(5, bin1.getLen());
	TEST_HEXDATA_EQUAL("4142434445", bin1.getData(), bin1.getLen());
	return true;
}

bool NetworkProtocolTest::testLEUint() {
	Ubcd1 bcd1;
	bcd1.set(45);
	TEST_NUMBER_EQUAL(0x45, bcd1.value[0]);
	TEST_NUMBER_EQUAL(45, bcd1.get());

	LEUbcd2 bcd2;
	bcd2.set(5678);
	TEST_NUMBER_EQUAL(0x56, bcd2.value[0]);
	TEST_NUMBER_EQUAL(0x78, bcd2.value[1]);
	TEST_NUMBER_EQUAL(5678, bcd2.get());

	LEUbcd4 bcd4;
	bcd4.set(12345678);
	TEST_NUMBER_EQUAL(0x12, bcd4.value[0]);
	TEST_NUMBER_EQUAL(0x34, bcd4.value[1]);
	TEST_NUMBER_EQUAL(0x56, bcd4.value[2]);
	TEST_NUMBER_EQUAL(0x78, bcd4.value[3]);
	TEST_NUMBER_EQUAL(12345678, bcd4.get());

	return true;
}

bool NetworkProtocolTest::testLEUnum() {
	LEUnum1 bcd1;
	TEST_NUMBER_EQUAL(0x30, bcd1.value[0]);
	TEST_NUMBER_EQUAL(0, bcd1.get());
	bcd1.set(60);
	TEST_NUMBER_EQUAL(0x6C, bcd1.value[0]);
	TEST_NUMBER_EQUAL(60, bcd1.get());

	LEUnum2 bcd2;
	TEST_NUMBER_EQUAL(0x30, bcd2.value[0]);
	TEST_NUMBER_EQUAL(0x30, bcd2.value[1]);
	TEST_NUMBER_EQUAL(0, bcd2.get());
	bcd2.set(57);
	TEST_NUMBER_EQUAL(0x35, bcd2.value[0]);
	TEST_NUMBER_EQUAL(0x37, bcd2.value[1]);
	TEST_NUMBER_EQUAL(57, bcd2.get());

	LEUnum3 bcd3;
	TEST_NUMBER_EQUAL(0x30, bcd3.value[0]);
	TEST_NUMBER_EQUAL(0x30, bcd3.value[1]);
	TEST_NUMBER_EQUAL(0x30, bcd3.value[2]);
	TEST_NUMBER_EQUAL(0, bcd3.get());
	bcd3.set(345);
	TEST_NUMBER_EQUAL(0x33, bcd3.value[0]);
	TEST_NUMBER_EQUAL(0x34, bcd3.value[1]);
	TEST_NUMBER_EQUAL(0x35, bcd3.value[2]);
	TEST_NUMBER_EQUAL(345, bcd3.get());
	return true;
}
