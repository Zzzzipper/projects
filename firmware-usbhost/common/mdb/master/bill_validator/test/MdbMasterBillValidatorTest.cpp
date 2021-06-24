#include "mdb/master/bill_validator/MdbMasterBillValidator.h"
#include "mdb/master/MdbMasterTester.h"
#include "timer/include/TestRealTime.h"
#include "event/include/TestEventEngine.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class TestBillValidatorEventEngine : public TestEventEngine {
public:
	TestBillValidatorEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Ready: *result << "<event=Ready>"; break;
		case MdbMasterBillValidator::Event_Error: procEventUint16(envelope, MdbMasterBillValidator::Event_Error, "Error"); break;
		case MdbMasterBillValidator::Event_Deposite: procEventDeposite(envelope, MdbMasterBillValidator::Event_Deposite, "Deposite"); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}

	void procEventDeposite(EventEnvelope *envelope, uint16_t type, const char *name) {
		Mdb::EventDeposite event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { *result << "<event-pack-error>"; return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.getRoute() << "," << event.getNominal() << ">";
	}
};

class MdbMasterBillValidatorTest : public TestSet {
public:
	MdbMasterBillValidatorTest();
	bool init();
	void cleanup();
	bool testReset();
	bool testResetNoJustReset();
	bool testInit();
	bool testDeviceReboot();
	bool testDeviceJustReset();
	bool gotoStatePoll(MdbMasterTester *sender);
	bool testEnable();
	bool testDisable();
	bool testRemoveStacker();
	bool testBtDepositeBill();
	bool testBtEscrowError();
	bool testJsmDepositeBill();
	bool testAvroraDepositeBill();
	bool testScalingFactor1000();

private:
	StringBuilder *result;
	TestRealTime *realTime;
	TestBillValidatorEventEngine *eventEngine;
	MdbBillValidatorContext *context;
	MdbMasterBillValidator *bv;
	MdbMasterTester *tester;
};

TEST_SET_REGISTER(MdbMasterBillValidatorTest);

MdbMasterBillValidatorTest::MdbMasterBillValidatorTest() {
	TEST_CASE_REGISTER(MdbMasterBillValidatorTest, testReset);
	TEST_CASE_REGISTER(MdbMasterBillValidatorTest, testResetNoJustReset);
	TEST_CASE_REGISTER(MdbMasterBillValidatorTest, testInit);
	TEST_CASE_REGISTER(MdbMasterBillValidatorTest, testDeviceReboot);
	TEST_CASE_REGISTER(MdbMasterBillValidatorTest, testDeviceJustReset);
	TEST_CASE_REGISTER(MdbMasterBillValidatorTest, testEnable);
	TEST_CASE_REGISTER(MdbMasterBillValidatorTest, testDisable);
	TEST_CASE_REGISTER(MdbMasterBillValidatorTest, testRemoveStacker);
	TEST_CASE_REGISTER(MdbMasterBillValidatorTest, testBtDepositeBill);
	TEST_CASE_REGISTER(MdbMasterBillValidatorTest, testBtEscrowError);
	TEST_CASE_REGISTER(MdbMasterBillValidatorTest, testJsmDepositeBill);
	TEST_CASE_REGISTER(MdbMasterBillValidatorTest, testAvroraDepositeBill);
	TEST_CASE_REGISTER(MdbMasterBillValidatorTest, testScalingFactor1000);
}

bool MdbMasterBillValidatorTest::init() {
	result = new StringBuilder;
	realTime = new TestRealTime;
	eventEngine = new TestBillValidatorEventEngine(result);
	context = new MdbBillValidatorContext(2, realTime, 50000);
	bv = new MdbMasterBillValidator(context, eventEngine);
	tester = new MdbMasterTester(bv);
	bv->reset();
	return true;
}

void MdbMasterBillValidatorTest::cleanup() {
	delete tester;
	delete bv;
	delete context;
	delete eventEngine;
	delete realTime;
	delete result;
}

bool MdbMasterBillValidatorTest::testReset() {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("30", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("30", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("30", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Poll (wait JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("06"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup
	tester->poll();
	TEST_HEXDATA_EQUAL("31", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x01x00x07x00x64x01x00x00xFFxFFxFFx01x05x0Ax32x00x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	return true;
}

bool MdbMasterBillValidatorTest::testResetNoJustReset() {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("30", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Poll (wait JustReset)
	for(uint16_t i = 0; i < MDB_JUST_RESET_COUNT; i++) {
		tester->poll();
		TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
		TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
		TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());
	}

	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("30", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Poll (JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("06"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup
	tester->poll();
	TEST_HEXDATA_EQUAL("31", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x01x00x07x00x64x01x00x00xFFxFFxFFx01x05x0Ax32x00x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	return true;
}

bool MdbMasterBillValidatorTest::testInit() {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("30", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("30", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Poll (wait JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("06"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup without answer
	tester->poll();
	TEST_HEXDATA_EQUAL("31", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Setup
	tester->poll();
	TEST_HEXDATA_EQUAL("31", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x01x00x07x00x64x01x00x00xFFxFFxFFx01x05x0Ax32x00x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Extenstion Identification
	tester->poll();
	TEST_HEXDATA_EQUAL("3700", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x4Ax4Fx46x34x33x32x36x20x20x20x20x20x20x20x20x31x31x4Dx44x52x55x5Fx5Fx30x30x30x34x00x25"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Disable without answer
	tester->poll();
	TEST_HEXDATA_EQUAL("3400000000", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Disable
	tester->poll();
	TEST_HEXDATA_EQUAL("3400000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());
	return true;
}

bool MdbMasterBillValidatorTest::testDeviceReboot() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Enable
	bv->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("34000F000F", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Timeouts
	for(uint16_t i = 0; i < MDB_TRY_NUMBER; i++) {
		tester->poll();
		TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
		tester->recvTimeout();
	}

	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("30", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll (wait JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("06"));

	// Setup
	tester->poll();
	TEST_HEXDATA_EQUAL("31", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x01x00x07x00x64x01x00x00xFFxFFxFFx01x05x0Ax32x00x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Extenstion Identification
	tester->poll();
	TEST_HEXDATA_EQUAL("3700", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x4Ax4Fx46x34x33x32x36x20x20x20x20x20x20x20x20x31x31x4Dx44x52x55x5Fx5Fx30x30x30x34x00x25"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Disable
	tester->poll();
	TEST_HEXDATA_EQUAL("3400000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Enable
	tester->poll();
	TEST_HEXDATA_EQUAL("34000F000F", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	return true;
}

bool MdbMasterBillValidatorTest::testDeviceJustReset() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Enable
	bv->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("34000F000F", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll (JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("06"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup
	tester->poll();
	TEST_HEXDATA_EQUAL("31", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x01x00x07x00x64x01x00x00xFFxFFxFFx01x05x0Ax32x00x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Extenstion Identification
	tester->poll();
	TEST_HEXDATA_EQUAL("3700", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x4Ax4Fx46x34x33x32x36x20x20x20x20x20x20x20x20x31x31x4Dx44x52x55x5Fx5Fx30x30x30x34x00x25"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Disable
	tester->poll();
	TEST_HEXDATA_EQUAL("3400000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Enable
	tester->poll();
	TEST_HEXDATA_EQUAL("34000F000F", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	return true;
}

bool MdbMasterBillValidatorTest::gotoStatePoll(MdbMasterTester *tester) {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("30", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll (wait JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("06"));

	// Setup
	tester->poll();
	TEST_HEXDATA_EQUAL("31", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x01x00x07x00x64x01x00x00xFFxFFxFFx01x05x0Ax32x00x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Extenstion Identification
	tester->poll();
	TEST_HEXDATA_EQUAL("3700", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x4Ax4Fx46x34x33x32x36x20x20x20x20x20x20x20x20x31x31x4Dx44x52x55x5Fx5Fx30x30x30x34x00x25"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Disable
	tester->poll();
	TEST_HEXDATA_EQUAL("3400000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("<event=Ready>", result->getString());
	result->clear();

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	return true;
}

bool MdbMasterBillValidatorTest::testEnable() {
	context->setMaxBill(15000);
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Enable RET
	bv->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("3400070007", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0xAA));
	TEST_STRING_EQUAL("", result->getString());

	// Enable not answer
	tester->poll();
	TEST_HEXDATA_EQUAL("3400070007", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Enable
	tester->poll();
	TEST_HEXDATA_EQUAL("3400070007", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	return true;
}

bool MdbMasterBillValidatorTest::testDisable() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Enable
	bv->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("34000F000F", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Disable RET
	bv->disable();
	tester->poll();
	TEST_HEXDATA_EQUAL("3400000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0xAA));
	TEST_STRING_EQUAL("", result->getString());

	// Disable not answer
	tester->poll();
	TEST_HEXDATA_EQUAL("3400000000", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Disable
	tester->poll();
	TEST_HEXDATA_EQUAL("3400000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	return true;
}

/*
 * Некоторые купюрники перезагружаются после снятия стекера.
 */
bool MdbMasterBillValidatorTest::testRemoveStacker() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0809"));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("09"));
	TEST_STRING_EQUAL("", result->getString());

	// Just Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("06"));
	TEST_STRING_EQUAL("", result->getString());

	// Setup
	tester->poll();
	TEST_HEXDATA_EQUAL("31", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x01x00x07x00x64x01x00x00xFFxFFxFFx01x05x0Ax32x00x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Extenstion Identification
	tester->poll();
	TEST_HEXDATA_EQUAL("3700", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x4Ax4Fx46x34x33x32x36x20x20x20x20x20x20x20x20x31x31x4Dx44x52x55x5Fx5Fx30x30x30x34x00x25"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Disable
	tester->poll();
	TEST_HEXDATA_EQUAL("3400000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("<event=Ready>", result->getString());
	result->clear();

	// Enable
	bv->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("34000F000F", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Just Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("06"));
	TEST_STRING_EQUAL("", result->getString());

	// Setup
	tester->poll();
	TEST_HEXDATA_EQUAL("31", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x01x00x07x00x64x01x00x00xFFxFFxFFx01x05x0Ax32x00x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Extenstion Identification
	tester->poll();
	TEST_HEXDATA_EQUAL("3700", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x4Ax4Fx46x34x33x32x36x20x20x20x20x20x20x20x20x31x31x4Dx44x52x55x5Fx5Fx30x30x30x34x00x25"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Disable
	tester->poll();
	TEST_HEXDATA_EQUAL("3400000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("<event=Ready>", result->getString());
	result->clear();

	// Enable
	tester->poll();
	TEST_HEXDATA_EQUAL("34000F000F", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	return true;
}

// BT, CashCode
bool MdbMasterBillValidatorTest::testBtDepositeBill() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("03"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("03"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll (EscrowPosition)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("3392"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Escrow
	tester->poll();
	TEST_HEXDATA_EQUAL("3501", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("09"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("09"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll (BillStacked)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("3382"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,Deposite,1,10000>", result->getString());
	return true;
}

bool MdbMasterBillValidatorTest::testBtEscrowError() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("03"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("03"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll (EscrowPosition)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("3392"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Escrow
	tester->poll();
	TEST_HEXDATA_EQUAL("3501", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("09"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("09"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll (error without escrow response)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("03"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("03"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll (EscrowPosition)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("3392"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Escrow
	tester->poll();
	TEST_HEXDATA_EQUAL("3501", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("09"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("09"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll (BillStacked)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("3382"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,Deposite,1,10000>", result->getString());
	return true;
}

// JSM, ICT
bool MdbMasterBillValidatorTest::testJsmDepositeBill() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("03"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("03"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll (EscrowPosition)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("339209"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Escrow
	tester->poll();
	TEST_HEXDATA_EQUAL("3501", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("09"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("09"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll (BillStacked)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("3382"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,Deposite,1,10000>", result->getString());
	return true;
}

// Aurora, old firmware
bool MdbMasterBillValidatorTest::testAvroraDepositeBill() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("03"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("03"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll (EscrowPosition)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("339209"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Escrow
	tester->poll();
	TEST_HEXDATA_EQUAL("3501", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll (BillStacked)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("3382"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,Deposite,1,10000>", result->getString());
	return true;
}

bool MdbMasterBillValidatorTest::testScalingFactor1000() {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("30", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll (wait JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("06"));

	// Setup (ScaleFactor=1000)
	tester->poll();
	TEST_HEXDATA_EQUAL("31", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x01x00x07x03xE8x01x00x00xFFxFFxFFx01x05x0Ax32x00x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Extenstion Identification
	tester->poll();
	TEST_HEXDATA_EQUAL("3700", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x4Ax4Fx46x34x33x32x36x20x20x20x20x20x20x20x20x31x31x4Dx44x52x55x5Fx5Fx30x30x30x34x00x25"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Disable
	tester->poll();
	TEST_HEXDATA_EQUAL("3400000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("<event=Ready>", result->getString());
	result->clear();

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll (EscrowPosition)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("3392"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Escrow
	tester->poll();
	TEST_HEXDATA_EQUAL("3501", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("09"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("09"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll (BillStacked)
	tester->poll();
	TEST_HEXDATA_EQUAL("33", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("3382"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,Deposite,1,100000>", result->getString());
	return true;
}
