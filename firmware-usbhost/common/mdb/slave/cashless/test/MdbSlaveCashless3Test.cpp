#include "mdb/slave/cashless/MdbSlaveCashless3.h"
#include "mdb/slave/MdbSlaveTester.h"
#include "mdb/master/cashless/MdbMasterCashless.h"
#include "timer/include/TestRealTime.h"
#include "event/include/TestEventEngine.h"
#include "timer/include/TimerEngine.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class TestSlaveCashless3EventEngine : public TestEventEngine {
public:
	TestSlaveCashless3EventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case MdbSlaveCashlessInterface::Event_Reset: procEvent(envelope, MdbSlaveCashlessInterface::Event_Reset, "Reset"); break;
		case MdbSlaveCashlessInterface::Event_Enable: procEvent(envelope, MdbSlaveCashlessInterface::Event_Enable, "Enable"); break;
		case MdbSlaveCashlessInterface::Event_Disable: procEvent(envelope, MdbSlaveCashlessInterface::Event_Disable, "Disable"); break;
		case MdbSlaveCashlessInterface::Event_VendRequest: procEventVendRequest(envelope, MdbSlaveCashlessInterface::Event_VendRequest, "VendRequest"); break;
		case MdbSlaveCashlessInterface::Event_VendComplete: procEvent(envelope, MdbSlaveCashlessInterface::Event_VendComplete, "VendComplete"); break;
		case MdbSlaveCashlessInterface::Event_VendCancel: procEvent(envelope, MdbSlaveCashlessInterface::Event_Disable, "VendCancel"); break;
		case MdbSlaveCashlessInterface::Event_CashSale: procEventVendRequest(envelope, MdbSlaveCashlessInterface::Event_CashSale, "CashSale"); break;
		case MdbSlaveCashlessInterface::Event_Error: procEventError(envelope, MdbSlaveCashlessInterface::Event_Error, "Error"); break;
		case MdbSlaveCashlessInterface::Event_SessionClosed: procEvent(envelope, MdbSlaveCashlessInterface::Event_SessionClosed, "SessionClosed"); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}

private:
	void procEventVendRequest(EventEnvelope *envelope, uint16_t type, const char *name) {
		MdbSlaveCashlessInterface::EventVendRequest event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.getProductId() << "," << event.getPrice() << ">";
	}

	void procEventError(EventEnvelope *envelope, uint16_t type, const char *name) {
		Mdb::EventError event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.code << "," << event.data.getString() << ">";
	}
};

class MdbSlaveCashless3Test : public TestSet {
public:
	MdbSlaveCashless3Test();
	bool init();
	void cleanup();
	bool testLevel1Init();
	bool gotoLevel1StateEnabled();
	bool testVend();
	bool testSelfReset();
	bool testDenyAndSetCredit();
	bool testSessionComplete();
	bool testSessionEndMiss();
	bool testStateSessionIdleEventRevalueLimit();
	bool testStateSessionCancelEventVendRequest();
	bool testStateSessionIdleEventCancelVend();
	bool testStateVendApprovingEventCancelVend();
	bool testStateVendingTimeout();
	bool testCashSale();
	bool testStateResetUnsupportedPacket();
	bool testStateResetUnwaitedPacket();
	bool testStateSessionIdleCommandDisable();
	bool testStateSessionEndCommandDisable();
	bool testStateEnabledCommandRequestCancel();
	bool testL3Init();
	bool gotoL3StateEnabled();
	bool testL3Vend();
	bool testL3AlwaysIdle();
	bool gotoECStateEnabled();
	bool testECVend();
	bool testPepsiIgnoreSessionCancelRequest();

private:
	StringBuilder *result;
	TestRealTime *realtime;
	TimerEngine *timerEngine;
	Mdb::DeviceContext *automat;
	TestSlaveCashless3EventEngine *eventEngine;
	StatStorage *stat;
	MdbSlaveCashless3 *slave;
	MdbSlaveTester *tester;
};

TEST_SET_REGISTER(MdbSlaveCashless3Test);

MdbSlaveCashless3Test::MdbSlaveCashless3Test() {
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testLevel1Init);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testVend);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testSelfReset);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testDenyAndSetCredit);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testSessionComplete);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testSessionEndMiss);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testStateSessionIdleEventRevalueLimit);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testStateSessionCancelEventVendRequest);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testStateSessionIdleEventCancelVend);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testStateVendApprovingEventCancelVend);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testStateVendingTimeout);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testCashSale);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testStateResetUnsupportedPacket);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testStateResetUnwaitedPacket);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testStateSessionIdleCommandDisable);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testStateSessionEndCommandDisable);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testStateEnabledCommandRequestCancel);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testL3Init);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testL3Vend);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testL3AlwaysIdle);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testECVend);
	TEST_CASE_REGISTER(MdbSlaveCashless3Test, testPepsiIgnoreSessionCancelRequest);
}

bool MdbSlaveCashless3Test::init() {
	result = new StringBuilder;
	realtime = new TestRealTime;
	timerEngine = new TimerEngine;
	automat = new Mdb::DeviceContext(2, realtime);
	automat->init(1, 5);
	automat->setCurrency(RUSSIAN_CURRENCY_RUB);
	eventEngine = new TestSlaveCashless3EventEngine(result);
	stat = new StatStorage;
	slave = new MdbSlaveCashless3(Mdb::Device_CashlessDevice1, 3, automat, timerEngine, eventEngine, stat);
	tester = new MdbSlaveTester(slave);
	slave->reset();
	return true;
}

void MdbSlaveCashless3Test::cleanup() {
	delete tester;
	delete slave;
	delete stat;
	delete eventEngine;
	delete realtime;
	delete result;
}

bool MdbSlaveCashless3Test::testLevel1Init() {
	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("10"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup Config
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110001100201"));
	TEST_HEXDATA_EQUAL("0101164305017808", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("110001100201"));
	TEST_HEXDATA_EQUAL("0101164305017808", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup Max Prices
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110100000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("17004A4F46202020202030303030313338202020472D323530000001004630"));
	TEST_HEXDATA_EQUAL("094546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("17004A4F46202020202030303030313338202020472D323530000001004630"));
	TEST_HEXDATA_EQUAL("094546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	TEST_STRING_EQUAL("JOF", automat->getManufacturer());
	TEST_STRING_EQUAL("   G-250", automat->getModel());
	TEST_STRING_EQUAL("     0000138", automat->getSerialNumber());
	TEST_NUMBER_EQUAL(4630, automat->getSoftwareVersion());
	return true;
}

bool MdbSlaveCashless3Test::testL3Init() {
	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("10"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup Config
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110003100201"));
	TEST_HEXDATA_EQUAL("0103164305017808", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("110003100201"));
	TEST_HEXDATA_EQUAL("0103164305017808", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup Max Prices
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110100000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("17004A4F46202020202030303030313338202020472D323530000001004630"));
	TEST_HEXDATA_EQUAL("09454652303132333435363738394142303132333435363738394142010000000020", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("17004A4F46202020202030303030313338202020472D323530000001004630"));
	TEST_HEXDATA_EQUAL("09454652303132333435363738394142303132333435363738394142010000000020", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionEnableOptions
	TEST_NUMBER_EQUAL(true, tester->recvCommand("170400000020"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	TEST_STRING_EQUAL("JOF", automat->getManufacturer());
	TEST_STRING_EQUAL("   G-250", automat->getModel());
	TEST_STRING_EQUAL("     0000138", automat->getSerialNumber());
	TEST_NUMBER_EQUAL(4630, automat->getSoftwareVersion());
	return true;
}

bool MdbSlaveCashless3Test::gotoLevel1StateEnabled() {
	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("10"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Reset>", result->getString());
	result->clear();

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup Config
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110002100201"));
	TEST_HEXDATA_EQUAL("0101164305017808", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup Max Prices
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110100000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("17004A4F46202020202030303030313338202020472D323530000001004630"));
	TEST_HEXDATA_EQUAL("094546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	return true;
}

bool MdbSlaveCashless3Test::gotoL3StateEnabled() {
	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("10"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Reset>", result->getString());
	result->clear();

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup Config
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110003100201"));
	TEST_HEXDATA_EQUAL("0103164305017808", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup Max Prices
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110100000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("17004A4F46202020202030303030313338202020472D323530000001004630"));
	TEST_HEXDATA_EQUAL("09454652303132333435363738394142303132333435363738394142010000000020", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionEnableOptions
	TEST_NUMBER_EQUAL(true, tester->recvCommand("170400000020"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	return true;
}

bool MdbSlaveCashless3Test::testL3Vend() {
	TEST_NUMBER_EQUAL(true, gotoL3StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Begin session
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032FFFFFFFF000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Vend request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0300000A0007"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendRequest,7,500>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Vend approved
	slave->approveVend(500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("05000A", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("13020006"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendComplete>", result->getString());
	result->clear();

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	// Session end
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbSlaveCashless3Test::testL3AlwaysIdle() {
	TEST_NUMBER_EQUAL(true, gotoL3StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Vend request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0300000A0007"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendRequest,7,500>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Vend approved
	slave->approveVend(500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("05000A", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("13020006"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendComplete>", result->getString());
	result->clear();

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	// Session end
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbSlaveCashless3Test::testVend() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Begin session
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Vend request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0300000A0007"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendRequest,7,500>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Vend approved
	slave->approveVend(500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("05000A", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("13020006"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendComplete>", result->getString());
	result->clear();

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	// Session end
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbSlaveCashless3Test::testSelfReset() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll after reset
	slave->reset();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	return true;
}

bool MdbSlaveCashless3Test::testDenyAndSetCredit() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Begin session
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Vend request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0300000A0007"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendRequest,7,500>", result->getString());
	result->clear();

	// Vend deny
	slave->denyVend(true);
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("06", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	// Session end
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbSlaveCashless3Test::testSessionComplete() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Begin session
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbSlaveCashless3Test::testSessionEndMiss() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Begin session
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	// Session end
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbSlaveCashless3Test::testStateSessionIdleEventRevalueLimit() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Begin session
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Revalue Limit
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1501"));
	TEST_HEXDATA_EQUAL("0F0000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Session cancel
	slave->cancelVend();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("04", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	// Session end
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbSlaveCashless3Test::testStateSessionCancelEventVendRequest() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Begin session
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session restart
	slave->setCredit(3500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0300000A0006"));
	TEST_HEXDATA_EQUAL("06", tester->getSendData(), tester->getSendDataLen());

	// Session cancel
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("04", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	// Session end
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbSlaveCashless3Test::testStateSessionIdleEventCancelVend() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Begin session
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Cancel vend
	slave->cancelVend();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("04", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	// Session end
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbSlaveCashless3Test::testStateVendApprovingEventCancelVend() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Begin session
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0300000A0006"));
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Cancel vend
	slave->cancelVend();

	// Session cancel
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("04", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	// Session end
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbSlaveCashless3Test::testStateVendingTimeout() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Begin session
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Vend request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0300000A0007"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendRequest,7,500>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Vend approved
	slave->approveVend(500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("05000A", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Vending timeout
	timerEngine->tick(MDB_CL_VENDING_TIMEOUT);
	timerEngine->execute();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("04", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	// Session end
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbSlaveCashless3Test::testCashSale() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// CashSale
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0305012C001E"));
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("<event=1,CashSale,30,15000>", result->getString());
	result->clear();
	return true;
}

bool MdbSlaveCashless3Test::testStateResetUnsupportedPacket() {
	// Unsupproted packet
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0FAA"));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendDataLen());
	TEST_STRING_EQUAL("<event=1,Error,1283,scl1*16*07AA*>", result->getString());
	return true;
}

bool MdbSlaveCashless3Test::testStateResetUnwaitedPacket() {
	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendDataLen());
	TEST_STRING_EQUAL("<event=1,Error,1283,scl1*16*0401*0401>", result->getString());
	return true;
}

bool MdbSlaveCashless3Test::testStateSessionIdleCommandDisable() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Begin session
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Disable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0400"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Check state
	TEST_NUMBER_EQUAL(false, slave->isEnable());
	return true;
}

bool MdbSlaveCashless3Test::testStateSessionEndCommandDisable() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Begin session
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Cancel vend
	slave->cancelVend();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("04", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	// Disable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0400"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Session end
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("<event=1,SessionClosed>", result->getString());
	result->clear();

	// Begin session
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbSlaveCashless3Test::testStateEnabledCommandRequestCancel() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// RequestCancel
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0402"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("08", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	return true;
}

bool MdbSlaveCashless3Test::gotoECStateEnabled() {
	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("10"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Reset>", result->getString());
	result->clear();

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup Config
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110003100201"));
	TEST_HEXDATA_EQUAL("0103164305017808", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("17004A4F46202020202030303030313338202020472D323530000001004630"));
	TEST_HEXDATA_EQUAL("09454652303132333435363738394142303132333435363738394142010000000020", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionEnableOptions
	TEST_NUMBER_EQUAL(true, tester->recvCommand("170400000022"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Setup Max/Min Prices
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1101FFFFFFFF000000001643"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	return true;
}

bool MdbSlaveCashless3Test::testECVend() {
	TEST_NUMBER_EQUAL(true, gotoECStateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Begin session
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("0300000032FFFFFFFF0000007275164300", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Vend request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("03000000000A0007"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendRequest,7,500>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Vend approved
	slave->approveVend(500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("05000A", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("13020006"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendComplete>", result->getString());
	result->clear();

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	// Session end
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

// Тест не дописан. Автомат pepsi иногда игорирует CancelSessionRequest и шлет запросы POLL неприрывно.
bool MdbSlaveCashless3Test::testPepsiIgnoreSessionCancelRequest() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Begin session
	slave->setCredit(2500);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030032", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Vend request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0300000A0007"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendRequest,7,500>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll
	slave->cancelVend();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("04", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	slave->cancelVend();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("04", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	slave->cancelVend();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("04", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	// Session end
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}
