#include "config/v2/automat/Config2AutomatConverter.h"
#include "config/v2/automat/Config2Automat.h"
#include "config/v3/automat/Config3Automat.h"
#include "memory/include/RamMemory.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class Config2ToConfig3AutomatTest : public TestSet {
public:
	Config2ToConfig3AutomatTest();
	bool testConvert();
};

TEST_SET_REGISTER(Config2ToConfig3AutomatTest);

Config2ToConfig3AutomatTest::Config2ToConfig3AutomatTest() {
	TEST_CASE_REGISTER(Config2ToConfig3AutomatTest, testConvert);
}

bool Config2ToConfig3AutomatTest::testConvert() {
	RamMemory memory(32000);

	// init
	Config2Automat automat1;
	automat1.addPriceList("CA", 0, Config3PriceIndexType_Base);
	automat1.addPriceList("DA", 1, Config3PriceIndexType_Base);
	automat1.addProduct("01", 1);
	automat1.addProduct("02", 2);
	automat1.addProduct("03", 3);
	automat1.init();

	Config2ProductList *products1 = automat1.getProductList();
	Config2Product *product1 = products1->getByIndex(1);
	TEST_POINTER_NOT_NULL(product1);
	product1->data.name.set("Test1");
	product1->data.taxRate = 11;
	product1->data.totalCount = 110;
	product1->data.totalMoney = 11000;
	product1->data.count = 11;
	product1->data.money = 1100;

	Config2Price *price1 = product1->prices.getByIndex(0);
	TEST_POINTER_NOT_NULL(price1);
	price1->data.totalCount = 100;
	price1->data.totalMoney = 10000;
	price1->data.count = 10;
	price1->data.money = 1000;

	price1 = product1->prices.getByIndex(1);
	TEST_POINTER_NOT_NULL(price1);
	price1->data.totalCount = 10;
	price1->data.totalMoney = 1000;
	price1->data.count = 1;
	price1->data.money = 100;

	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat1.save(&memory));

	// conversion
	Config2AutomatConverter converter;
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, converter.load(&memory));
	memory.setAddress(10000);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, converter.save(&memory));

	// check
	Config3Automat automat2(NULL);
	memory.setAddress(10000);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat2.loadAndCheck(&memory));
	Config3PriceIndexList *priceIndexes2 = automat2.getPriceIndexList();
	TEST_NUMBER_EQUAL(4, priceIndexes2->getSize());

	Config3PriceIndex *index2 = priceIndexes2->get(0);
	TEST_POINTER_NOT_NULL(index2);
	TEST_STRING_EQUAL("CA", index2->device.get());
	TEST_NUMBER_EQUAL(0, index2->number);
	TEST_NUMBER_EQUAL(Config3PriceIndexType_Base, index2->type);

	index2 = priceIndexes2->get(3);
	TEST_POINTER_NOT_NULL(index2);
	TEST_STRING_EQUAL("DA", index2->device.get());
	TEST_NUMBER_EQUAL(1, index2->number);
	TEST_NUMBER_EQUAL(Config3PriceIndexType_Base, index2->type);
	return true;
}
