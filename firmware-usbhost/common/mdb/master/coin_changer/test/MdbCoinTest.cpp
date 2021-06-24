#include "mdb/master/coin_changer/MdbCoin.h"
#include "timer/include/TestRealTime.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class MdbCoinChangerContextTest : public TestSet {
public:
	MdbCoinChangerContextTest();
	bool testConstructor();
	bool testInit();
	bool testChanged();
	bool testMaxValue();
};

TEST_SET_REGISTER(MdbCoinChangerContextTest);

MdbCoinChangerContextTest::MdbCoinChangerContextTest() {
	TEST_CASE_REGISTER(MdbCoinChangerContextTest, testConstructor);
	TEST_CASE_REGISTER(MdbCoinChangerContextTest, testInit);
	TEST_CASE_REGISTER(MdbCoinChangerContextTest, testChanged);
	TEST_CASE_REGISTER(MdbCoinChangerContextTest, testMaxValue);	
}

bool MdbCoinChangerContextTest::testConstructor() {
	TestRealTime realtime;
	MdbCoinChangerContext context(2, &realtime);

	TEST_HEXDATA_EQUAL("202020", (uint8_t*)context.getManufacturer(), context.getManufacturerSize());
	TEST_HEXDATA_EQUAL("202020202020202020202020", (uint8_t*)context.getModel(), context.getModelSize());
	TEST_HEXDATA_EQUAL("202020202020202020202020", (uint8_t*)context.getSerialNumber(), context.getSerialNumberSize());

	TEST_NUMBER_EQUAL(2, context.getDecimalPoint());
	TEST_NUMBER_EQUAL(1, context.getScalingFactor());
	TEST_NUMBER_EQUAL(0, context.get(0)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(1)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(2)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(3)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(4)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(5)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(6)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(7)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(8)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(9)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(10)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(11)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(12)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(13)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(14)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(15)->getNominal());
	return true;
}

bool MdbCoinChangerContextTest::testInit() {
	TestRealTime realtime;
	MdbCoinChangerContext context(2, &realtime);
	uint8_t data[] = { 0x01, 0x05, 0x0A, 0x0A, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	context.init(1, 1, 10, data, sizeof(data), 0xFF);

	TEST_NUMBER_EQUAL(1, context.getDecimalPoint());
	TEST_NUMBER_EQUAL(10, context.getScalingFactor());
	TEST_NUMBER_EQUAL(100, context.get(0)->getNominal());
	TEST_NUMBER_EQUAL(500, context.get(1)->getNominal());
	TEST_NUMBER_EQUAL(1000, context.get(2)->getNominal());
	TEST_NUMBER_EQUAL(1000, context.get(3)->getNominal());
	TEST_NUMBER_EQUAL(MDB_CC_TOKEN, context.get(4)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(5)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(6)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(7)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(8)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(9)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(10)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(11)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(12)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(13)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(14)->getNominal());
	TEST_NUMBER_EQUAL(0, context.get(15)->getNominal());

	TEST_NUMBER_EQUAL(MDB_CC_TOKEN, context.money2value(MDB_CC_TOKEN));
	TEST_NUMBER_EQUAL(MDB_CC_TOKEN, context.value2money(MDB_CC_TOKEN));
	return true;
}

bool MdbCoinChangerContextTest::testChanged() {
	TestRealTime realtime;
	MdbCoinChangerContext context(2, &realtime);

	TEST_NUMBER_EQUAL(false, context.getChanged());

	uint8_t data1[] = { 0x01, 0x05, 0x0A, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	context.init(1, 1, 100, data1, sizeof(data1), 0xFF);
	TEST_NUMBER_EQUAL(true, context.getChanged());
	context.resetChanged();
	TEST_NUMBER_EQUAL(false, context.getChanged());

	context.init(1, 1, 100, data1, sizeof(data1), 0xFF);
	TEST_NUMBER_EQUAL(false, context.getChanged());

	uint8_t data2[] = { 0x01, 0x05, 0x0A, 0x32, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	context.init(1, 1, 100, data2, sizeof(data2), 0xFF);
	TEST_NUMBER_EQUAL(true, context.getChanged());
	context.resetChanged();
	TEST_NUMBER_EQUAL(false, context.getChanged());

	context.init(1, 2, 100, data2, sizeof(data2), 0xFF);
	TEST_NUMBER_EQUAL(true, context.getChanged());
	context.resetChanged();
	TEST_NUMBER_EQUAL(false, context.getChanged());

	context.init(1, 2, 10, data2, sizeof(data2), 0xFF);
	TEST_NUMBER_EQUAL(true, context.getChanged());
	context.resetChanged();
	TEST_NUMBER_EQUAL(false, context.getChanged());
	return true;
}

bool MdbCoinChangerContextTest::testMaxValue() {
	TestRealTime realtime;
	MdbCoinChangerContext context(2, &realtime);

	TEST_NUMBER_EQUAL(false, context.getChanged());

	uint8_t nominal1[] = { 0x01, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	context.init(1, 1, 10, nominal1, sizeof(nominal1), 0xFF);
	TEST_NUMBER_EQUAL(true, context.getChanged());

	uint8_t number1[] = { 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	context.update(0xFF, number1, sizeof(number1));

	TEST_NUMBER_EQUAL(408000, context.getInTubeValue());
	return true;
}
