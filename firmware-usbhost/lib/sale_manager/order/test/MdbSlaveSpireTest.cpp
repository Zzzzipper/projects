#include "lib/sale_manager/order/MdbSlaveSpire.h"
#include "mdb/slave/MdbSlaveTester.h"
#include "timer/include/TestRealTime.h"
#include "event/include/TestEventEngine.h"
#include "timer/include/TimerEngine.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class TestMdbSlaveSpireEventEngine : public TestEventEngine {
public:
	TestMdbSlaveSpireEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case OrderDeviceInterface::Event_PinCodeCompleted: procEvent(envelope, OrderDeviceInterface::Event_PinCodeCompleted, "PinCodeCompleted"); break;
		case OrderDeviceInterface::Event_PinCodeCancelled: procEvent(envelope, OrderDeviceInterface::Event_PinCodeCancelled, "PinCodeCancelled"); break;
		case OrderDeviceInterface::Event_VendRequest: procEventVendRequest(envelope, OrderDeviceInterface::Event_VendRequest, "VendRequest"); break;
		case OrderDeviceInterface::Event_VendCompleted: procEvent(envelope, OrderDeviceInterface::Event_VendCompleted, "VendCompleted"); break;
		case OrderDeviceInterface::Event_VendCancelled: procEvent(envelope, OrderDeviceInterface::Event_VendCancelled, "VendCancelled"); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}

private:
	void procEventVendRequest(EventEnvelope *envelope, uint16_t type, const char *name) {
		EventUint16Interface event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.getValue() << ">";
	}
};

class MdbSlaveSpireTest : public TestSet {
public:
	MdbSlaveSpireTest();
	bool init();
	void cleanup();
	bool testLevel1Init();
	bool gotoLevel1StateEnabled();
	bool gotoLevel3StateEnabled();
	bool testOrder();
	bool testOrderFailed();
	bool testPinCode();
	bool testPinCodeSpireError();
	bool testPinCodeCancelled();

private:
	StringBuilder *result;
	TestRealTime *realtime;
	TimerEngine *timerEngine;
	Mdb::DeviceContext *automat;
	TestMdbSlaveSpireEventEngine *eventEngine;
	StatStorage *stat;
	Order *order;
	MdbSlaveSpire *slave;
	MdbSlaveTester *tester;
};

TEST_SET_REGISTER(MdbSlaveSpireTest);

MdbSlaveSpireTest::MdbSlaveSpireTest() {
	TEST_CASE_REGISTER(MdbSlaveSpireTest, testLevel1Init);
	TEST_CASE_REGISTER(MdbSlaveSpireTest, testOrder);
	TEST_CASE_REGISTER(MdbSlaveSpireTest, testOrderFailed);
	TEST_CASE_REGISTER(MdbSlaveSpireTest, testPinCode);
	TEST_CASE_REGISTER(MdbSlaveSpireTest, testPinCodeSpireError);
	TEST_CASE_REGISTER(MdbSlaveSpireTest, testPinCodeCancelled);
}

bool MdbSlaveSpireTest::init() {
	result = new StringBuilder;
	realtime = new TestRealTime;
	timerEngine = new TimerEngine;
	automat = new Mdb::DeviceContext(2, realtime);
	automat->init(1, 5);
	automat->setCurrency(RUSSIAN_CURRENCY_RUB);
	eventEngine = new TestMdbSlaveSpireEventEngine(result);
	stat = new StatStorage;
	order = new Order;
	slave = new MdbSlaveSpire(Mdb::Device_CashlessDevice1, 3, automat, timerEngine, eventEngine, stat);
	tester = new MdbSlaveTester(slave);
	slave->setOrder(order);
	slave->reset();
	return true;
}

void MdbSlaveSpireTest::cleanup() {
	delete tester;
	delete slave;
	delete order;
	delete stat;
	delete eventEngine;
	delete realtime;
	delete result;
}

bool MdbSlaveSpireTest::testLevel1Init() {
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
	TEST_HEXDATA_EQUAL("0101164305017800", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("110001100201"));
	TEST_HEXDATA_EQUAL("0101164305017800", tester->getSendData(), tester->getSendDataLen());
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

bool MdbSlaveSpireTest::gotoLevel1StateEnabled() {
	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("10"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup Config
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110002100201"));
	TEST_HEXDATA_EQUAL("0101164305017800", tester->getSendData(), tester->getSendDataLen());
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

bool MdbSlaveSpireTest::gotoLevel3StateEnabled() {
	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("10"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup Config
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110003100201"));
	TEST_HEXDATA_EQUAL("0103164305017800", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup Max Prices
	TEST_NUMBER_EQUAL(true, tester->recvCommand("110100000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("17004A4F46202020202030303030313338202020472D323530000001004630"));
	TEST_HEXDATA_EQUAL("09454652303132333435363738394142303132333435363738394142010080000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionEnableOptions
	TEST_NUMBER_EQUAL(true, tester->recvCommand("170400000020"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	return true;
}

bool MdbSlaveSpireTest::testOrder() {
	TEST_NUMBER_EQUAL(true, gotoLevel3StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Begin session
	slave->enable();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030002FFFFFFFF000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// -------------------------
	// First order first product
	// -------------------------
	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311000000000000000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendRequest,0>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Order approved
	order->add(1, 1);
	order->add(2, 1);
	slave->approveVend();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("FE000101", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("13020006"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendCompleted>", result->getString());
	result->clear();

	// Poll
	order->remove(1);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// --------------------------
	// First order second product
	// --------------------------
	slave->approveVend();

	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311000000000000000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Order approved
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("FE000200", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("13020006"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendCompleted>", result->getString());
	result->clear();

	// Poll
	order->remove(2);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// ---------------
	// First order end
	// ---------------
	slave->denyVend();

	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311000000000000000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Vend deny
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

	// Begin session
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030002FFFFFFFF000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	return true;
}

bool MdbSlaveSpireTest::testOrderFailed() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Begin session
	slave->enable();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030002", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// -------------------------
	// First order first product
	// -------------------------
	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311000000000000000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendRequest,0>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Order approved
	order->add(1, 1);
	order->add(2, 1);
	slave->approveVend();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("FE000101", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("13020006"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendCompleted>", result->getString());
	result->clear();

	// Poll
	order->remove(1);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// --------------------------
	// First order second product
	// --------------------------
	slave->approveVend();
	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311000000000000000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Order approved
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("FE000200", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("1303"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendCancelled>", result->getString());
	result->clear();

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

	// Begin session
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030002", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	return true;
}

bool MdbSlaveSpireTest::testPinCode() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Begin session
	slave->enable();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030002", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// ------------
	// Need pincode
	// ------------
	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311000000000000000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendRequest,0>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Need pincode
	slave->requestPinCode();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("060A33", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// -------------------------
	// First order first product
	// -------------------------
	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311043132333400000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,PinCodeCompleted>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Order approved
	order->add(1, 1);
	order->add(2, 1);
	slave->approveVend();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("FE000101", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("13020006"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendCompleted>", result->getString());
	result->clear();

	// Poll
	order->remove(1);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// --------------------------
	// First order second product
	// --------------------------
	slave->approveVend();

	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311000000000000000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Order approved
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("FE000200", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("13020006"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendCompleted>", result->getString());
	result->clear();

	// Poll
	order->remove(2);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// ---------------
	// First order end
	// ---------------
	slave->denyVend();

	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311000000000000000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Vend deny
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

	// Begin session
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030002", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	return true;
}

bool MdbSlaveSpireTest::testPinCodeSpireError() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Begin session
	slave->enable();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030002", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// ------------
	// Need pincode
	// ------------
	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311000000000000000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendRequest,0>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Need pincode
	slave->requestPinCode();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("060A33", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// -------------------------
	// First order first product
	// -------------------------
	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311043132333400000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,PinCodeCompleted>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Order approved
	order->add(1, 1);
	order->add(2, 1);
	slave->approveVend();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("FE000101", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("13020006"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendCompleted>", result->getString());
	result->clear();

	// Session complete
	order->remove(1);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	// Session end
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Begin session
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030002", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// --------------------------
	// First order second product
	// --------------------------
	slave->approveVend();

	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311000000000000000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Order approved
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("FE000200", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("13020006"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendCompleted>", result->getString());
	result->clear();

	// Session complete
	order->remove(2);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0304"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());

	// Session end
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("07", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Begin session
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030002", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ---------------
	// First order end
	// ---------------
	slave->denyVend();

	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311000000000000000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Vend deny
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

	// Begin session
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030002", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	return true;
}

bool MdbSlaveSpireTest::testPinCodeCancelled() {
	TEST_NUMBER_EQUAL(true, gotoLevel1StateEnabled());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0401"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Begin session
	slave->enable();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030002", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// ------------
	// Need pincode
	// ------------
	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311000000000000000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendRequest,0>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Need pincode
	slave->requestPinCode();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("060A33", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// -------------------------
	// First order first product
	// -------------------------
	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311043132333400000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,PinCodeCompleted>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Order approved
	order->add(1, 1);
	order->add(2, 1);
	slave->approveVend();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("FE000101", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("13020006"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendCompleted>", result->getString());
	result->clear();

	// Poll
	order->remove(1);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// --------------------------
	// First order second product
	// --------------------------
	slave->approveVend();

	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311000000000000000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Order approved
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("FE000200", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Vend complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("13020006"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,VendCompleted>", result->getString());
	result->clear();

	// Poll
	order->remove(2);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// ---------------
	// First order end
	// ---------------
	slave->denyVend();

	// Order request
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0311000000000000000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Vend deny
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

	// Begin session
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("030002", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	return true;
}
