#include "mdb/master/bill_validator/MdbBill.h"
#include "timer/include/TestRealTime.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class MdbBillValidatorContextTest : public TestSet {
public:
	MdbBillValidatorContextTest();
	bool testMask();
	bool testChanged();
	bool testScaleFactor1000();
};

TEST_SET_REGISTER(MdbBillValidatorContextTest);

MdbBillValidatorContextTest::MdbBillValidatorContextTest() {
	TEST_CASE_REGISTER(MdbBillValidatorContextTest, testMask);
	TEST_CASE_REGISTER(MdbBillValidatorContextTest, testChanged);
	TEST_CASE_REGISTER(MdbBillValidatorContextTest, testScaleFactor1000);
}

bool MdbBillValidatorContextTest::testMask() {
	TestRealTime realtime;
	MdbBillValidatorContext context(2, &realtime, 25000);
	uint8_t data[] = { 0x01, 0x05, 0x0A, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	context.init(1, 1, 100, data, sizeof(data));

	TEST_NUMBER_EQUAL(1000, context.getBillNominal(0));
	TEST_NUMBER_EQUAL(5000, context.getBillNominal(1));
	TEST_NUMBER_EQUAL(10000, context.getBillNominal(2));
	TEST_NUMBER_EQUAL(50000, context.getBillNominal(3));

	context.setMaxBill(100000);
	TEST_NUMBER_EQUAL(0x000F, context.getMask());
	context.setMaxBill(10000);
	TEST_NUMBER_EQUAL(0x0007, context.getMask());
	context.setMaxBill(1000);
	TEST_NUMBER_EQUAL(0x0001, context.getMask());
	return true;
}

bool MdbBillValidatorContextTest::testChanged() {
	TestRealTime realtime;
	MdbBillValidatorContext context(2, &realtime, 25000);

	TEST_NUMBER_EQUAL(false, context.getChanged());

	uint8_t data1[] = { 0x01, 0x05, 0x0A, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	context.init(1, 1, 100, data1, sizeof(data1));
	TEST_NUMBER_EQUAL(true, context.getChanged());
	context.resetChanged();
	TEST_NUMBER_EQUAL(false, context.getChanged());

	context.init(1, 1, 100, data1, sizeof(data1));
	TEST_NUMBER_EQUAL(false, context.getChanged());

	uint8_t data2[] = { 0x01, 0x05, 0x0A, 0x32, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	context.init(1, 1, 100, data2, sizeof(data2));
	TEST_NUMBER_EQUAL(true, context.getChanged());
	context.resetChanged();
	TEST_NUMBER_EQUAL(false, context.getChanged());

	context.init(1, 2, 100, data2, sizeof(data2));
	TEST_NUMBER_EQUAL(true, context.getChanged());
	context.resetChanged();
	TEST_NUMBER_EQUAL(false, context.getChanged());

	context.init(1, 2, 10, data2, sizeof(data2));
	TEST_NUMBER_EQUAL(true, context.getChanged());
	context.resetChanged();
	TEST_NUMBER_EQUAL(false, context.getChanged());
	return true;
}

bool MdbBillValidatorContextTest::testScaleFactor1000() {
	TestRealTime realtime;
	MdbBillValidatorContext context(0, &realtime, 25000);
	uint8_t level = 1;
	uint32_t decimalPoint = 2;
	uint32_t scaleFactor = 1000;
	uint8_t data[] = { 0x01, 0x05, 0x0A, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	context.init(level, decimalPoint, scaleFactor, data, sizeof(data));

	TEST_NUMBER_EQUAL(10, context.getBillNominal(0));
	TEST_NUMBER_EQUAL(50, context.getBillNominal(1));
	TEST_NUMBER_EQUAL(100, context.getBillNominal(2));
	TEST_NUMBER_EQUAL(500, context.getBillNominal(3));
	return true;
}
