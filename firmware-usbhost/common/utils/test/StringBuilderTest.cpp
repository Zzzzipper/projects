#include "utils/include/StringBuilder.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class StringBuilderTest : public TestSet {
public:
	StringBuilderTest();
	bool testResize();
	bool testFixedSize();
	bool testStaticBuffer();
	bool testStreamMethods();
	bool testSetLen();
};

TEST_SET_REGISTER(StringBuilderTest);

StringBuilderTest::StringBuilderTest() {
	TEST_CASE_REGISTER(StringBuilderTest, testResize);
	TEST_CASE_REGISTER(StringBuilderTest, testFixedSize);
	TEST_CASE_REGISTER(StringBuilderTest, testStaticBuffer);
	TEST_CASE_REGISTER(StringBuilderTest, testStreamMethods);
	TEST_CASE_REGISTER(StringBuilderTest, testSetLen);
}

bool StringBuilderTest::testResize() {
	StringBuilder str1;
	str1.set("0123456789");
	TEST_NUMBER_EQUAL(10, str1.getLen());
	TEST_NUMBER_EQUAL(16, str1.getSize());
	TEST_STRING_EQUAL("0123456789", str1.getString());

	StringBuilder str2;
	str2.set("0123456789ABCDEF01234567");
	TEST_NUMBER_EQUAL(24, str2.getLen());
	TEST_NUMBER_EQUAL(24, str2.getSize());
	TEST_STRING_EQUAL("0123456789ABCDEF01234567", str2.getString());

	str2.addStr("GHIJKL");
	TEST_NUMBER_EQUAL(30, str2.getLen());
	TEST_NUMBER_EQUAL(40, str2.getSize());
	TEST_STRING_EQUAL("0123456789ABCDEF01234567GHIJKL", str2.getString());

	StringBuilder str3("0123456789");
	TEST_NUMBER_EQUAL(10, str3.getLen());
	TEST_NUMBER_EQUAL(10, str3.getSize());
	TEST_STRING_EQUAL("0123456789", str3.getString());

	StringBuilder str4("FEDCBA9876543210", 6);
	TEST_NUMBER_EQUAL(6, str4.getLen());
	TEST_NUMBER_EQUAL(6, str4.getSize());
	TEST_STRING_EQUAL("FEDCBA", str4.getString());

	return true;
}

bool StringBuilderTest::testFixedSize() {
	StringBuilder str1(5, 10);
	str1 = "0123456789ABCDEF";
	TEST_NUMBER_EQUAL(10, str1.getLen());
	TEST_NUMBER_EQUAL(10, str1.getSize());
	TEST_STRING_EQUAL("0123456789", str1.getString());
	return true;
}

bool StringBuilderTest::testStaticBuffer() {
	uint8_t buf[11];
	StringBuilder str1(sizeof(buf), buf);
	str1 = "0123456789ABCDEF";
	TEST_NUMBER_EQUAL(10, str1.getLen());
	TEST_NUMBER_EQUAL(10, str1.getSize());
	TEST_STRING_EQUAL("0123456789", str1.getString());
	return true;
}

bool StringBuilderTest::testStreamMethods() {
	StringBuilder str1;

	str1 << "ABCDE";
	TEST_NUMBER_EQUAL(5, str1.getLen());
	TEST_NUMBER_EQUAL(16, str1.getSize());
	TEST_STRING_EQUAL("ABCDE", str1.getString());

	str1 << 'F';
	TEST_NUMBER_EQUAL(6, str1.getLen());
	TEST_NUMBER_EQUAL(16, str1.getSize());
	TEST_STRING_EQUAL("ABCDEF", str1.getString());

	str1 << (uint16_t)0;
	TEST_NUMBER_EQUAL(7, str1.getLen());
	TEST_NUMBER_EQUAL(16, str1.getSize());
	TEST_STRING_EQUAL("ABCDEF0", str1.getString());

	str1 << (uint16_t)12;
	TEST_NUMBER_EQUAL(9, str1.getLen());
	TEST_NUMBER_EQUAL(16, str1.getSize());
	TEST_STRING_EQUAL("ABCDEF012", str1.getString());

	str1 << (uint32_t)34;
	TEST_NUMBER_EQUAL(11, str1.getLen());
	TEST_NUMBER_EQUAL(16, str1.getSize());
	TEST_STRING_EQUAL("ABCDEF01234", str1.getString());

	str1 << (int)56;
	TEST_NUMBER_EQUAL(13, str1.getLen());
	TEST_NUMBER_EQUAL(16, str1.getSize());
	TEST_STRING_EQUAL("ABCDEF0123456", str1.getString());

	str1 << (long)789;
	TEST_NUMBER_EQUAL(16, str1.getLen());
	TEST_NUMBER_EQUAL(16, str1.getSize());
	TEST_STRING_EQUAL("ABCDEF0123456789", str1.getString());

	StringBuilder str2("GHIJKLMNOPQRSTUVWXYZ");
	str1 << str2;
	TEST_NUMBER_EQUAL(36, str1.getLen());
	TEST_NUMBER_EQUAL(48, str1.getSize());
	TEST_STRING_EQUAL("ABCDEF0123456789GHIJKLMNOPQRSTUVWXYZ", str1.getString());

	return true;
}

bool StringBuilderTest::testSetLen() {
	StringBuilder str1(5, 10);
	str1 = "01234";
	str1.setLen(7);
	TEST_NUMBER_EQUAL(5, str1.getLen());
	TEST_NUMBER_EQUAL(5, str1.getSize());
	TEST_STRING_EQUAL("01234", str1.getString());

	strcpy((char*)str1.getData(), "ABC");
	str1.setLen(3);
	TEST_NUMBER_EQUAL(3, str1.getLen());
	TEST_NUMBER_EQUAL(5, str1.getSize());
	TEST_STRING_EQUAL("ABC", str1.getString());

	return true;
}
