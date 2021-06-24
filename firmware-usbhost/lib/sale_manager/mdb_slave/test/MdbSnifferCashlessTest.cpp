#include "lib/sale_manager/mdb_slave/MdbSnifferCashless.h"

#include "common/mdb/slave/MdbSlaveTester.h"
#include "common/utils/include/Hex.h"
#include "common/timer/include/TestRealTime.h"
#include "common/event/include/TestEventEngine.h"
#include "common/logger/include/Logger.h"
#include "common/test/include/Test.h"

class TestSnifferCashlessEventEngine : public TestEventEngine {
public:
	TestSnifferCashlessEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case MdbSnifferCashless::Event_Enable: procEvent(envelope, MdbSnifferCashless::Event_Enable, "Enable"); break;
		case MdbSnifferCashless::Event_Disable: procEvent(envelope, MdbSnifferCashless::Event_Disable, "Disable"); break;
		case MdbSnifferCashless::Event_VendComplete: procEventVendComplete(envelope); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}

private:
	void procEventVendComplete(EventEnvelope *envelope) {
		MdbSnifferCashless::EventVend event;
		if(event.open(envelope) == false) { return; }
		*result << "<event=VendComplete," << event.getProductId() << "," << event.getPrice() << ">";
	}
};

class MdbSnifferCashlessTest : public TestSet {
public:
	MdbSnifferCashlessTest();
	bool init();
	void cleanup();
	bool testEnableDisable();
	bool testVendRequestL1();
	bool testExpansionIdInPoll();
	bool testVendRequestL3();

private:
	StringBuilder *result;
	TestRealTime *realtime;
	TestSnifferCashlessEventEngine *eventEngine;
	Mdb::DeviceContext *context;
	MdbSlaveTester *tester;
	MdbSnifferCashless *sniffer;

	bool gotoStateSale();
	void recvResponse(const char *hex, bool crc);
};

TEST_SET_REGISTER(MdbSnifferCashlessTest);

MdbSnifferCashlessTest::MdbSnifferCashlessTest() {
	TEST_CASE_REGISTER(MdbSnifferCashlessTest, testEnableDisable);
	TEST_CASE_REGISTER(MdbSnifferCashlessTest, testVendRequestL1);
	TEST_CASE_REGISTER(MdbSnifferCashlessTest, testExpansionIdInPoll);
	TEST_CASE_REGISTER(MdbSnifferCashlessTest, testVendRequestL3);
}

bool MdbSnifferCashlessTest::init() {
	result = new StringBuilder;
	realtime = new TestRealTime;
	eventEngine = new TestSnifferCashlessEventEngine(result);
	context = new Mdb::DeviceContext(2, realtime);
	sniffer = new MdbSnifferCashless(Mdb::Device_CashlessDevice2, context, eventEngine);
	tester = new MdbSlaveTester(sniffer);
	sniffer->reset();
	return true;
}

void MdbSnifferCashlessTest::cleanup() {
	delete tester;
	delete sniffer;
	delete context;
	delete eventEngine;
	delete realtime;
	delete result;
}

void MdbSnifferCashlessTest::recvResponse(const char *hex, bool crc) {
	uint8_t buf[256];
	uint16_t len =  hexToData(hex, strlen(hex), buf, sizeof(buf));
	sniffer->procResponse(buf, len ,crc);
}

bool MdbSnifferCashlessTest::gotoStateSale() {
	TEST_STRING_EQUAL("   ", context->getManufacturer());
	TEST_STRING_EQUAL("            ", context->getModel());
	TEST_STRING_EQUAL("            ", context->getSerialNumber());
	TEST_NUMBER_EQUAL(1643, context->getCurrency());
	TEST_NUMBER_EQUAL(2, context->getDecimalPoint());

	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("10"));
	recvResponse("00", true);

	// Setup Config
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110002000000"));
	recvResponse("010118100A01140B", true);
	tester->recvConfirm(0x00);

	// Setup Max Prices
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1101FFFF0000"));
	recvResponse("00", false);

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("17004546523031323334353637383941423031323334353637383941420100"));
	recvResponse("094546523031323334353637383941423031323334353637383941423031", true);
	tester->recvConfirm(0x00);

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("00", false);

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1401"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();
	return true;
}

bool MdbSnifferCashlessTest::testEnableDisable() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Disable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1400"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,Disable>", result->getString());
	result->clear();

	// Disable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1400"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("", result->getString());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1401"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1401"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool MdbSnifferCashlessTest::testVendRequestL1() {
	TEST_STRING_EQUAL("   ", context->getManufacturer());
	TEST_STRING_EQUAL("            ", context->getModel());
	TEST_STRING_EQUAL("            ", context->getSerialNumber());
	TEST_NUMBER_EQUAL(1643, context->getCurrency());
	TEST_NUMBER_EQUAL(2, context->getDecimalPoint());

	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("10"));
	recvResponse("00", true);

	// Setup Config
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110002000000"));
	recvResponse("010118100A01140B", true);
	tester->recvConfirm(0x00);

	// Setup Max Prices
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1101FFFF0000"));
	recvResponse("00", false);

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("17004546523031323334353637383941423031323334353637383941420100"));
	recvResponse("094546523031323334353637383941423031323334353637383941423031", true);
	tester->recvConfirm(0x00);

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("00", false);

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1401"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("00", false);

	// Session start
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("0307D0FF", true);
	tester->recvConfirm(0x00);

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("00", false);

	// Vend request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1300000A0006"));
	recvResponse("00", false);

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("00", false);

	// Vend approved
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("05000A", true);
	tester->recvConfirm(0x00);

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("13020006"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=VendComplete,6,1000>", result->getString());

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1304"));
	recvResponse("00", false);

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("00", false);

	// Poll (Session end)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("07", true);
	tester->recvConfirm(0x00);

	TEST_STRING_EQUAL("EFR", context->getManufacturer());
	TEST_STRING_EQUAL("0123456789AB", context->getModel());
	TEST_STRING_EQUAL("0123456789AB", context->getSerialNumber());
	TEST_NUMBER_EQUAL(1810, context->getCurrency());
	TEST_NUMBER_EQUAL(1, context->getDecimalPoint());
	return true;
}

bool MdbSnifferCashlessTest::testExpansionIdInPoll() {
	TEST_STRING_EQUAL("   ", context->getManufacturer());
	TEST_STRING_EQUAL("            ", context->getModel());
	TEST_STRING_EQUAL("            ", context->getSerialNumber());
	TEST_NUMBER_EQUAL(1643, context->getCurrency());
	TEST_NUMBER_EQUAL(2, context->getDecimalPoint());

	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("10"));
	recvResponse("00", true);

	// Setup Config
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110002000000"));
	recvResponse("010118100A01140B", true);
	tester->recvConfirm(0x00);

	// Setup Max Prices
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1101FFFF0000"));
	recvResponse("00", false);

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("17004546523031323334353637383941423031323334353637383941420100"));
	recvResponse("00", false);

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("094546523031323334353637383941423031323334353637383941423031", true);
	tester->recvConfirm(0x00);

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("00", false);

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1401"));
	recvResponse("00", false);

	TEST_STRING_EQUAL("EFR", context->getManufacturer());
	TEST_STRING_EQUAL("0123456789AB", context->getModel());
	TEST_STRING_EQUAL("0123456789AB", context->getSerialNumber());
	TEST_NUMBER_EQUAL(1810, context->getCurrency());
	TEST_NUMBER_EQUAL(1, context->getDecimalPoint());
	return true;
}

bool MdbSnifferCashlessTest::testVendRequestL3() {
	TEST_STRING_EQUAL("   ", context->getManufacturer());
	TEST_STRING_EQUAL("            ", context->getModel());
	TEST_STRING_EQUAL("            ", context->getSerialNumber());
	TEST_NUMBER_EQUAL(1643, context->getCurrency());
	TEST_NUMBER_EQUAL(2, context->getDecimalPoint());

	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("10"));
	recvResponse("00", true);

	// Setup Config
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110002000000"));
	recvResponse("010116430A01140B", true);
	tester->recvConfirm(0x00);

	// Setup Max/Min Prices L3
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1101FFFFFFFF000000001643"));
	recvResponse("00", false);

	// ExpansionIdentification L3
	TEST_NUMBER_EQUAL(true, tester->recvCommand("17004546523031323334353637383941423031323334353637383941420100"));
	recvResponse("09454652303132333435363738394142303132333435363738394142303100000000", true);
	tester->recvConfirm(0x00);

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("00", false);

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1401"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("00", false);

	// Session start L3
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("0307D0FFFFFFFF000000", true);
	tester->recvConfirm(0x00);

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("00", false);

	// Vend request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1300000A0006"));
	recvResponse("00", false);

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("00", false);

	// Vend approved
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("05000A", true);
	tester->recvConfirm(0x00);

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("13020006"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=VendComplete,6,1000>", result->getString());
	result->clear();

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1304"));
	recvResponse("00", false);

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("00", false);

	// Poll (Session end)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	recvResponse("07", true);
	tester->recvConfirm(0x00);

	TEST_STRING_EQUAL("EFR", context->getManufacturer());
	TEST_STRING_EQUAL("0123456789AB", context->getModel());
	TEST_STRING_EQUAL("0123456789AB", context->getSerialNumber());
	TEST_NUMBER_EQUAL(1643, context->getCurrency());
	TEST_NUMBER_EQUAL(1, context->getDecimalPoint());
	return true;
}
