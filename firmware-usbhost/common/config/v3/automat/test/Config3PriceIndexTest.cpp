#include "memory/include/RamMemory.h"
#include "config/v3/automat/Config3PriceIndex.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class Config3PriceIndexTest : public TestSet {
public:
	Config3PriceIndexTest();
	bool testSaveLoad();
	bool testDiscounts();
};

TEST_SET_REGISTER(Config3PriceIndexTest);

Config3PriceIndexTest::Config3PriceIndexTest() {
	TEST_CASE_REGISTER(Config3PriceIndexTest, testSaveLoad);
	TEST_CASE_REGISTER(Config3PriceIndexTest, testDiscounts);
}

bool Config3PriceIndexTest::testSaveLoad() {
	RamMemory memory(32000);
	Config3PriceIndexList list1;
	TEST_NUMBER_EQUAL(true, list1.add("CA", 0, Config3PriceIndexType_Base));
	TEST_NUMBER_EQUAL(true, list1.add("DA", 1, Config3PriceIndexType_Base));
	TEST_NUMBER_EQUAL(true, list1.add("DA", 2, Config3PriceIndexType_Base));
	TEST_NUMBER_EQUAL(true, list1.add("DA", 3, Config3PriceIndexType_Base));
	list1.init(&memory);

	Config3PriceIndexList list2;
	memory.setAddress(0);
	list2.load(4, &memory);
	TEST_NUMBER_EQUAL(4, list2.getSize());

	memory.setAddress(0);
	list2.load(4, &memory);
	TEST_NUMBER_EQUAL(4, list2.getSize());
	return true;
}

bool Config3PriceIndexTest::testDiscounts() {
	RamMemory memory(32000);
	TimeTable tt1;
	tt1.setWeekDay(WeekDay_Monday, true);
	tt1.setWeekDay(WeekDay_Tuesday, true);
	tt1.setInterval(10, 30, 0, 5*3600);
	Config3PriceIndexList list1;
	TEST_NUMBER_EQUAL(true, list1.add("FA", 0, Config3PriceIndexType_Base));
	TEST_NUMBER_EQUAL(true, list1.add("CA", 0, Config3PriceIndexType_None));
	TEST_NUMBER_EQUAL(true, list1.add("CA", 1, &tt1));
	TEST_NUMBER_EQUAL(true, list1.add("CA", 2, Config3PriceIndexType_Base));
	TEST_NUMBER_EQUAL(true, list1.add("DA", 1, Config3PriceIndexType_None));
	TEST_NUMBER_EQUAL(true, list1.add("DA", 2, &tt1));
	TEST_NUMBER_EQUAL(true, list1.add("DA", 3, Config3PriceIndexType_None));
	list1.init(&memory);

	Config3PriceIndexList list2;
	memory.setAddress(0);
	list2.load(list1.getSize(), &memory);
	TEST_NUMBER_EQUAL(list1.getSize(), list2.getSize());

	memory.setAddress(0);
	list2.load(list1.getSize(), &memory);
	TEST_NUMBER_EQUAL(list1.getSize(), list2.getSize());

	DateTime dt1(18, 05, 21, 10, 0, 0);
	TEST_NUMBER_EQUAL(3, list2.getIndexByDateTime("CA", &dt1));
	TEST_NUMBER_EQUAL(CONFIG_INDEX_UNDEFINED, list2.getIndexByDateTime("DA", &dt1));
	DateTime dt2(18, 05, 21, 12, 0, 0);
	TEST_NUMBER_EQUAL(2, list2.getIndexByDateTime("CA", &dt2));
	TEST_NUMBER_EQUAL(5, list2.getIndexByDateTime("DA", &dt2));
	return true;
}
