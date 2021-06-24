#include "executive/include/ExeSniffer.h"
#include "uart/include/TestUart.h"
#include "event/include/TestEventEngine.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class ExeSnifferTest : public TestSet {
public:
	ExeSnifferTest();
	bool testVendOk();
	bool testVendError();
};

TEST_SET_REGISTER(ExeSnifferTest);

ExeSnifferTest::ExeSnifferTest() {
	TEST_CASE_REGISTER(ExeSnifferTest, testVendOk);
	TEST_CASE_REGISTER(ExeSnifferTest, testVendError);
}

bool ExeSnifferTest::testVendOk() {
	StringBuilder result;
	TestEventEngine eventEngine(&result);
	StatStorage stat;
	ExeSniffer sniffer(&eventEngine, &stat);
	TestUart masterUart(64);
	TestUart slaveUart(64);

	sniffer.init(&masterUart, &slaveUart);
	sniffer.reset();
	TEST_NUMBER_EQUAL(ExeSniffer::State_NotReady, sniffer.getState());

	// automat not ready
	masterUart.addRecvData("31");
	slaveUart.addRecvData("40");
	TEST_NUMBER_EQUAL(ExeSniffer::State_NotReady, sniffer.getState());

	masterUart.addRecvData("31");
	slaveUart.addRecvData("40");
	TEST_NUMBER_EQUAL(ExeSniffer::State_NotReady, sniffer.getState());

	// automat ready
	masterUart.addRecvData("31");
	slaveUart.addRecvData("00");
	TEST_NUMBER_EQUAL(ExeSniffer::State_Ready, sniffer.getState());

	masterUart.addRecvData("32");
	slaveUart.addRecvData("FE");
	TEST_NUMBER_EQUAL(ExeSniffer::State_Ready, sniffer.getState());

	masterUart.addRecvData("31");
	slaveUart.addRecvData("00");
	TEST_NUMBER_EQUAL(ExeSniffer::State_Ready, sniffer.getState());

	masterUart.addRecvData("32");
	slaveUart.addRecvData("FE");
	TEST_NUMBER_EQUAL(ExeSniffer::State_Ready, sniffer.getState());

	// credit 10 rub
	masterUart.addRecvData("38"); // Command = VMC:ACCEPT DATA
	slaveUart.addRecvData("00");
	masterUart.addRecvData("2A"); // Credit=0x000A
	slaveUart.addRecvData("00");
	masterUart.addRecvData("20");
	slaveUart.addRecvData("00");
	masterUart.addRecvData("20");
	slaveUart.addRecvData("00");
	masterUart.addRecvData("20");
	slaveUart.addRecvData("00");
	masterUart.addRecvData("21"); // ScallingFactor=0x01
	slaveUart.addRecvData("00");
	masterUart.addRecvData("20");
	slaveUart.addRecvData("00");
	masterUart.addRecvData("21"); // DecimalPoint=0x(0)1
	slaveUart.addRecvData("00");
	masterUart.addRecvData("21"); // ChangeStatus=0x(0)1 (наличие сдачи)
	slaveUart.addRecvData("00");
	masterUart.addRecvData("39"); // Command = VMC:DATA SYNC (конец передачи данных)
	slaveUart.addRecvData("00");

	masterUart.addRecvData("32"); // Command = VMC:CREDIT
	slaveUart.addRecvData("FE");
	TEST_NUMBER_EQUAL(ExeSniffer::State_Ready, sniffer.getState());

	// automat credit
	masterUart.addRecvData("31");
	slaveUart.addRecvData("00");
	TEST_NUMBER_EQUAL(ExeSniffer::State_Ready, sniffer.getState());

	masterUart.addRecvData("32");
	slaveUart.addRecvData("FE");
	TEST_NUMBER_EQUAL(ExeSniffer::State_Ready, sniffer.getState());

	masterUart.addRecvData("31");
	slaveUart.addRecvData("00");
	TEST_NUMBER_EQUAL(ExeSniffer::State_Ready, sniffer.getState());

	masterUart.addRecvData("32");
	slaveUart.addRecvData("FE");
	TEST_NUMBER_EQUAL(ExeSniffer::State_Ready, sniffer.getState());

	// vend request
	masterUart.addRecvData("32"); // Command = VMC:CREDIT, запрос продажи от VMC
	slaveUart.addRecvData("08");  // ProductLine=8 (PriceHolding)
	TEST_NUMBER_EQUAL(ExeSniffer::State_Approving, sniffer.getState());

	masterUart.addRecvData("38"); // Command = VMC:ACCEPT DATA
	slaveUart.addRecvData("00");
	masterUart.addRecvData("20"); // Credit=0x0000
	slaveUart.addRecvData("00");
	masterUart.addRecvData("20");
	slaveUart.addRecvData("00");
	masterUart.addRecvData("20");
	slaveUart.addRecvData("00");
	masterUart.addRecvData("20");
	slaveUart.addRecvData("00");
	masterUart.addRecvData("21"); // ScallingFactor=0x01
	slaveUart.addRecvData("00");
	masterUart.addRecvData("20");
	slaveUart.addRecvData("00");
	masterUart.addRecvData("21"); // DecimalPoint=0x(0)1
	slaveUart.addRecvData("00");
	masterUart.addRecvData("20"); // ChangeStatus=0x(0)0 (наличие сдачи)
	slaveUart.addRecvData("00");
	masterUart.addRecvData("39"); // Command = VMC:DATA SYNC (конец передачи данных)
	slaveUart.addRecvData("00");
	TEST_STRING_EQUAL("", result.getString());

	masterUart.addRecvData("33"); // Command = VMC:VEND, разрешение продажи VMC
	slaveUart.addRecvData("00");  // 7 бит - VEND SUCCEED
	TEST_NUMBER_EQUAL(ExeSniffer::State_Ready, sniffer.getState());
	TEST_STRING_EQUAL("<event=1,0x1001>", result.getString());
	result.clear();

	// vend selling
	masterUart.addRecvData("31");
	slaveUart.addRecvData("40");  // продажа невозможна (идет продажа)
	TEST_NUMBER_EQUAL(ExeSniffer::State_NotReady, sniffer.getState());

	masterUart.addRecvData("31");
	slaveUart.addRecvData("40");  // продажа невозможна (идет продажа)
	TEST_NUMBER_EQUAL(ExeSniffer::State_NotReady, sniffer.getState());

	// vend complete
	masterUart.addRecvData("31");
	slaveUart.addRecvData("00");  // продажа возможна
	TEST_NUMBER_EQUAL(ExeSniffer::State_Ready, sniffer.getState());

	masterUart.addRecvData("31");
	slaveUart.addRecvData("00");
	TEST_NUMBER_EQUAL(ExeSniffer::State_Ready, sniffer.getState());

	return true;
}

//todo: тест ошибки продажи
bool ExeSnifferTest::testVendError() {
	return true;
}
