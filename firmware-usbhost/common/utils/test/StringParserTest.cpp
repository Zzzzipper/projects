#include "utils/include/StringParser.h"
#include "utils/include/Buffer.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

#include <string.h>

class StringParserTest : public TestSet {
public:
	StringParserTest();
	bool testParse();
	bool testNumberInStack();
	bool testEscapeSymbol();
	bool testGetNumberLimit();
};

TEST_SET_REGISTER(StringParserTest);

StringParserTest::StringParserTest() {
	TEST_CASE_REGISTER(StringParserTest, testParse);
	TEST_CASE_REGISTER(StringParserTest, testNumberInStack);
	TEST_CASE_REGISTER(StringParserTest, testEscapeSymbol);
	TEST_CASE_REGISTER(StringParserTest, testGetNumberLimit);
}

bool StringParserTest::testParse() {
	const char *s1 = "Content-Range: bytes 0-256/1024";
	StringParser p1(s1, strlen(s1));
	uint32_t value;

	TEST_NUMBER_EQUAL(true, p1.compareAndSkip("Content-Range:"));
	TEST_SUBSTR_EQUAL(" bytes 0-256/1024", p1.unparsed(), p1.unparsedLen());

	p1.skipEqual(" \t");
	TEST_SUBSTR_EQUAL("bytes 0-256/1024", p1.unparsed(), p1.unparsedLen());

	TEST_NUMBER_EQUAL(true, p1.compareAndSkip("bytes"));
	TEST_SUBSTR_EQUAL(" 0-256/1024", p1.unparsed(), p1.unparsedLen());

	p1.skipEqual(" \t");
	TEST_SUBSTR_EQUAL("0-256/1024", p1.unparsed(), p1.unparsedLen());

	TEST_NUMBER_EQUAL(true, p1.getNumber(&value));
	TEST_NUMBER_EQUAL(0, value);
	TEST_SUBSTR_EQUAL("-256/1024", p1.unparsed(), p1.unparsedLen());

	TEST_NUMBER_EQUAL(true, p1.compareAndSkip("-"));
	TEST_SUBSTR_EQUAL("256/1024", p1.unparsed(), p1.unparsedLen());

	TEST_NUMBER_EQUAL(true, p1.getNumber(&value));
	TEST_NUMBER_EQUAL(256, value);
	TEST_SUBSTR_EQUAL("/1024", p1.unparsed(), p1.unparsedLen());

	TEST_NUMBER_EQUAL(true, p1.compareAndSkip("/"));
	TEST_SUBSTR_EQUAL("1024", p1.unparsed(), p1.unparsedLen());

	TEST_NUMBER_EQUAL(true, p1.getNumber(&value));
	TEST_NUMBER_EQUAL(1024, value);
	TEST_SUBSTR_EQUAL("", p1.unparsed(), p1.unparsedLen());

	return true;
}

bool StringParserTest::testNumberInStack() {
	const char *s1 = "12345678";
	StringParser p1(s1, 4);
	uint32_t value;

	TEST_NUMBER_EQUAL(true, p1.getNumber(&value));
	TEST_NUMBER_EQUAL(1234, value);
	TEST_SUBSTR_EQUAL("", p1.unparsed(), p1.unparsedLen());

	return true;
}

bool StringParserTest::testEscapeSymbol() {
	const char *s1 = "abc\\\"def\"ghi";
	StringParser p1(s1);

	p1.skipNotEqual("\"", '\\');
	TEST_SUBSTR_EQUAL("\"ghi", p1.unparsed(), p1.unparsedLen());
	return true;
}

bool StringParserTest::testGetNumberLimit() {
	const char *s1 = "20190601T1647";
	StringParser p1(s1);
	uint32_t n1;
	TEST_NUMBER_EQUAL(true, p1.getNumber(4, &n1));
	TEST_NUMBER_EQUAL(2019, n1);
	uint16_t n2;
	TEST_NUMBER_EQUAL(true, p1.getNumber(2, &n2));
	TEST_NUMBER_EQUAL(06, n2);
	uint8_t n3;
	TEST_NUMBER_EQUAL(false, p1.getNumber(4, &n3));
	TEST_NUMBER_EQUAL(1, n3);
	return true;
}
