#include "test/include/Test.h"
#include "logger/include/Logger.h"

class BufferTest : public TestSet {
public:
	BufferTest();
	bool test();
};

TEST_SET_REGISTER(BufferTest);

BufferTest::BufferTest() {
	TEST_CASE_REGISTER(BufferTest, test);
}

bool BufferTest::test() {
	Buffer buf(10);

	buf.addUint8(0x00);
	buf.addUint8(0x01);
	buf.addUint8(0x02);
	buf.addUint8(0x03);
	buf.addUint8(0x04);
	TEST_HEXDATA_EQUAL("0001020304", buf.getData(), buf.getLen());

	uint8_t data[] = { 0x05, 0x06, 0x07, 0x08, 0x09 };
	buf.add(data, sizeof(data));
	TEST_HEXDATA_EQUAL("00010203040506070809", buf.getData(), buf.getLen());

	buf.add(data, sizeof(data));
	TEST_HEXDATA_EQUAL("00010203040506070809", buf.getData(), buf.getLen());
	TEST_NUMBER_EQUAL(10, buf.getLen());
	TEST_NUMBER_EQUAL(10, buf.getSize());
	buf.addUint8(0x0A);
	TEST_HEXDATA_EQUAL("00010203040506070809", buf.getData(), buf.getLen());
	TEST_NUMBER_EQUAL(10, buf.getLen());
	TEST_NUMBER_EQUAL(10, buf.getSize());

	buf.remove(2,3);
	TEST_HEXDATA_EQUAL("00010506070809", buf.getData(), buf.getLen());
	TEST_NUMBER_EQUAL(7, buf.getLen());
	TEST_NUMBER_EQUAL(10, buf.getSize());
	buf.remove(10,3);
	TEST_HEXDATA_EQUAL("00010506070809", buf.getData(), buf.getLen());
	TEST_NUMBER_EQUAL(7, buf.getLen());
	TEST_NUMBER_EQUAL(10, buf.getSize());
	buf.remove(2,10);
	TEST_HEXDATA_EQUAL("0001", buf.getData(), buf.getLen());
	TEST_NUMBER_EQUAL(2, buf.getLen());
	TEST_NUMBER_EQUAL(10, buf.getSize());

	return true;
}
