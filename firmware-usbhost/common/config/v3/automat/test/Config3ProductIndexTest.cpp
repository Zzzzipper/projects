#include "memory/include/RamMemory.h"
#include "config/v3/automat/Config3ProductIndex.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class Config3ProductIndexTest : public TestSet {
public:
	Config3ProductIndexTest();
	bool testGetIndex();
	bool testSaveLoad();
};

TEST_SET_REGISTER(Config3ProductIndexTest);

Config3ProductIndexTest::Config3ProductIndexTest() {
	TEST_CASE_REGISTER(Config3ProductIndexTest, testGetIndex);
	TEST_CASE_REGISTER(Config3ProductIndexTest, testSaveLoad);
}

bool Config3ProductIndexTest::testGetIndex() {
	RamMemory memory(32000);
	Config3ProductIndexList list;

	TEST_NUMBER_EQUAL(true, list.add("0", 0));
	TEST_NUMBER_EQUAL(true, list.add("001", 1));
	TEST_NUMBER_EQUAL(true, list.add("123", 123));
	TEST_NUMBER_EQUAL(true, list.add("4", 4));
	TEST_NUMBER_EQUAL(4, list.getSize());

	TEST_NUMBER_EQUAL(0, list.getIndex("0"));
	TEST_NUMBER_EQUAL(1, list.getIndex("001"));
	TEST_NUMBER_EQUAL(2, list.getIndex("123"));
	TEST_NUMBER_EQUAL(3, list.getIndex("4"));

	TEST_NUMBER_EQUAL(0, list.getIndex((uint16_t)0));
	TEST_NUMBER_EQUAL(1, list.getIndex(1));
	TEST_NUMBER_EQUAL(2, list.getIndex(123));

	TEST_STRING_EQUAL("4", list.get(3)->selectId.get());
	TEST_NUMBER_EQUAL(4, list.get(3)->cashlessId);
	return true;
}

bool Config3ProductIndexTest::testSaveLoad() {
	RamMemory memory(32000);
	Config3ProductIndexList list1;
	TEST_NUMBER_EQUAL(true, list1.add("01", 0));
	TEST_NUMBER_EQUAL(true, list1.add("02", 1));
	TEST_NUMBER_EQUAL(true, list1.add("03", 2));
	TEST_NUMBER_EQUAL(true, list1.add("04", 3));
	list1.init(&memory);

	Config3ProductIndexList list2;
	memory.setAddress(0);
	list2.load(4, &memory);
	TEST_NUMBER_EQUAL(4, list2.getSize());

	memory.setAddress(0);
	list2.load(4, &memory);
	TEST_NUMBER_EQUAL(4, list2.getSize());
	return true;
}
