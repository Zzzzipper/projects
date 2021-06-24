#include "common/cardreader/sberbank/BluetoothCrc.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class BluetoothCrcTest : public TestSet {
public:
	BluetoothCrcTest();
	bool test();
};

TEST_SET_REGISTER(BluetoothCrcTest);

BluetoothCrcTest::BluetoothCrcTest() {
	TEST_CASE_REGISTER(BluetoothCrcTest, test);
}

bool BluetoothCrcTest::test() {
	BluetoothCrc crc1;
	crc1.start();
	crc1.add(0x00);
	crc1.add(0x07);
	crc1.add(0x50);
	crc1.add(0x00);
	crc1.add(0x00);
	crc1.add(0xe4);
	crc1.add(0xdf);
	crc1.add(0x03);
	crc1.add(0x00);
	TEST_NUMBER_EQUAL(0x6710, crc1.getCrc());
	TEST_NUMBER_EQUAL(0x67, crc1.getHighByte());
	TEST_NUMBER_EQUAL(0x10, crc1.getLowByte());

	BluetoothCrc crc2;
	crc2.start();
	crc2.add(0x00);
	crc2.add(0x13);
	crc2.add(0xc0);
	crc2.add(0x0c);
	crc2.add(0x00);
	crc2.add(0x1d);
	crc2.add(0x44);
	crc2.add(0x0a);
	crc2.add(0x00);
	crc2.add(0xd3);
	crc2.add(0x00);
	crc2.add(0x00);
	crc2.add(0x00);
	crc2.add(0x00);
	crc2.add(0x00);
	crc2.add(0x00);
	crc2.add(0x00);
	crc2.add(0x01);
	crc2.add(0x00);
	crc2.add(0x00);
	crc2.add(0x00);
	TEST_NUMBER_EQUAL(0xd5db, crc2.getCrc());
	TEST_NUMBER_EQUAL(0xd5, crc2.getHighByte());
	TEST_NUMBER_EQUAL(0xdb, crc2.getLowByte());
	return true;
}
