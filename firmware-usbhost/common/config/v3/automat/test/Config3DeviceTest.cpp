#include "memory/include/RamMemory.h"
#include "config/v3/automat/Config3Device.h"
#include "mdb/master/bill_validator/MdbBill.h"
#include "timer/include/TestRealTime.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class Config3DeviceTest : public TestSet {
public:
	Config3DeviceTest();
	bool testSaveLoad();
};

TEST_SET_REGISTER(Config3DeviceTest);

Config3DeviceTest::Config3DeviceTest() {
	TEST_CASE_REGISTER(Config3DeviceTest, testSaveLoad);
}

bool Config3DeviceTest::testSaveLoad() {
	RamMemory memory(32000);
	TestRealTime realtime;
	Config3DeviceList config1(2, &realtime, 25000);
	config1.init(&memory);

	MdbBillValidatorContext *bvContext1 = config1.getBVContext();
	TEST_NUMBER_EQUAL(2, bvContext1->getDecimalPoint());
	TEST_NUMBER_EQUAL(1, bvContext1->getScalingFactor());
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(0));
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(1));
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(2));
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(3));
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(4));
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(5));
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(6));
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(7));
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(8));
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(9));
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(10));
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(11));
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(12));
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(13));
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(14));
	TEST_NUMBER_EQUAL(0, bvContext1->getBillNominal(15));

	uint8_t bills[] = { 0x01, 0x02, 0x05, 0x0A, 0x00 };
	bvContext1->init(1, 1, 100, bills, sizeof(bills));
	TEST_NUMBER_EQUAL(true, bvContext1->getChanged());
	config1.save();

	memory.setAddress(0);
	Config3DeviceList config2(2, &realtime, 25000);
	config2.load(&memory);
	MdbBillValidatorContext *bvContext2 = config2.getBVContext();
	TEST_NUMBER_EQUAL(1, bvContext2->getDecimalPoint());
	TEST_NUMBER_EQUAL(100, bvContext2->getScalingFactor());
	TEST_NUMBER_EQUAL(1000, bvContext2->getBillNominal(0));
	TEST_NUMBER_EQUAL(2000, bvContext2->getBillNominal(1));
	TEST_NUMBER_EQUAL(5000, bvContext2->getBillNominal(2));
	TEST_NUMBER_EQUAL(10000, bvContext2->getBillNominal(3));
	TEST_NUMBER_EQUAL(0, bvContext2->getBillNominal(4));
	TEST_NUMBER_EQUAL(0, bvContext2->getBillNominal(5));
	TEST_NUMBER_EQUAL(0, bvContext2->getBillNominal(6));
	TEST_NUMBER_EQUAL(0, bvContext2->getBillNominal(7));
	TEST_NUMBER_EQUAL(0, bvContext2->getBillNominal(8));
	TEST_NUMBER_EQUAL(0, bvContext2->getBillNominal(9));
	TEST_NUMBER_EQUAL(0, bvContext2->getBillNominal(10));
	TEST_NUMBER_EQUAL(0, bvContext2->getBillNominal(11));
	TEST_NUMBER_EQUAL(0, bvContext2->getBillNominal(12));
	TEST_NUMBER_EQUAL(0, bvContext2->getBillNominal(13));
	TEST_NUMBER_EQUAL(0, bvContext2->getBillNominal(14));
	TEST_NUMBER_EQUAL(0, bvContext2->getBillNominal(15));

	return true;
}
