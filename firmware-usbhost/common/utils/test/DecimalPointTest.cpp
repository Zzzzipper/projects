#include "utils/include/DecimalPoint.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class DecimalPointTest : public TestSet {
public:
	DecimalPointTest();
	bool testConvertDecimalPoint();
	bool testDecimalPointConverter();
	bool testScalingFactor();
	bool testEvenx();
};

TEST_SET_REGISTER(DecimalPointTest);

DecimalPointTest::DecimalPointTest() {
	TEST_CASE_REGISTER(DecimalPointTest, testConvertDecimalPoint);
	TEST_CASE_REGISTER(DecimalPointTest, testDecimalPointConverter);
	TEST_CASE_REGISTER(DecimalPointTest, testScalingFactor);
	TEST_CASE_REGISTER(DecimalPointTest, testEvenx);
}

bool DecimalPointTest::testConvertDecimalPoint() {
	TEST_NUMBER_EQUAL(10, convertDecimalPoint(2, 0, 1000));
	TEST_NUMBER_EQUAL(200, convertDecimalPoint(2, 1, 2000));
	TEST_NUMBER_EQUAL(3000, convertDecimalPoint(2, 2, 3000));

	TEST_NUMBER_EQUAL(400, convertDecimalPoint(1, 0, 4000));
	TEST_NUMBER_EQUAL(5000, convertDecimalPoint(1, 1, 5000));
	TEST_NUMBER_EQUAL(60000, convertDecimalPoint(1, 2, 6000));

	TEST_NUMBER_EQUAL(7000, convertDecimalPoint(0, 0, 7000));
	TEST_NUMBER_EQUAL(80000, convertDecimalPoint(0, 1, 8000));
	TEST_NUMBER_EQUAL(900000, convertDecimalPoint(0, 2, 9000));
	return true;
}

bool DecimalPointTest::testDecimalPointConverter() {
	DecimalPointConverter config(2);

	config.setMasterDecimalPoint(0);
	config.setDeviceDecimalPoint(0);
	TEST_NUMBER_EQUAL(3000, config.convertMasterToDevice(3000));
	TEST_NUMBER_EQUAL(4000, config.convertDeviceToMaster(4000));
	config.setDeviceDecimalPoint(1);
	TEST_NUMBER_EQUAL(5000, config.convertMasterToDevice(500));
	TEST_NUMBER_EQUAL(600, config.convertDeviceToMaster(6000));
	config.setDeviceDecimalPoint(2);
	TEST_NUMBER_EQUAL(7000, config.convertMasterToDevice(70));
	TEST_NUMBER_EQUAL(80, config.convertDeviceToMaster(8000));

	config.setMasterDecimalPoint(1);
	config.setDeviceDecimalPoint(0);
	TEST_NUMBER_EQUAL(300, config.convertMasterToDevice(3000));
	TEST_NUMBER_EQUAL(4000, config.convertDeviceToMaster(400));
	config.setDeviceDecimalPoint(1);
	TEST_NUMBER_EQUAL(5000, config.convertMasterToDevice(5000));
	TEST_NUMBER_EQUAL(6000, config.convertDeviceToMaster(6000));
	config.setDeviceDecimalPoint(2);
	TEST_NUMBER_EQUAL(7000, config.convertMasterToDevice(700));
	TEST_NUMBER_EQUAL(800, config.convertDeviceToMaster(8000));

	config.setMasterDecimalPoint(2);
	config.setDeviceDecimalPoint(0);
	TEST_NUMBER_EQUAL(30, config.convertMasterToDevice(3000));
	TEST_NUMBER_EQUAL(4000, config.convertDeviceToMaster(40));
	config.setDeviceDecimalPoint(1);
	TEST_NUMBER_EQUAL(50, config.convertMasterToDevice(500));
	TEST_NUMBER_EQUAL(600, config.convertDeviceToMaster(60));
	config.setDeviceDecimalPoint(2);
	TEST_NUMBER_EQUAL(70, config.convertMasterToDevice(70));
	TEST_NUMBER_EQUAL(80, config.convertDeviceToMaster(80));
	return true;
}

bool DecimalPointTest::testScalingFactor() {
	DecimalPointConverter config(2);

	config.setMasterDecimalPoint(0);
	config.setDeviceDecimalPoint(0);
	config.setScalingFactor(5);
	TEST_NUMBER_EQUAL(600, config.convertMasterToDevice(3000));
	TEST_NUMBER_EQUAL(4000, config.convertDeviceToMaster(800));
	config.setDeviceDecimalPoint(1);
	TEST_NUMBER_EQUAL(1000, config.convertMasterToDevice(500));
	TEST_NUMBER_EQUAL(3000, config.convertDeviceToMaster(6000));
	config.setDeviceDecimalPoint(2);
	TEST_NUMBER_EQUAL(1400, config.convertMasterToDevice(70));
	TEST_NUMBER_EQUAL(400, config.convertDeviceToMaster(8000));

	config.setMasterDecimalPoint(1);
	config.setDeviceDecimalPoint(0);
	config.setScalingFactor(5);
	TEST_NUMBER_EQUAL(60, config.convertMasterToDevice(3000));
	TEST_NUMBER_EQUAL(20000, config.convertDeviceToMaster(400));
	config.setDeviceDecimalPoint(1);
	TEST_NUMBER_EQUAL(1000, config.convertMasterToDevice(5000));
	TEST_NUMBER_EQUAL(30000, config.convertDeviceToMaster(6000));
	config.setDeviceDecimalPoint(2);
	TEST_NUMBER_EQUAL(1400, config.convertMasterToDevice(700));
	TEST_NUMBER_EQUAL(4000, config.convertDeviceToMaster(8000));

	config.setMasterDecimalPoint(2);
	config.setDeviceDecimalPoint(0);
	config.setScalingFactor(5);
	TEST_NUMBER_EQUAL(6, config.convertMasterToDevice(3000));
	TEST_NUMBER_EQUAL(20000, config.convertDeviceToMaster(40));
	config.setDeviceDecimalPoint(1);
	TEST_NUMBER_EQUAL(10, config.convertMasterToDevice(500));
	TEST_NUMBER_EQUAL(3000, config.convertDeviceToMaster(60));
	config.setDeviceDecimalPoint(2);
	TEST_NUMBER_EQUAL(14, config.convertMasterToDevice(70));
	TEST_NUMBER_EQUAL(400, config.convertDeviceToMaster(80));
	return true;
}

bool DecimalPointTest::testEvenx() {
	DecimalPointConverter config(2);

	config.setMasterDecimalPoint(1);
	config.setDeviceDecimalPoint(2);
	config.setScalingFactor(100);
	TEST_NUMBER_EQUAL(2, config.convertMasterToDevice(20));
	TEST_NUMBER_EQUAL(50, config.convertDeviceToMaster(5));
	return true;
}
