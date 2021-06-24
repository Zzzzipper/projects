#include "config/v2/fiscal/Config2Fiscal.h"
#include "config/v1/fiscal/Config1FiscalConverter.h"
#include "memory/include/RamMemory.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class ConfigFiscal1ConverterTest : public TestSet {
public:
	ConfigFiscal1ConverterTest();
	bool testConvert();
};

TEST_SET_REGISTER(ConfigFiscal1ConverterTest);

ConfigFiscal1ConverterTest::ConfigFiscal1ConverterTest() {
	TEST_CASE_REGISTER(ConfigFiscal1ConverterTest, testConvert);
}

bool ConfigFiscal1ConverterTest::testConvert() {
	RamMemory memory(32000);

	// init
	Config1Fiscal fiscal1;
	fiscal1.setDefault();
	fiscal1.setKkt(4);
	fiscal1.setKktInterface(7);
	fiscal1.setKktAddr("1.2.3.4");
	fiscal1.setKktPort(1234);
	fiscal1.setOfdAddr("2.3.4.5");
	fiscal1.setOfdPort(2345);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, fiscal1.save(&memory));

	// convertion
	ConfigFiscal1Converter converter;
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, converter.load(&memory));
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, converter.save(&memory));

	// check
	Config2Fiscal fiscal2;
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, fiscal2.load(&memory));
	TEST_NUMBER_EQUAL(4, fiscal2.getKkt());
	TEST_NUMBER_EQUAL(7, fiscal2.getKktInterface());
	TEST_STRING_EQUAL("1.2.3.4", fiscal2.getKktAddr());
	TEST_NUMBER_EQUAL(1234, fiscal2.getKktPort());
	TEST_STRING_EQUAL("2.3.4.5", fiscal2.getOfdAddr());
	TEST_NUMBER_EQUAL(2345, fiscal2.getOfdPort());
	return true;
}
