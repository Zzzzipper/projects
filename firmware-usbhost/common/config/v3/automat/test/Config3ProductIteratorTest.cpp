#include "memory/include/RamMemory.h"
#include "config/v3/automat/Config3ProductIterator.h"
#include "config/v3/automat/Config3Automat.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class Config3ProductIteratorTest : public TestSet {
public:
	Config3ProductIteratorTest();
	bool testFind();
};

TEST_SET_REGISTER(Config3ProductIteratorTest);

Config3ProductIteratorTest::Config3ProductIteratorTest() {
	TEST_CASE_REGISTER(Config3ProductIteratorTest, testFind);
}

bool Config3ProductIteratorTest::testFind() {
	RamMemory memory(32000);
	Config3Automat automat1(NULL);
	automat1.addPriceList("CA", 0, Config3PriceIndexType_Base);
	automat1.addPriceList("DA", 1, Config3PriceIndexType_Base);
	automat1.addProduct("01", 1);
	automat1.addProduct("02", 2);
	automat1.addProduct("03", 3);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat1.init(&memory));

	Config3ProductIterator *iterator = automat1.createIterator();
	TEST_NUMBER_EQUAL(true, iterator->findByIndex(0));
	iterator->setPrice("CA", 0, 100);
	iterator->setPrice("DA", 1, 110);
	TEST_NUMBER_EQUAL(true, iterator->findByIndex(1));
	iterator->setPrice("CA", 0, 200);
	iterator->setPrice("DA", 1, 210);
	TEST_NUMBER_EQUAL(true, iterator->findByIndex(2));
	iterator->setPrice("CA", 0, 300);
	iterator->setPrice("DA", 1, 310);

	TEST_NUMBER_EQUAL(true, iterator->findByPrice("CA", 0, 200));
	TEST_STRING_EQUAL("02", iterator->getId());
	TEST_NUMBER_EQUAL(2, iterator->getCashlessId());
	TEST_NUMBER_EQUAL(true, iterator->findByPrice("DA", 1, 310));
	TEST_STRING_EQUAL("03", iterator->getId());
	TEST_NUMBER_EQUAL(3, iterator->getCashlessId());
	TEST_NUMBER_EQUAL(false, iterator->findByPrice("CA", 0, 400));
	TEST_NUMBER_EQUAL(false, iterator->findByPrice("DA", 0, 310));
	return true;
}
