#include "mdb/slave/com_gateway/MdbSlaveComGateway.h"
#include "mdb/slave/MdbSlaveTester.h"
#include "timer/include/TestRealTime.h"
#include "event/include/TestEventEngine.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class TestSlaveComGatewayEventEngine : public TestEventEngine {
public:
	TestSlaveComGatewayEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case MdbSlaveComGateway::Event_Reset: procEvent(envelope, MdbSlaveComGateway::Event_Reset, "Reset"); break;
		case MdbSlaveComGateway::Event_ReportTranscation: procReportTransaction(envelope, MdbSlaveComGateway::Event_ReportTranscation, "ReportTranscation"); break;
		case MdbSlaveComGateway::Event_ReportEvent: procReportEvent(envelope, MdbSlaveComGateway::Event_ReportEvent, "ReportEvent"); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}

private:
	void procReportTransaction(EventEnvelope *envelope, uint16_t type, const char *name) {
		MdbSlaveComGateway::ReportTransaction event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { return; }
		*result << "<event=" << event.getDevice() << "," << name << ","
				<< event.data.transactionType << ","
				<< event.data.itemNumber << ","
				<< event.data.price << ","
				<< event.data.cashInCoinTubes << ","
				<< event.data.cashInCashbox << ","
				<< event.data.cashInBills << ","
				<< event.data.valueInCashless1 << ","
				<< event.data.valueInCashless2 << ","
				<< event.data.revalueInCashless1 << ","
				<< event.data.revalueInCashless2 << ","
				<< event.data.cashOut << ","
				<< event.data.discountAmount << ","
				<< event.data.surchargeAmount << ">";
	}

	void procReportEvent(EventEnvelope *envelope, uint16_t type, const char *name) {
		MdbSlaveComGateway::ReportEvent event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { return; }
		*result << "<event=" << event.getDevice() << "," << name << ","
				<< event.data.code.get() << ","
				<< LOG_DATETIME(event.data.datetime) << ","
				<< event.data.duration << ","
				<< event.data.activity << ">";
	}
};

class MdbSlaveComGatewayTest : public TestSet {
public:
	MdbSlaveComGatewayTest();
	bool init();
	void cleanup();
	bool testInit();
	bool gotoStateEnabled();
	bool testDtsEvent();

private:
	StringBuilder *result;
	TestRealTime *realtime;
	Mdb::DeviceContext *automat;
	TestSlaveComGatewayEventEngine *eventEngine;
	MdbSlaveComGateway *slave;
	MdbSlaveTester *tester;
};

TEST_SET_REGISTER(MdbSlaveComGatewayTest);

MdbSlaveComGatewayTest::MdbSlaveComGatewayTest() {
	TEST_CASE_REGISTER(MdbSlaveComGatewayTest, testInit);
	TEST_CASE_REGISTER(MdbSlaveComGatewayTest, testDtsEvent);
}

bool MdbSlaveComGatewayTest::init() {
	result = new StringBuilder;
	realtime = new TestRealTime;
	automat = new Mdb::DeviceContext(2, realtime);
	automat->init(1, 5);
	automat->setCurrency(RUSSIAN_CURRENCY_RUB);
	eventEngine = new TestSlaveComGatewayEventEngine(result);
	slave = new MdbSlaveComGateway(automat, eventEngine);
	tester = new MdbSlaveTester(slave);
	slave->reset();
	return true;
}

void MdbSlaveComGatewayTest::cleanup() {
	delete tester;
	delete slave;
	delete eventEngine;
	delete realtime;
	delete result;
}

bool MdbSlaveComGatewayTest::testInit() {
	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("18"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Reset>", result->getString());
	result->clear();

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1A"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("1A"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Setup Config
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x19x01x01x01"));
	TEST_HEXDATA_EQUAL("01010005", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("x19x01x01x01"));
	TEST_HEXDATA_EQUAL("01010005", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Fx00"));
	TEST_HEXDATA_EQUAL("06454652303132333435363738394142303132333435363738394142010000000002", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Fx00"));
	TEST_HEXDATA_EQUAL("06454652303132333435363738394142303132333435363738394142010000000002", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// ExpansionFeatureEnbale
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Fx01x00x00x00x02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Control/Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Cx01"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1A"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Report sale by coins
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Bx01x01x24x00x00xC8x00xC8x00x00x00x00x00x00x00x00x00x00x00x00x00x00x00x00x00x00x00x00x20x19x03x31x20x34"));
/*
uint8_t command;x1B
uint8_t subcommand;x01
uint8_t transactionType;x01
LEUint2 itemNumber;x24x00
LEUint2 price;x00xC8
LEUint2 cashInCoinTubes;x00xC8
LEUint2 cashInCashbox;
LEUint2 cashInBills;
LEUint2 valueInCashless1;
LEUint2 valueInCashless2;
LEUint2 revalueInCashless1;
LEUint2 revalueInCashless2;
LEUint2 cashOut;
LEUint2 discountAmount;
LEUint2 surchargeAmount;
uint8_t userGroup;
uint8_t priceList;
uint8_t date[4];x20x19x03x31
uint8_t time[2];x20x34
 */
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,ReportTranscation,1,9216,2000,2000,0,0,0,0,0,0,0,0,0>", result->getString());
	result->clear();

	// Report sale by bills with change
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Bx01x01x00x21x01x5Ex00x00x00x00x03xE8x00x00x00x00x00x00x00x00x02x8Ax00x00x00x00x00x00x20x19x03x31x21x00"));
/*
uint8_t command;x1B
uint8_t subcommand;x01
uint8_t transactionType;x01
LEUint2 itemNumber;x00x21
LEUint2 price;x01x5E
LEUint2 cashInCoinTubes;
LEUint2 cashInCashbox;
LEUint2 cashInBills;x03xE8
LEUint2 valueInCashless1;
LEUint2 valueInCashless2;
LEUint2 revalueInCashless1;
LEUint2 revalueInCashless2;
LEUint2 cashOut;x02x8A
LEUint2 discountAmount;
LEUint2 surchargeAmount;
uint8_t userGroup;
uint8_t priceList;
uint8_t date[4];x20x19x03x31
uint8_t time[2];x21x00
 */
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,ReportTranscation,1,33,3500,0,0,10000,0,0,0,0,6500,0,0>", result->getString());
	result->clear();

	// Report sale by cashless
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Bx01x01x00x21x01x5Ex00x00x00x00x00x00x00x00x03xE8x00x00x00x00x00x00x00x00x00x00x00x00x20x19x03x31x21x00"));
/*
uint8_t command;x1B
uint8_t subcommand;x01
uint8_t transactionType;x01
LEUint2 itemNumber;x00x21
LEUint2 price;x01x5E
LEUint2 cashInCoinTubes;
LEUint2 cashInCashbox;
LEUint2 cashInBills;
LEUint2 valueInCashless1;
LEUint2 valueInCashless2;x03xE8
LEUint2 revalueInCashless1;
LEUint2 revalueInCashless2;
LEUint2 cashOut;x02x8A
LEUint2 discountAmount;
LEUint2 surchargeAmount;
uint8_t userGroup;
uint8_t priceList;
uint8_t date[4];x20x19x03x31
uint8_t time[2];x21x00
 */
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,ReportTranscation,1,33,3500,0,0,0,0,10000,0,0,0,0,0>", result->getString());
	result->clear();

	// Report vendless
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1B01070002012C00000000000000000000000000000000000000000002202002181308"));
/*
uint8_t command;x1B
uint8_t subcommand;x01
uint8_t transactionType;x07
LEUint2 itemNumber;x00x02
LEUint2 price;x01x2C
LEUint2 cashInCoinTubes;
LEUint2 cashInCashbox;
LEUint2 cashInBills;
LEUint2 valueInCashless1;
LEUint2 valueInCashless2;
LEUint2 revalueInCashless1;
LEUint2 revalueInCashless2;
LEUint2 cashOut;
LEUint2 discountAmount;
LEUint2 surchargeAmount;
uint8_t userGroup;
uint8_t priceList;x02
uint8_t date[4];x20x20x02x18
uint8_t time[2];x13x08
 */
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,ReportTranscation,7,2,3000,0,0,0,0,0,0,0,0,0,0>", result->getString());
	result->clear();
	return true;
}

bool MdbSlaveComGatewayTest::gotoStateEnabled() {
	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("18"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1A"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup Config
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x19x01x01x01"));
	TEST_HEXDATA_EQUAL("01010005", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Fx00"));
	TEST_HEXDATA_EQUAL("06454652303132333435363738394142303132333435363738394142010000000002", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionFeatureEnbale
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Fx01x00x00x00x02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Control/Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Cx01"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1A"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	result->clear();
	return true;
}

bool MdbSlaveComGatewayTest::testDtsEvent() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Power outage detect (EA7=1)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Bx02x45x41x37x00x00x00x00x00x00x00x20x19x04x10x20x12x00x00x00x00x01"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,ReportEvent,EA7,2019.4.10 20:12:0,0,1>", result->getString());
	result->clear();

	// BillValidator (ENK=0)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Bx02x45x4Ex4Bx00x00x00x00x00x00x00x20x19x04x10x20x12x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,ReportEvent,ENK,2019.4.10 20:12:0,0,0>", result->getString());
	result->clear();

	// wft? (EA_01=1)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Bx02x45x41x5Fx30x31x00x00x00x00x00x20x19x04x10x20x12x00x00x00x00x01"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,ReportEvent,EA_01,2019.4.10 20:12:0,0,1>", result->getString());
	result->clear();

	// CoinChanger found (EAR=0)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Bx02x45x41x52x00x00x00x00x00x00x00x20x19x04x10x20x12x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,ReportEvent,EAR,2019.4.10 20:12:0,0,0>", result->getString());
	result->clear();

	// Cashless2 found (EK2M=0)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Bx02x45x4Bx32x4Dx00x00x00x00x00x00x20x19x04x10x20x12x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,ReportEvent,EK2M,2019.4.10 20:12:0,0,0>", result->getString());
	result->clear();

	// Water overflow (OIB=1)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Bx02x4Fx42x49x00x00x00x00x00x00x00x20x19x04x10x19x27x00x00x00x00x01"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,ReportEvent,OBI,2019.4.10 19:27:0,0,1>", result->getString());
	result->clear();

	// Water overflow (OIB=1)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Bx02x4Fx42x49x00x00x00x00x00x00x00x20x19x04x10x19x27x00x00x00x00x01"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,ReportEvent,OBI,2019.4.10 19:27:0,0,1>", result->getString());
	result->clear();

	// Water overflow (OIB=0)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("x1Bx02x4Fx42x49x00x00x00x00x00x00x00x20x19x04x10x19x27x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,ReportEvent,OBI,2019.4.10 19:27:0,0,0>", result->getString());
	result->clear();
	return true;
}
