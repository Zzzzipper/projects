#include "mdb/master/cashless/MdbMasterCashless.h"
#include "mdb/MdbProtocolCashless.h"
#include "mdb/master/MdbMasterTester.h"
#include "timer/include/TimerEngine.h"
#include "timer/include/TestRealTime.h"
#include "mdb/master/cashless/test/TestCashlessEventEngine.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class MdbMasterCashlessTest : public TestSet {
public:
	MdbMasterCashlessTest();
	bool init();
	void cleanup();
	bool testReset();
	bool testResetNoJustReset();
	bool testInit();
	bool testInitThroughPoll();
	bool testDeviceReboot();
	bool testDeviceJustReset();
	bool gotoStateWork(MdbMasterTester *sender);
	bool testEnable();
	bool testDisable();
	bool testPaxD200Sberbank();
	bool testPaxD200SberbankVendDenied();
	bool testPaxD200SberbankVendFailed();
	bool testPaxD200SberbankSessionCancel();
	bool gotoSibaStateWork(MdbMasterTester *tester);
	bool testSibaKeySessionCancel();
	bool testSibaKeyRevalue();
	bool testVendotek2();
	bool testCloseSessionStateApproving();
	bool testCloseSessionStateVending();
	bool testTimeoutStateSession();
	bool testTimeoutStateVendRequest();
	bool testTimeoutStateVendApproving();
	bool testTimeoutStateVendSuccess();
	bool testTimeoutStateVendFailure();
	bool testTimeoutStateVendCancel();
	bool testTimeoutStateSessionComplete();
	bool testTimeoutStateSessionEnd();

private:
	StringBuilder *result;
	TestRealTime *realtime;
	TimerEngine *timerEngine;
	TestCashlessEventEngine *eventEngine;
	Mdb::DeviceContext *context;
	MdbMasterCashless *cc;
	MdbMasterTester *tester;
};

TEST_SET_REGISTER(MdbMasterCashlessTest);

MdbMasterCashlessTest::MdbMasterCashlessTest() {
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testReset);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testResetNoJustReset);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testInit);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testInitThroughPoll);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testDeviceReboot);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testDeviceJustReset);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testEnable);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testDisable);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testPaxD200Sberbank);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testPaxD200SberbankVendDenied);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testPaxD200SberbankVendFailed);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testPaxD200SberbankSessionCancel);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testSibaKeySessionCancel);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testSibaKeyRevalue);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testVendotek2);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testCloseSessionStateApproving);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testCloseSessionStateVending);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testTimeoutStateSession);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testTimeoutStateVendRequest);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testTimeoutStateVendApproving);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testTimeoutStateVendSuccess);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testTimeoutStateVendFailure);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testTimeoutStateVendCancel);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testTimeoutStateSessionComplete);
	TEST_CASE_REGISTER(MdbMasterCashlessTest, testTimeoutStateSessionEnd);
}

bool MdbMasterCashlessTest::init() {
	result = new StringBuilder;
	realtime = new TestRealTime;
	timerEngine = new TimerEngine;
	eventEngine = new TestCashlessEventEngine(result);
	context = new Mdb::DeviceContext(2, realtime);
	cc = new MdbMasterCashless(Mdb::Device_CashlessDevice1, context, timerEngine, eventEngine);
	tester = new MdbMasterTester(cc);
	cc->reset();
	return true;
}

void MdbMasterCashlessTest::cleanup() {
	delete tester;
	delete cc;
	delete context;
	delete eventEngine;
	delete timerEngine;
	delete realtime;
	delete result;
}

bool MdbMasterCashlessTest::testReset() {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Poll (wait JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Config
	tester->poll();
	TEST_HEXDATA_EQUAL("110002000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0101164301011400"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	return true;
}

bool MdbMasterCashlessTest::testResetNoJustReset() {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Poll (wait JustReset)
	for(uint16_t i = 0; i < MDB_JUST_RESET_COUNT; i++) {
		tester->poll();
		TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
		TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
		TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());
	}

	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Poll (JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Config
	tester->poll();
	TEST_HEXDATA_EQUAL("110002000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0101164301011400"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	return true;
}

bool MdbMasterCashlessTest::testInit() {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Config
	tester->poll();
	TEST_HEXDATA_EQUAL("110002000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0101164301011400"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Max Prices
	tester->poll();
	TEST_HEXDATA_EQUAL("1101FFFF0000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// ExpansionIdentification
	tester->poll();
	TEST_HEXDATA_EQUAL("17004546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("094546523031323334353637383941423031323334353637383941420100"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());
	return true;
}

bool MdbMasterCashlessTest::testInitThroughPoll() {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// Poll (wait JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Config
	tester->poll();
	TEST_HEXDATA_EQUAL("110002000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0101164301011400"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Max Prices
	tester->poll();
	TEST_HEXDATA_EQUAL("1101FFFF0000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	// ExpansionIdentification
	tester->poll();
	TEST_HEXDATA_EQUAL("17004546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("094546523031323334353637383941423031323334353637383941420100"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendLen());
	return true;
}

bool MdbMasterCashlessTest::testDeviceReboot() {
	TEST_NUMBER_EQUAL(true, gotoStateWork(tester));

	// Timeouts
	for(uint16_t i = 0; i < MDB_TRY_NUMBER; i++) {
		tester->poll();
		TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
		tester->recvTimeout();
	}

	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();

	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();

	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Config
	tester->poll();
	TEST_HEXDATA_EQUAL("110002000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0101164301011400"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Max Prices
	tester->poll();
	TEST_HEXDATA_EQUAL("1101FFFF0000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification
	tester->poll();
	TEST_HEXDATA_EQUAL("17004546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("094546523031323334353637383941423031323334353637383941420100"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Enable
	tester->poll();
	TEST_HEXDATA_EQUAL("1401", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	return true;
}

bool MdbMasterCashlessTest::testDeviceJustReset() {
	TEST_NUMBER_EQUAL(true, gotoStateWork(tester));

	// JustReset
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Config
	tester->poll();
	TEST_HEXDATA_EQUAL("110002000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0101164301011400"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Max Prices
	tester->poll();
	TEST_HEXDATA_EQUAL("1101FFFF0000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification
	tester->poll();
	TEST_HEXDATA_EQUAL("17004546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("094546523031323334353637383941423031323334353637383941420100"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Enable
	tester->poll();
	TEST_HEXDATA_EQUAL("1401", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbMasterCashlessTest::gotoStateWork(MdbMasterTester *tester) {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Config
	tester->poll();
	TEST_HEXDATA_EQUAL("110002000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0101164301011400"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Max Prices
	tester->poll();
	TEST_HEXDATA_EQUAL("1101FFFF0000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification
	tester->poll();
	TEST_HEXDATA_EQUAL("17004546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("094546523031323334353637383941423031323334353637383941420100"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Enable
	cc->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("1401", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbMasterCashlessTest::testEnable() {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Config
	tester->poll();
	TEST_HEXDATA_EQUAL("110002000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0101164301011400"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Max Prices
	tester->poll();
	TEST_HEXDATA_EQUAL("1101FFFF0000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification
	tester->poll();
	TEST_HEXDATA_EQUAL("17004546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("094546523031323334353637383941423031323334353637383941420100"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Enable
	cc->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("1401", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();

	tester->poll();
	TEST_HEXDATA_EQUAL("1401", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	return true;
}

bool MdbMasterCashlessTest::testDisable() {
	TEST_NUMBER_EQUAL(true, gotoStateWork(tester));

	// Disable
	cc->disable();
	tester->poll();
	TEST_HEXDATA_EQUAL("1400", tester->getSendData(), tester->getSendLen());
	tester->recvTimeout();

	tester->poll();
	TEST_HEXDATA_EQUAL("1400", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	return true;
}

/*
====================================
Äàìï ñåññèè Necta Kikko Max c Pax D200 Sberbank
====================================
mx10;//Reset
mx10;//CRC
sx00;

mx12;mx12;
sx00;

mx12;//Poll
mx12;//CRC
sx00;//JustReset
sx00;/CRC
mx00;

mx11;//Setup
mx00;//SetupConfig
mx02;//FeatureLevel=2
mx00;//Columns=0
mx00;//Rows=0
mx02;
mx15;//CRC
sx00;

mx12;mx12;
sx00;

mx12;//Poll
mx12;
sx01;//SetupConfigResponse
sx01;//FeatureLevel=1
sx16;sx43;//CountryCode=x16x43
sx01;//ScaleFactor=1
sx02;//DecimalPlaces=2
sx2D;//AppMaxResponseTime=x2D
sx05;//Options=b00000101
//b0=1: The payment media reader is capable of restoring funds to the user’s payment media or  account. Refunds may be requested.
//b2=1: The payment media reader does have its own display.
sx90;
mx00;

mx11;//Setup
mx01;//SetupPrices
mxFF;mxFF;//no maximum limit
mx00;mx00;//no minimum limit
mx10;//CRC
sx00;

mx17;//Expansion
mx00;//RequestId
mx00;mx00;mx00;
mx00;mx00;mx00;mx00;mx00;mx00;mx00;mx00;mx00;mx00;mx00;mx00;
mx00;mx00;mx00;mx00;mx00;mx00;mx00;mx00;mx00;mx00;mx00;mx00;
mx00;mx00;
mx17;//CRC
sx00;

mx12;mx12;
sx00;

mx12;//POLL
mx12;
sx09;//PeripheralId
sx53;sx42;sx54;//ManufactureCode=SBT
sx31;sx31;sx31;sx31;sx31;sx31;sx31;sx31;sx31;sx31;sx31;sx31;//SerialNumber=111111111111
sx53;sx42;sx54;sx56;sx45;sx4E;sx44;sx49;sx4E;sx47;sx30;sx30;//ModelNumber=SBTVENDING00
sx01;sx01;//SoftwareVersion=1.1
sx94;//CRC
mx00;//ACK

mx14;//Reader
mx01;//Enable
mx15;//CRC
sx00;

mx12;mx12;
sx00;

mx14;//Reader
mx00;//Disable
mx14;//CRC
sx00;

mx12;mx12;
sx00;

mx12;mx12;
sx00;

mx12;mx12;
sx00;

mx12;mx12;
sx00;

mx14;//Reader
mx01;//Disable
mx15;
sx00;

mx12;mx12;
sx00;

====================================
Session1
====================================
mx12;//Poll
mx12;
sx03;//BeginSession
sx4E;sx20;//FundsAvailable=20000(200rub)
sxFF;//DiagnosticsResponse (User Defined Data Z2-Zn)
sx70;//CRC
mx00;

mx12;mx12;
sx00;

mx12;mx12;
sx04;//Status_SessionCancelRequest
sx04;//CRC
mx00;

mx13;//Vend
mx04;//Session Complete
mx17;//CRC
sx00;

mx12;mx12;
sx00;

mx12;mx12;
sx07;//End Session
sx07;
mx00;

mx12;mx12;
sx00;

====================================
Session2
====================================
mx12;//Poll
mx12;
sx03;//BeginSession
sx4E;sx20;//FundsAvailable=20000(200rub)
sxFF;//DiagnosticsResponse (User Defined Data Z2-Zn)
sx70;//CRC
mx00;

mx12;mx12;
sx00;

mx12;mx12;
sx00;

mx12;mx12;
sx00;

mx13;//Vend
mx00;//VendRequest
mx00;mx0A;//ItemPrice=10
mx00;mx06;//ItemNumber=6
mx23;//CRC
sx00;

mx12;mx12;
sx00;

mx12;mx12;
sx05;//VendApproved
sx00;sx0A;//VendAmmount=10
sx0F;mx00;

mx12;mx12;
sx00;

mx12;mx12;
sx00;

mx13;//Vend
mx02;//VendSuccess
mx00;mx06;//ItemNumber=6
mx1B;//CRC
sx00;

mx13;//Vend
mx04;//VendComplete
mx17;//CRC
sx00;

mx12;mx12;
sx00;

mx12;mx12;
sx07;//End Session
sx07;
mx00;

mx12;mx12;
sx00;

 */
bool MdbMasterCashlessTest::testPaxD200Sberbank() {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll (wait JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Config
	tester->poll();
	TEST_HEXDATA_EQUAL("110002000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("010116430A022D05"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Max Prices
	tester->poll();
	TEST_HEXDATA_EQUAL("1101FFFF0000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification
	tester->poll();
	TEST_HEXDATA_EQUAL("17004546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0953425431313131313131313131313153425456454E44494E4730300101"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Enable
	cc->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("1401", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0307D0FF"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend request
	TEST_NUMBER_EQUAL(true, cc->sale(6, 100, "", 0));
	tester->poll();
	TEST_HEXDATA_EQUAL("1300000A0006", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend approved
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("05000A"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,VendApproved,1,100,0,0>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend complete
	TEST_NUMBER_EQUAL(true, cc->saleComplete());
	tester->poll();
	TEST_HEXDATA_EQUAL("13020006", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("1304", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("07"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	return true;
}

bool MdbMasterCashlessTest::testPaxD200SberbankVendDenied() {
	TEST_NUMBER_EQUAL(true, gotoStateWork(tester));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0307D0FF"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend request
	TEST_NUMBER_EQUAL(true, cc->sale(6, 100, "", 0));
	tester->poll();
	TEST_HEXDATA_EQUAL("1300000A0006", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend denied
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("06"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,VendDenied>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend complete
	TEST_NUMBER_EQUAL(true, cc->closeSession());
	tester->poll();
	TEST_HEXDATA_EQUAL("1304", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("07"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	return true;
}

bool MdbMasterCashlessTest::testPaxD200SberbankVendFailed() {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll (wait JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Config
	tester->poll();
	TEST_HEXDATA_EQUAL("110002000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0101164301022D05"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Max Prices
	tester->poll();
	TEST_HEXDATA_EQUAL("1101FFFF0000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification
	tester->poll();
	TEST_HEXDATA_EQUAL("17004546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0953425431313131313131313131313153425456454E44494E4730300101"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Enable
	cc->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("1401", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("034E20FF"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend request
	TEST_NUMBER_EQUAL(true, cc->sale(6, 10, "", 0));
	tester->poll();
	TEST_HEXDATA_EQUAL("1300000A0006", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend approved
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("05000A"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,VendApproved,1,10,0,0>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend failed
	TEST_NUMBER_EQUAL(true, cc->saleFailed());
	tester->poll();
	TEST_HEXDATA_EQUAL("1303", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("1304", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("07"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	return true;
}

bool MdbMasterCashlessTest::testPaxD200SberbankSessionCancel() {
	TEST_NUMBER_EQUAL(true, gotoStateWork(tester));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0307D0FF"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session cancel request
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("04"));

	// Session complete
	tester->poll();
	TEST_HEXDATA_EQUAL("1304", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("07"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());

	return true;
}

bool MdbMasterCashlessTest::gotoSibaStateWork(MdbMasterTester *tester) {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());

	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Config
	tester->poll();
	TEST_HEXDATA_EQUAL("110002000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("010116430A01140B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Max Prices
	tester->poll();
	TEST_HEXDATA_EQUAL("1101FFFF0000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification
	tester->poll();
	TEST_HEXDATA_EQUAL("17004546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("094546523031323334353637383941423031323334353637383941420100"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	return true;
}

bool MdbMasterCashlessTest::testSibaKeySessionCancel() {
	TEST_NUMBER_EQUAL(true, gotoSibaStateWork(tester));

	// Enable
	cc->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("1401", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0300C8"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session cancel request
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("04"));

	// Session complete
	tester->poll();
	TEST_HEXDATA_EQUAL("1304", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("07"));
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	return true;
}

bool MdbMasterCashlessTest::testSibaKeyRevalue() {
	TEST_NUMBER_EQUAL(true, gotoSibaStateWork(tester));

	// Enable
	cc->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("1401", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0300C8"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Revalue request
	cc->revalue(1000);
	tester->poll();
	TEST_HEXDATA_EQUAL("1500000A", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0D"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,RevalueApproved>", result->getString());
	result->clear();

	// Revalue request
	cc->revalue(1000);
	tester->poll();
	TEST_HEXDATA_EQUAL("1500000A", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0E"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,RevalueDenied>", result->getString());
	result->clear();

	// Revalue request
	cc->revalue(1000);
	tester->poll();
	TEST_HEXDATA_EQUAL("1500000A", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0D"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,RevalueApproved>", result->getString());
	result->clear();

	// Revalue request
	cc->revalue(1000);
	tester->poll();
	TEST_HEXDATA_EQUAL("1500000A", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0E"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,RevalueDenied>", result->getString());
	return true;
}

bool MdbMasterCashlessTest::testVendotek2() {
	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll (wait JustReset)
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Config
	tester->poll();
	TEST_HEXDATA_EQUAL("110002000000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0102FFFF01027809"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());

	// Setup Max Prices
	tester->poll();
	TEST_HEXDATA_EQUAL("1101FFFF0000", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification
	tester->poll();
	TEST_HEXDATA_EQUAL("17004546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("09544244303030303030303032313235554E4B4E4F574E20202020200100"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// Poll
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Enable
	cc->enable();
	tester->poll();
	TEST_HEXDATA_EQUAL("1401", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("034E20FFFFFFFF000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend request
	TEST_NUMBER_EQUAL(true, cc->sale(6, 100, "", 0));
	tester->poll();
	TEST_HEXDATA_EQUAL("130000640006", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend approved
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("050064"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,VendApproved,1,100,0,0>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend complete
	TEST_NUMBER_EQUAL(true, cc->saleComplete());
	tester->poll();
	TEST_HEXDATA_EQUAL("13020006", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("1304", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("07"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	return true;
}

bool MdbMasterCashlessTest::testCloseSessionStateApproving() {
	TEST_NUMBER_EQUAL(true, gotoStateWork(tester));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0307D0FF"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend request
	TEST_NUMBER_EQUAL(true, cc->sale(6, 100, "", 0));
	tester->poll();
	TEST_HEXDATA_EQUAL("1300000A0006", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Close session
	TEST_NUMBER_EQUAL(true, cc->closeSession());
	tester->poll();
	TEST_HEXDATA_EQUAL("1301", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("06"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	tester->poll();
	TEST_HEXDATA_EQUAL("1304", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("07"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	return true;
}

bool MdbMasterCashlessTest::testCloseSessionStateVending() {
	TEST_NUMBER_EQUAL(true, gotoStateWork(tester));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0307D0FF"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend request
	TEST_NUMBER_EQUAL(true, cc->sale(6, 100, "", 0));
	tester->poll();
	TEST_HEXDATA_EQUAL("1300000A0006", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend approved
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("05000A"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,VendApproved,1,100,0,0>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Close session
	TEST_NUMBER_EQUAL(true, cc->closeSession());
	tester->poll();
	tester->poll();
	TEST_HEXDATA_EQUAL("1303", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("1304", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("07"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	return true;
}

bool MdbMasterCashlessTest::testTimeoutStateSession() {
	TEST_NUMBER_EQUAL(true, gotoStateWork(tester));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0307D0FF"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session too long
	timerEngine->tick(MDB_CL_SESSION_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());

	// Session complete
	tester->poll();
	TEST_HEXDATA_EQUAL("1304", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session end
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("07"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	return true;
}

bool MdbMasterCashlessTest::testTimeoutStateVendRequest() {
	TEST_NUMBER_EQUAL(true, gotoStateWork(tester));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0307D0FF"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend request
	TEST_NUMBER_EQUAL(true, cc->sale(6, 100, "", 0));
	tester->poll();
	TEST_HEXDATA_EQUAL("1300000A0006", tester->getSendData(), tester->getSendLen());

	// Non-response timeout
	timerEngine->tick(5000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());

	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	result->clear();
	return true;
}

bool MdbMasterCashlessTest::testTimeoutStateVendApproving() {
	TEST_NUMBER_EQUAL(true, gotoStateWork(tester));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0307D0FF"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend request
	TEST_NUMBER_EQUAL(true, cc->sale(6, 100, "", 0));
	tester->poll();
	TEST_HEXDATA_EQUAL("1300000A0006", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend approved
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("05000A"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,VendApproved,1,100,0,0>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vending wait
	timerEngine->tick(60000);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vending wait
	timerEngine->tick(60000);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend complete
	TEST_NUMBER_EQUAL(true, cc->saleComplete());
	tester->poll();
	TEST_HEXDATA_EQUAL("13020006", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session complete
	tester->poll();
	TEST_HEXDATA_EQUAL("1304", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session end
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("07"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	return true;
}

bool MdbMasterCashlessTest::testTimeoutStateVendSuccess() {
	TEST_NUMBER_EQUAL(true, gotoStateWork(tester));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0307D0FF"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	// Vend request
	TEST_NUMBER_EQUAL(true, cc->sale(6, 100, "", 0));
	tester->poll();
	TEST_HEXDATA_EQUAL("1300000A0006", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend approved
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("05000A"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,VendApproved,1,100,0,0>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, cc->saleComplete());
	tester->poll();
	TEST_HEXDATA_EQUAL("13020006", tester->getSendData(), tester->getSendLen());

	// Non-response timeout
	timerEngine->tick(5000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	return true;
}

bool MdbMasterCashlessTest::testTimeoutStateVendFailure() {
	TEST_NUMBER_EQUAL(true, gotoStateWork(tester));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0307D0FF"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	// Vend request
	TEST_NUMBER_EQUAL(true, cc->sale(6, 100, "", 0));
	tester->poll();
	TEST_HEXDATA_EQUAL("1300000A0006", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend approved
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("05000A"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,VendApproved,1,100,0,0>", result->getString());
	result->clear();

	// Vend failed
	TEST_NUMBER_EQUAL(true, cc->saleFailed());
	tester->poll();
	TEST_HEXDATA_EQUAL("1303", tester->getSendData(), tester->getSendLen());

	// Non-response timeout
	timerEngine->tick(5000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	return true;
}

bool MdbMasterCashlessTest::testTimeoutStateVendCancel() {
	TEST_NUMBER_EQUAL(true, gotoStateWork(tester));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0307D0FF"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend request
	TEST_NUMBER_EQUAL(true, cc->sale(6, 100, "", 0));
	tester->poll();
	TEST_HEXDATA_EQUAL("1300000A0006", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Vend cancel
	cc->closeSession();
	tester->poll();
	TEST_HEXDATA_EQUAL("1301", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Non-response timeout
	timerEngine->tick(5000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());

	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	result->clear();
	return true;
}

bool MdbMasterCashlessTest::testTimeoutStateSessionComplete() {
	TEST_NUMBER_EQUAL(true, gotoSibaStateWork(tester));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0300C8"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session complete
	cc->closeSession();
	tester->poll();
	TEST_HEXDATA_EQUAL("1304", tester->getSendData(), tester->getSendLen());

	// Non-response timeout
	timerEngine->tick(5000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());

	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	result->clear();
	return true;
}

bool MdbMasterCashlessTest::testTimeoutStateSessionEnd() {
	TEST_NUMBER_EQUAL(true, gotoSibaStateWork(tester));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session start
	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvData("0300C8"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendLen());
	TEST_STRING_EQUAL("<event=1,SessionBegin,20000>", result->getString());
	result->clear();

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Session complete
	cc->closeSession();
	tester->poll();
	TEST_HEXDATA_EQUAL("1304", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	tester->poll();
	TEST_HEXDATA_EQUAL("12", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Non-response timeout
	timerEngine->tick(5000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());

	// Reset
	tester->poll();
	TEST_HEXDATA_EQUAL("10", tester->getSendData(), tester->getSendLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	result->clear();
	return true;
}
