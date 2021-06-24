#include "code_scanner/unitex1/UnitexScanner.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class Unitex1ScannerTest : public TestSet {
public:
	Unitex1ScannerTest();
	bool init();
	void cleanup();
	bool test();

private:
};

TEST_SET_REGISTER(Unitex1ScannerTest);

Unitex1ScannerTest::Unitex1ScannerTest() {
	TEST_CASE_REGISTER(Unitex1ScannerTest, test);
}

bool Unitex1ScannerTest::init() {
	return true;
}

void Unitex1ScannerTest::cleanup() {
}

bool Unitex1ScannerTest::test() {
	UnitexSales list1;
	UnitexSale sale1;
	sale1.cashlessId = 1;
	sale1.checkNum = 1;
	sale1.year = 20;
	sale1.month = 12;
	sale1.day = 16;

	for(uint16_t i = 0; i < 40; i++) {
		sale1.cashlessId += 1;
		sale1.checkNum += 1;
		TEST_NUMBER_EQUAL(false, list1.isUsed(&sale1));
		TEST_NUMBER_EQUAL(true, list1.push(&sale1));
		TEST_NUMBER_EQUAL(false, list1.push(&sale1));
		TEST_NUMBER_EQUAL(true, list1.isUsed(&sale1));
	}

	return true;
}
