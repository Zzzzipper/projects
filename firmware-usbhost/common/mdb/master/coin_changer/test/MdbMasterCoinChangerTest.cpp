#include "mdb/master/coin_changer/MdbMasterCoinChanger.h"
#include "mdb/master/MdbMasterTester.h"
#include "mdb/MdbProtocolCoinChanger.h"
#include "timer/include/TestRealTime.h"
#include "event/include/TestEventEngine.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class TestCoinChangerEventEngine : public TestEventEngine {
public:
	TestCoinChangerEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case MdbMasterCoinChanger::Event_Ready: procEvent(envelope, MdbMasterCoinChanger::Event_Ready, "Ready"); break;
		case MdbMasterCoinChanger::Event_Error: procEventUint16(envelope, MdbMasterCoinChanger::Event_Error, "Error"); break;
		case MdbMasterCoinChanger::Event_Deposite: procEventCoin(envelope, MdbMasterCoinChanger::Event_Deposite, "Deposite"); break;
		case MdbMasterCoinChanger::Event_DepositeToken: procEventCoin(envelope, MdbMasterCoinChanger::Event_DepositeToken, "DepositeToken"); break;
		case MdbMasterCoinChanger::Event_Dispense: procEventCoin(envelope, MdbMasterCoinChanger::Event_Dispense, "Dispense"); break;
		case MdbMasterCoinChanger::Event_DispenseCoin: procEventCoin(envelope, MdbMasterCoinChanger::Event_DispenseCoin, "DispenseCoin"); break;
		case MdbMasterCoinChanger::Event_DispenseManual: procEventCoin(envelope, MdbMasterCoinChanger::Event_DispenseManual, "DispenseManual"); break;
		case MdbMasterCoinChanger::Event_EscrowRequest: procEvent(envelope, MdbMasterCoinChanger::Event_EscrowRequest, "EscrowRequest"); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}

private:
	void procEventCoin(EventEnvelope *envelope, uint16_t type, const char *name) {
		MdbMasterCoinChanger::EventCoin event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.getNominal() << "," << event.getByte1() << "," << event.getByte2() << ">";
	}
};

class MdbMasterCoinChangerTest : public TestSet {
public:
	MdbMasterCoinChangerTest();
	bool init();
	void cleanup();
	bool testReset();
	bool testResetNoJustReset();
	bool testInit();
	bool testDeviceReboot();
	bool testDeviceJustReset();
	bool gotoStatePoll(MdbMasterTester *sender);
	bool testSelfReset();
	bool testExpansionPayout();
	bool testExpansionPayoutAeternaBug();
	bool testExpansionPayoutConfirmErrors();
	bool testEnable();
	bool testDisable();
	bool testPoll();
	bool testDeposite();
	bool testDispense();
	bool testEscrowRequest();
	bool testToken();

private:
	StringBuilder *result;
	TestRealTime *realTime;
	TestCoinChangerEventEngine *eventEngine;
	MdbCoinChangerContext *cl;
	MdbMasterCoinChanger *cc;
	MdbMasterTester *tester;
};

TEST_SET_REGISTER(MdbMasterCoinChangerTest);

MdbMasterCoinChangerTest::MdbMasterCoinChangerTest() {
	TEST_CASE_REGISTER(MdbMasterCoinChangerTest, testReset);
	TEST_CASE_REGISTER(MdbMasterCoinChangerTest, testResetNoJustReset);
	TEST_CASE_REGISTER(MdbMasterCoinChangerTest, testInit);
	TEST_CASE_REGISTER(MdbMasterCoinChangerTest, testDeviceReboot);
	TEST_CASE_REGISTER(MdbMasterCoinChangerTest, testDeviceJustReset);
	TEST_CASE_REGISTER(MdbMasterCoinChangerTest, testExpansionPayout);
	TEST_CASE_REGISTER(MdbMasterCoinChangerTest, testExpansionPayoutAeternaBug);
	TEST_CASE_REGISTER(MdbMasterCoinChangerTest, testExpansionPayoutConfirmErrors);
	TEST_CASE_REGISTER(MdbMasterCoinChangerTest, testEnable);
	TEST_CASE_REGISTER(MdbMasterCoinChangerTest, testDisable);
	TEST_CASE_REGISTER(MdbMasterCoinChangerTest, testPoll);
	TEST_CASE_REGISTER(MdbMasterCoinChangerTest, testDeposite);
	TEST_CASE_REGISTER(MdbMasterCoinChangerTest, testDispense);
	TEST_CASE_REGISTER(MdbMasterCoinChangerTest, testEscrowRequest);
	TEST_CASE_REGISTER(MdbMasterCoinChangerTest, testToken);
}

bool MdbMasterCoinChangerTest::init() {
	result = new StringBuilder;
	realTime = new TestRealTime;
	eventEngine = new TestCoinChangerEventEngine(result);
	cl = new MdbCoinChangerContext(2, realTime);
	cc = new MdbMasterCoinChanger(cl, eventEngine);
	tester = new MdbMasterTester(cc);
	cc->reset();
	return true;
}

void MdbMasterCoinChangerTest::cleanup() {
	delete tester;
	delete cc;
	delete cl;
	delete eventEngine;
	delete realTime;
	delete result;
}

bool MdbMasterCoinChangerTest::testReset() {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("08", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("08", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("08", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Poll (wait JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup
	tester->poll();
	TEST_HEXDATA_EQUAL("09", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x03x00x07x0Ax01x00x5Cx00x00x01x02x05x0Ax0AxFFx00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	return true;
}

bool MdbMasterCoinChangerTest::testResetNoJustReset() {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("08", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Poll (wait JustReset)
	for(uint16_t i = 0; i < MDB_JUST_RESET_COUNT; i++) {
		tester->poll();
		TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
		TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
		TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());
	}

	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("08", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Poll (JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup
	tester->poll();
	TEST_HEXDATA_EQUAL("09", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x03x00x07x0Ax01x00x5Cx00x00x01x02x05x0Ax0AxFFx00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	return true;
}

bool MdbMasterCoinChangerTest::testInit() {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("08", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("08", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Poll (wait JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup
	tester->poll();
	TEST_HEXDATA_EQUAL("09", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("09", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x03x00x07x0Ax01x00x5Cx00x00x01x02x05x0Ax0AxFFx00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// ExpansionIdentification
	tester->poll();
	TEST_HEXDATA_EQUAL("0F00", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("0F00", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData(
		"x4Ax4Fx46x30x35x33x36x37x39x20x20x20x20x20x20x4A"
		"x32x30x30x30x4Dx44x42x20x50x33x32x90x10x00x00x00x01"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// ExpansionFeatureEnable
	tester->poll();
	TEST_HEXDATA_EQUAL("0F010000000F", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("0F010000000F", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// TubeStatus
	tester->poll();
	TEST_HEXDATA_EQUAL("0A", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("0A", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x00x40x00x00x41x37x57x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// CoinType
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00000000", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("0C00000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());
	return true;
}

bool MdbMasterCoinChangerTest::testDeviceReboot() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Enable
	cc->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00FC005C", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Timeouts
	for(uint16_t i = 0; i < MDB_TRY_NUMBER; i++) {
		tester->poll();
		TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
		tester->recvTimeout();
	}

	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("08", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll (wait JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup
	tester->poll();
	TEST_HEXDATA_EQUAL("09", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x03x00x07x0Ax01x00x5Cx00x00x01x02x05x0Ax0AxFFx00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// ExpansionIdentification
	tester->poll();
	TEST_HEXDATA_EQUAL("0F00", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData(
		"x4Ax4Fx46x30x35x33x36x37x39x20x20x20x20x20x20x4A"
		"x32x30x30x30x4Dx44x42x20x50x33x32x90x10x00x00x00x01"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// ExpansionFeatureEnable
	tester->poll();
	TEST_HEXDATA_EQUAL("0F010000000F", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// TubeStatus
	tester->poll();
	TEST_HEXDATA_EQUAL("0A", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x00x40x00x00x41x37x57x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// CoinType
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Enable
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00FC005C", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	return true;
}

bool MdbMasterCoinChangerTest::testDeviceJustReset() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Enable
	cc->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00FC005C", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll (wait JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup
	tester->poll();
	TEST_HEXDATA_EQUAL("09", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x03x00x07x0Ax01x00x5Cx00x00x01x02x05x0Ax0AxFFx00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// ExpansionIdentification
	tester->poll();
	TEST_HEXDATA_EQUAL("0F00", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData(
		"x4Ax4Fx46x30x35x33x36x37x39x20x20x20x20x20x20x4A"
		"x32x30x30x30x4Dx44x42x20x50x33x32x90x10x00x00x00x01"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// ExpansionFeatureEnable
	tester->poll();
	TEST_HEXDATA_EQUAL("0F010000000F", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// TubeStatus
	tester->poll();
	TEST_HEXDATA_EQUAL("0A", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x00x40x00x00x41x37x57x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// CoinType
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Enable
	cc->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00FC005C", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	return true;
}

bool MdbMasterCoinChangerTest::gotoStatePoll(MdbMasterTester *tester) {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("08", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll (wait JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0B"));

	// Setup
	tester->poll();
	TEST_HEXDATA_EQUAL("09", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x03x00x07x0Ax01x00x5Cx00x00x01x02x05x0Ax0AxFFx00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// ExpansionIdentification
	tester->poll();
	TEST_HEXDATA_EQUAL("0F00", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData(
		"x4Ax4Fx46x30x35x33x36x37x39x20x20x20x20x20x20x4A"
		"x32x30x30x30x4Dx44x42x20x50x33x32x90x10x00x00x00x02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// ExpansionFeatureEnable
	tester->poll();
	TEST_HEXDATA_EQUAL("0F010000000F", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// TubeStatus
	tester->poll();
	TEST_HEXDATA_EQUAL("0A", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x00x40x00x00x41x37x57x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// CoinType
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbMasterCoinChangerTest::testExpansionPayout() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Dispense
	cc->dispense(30000);
	tester->poll();
	TEST_HEXDATA_EQUAL("0F02C8", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// PayoutPoll (PayoutBusy)
	tester->poll();
	TEST_HEXDATA_EQUAL("0F04", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("02"));
	TEST_STRING_EQUAL("", result->getString());

	// PayoutPoll (PayoutBusy)
	tester->poll();
	TEST_HEXDATA_EQUAL("0F04", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("02"));
	TEST_STRING_EQUAL("", result->getString());

	// PayoutPoll
	tester->poll();
	TEST_HEXDATA_EQUAL("0F04", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// PayoutStatus
	tester->poll();
	TEST_HEXDATA_EQUAL("0F03", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("02"));
	TEST_STRING_EQUAL("", result->getString());

	// Dispense
	tester->poll();
	TEST_HEXDATA_EQUAL("0F0264", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// PayoutPoll (PayoutBusy)
	tester->poll();
	TEST_HEXDATA_EQUAL("0F04", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("02"));
	TEST_STRING_EQUAL("", result->getString());

	// PayoutPoll (PayoutBusy)
	tester->poll();
	TEST_HEXDATA_EQUAL("0F04", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("02"));
	TEST_STRING_EQUAL("", result->getString());

	// PayoutPoll
	tester->poll();
	TEST_HEXDATA_EQUAL("0F04", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// PayoutStatus
	tester->poll();
	TEST_HEXDATA_EQUAL("0F03", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("02")); //todo: тут должны будть другие данные
	TEST_STRING_EQUAL("", result->getString());

	// Update Tube Status
	tester->poll();
	TEST_HEXDATA_EQUAL("0A", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x00x40x00x00x41x37x57x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,Dispense,0,0,0>", result->getString());
	return true;
}

// Codges Aeterna BUG: иногда не сообщает о завершении выдачи сдачи
// See Note 2 under the DISPENSE (0DH) command
bool MdbMasterCoinChangerTest::testExpansionPayoutAeternaBug() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Dispense
	cc->dispense(10000);
	tester->poll();
	TEST_HEXDATA_EQUAL("0F0264", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// PayoutPoll (PayoutBusy)
	for(uint16_t i = 0; i < (MDB_CC_EXP_PAYOUT_POLL_MAX - 1); i++) {
		tester->poll();
		TEST_HEXDATA_EQUAL("0F04", tester->getSendData(), tester->getSendLen());
		TEST_NUMBER_EQUAL(true, tester->recvData("02"));
		TEST_STRING_EQUAL("", result->getString());
	}

	// PayoutPoll (PayoutBusy)
	tester->poll();
	TEST_HEXDATA_EQUAL("0F04", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("02"));
	TEST_STRING_EQUAL("<event=1,Dispense,0,0,0>", result->getString());
	result->clear();

	// Restart
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));
	return true;
}

bool MdbMasterCoinChangerTest::testExpansionPayoutConfirmErrors() {
#if 0
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Dispense RET
	cc->dispense(100);
	tester->poll();
	TEST_HEXDATA_EQUAL("0F0201", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0xAA));
	TEST_STRING_EQUAL("", result->getString());

	// Dispense not answer
	tester->poll();
	TEST_HEXDATA_EQUAL("0F0201", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Dispense
	tester->poll();
	TEST_HEXDATA_EQUAL("0F0201", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Update Tube Status
	tester->poll();
	TEST_HEXDATA_EQUAL("0A", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x00x40x00x00x41x37x57x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=Dispense,0,0,0>", result->getString());
	return true;
#else
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Dispense RET
	cc->dispense(100);
	tester->poll();
	TEST_HEXDATA_EQUAL("0F0201", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0xAA));
	TEST_STRING_EQUAL("", result->getString());

	// Dispense not answer
	tester->poll();
	TEST_HEXDATA_EQUAL("0F0201", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Dispense
	tester->poll();
	TEST_HEXDATA_EQUAL("0F0201", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0F04", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// PayoutStatus
	tester->poll();
	TEST_HEXDATA_EQUAL("0F03", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("02")); //todo: тут должны будть другие данные
	TEST_STRING_EQUAL("", result->getString());

	// Update Tube Status
	tester->poll();
	TEST_HEXDATA_EQUAL("0A", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x00x40x00x00x41x37x57x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,Dispense,0,0,0>", result->getString());
	return true;
#endif
}

bool MdbMasterCoinChangerTest::testEnable() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Enable RET
	cc->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00FC005C", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0xAA));
	TEST_STRING_EQUAL("", result->getString());

	// Enable not answer
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00FC005C", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Enable
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00FC005C", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	return true;
}

bool MdbMasterCoinChangerTest::testDisable() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Enable
	cc->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00FC005C", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Disable RET
	cc->disable();
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0xAA));
	TEST_STRING_EQUAL("", result->getString());

	// Disable not answer
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00000000", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Disable
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	return true;
}

bool MdbMasterCoinChangerTest::testPoll() {
#if 0
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Enable
	cc->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00FC005C", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	for(uint16_t j = 0; j < 5; j++) {
		// Poll
		for(uint16_t i = 0; i < MDB_TUBE_STATUS_COUNT; i++) {
			tester->poll();
			TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
			TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
			TEST_STRING_EQUAL("", result->getString());
		}

		// TubeStatus
		tester->poll();
		TEST_HEXDATA_EQUAL("0A", tester->getSendData(), tester->getSendLen());
		TEST_NUMBER_EQUAL(true, tester->recvData("x00x40x00x00x41x37x57x00x00x00x00x00x00x00x00x00x00x00"));
		TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	}

	// Enable
	cc->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00FC005C", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());
	return true;
#else
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Enable
	cc->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00FC005C", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	for(uint16_t i = 0; i < MDB_POLL_TUBE_STATUS_COUNT; i++) {
		tester->poll();
		TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
		TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
		TEST_STRING_EQUAL("", result->getString());
	}

	// TubeStatus
	tester->poll();
	TEST_HEXDATA_EQUAL("0A", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x00x40x00x00x41x37x57x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Poll
	for(uint16_t i = 0; i < MDB_POLL_TUBE_STATUS_COUNT; i++) {
		tester->poll();
		TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
		TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
		TEST_STRING_EQUAL("", result->getString());
	}

	// Diagnostic
	tester->poll();
	TEST_HEXDATA_EQUAL("0F05", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("03001130"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Poll
	for(uint16_t i = 0; i < MDB_POLL_TUBE_STATUS_COUNT; i++) {
		tester->poll();
		TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
		TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
		TEST_STRING_EQUAL("", result->getString());
	}

	// Enable
	tester->poll();
	TEST_HEXDATA_EQUAL("0C00FC005C", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	for(uint16_t i = 0; i < MDB_POLL_TUBE_STATUS_COUNT; i++) {
		tester->poll();
		TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
		TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
		TEST_STRING_EQUAL("", result->getString());
	}

	// TubeStatus
	tester->poll();
	TEST_HEXDATA_EQUAL("0A", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x00x40x00x00x41x37x57x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	return true;
#endif
}

bool MdbMasterCoinChangerTest::testDeposite() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Deposite to CashBox
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("4657"));
	TEST_STRING_EQUAL("<event=1,Deposite,1000,70,87>", result->getString());
	result->clear();
	TEST_NUMBER_EQUAL(0, cl->get(6)->getNumber());

	// Deposite to Tube
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("5658"));
	TEST_STRING_EQUAL("<event=1,Deposite,1000,86,88>", result->getString());
	result->clear();
	TEST_NUMBER_EQUAL(0x58, cl->get(6)->getNumber());

	// Reject Deposite
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("7658"));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	return true;
}

bool MdbMasterCoinChangerTest::testDispense() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Dispense 1 coin with index 2
	cc->dispenseCoin(0x12);
	tester->poll();
	TEST_HEXDATA_EQUAL("0D12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("02"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Update
	tester->poll();
	TEST_HEXDATA_EQUAL("0A", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("x00x40x00x00x41x37x57x00x00x00x00x00x00x00x00x00x00x00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,DispenseCoin,0,18,0>", result->getString());

	return true;
}

bool MdbMasterCoinChangerTest::testEscrowRequest() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Escrow request
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("01"));
	TEST_STRING_EQUAL("<event=1,EscrowRequest>", result->getString());
	result->clear();

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	return true;
}

bool MdbMasterCoinChangerTest::testToken() {
	TEST_NUMBER_EQUAL(true, gotoStatePoll(tester));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Deposite to CashBox
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("4757"));
	TEST_STRING_EQUAL("<event=1,DepositeToken,0,71,87>", result->getString());
	result->clear();

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());
	return true;
}
