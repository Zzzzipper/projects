#include "config/v1/boot/Config1Boot.h"
#include "config/v1/fiscal/Config1Fiscal.h"
#include "config/v1/event/Config1EventList.h"
#include "config/v1/automat/Config1Automat.h"
#include "config/v2/event/Config2EventList.h"
#include "config/v2/automat/Config2Automat.h"
#include "config/v3/Config3Modem.h"
#include "config/v3/Config3Repairer.h"
#include "timer/include/TestRealTime.h"
#include "memory/include/RamMemory.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class Config3ConverterTest : public TestSet {
public:
	Config3ConverterTest();
	bool testConvert1();
	bool testConvert2();
};

TEST_SET_REGISTER(Config3ConverterTest);

Config3ConverterTest::Config3ConverterTest() {
	TEST_CASE_REGISTER(Config3ConverterTest, testConvert1);
	TEST_CASE_REGISTER(Config3ConverterTest, testConvert2);
}

bool Config3ConverterTest::testConvert1() {
	RamMemory memory(64000);
	TestRealTime realtime;
	StatStorage stat;

	// init
	Config1Boot boot1;
	TEST_NUMBER_EQUAL(MemoryResult_Ok, boot1.init(&memory));
	boot1.setFirmwareRelease(234);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, boot1.save());

	Config1Fiscal fiscal1;
	fiscal1.setDefault();
	fiscal1.setKkt(5);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, fiscal1.save(&memory));

	Config1EventList events1;
	events1.add(1111);
	events1.add(2222);
	events1.add(3333);
	events1.add(4444);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, events1.save(&memory));

	Config1Automat automat1;
	automat1.setDefault();
	automat1.setAutomatId(12345678);
	automat1.addPriceList("CA", 0);
	automat1.addPriceList("DA", 1);
	automat1.addProduct("01", 111);
	automat1.addProduct("02", 222);
	automat1.addProduct("03", 333);
	automat1.init();
	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat1.save(&memory));

	// convert
	Config3Modem config2(&memory, &realtime, &stat);
	Config3Repairer repairer2(&config2, &memory);
	TEST_NUMBER_EQUAL(MemoryResult_WrongCrc, config2.load());
	TEST_NUMBER_EQUAL(MemoryResult_Ok, repairer2.repair());
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.load());

	// check boot
	Config1Boot *boot2 = config2.getBoot();
	TEST_NUMBER_EQUAL(234, boot2->getFirmwareRelease());

	// check fiscal
	Config2Fiscal *fiscal2 = config2.getFiscal();
	TEST_NUMBER_EQUAL(5, fiscal2->getKkt());

	// check events
	Config3EventList *events2 = config2.getEvents();
	TEST_NUMBER_EQUAL(4, events2->getSize());

	// check automat
	Config3Automat *automat2 = config2.getAutomat();

	// check price indexes
	Config3PriceIndexList *priceIndexes2 = automat2->getPriceIndexList();
	TEST_NUMBER_EQUAL(4, priceIndexes2->getSize());
	Config3PriceIndex *priceIndex2 = priceIndexes2->get(0);
	TEST_STRING_EQUAL("CA", priceIndex2->device.get());
	TEST_NUMBER_EQUAL(0, priceIndex2->number);
	TEST_NUMBER_EQUAL(Config3PriceIndexType_Base, priceIndex2->type);
	priceIndex2 = priceIndexes2->get(3);
	TEST_STRING_EQUAL("DA", priceIndex2->device.get());
	TEST_NUMBER_EQUAL(1, priceIndex2->number);
	TEST_NUMBER_EQUAL(Config3PriceIndexType_Base, priceIndex2->type);

	// check products
	Config3ProductList *products2 = automat2->getProductList();
	TEST_NUMBER_EQUAL(3, products2->getProductNum());
	return true;
}

bool Config3ConverterTest::testConvert2() {
	RamMemory memory(64000);
	TestRealTime realtime;
	StatStorage stat;

	// init
	Config1Boot boot1;
	TEST_NUMBER_EQUAL(MemoryResult_Ok, boot1.init(&memory));
	boot1.setFirmwareRelease(234);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, boot1.save());

	Config1Fiscal fiscal1;
	fiscal1.setDefault();
	fiscal1.setKkt(5);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, fiscal1.save(&memory));

	Config2EventList events1;
	events1.add(1111);
	events1.add(2222);
	events1.add(3333);
	events1.add(4444);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, events1.save(&memory));

	Config2Automat automat1;
	automat1.setDefault();
	automat1.setAutomatId(12345678);
	automat1.addPriceList("CA", 0, Config3PriceIndexType_Base);
	automat1.addPriceList("CA", 1, Config3PriceIndexType_None);
	automat1.addPriceList("CA", 2, Config3PriceIndexType_None);
	automat1.addPriceList("CA", 3, Config3PriceIndexType_None);
	automat1.addPriceList("DA", 0, Config3PriceIndexType_None);
	automat1.addPriceList("DA", 1, Config3PriceIndexType_Base);
	automat1.addPriceList("DA", 2, Config3PriceIndexType_None);
	automat1.addPriceList("DA", 3, Config3PriceIndexType_None);
	automat1.addProduct("01", 111);
	automat1.addProduct("02", 222);
	automat1.addProduct("03", 333);
	automat1.init();
	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat1.save(&memory));

	// convert
	Config3Modem config2(&memory, &realtime, &stat);
	TEST_NUMBER_EQUAL(MemoryResult_WrongCrc, config2.load());
	Config3Repairer repairer2(&config2, &memory);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, repairer2.repair());
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.load());

	// check boot
	Config1Boot *boot2 = config2.getBoot();
	TEST_NUMBER_EQUAL(234, boot2->getFirmwareRelease());

	// check fiscal
	Config2Fiscal *fiscal2 = config2.getFiscal();
	TEST_NUMBER_EQUAL(5, fiscal2->getKkt());

	// check events
	Config3EventList *events2 = config2.getEvents();
	TEST_NUMBER_EQUAL(4, events2->getSize());

	// check automat
	Config3Automat *automat2 = config2.getAutomat();

	// check price indexes
	Config3PriceIndexList *priceIndexes2 = automat2->getPriceIndexList();
	TEST_NUMBER_EQUAL(4, priceIndexes2->getSize());
	Config3PriceIndex *priceIndex2 = priceIndexes2->get(0);
	TEST_STRING_EQUAL("CA", priceIndex2->device.get());
	TEST_NUMBER_EQUAL(0, priceIndex2->number);
	TEST_NUMBER_EQUAL(Config3PriceIndexType_Base, priceIndex2->type);
	priceIndex2 = priceIndexes2->get(1);
	TEST_STRING_EQUAL("CA", priceIndex2->device.get());
	TEST_NUMBER_EQUAL(1, priceIndex2->number);
	TEST_NUMBER_EQUAL(Config3PriceIndexType_None, priceIndex2->type);
	priceIndex2 = priceIndexes2->get(2);
	TEST_STRING_EQUAL("DA", priceIndex2->device.get());
	TEST_NUMBER_EQUAL(0, priceIndex2->number);
	TEST_NUMBER_EQUAL(Config3PriceIndexType_None, priceIndex2->type);
	priceIndex2 = priceIndexes2->get(3);
	TEST_STRING_EQUAL("DA", priceIndex2->device.get());
	TEST_NUMBER_EQUAL(1, priceIndex2->number);
	TEST_NUMBER_EQUAL(Config3PriceIndexType_Base, priceIndex2->type);

	// check products
	Config3ProductList *products2 = automat2->getProductList();
	TEST_NUMBER_EQUAL(3, products2->getProductNum());
	return true;
}
