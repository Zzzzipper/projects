#include "config/v2/event/Config2EventList.h"
#include "config/v3/event/Config3EventList.h"
#include "memory/include/RamMemory.h"
#include "timer/include/TestRealTime.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class Config2ToConfig3EventListTest : public TestSet {
public:
	Config2ToConfig3EventListTest();
	bool testConvert1();
	bool testCopy2();
};

TEST_SET_REGISTER(Config2ToConfig3EventListTest);

Config2ToConfig3EventListTest::Config2ToConfig3EventListTest() {
	TEST_CASE_REGISTER(Config2ToConfig3EventListTest, testConvert1);
	TEST_CASE_REGISTER(Config2ToConfig3EventListTest, testCopy2);
}

bool Config2ToConfig3EventListTest::testConvert1() {
	RamMemory memory(32000);
	TestRealTime realtime;

	// init
	Config3EventList events1(&realtime);
	events1.init(10, &memory);
	events1.add(Config3Event::Type_PriceNotEqual);
	events1.add(Config3Event::Type_PriceNotEqual);
	events1.add(Config3Event::Type_PriceNotEqual);

	// conversion
	Config2EventList events2;
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, events2.load(&memory));
	memory.setAddress(10000);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, events2.save(&memory));

	// check
	Config3EventList events3(&realtime);
	memory.setAddress(10000);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, events3.load(&memory));
	TEST_NUMBER_EQUAL(10, events3.getSize());
	TEST_NUMBER_EQUAL(3, events3.getLen());
	return true;
}

bool Config2ToConfig3EventListTest::testCopy2() {
	RamMemory memory(32000);
	TestRealTime realtime;

	// init
	Config3EventList events1(&realtime);
	events1.init(10, &memory);
	events1.add(Config3Event::Type_PriceNotEqual);
	events1.add(Config3Event::Type_PriceNotEqual);
	events1.add(Config3Event::Type_PriceNotEqual);

	// conversion
	Config2EventList events2;
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, events2.load(&memory));
	memory.setAddress(10000);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, events2.save(&memory));

	// check
	Config3EventList events3(&realtime);
	memory.setAddress(10000);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, events3.load(&memory));
	TEST_NUMBER_EQUAL(10, events3.getSize());
	TEST_NUMBER_EQUAL(3, events3.getLen());
	return true;
}
