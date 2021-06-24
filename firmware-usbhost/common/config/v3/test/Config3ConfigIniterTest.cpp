#include "test/include/Test.h"
#include "memory/include/RamMemory.h"
#include "config/v3/Config3ConfigIniter.h"
#include "config/v3/Config3Modem.h"
#include "logger/include/Logger.h"

class Config3IniterTest : public TestSet {
public:
	Config3IniterTest();
	bool testExeMaster();
	bool testMdbMaster();
	bool testCashlessIdNotSpecified();
	bool testPC9NotSpecified();
};

TEST_SET_REGISTER(Config3IniterTest);

Config3IniterTest::Config3IniterTest() {
	TEST_CASE_REGISTER(Config3IniterTest, testExeMaster);
	TEST_CASE_REGISTER(Config3IniterTest, testMdbMaster);
	TEST_CASE_REGISTER(Config3IniterTest, testCashlessIdNotSpecified);
	TEST_CASE_REGISTER(Config3IniterTest, testPC9NotSpecified);
}

bool Config3IniterTest::testExeMaster() {
	const char data1[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"IC1******0*6*1\r\n"
"IC4*1*7*\r\n"
"AC2*****3\r\n"
"PC1*101*250*0\r\n"
"PC7*101*CA*0*250\r\n"
"PC7*101*CA*1*250\r\n"
"PC7*101*DA*1*100\r\n"
"PC7*101*DA*2*100\r\n"
"PC9*101*1*3\r\n"
"PC1*102*270*0\r\n"
"PC7*102*CA*0*270\r\n"
"PC7*102*CA*1*270\r\n"
"PC7*102*DA*1*100\r\n"
"PC7*102*DA*2*100\r\n"
"PC9*102*2*3\r\n"
"PC1*103*250*0\r\n"
"PC7*103*CA*0*250\r\n"
"PC7*103*CA*1*250\r\n"
"PC7*103*DA*1*100\r\n"
"PC7*103*DA*2*100\r\n"
"PC9*103**3\r\n"
"PC1*104*250*0\r\n"
"PC7*104*CA*0*250\r\n"
"PC7*104*CA*1*250\r\n"
"PC7*104*DA*1*100\r\n"
"PC7*104*DA*2*100\r\n"
"PC9*104**3\r\n"
"G85*A4CB\r\n"
"SE*136*0001\r\n"
"DXE*1*1\r\n";

	Config3ConfigIniter initer;
	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());

	TEST_NUMBER_EQUAL(4, initer.getProducts()->getSize());

	TEST_NUMBER_EQUAL(0, initer.getProducts()->getIndex("101"));
	TEST_NUMBER_EQUAL(3, initer.getProducts()->getIndex("104"));
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, initer.getProducts()->getIndex("201"));

	TEST_NUMBER_EQUAL(0, initer.getProducts()->getIndex(1));
	TEST_NUMBER_EQUAL(1, initer.getProducts()->getIndex(2));
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, initer.getProducts()->getIndex(3));

	Config3ProductIndex *productIndex = initer.getProducts()->get(3);
	TEST_POINTER_NOT_NULL(productIndex);
	TEST_STRING_EQUAL("104", productIndex ->selectId.get());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, productIndex->cashlessId);

	Config3PriceIndexList *priceIndexList = initer.getPrices();
	TEST_NUMBER_EQUAL(4, priceIndexList->getSize());
	TEST_NUMBER_EQUAL(0, priceIndexList->getIndex("CA", 0));
	TEST_NUMBER_EQUAL(1, priceIndexList->getIndex("CA", 1));
	TEST_NUMBER_EQUAL(2, priceIndexList->getIndex("DA", 0));
	TEST_NUMBER_EQUAL(3, priceIndexList->getIndex("DA", 1));

	Config3PriceIndex *priceIndex = priceIndexList->get(0);
	TEST_NUMBER_EQUAL(Config3PriceIndexType_None, priceIndex->type);
	TEST_NUMBER_EQUAL(0, priceIndex->timeTable.getWeek()->getValue());
	TimeInterval *interval = priceIndex->timeTable.getInterval();
	TEST_NUMBER_EQUAL(0, interval->getTime()->getHour());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getMinute());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getSecond());
	TEST_NUMBER_EQUAL(0, interval->getInterval());

	priceIndex = priceIndexList->get(1);
	TEST_NUMBER_EQUAL(Config3PriceIndexType_None, priceIndex->type);
	TEST_NUMBER_EQUAL(0, priceIndex->timeTable.getWeek()->getValue());
	interval = priceIndex->timeTable.getInterval();
	TEST_NUMBER_EQUAL(0, interval->getTime()->getHour());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getMinute());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getSecond());
	TEST_NUMBER_EQUAL(0, interval->getInterval());

	priceIndex = priceIndexList->get(2);
	TEST_NUMBER_EQUAL(Config3PriceIndexType_None, priceIndex->type);
	TEST_NUMBER_EQUAL(0, priceIndex->timeTable.getWeek()->getValue());
	interval = priceIndex->timeTable.getInterval();
	TEST_NUMBER_EQUAL(0, interval->getTime()->getHour());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getMinute());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getSecond());
	TEST_NUMBER_EQUAL(0, interval->getInterval());

	return true;
}

bool Config3IniterTest::testMdbMaster() {
	Config3ConfigIniter initer;

	const char data1[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"IC1******0*6*1\r\n"
"IC4*1*7*\r\n"
"AC2*****2\r\n"
"PC1*101*250*0\r\n"
"PC7*101*CA*0*250\r\n"
"PC7*101*TA*0*250\r\n"
"PC7*101*DA*1*100\r\n"
"PC7*101*DB*1*100\r\n"
"PC9*101*1*3\r\n"
"PC1*102*270*0\r\n"
"PC7*102*CA*0*270\r\n"
"PC7*102*TA*0*270\r\n"
"PC7*102*DA*1*100\r\n"
"PC7*102*DB*1*100\r\n"
"PC9*102*2*3\r\n"
"PC1*103*250*0\r\n"
"PC7*103*CA*0*250\r\n"
"PC7*103*TA*0*250\r\n"
"PC7*103*DA*1*100\r\n"
"PC7*103*DB*1*100\r\n"
"PC9*103**3\r\n"
"PC1*104*250*0\r\n"
"PC7*104*CA*0*250\r\n"
"PC7*104*TA*0*250\r\n"
"PC7*104*DA*1*100\r\n"
"PC7*104*DB*1*100\r\n"
"PC9*104**3\r\n"
"G85*A4CB\r\n"
"SE*136*0001\r\n"
"DXE*1*1\r\n";

	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());

	TEST_NUMBER_EQUAL(4, initer.getProducts()->getSize());

	TEST_NUMBER_EQUAL(0, initer.getProducts()->getIndex("101"));
	TEST_NUMBER_EQUAL(3, initer.getProducts()->getIndex("104"));
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, initer.getProducts()->getIndex("201"));

	TEST_NUMBER_EQUAL(0, initer.getProducts()->getIndex(1));
	TEST_NUMBER_EQUAL(1, initer.getProducts()->getIndex(2));
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, initer.getProducts()->getIndex(3));

	Config3ProductIndex *productIndex = initer.getProducts()->get(3);
	TEST_POINTER_NOT_NULL(productIndex);
	TEST_STRING_EQUAL("104", productIndex ->selectId.get());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, productIndex->cashlessId);

	Config3PriceIndexList *priceIndexList = initer.getPrices();
	TEST_NUMBER_EQUAL(4, priceIndexList->getSize());
	TEST_NUMBER_EQUAL(0, priceIndexList->getIndex("CA", 0));
	TEST_NUMBER_EQUAL(0xFFFF, priceIndexList->getIndex("TA", 0));
	TEST_NUMBER_EQUAL(3, priceIndexList->getIndex("DA", 1));
	TEST_NUMBER_EQUAL(0xFFFF, priceIndexList->getIndex("DB", 1));
	return true;
}

bool Config3IniterTest::testCashlessIdNotSpecified() {
	Config3ConfigIniter initer;

	const char data1[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"IC1******0*6*1\r\n"
"IC4*1*7*\r\n"
"AC2*****2\r\n"
"PC1*101*250*0\r\n"
"PC7*101*CA*0*250\r\n"
"PC7*101*TA*0*250\r\n"
"PC7*101*DA*1*100\r\n"
"PC7*101*DB*1*100\r\n"
"PC9*101**3\r\n"
"PC1*102*270*0\r\n"
"PC7*102*CA*0*270\r\n"
"PC7*102*TA*0*270\r\n"
"PC7*102*DA*1*100\r\n"
"PC7*102*DB*1*100\r\n"
"PC9*102**3\r\n"
"PC1*103*250*0\r\n"
"PC7*103*CA*0*250\r\n"
"PC7*103*TA*0*250\r\n"
"PC7*103*DA*1*100\r\n"
"PC7*103*DB*1*100\r\n"
"PC9*103**3\r\n"
"PC1*104*250*0\r\n"
"PC7*104*CA*0*250\r\n"
"PC7*104*TA*0*250\r\n"
"PC7*104*DA*1*100\r\n"
"PC7*104*DB*1*100\r\n"
"PC9*104**3\r\n"
"G85*A4CB\r\n"
"SE*136*0001\r\n"
"DXE*1*1\r\n";

	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());

	Config3ProductIndexList *productIndexList = initer.getProducts();
	Config3ProductIndex *productIndex1 = productIndexList ->get(0);
	TEST_POINTER_NOT_NULL(productIndex1);
	TEST_STRING_EQUAL("101", productIndex1 ->selectId.get());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, productIndex1->cashlessId);
	Config3ProductIndex *productIndex2 = productIndexList ->get(1);
	TEST_POINTER_NOT_NULL(productIndex2);
	TEST_STRING_EQUAL("102", productIndex2 ->selectId.get());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, productIndex2->cashlessId);
	Config3ProductIndex *productIndex3 = productIndexList ->get(2);
	TEST_POINTER_NOT_NULL(productIndex3);
	TEST_STRING_EQUAL("103", productIndex3 ->selectId.get());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, productIndex3->cashlessId);
	Config3ProductIndex *productIndex4 = productIndexList ->get(3);
	TEST_POINTER_NOT_NULL(productIndex4);
	TEST_STRING_EQUAL("104", productIndex4 ->selectId.get());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, productIndex4->cashlessId);

	Config3PriceIndexList *priceIndexList = initer.getPrices();
	TEST_NUMBER_EQUAL(4, priceIndexList->getSize());
	TEST_NUMBER_EQUAL(0, priceIndexList->getIndex("CA", 0));
	TEST_NUMBER_EQUAL(0xFFFF, priceIndexList->getIndex("TA", 0));
	TEST_NUMBER_EQUAL(3, priceIndexList->getIndex("DA", 1));
	TEST_NUMBER_EQUAL(0xFFFF, priceIndexList->getIndex("DB", 1));
	return true;
}

bool Config3IniterTest::testPC9NotSpecified() {
	Config3ConfigIniter initer;

	const char data1[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"IC1******0*6*1\r\n"
"IC4*1*7*\r\n"
"AC2*****2\r\n"
"PC1*101*250*0\r\n"
"PC7*101*CA*0*250\r\n"
"PC7*101*TA*0*250\r\n"
"PC7*101*DA*1*100\r\n"
"PC7*101*DB*1*100\r\n"
"PC1*102*270*0\r\n"
"PC7*102*CA*0*270\r\n"
"PC7*102*TA*0*270\r\n"
"PC7*102*DA*1*100\r\n"
"PC7*102*DB*1*100\r\n"
"PC1*103*250*0\r\n"
"PC7*103*CA*0*250\r\n"
"PC7*103*TA*0*250\r\n"
"PC7*103*DA*1*100\r\n"
"PC7*103*DB*1*100\r\n"
"PC1*104*250*0\r\n"
"PC7*104*CA*0*250\r\n"
"PC7*104*TA*0*250\r\n"
"PC7*104*DA*1*100\r\n"
"PC7*104*DB*1*100\r\n"
"G85*A4CB\r\n"
"SE*136*0001\r\n"
"DXE*1*1\r\n";

	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());

	Config3ProductIndexList *productIndexList = initer.getProducts();
	Config3ProductIndex *productIndex1 = productIndexList ->get(0);
	TEST_POINTER_NOT_NULL(productIndex1);
	TEST_STRING_EQUAL("101", productIndex1 ->selectId.get());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, productIndex1->cashlessId);
	Config3ProductIndex *productIndex2 = productIndexList ->get(1);
	TEST_POINTER_NOT_NULL(productIndex2);
	TEST_STRING_EQUAL("102", productIndex2 ->selectId.get());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, productIndex2->cashlessId);
	Config3ProductIndex *productIndex3 = productIndexList ->get(2);
	TEST_POINTER_NOT_NULL(productIndex3);
	TEST_STRING_EQUAL("103", productIndex3 ->selectId.get());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, productIndex3->cashlessId);
	Config3ProductIndex *productIndex4 = productIndexList ->get(3);
	TEST_POINTER_NOT_NULL(productIndex4);
	TEST_STRING_EQUAL("104", productIndex4 ->selectId.get());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, productIndex4->cashlessId);

	Config3PriceIndexList *priceIndexList = initer.getPrices();
	TEST_NUMBER_EQUAL(4, priceIndexList->getSize());
	TEST_NUMBER_EQUAL(0, priceIndexList->getIndex("CA", 0));
	TEST_NUMBER_EQUAL(0xFFFF, priceIndexList->getIndex("TA", 0));
	TEST_NUMBER_EQUAL(3, priceIndexList->getIndex("DA", 1));
	TEST_NUMBER_EQUAL(0xFFFF, priceIndexList->getIndex("DB", 1));
	return true;
}
