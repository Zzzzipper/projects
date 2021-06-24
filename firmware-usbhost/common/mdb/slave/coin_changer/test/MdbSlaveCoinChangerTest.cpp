#include "mdb/slave/coin_changer/MdbSlaveCoinChanger.h"
#include "mdb/slave/MdbSlaveTester.h"
#include "event/include/TestEventEngine.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class TestSlaveCoinChangerEventEngine : public TestEventEngine {
public:
	TestSlaveCoinChangerEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case MdbSlaveCoinChanger::Event_Enable: procEvent(envelope, MdbSlaveCoinChanger::Event_Enable, "Enable"); break;
		case MdbSlaveCoinChanger::Event_Disable: procEvent(envelope, MdbSlaveCoinChanger::Event_Disable, "Disable"); break;
		case MdbSlaveCoinChanger::Event_DispenseCoin: procEvent(envelope, MdbSlaveCoinChanger::Event_DispenseCoin, "DispenseCoin"); break;
		case MdbSlaveCoinChanger::Event_Error: procEventError(envelope, MdbSlaveCoinChanger::Event_Error, "Error"); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}

private:
	void procEventError(EventEnvelope *envelope, uint16_t type, const char *name) {
		Mdb::EventError event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.code << "," << event.data.getString() << ">";
	}
};

class MdbSlaveCoinChangerTest : public TestSet {
public:
	MdbSlaveCoinChangerTest();
	bool init();
	void cleanup();
	bool testInit();
	bool gotoStateEnabled();
	bool testSelfReset();
	bool testManualFilling();
	bool testCoffemarMenuDispense();
	bool testCoffemarMenuDispenseBruteForce();
	bool testEscrowRequest();
	bool testStateResetUnsupportedPacket();
	bool testStateResetUnwaitedPacket();
	bool testStateDisabledExpansionPayout();
	bool testStateEnabledExpansionFeatureEnable();

private:
	StringBuilder *result;
	TestSlaveCoinChangerEventEngine *eventEngine;
	MdbCoinChangerContext *context;
	StatStorage *stat;
	MdbSlaveCoinChanger *slave;
	MdbSlaveTester *tester;
};

TEST_SET_REGISTER(MdbSlaveCoinChangerTest);

MdbSlaveCoinChangerTest::MdbSlaveCoinChangerTest() {
	TEST_CASE_REGISTER(MdbSlaveCoinChangerTest, testInit);
	TEST_CASE_REGISTER(MdbSlaveCoinChangerTest, testSelfReset);
	TEST_CASE_REGISTER(MdbSlaveCoinChangerTest, testManualFilling);
	TEST_CASE_REGISTER(MdbSlaveCoinChangerTest, testCoffemarMenuDispense);
	TEST_CASE_REGISTER(MdbSlaveCoinChangerTest, testCoffemarMenuDispenseBruteForce);
	TEST_CASE_REGISTER(MdbSlaveCoinChangerTest, testEscrowRequest);
	TEST_CASE_REGISTER(MdbSlaveCoinChangerTest, testStateResetUnsupportedPacket);
	TEST_CASE_REGISTER(MdbSlaveCoinChangerTest, testStateResetUnwaitedPacket);
	TEST_CASE_REGISTER(MdbSlaveCoinChangerTest, testStateDisabledExpansionPayout);
	TEST_CASE_REGISTER(MdbSlaveCoinChangerTest, testStateEnabledExpansionFeatureEnable);
}

bool MdbSlaveCoinChangerTest::init() {
	result = new StringBuilder;
	eventEngine = new TestSlaveCoinChangerEventEngine(result);
	stat = new StatStorage;

	context = new MdbCoinChangerContext(2, NULL);
	context->setManufacturer((uint8_t*)MDB_MANUFACTURER_CODE, MDB_MANUFACTURER_SIZE);
	context->setModel((uint8_t*)"0123456789AB", MDB_MODEL_SIZE);
	context->setSerialNumber((uint8_t*)"0123456789AB", MDB_SERIAL_NUMBER_SIZE);
	context->setCurrency(RUSSIAN_CURRENCY_RUB);
	uint8_t coinData[]   = { 0x00, 0x00, 0x01, 0x02, 0x05, 0x0A, 0x0A, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	uint8_t coinStatus[] = { 0x00, 0x00, 0x41, 0x37, 0x57, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	context->init(1, 1, 10, coinData, sizeof(coinData), 0x005C);
	context->update(0x0040, coinStatus, sizeof(coinStatus));

	slave = new MdbSlaveCoinChanger(context, eventEngine, stat);
	tester = new MdbSlaveTester(slave);
	slave->reset();
	return true;
}

void MdbSlaveCoinChangerTest::cleanup() {
	delete tester;
	delete slave;
	delete context;
	delete stat;
	delete eventEngine;
	delete result;
}

bool MdbSlaveCoinChangerTest::testInit() {
	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup
	TEST_NUMBER_EQUAL(true, tester->recvCommand("09"));
	TEST_HEXDATA_EQUAL("0316430A01005C00000102050A0AFF0000000000000000", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("09"));
	TEST_HEXDATA_EQUAL("0316430A01005C00000102050A0AFF0000000000000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification (последние 2 байта - это вкомпилированная версия и она меняется)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F00"));
	TEST_HEXDATA_EQUAL("454652303132333435363738394142303132333435363738394142010000000001", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F00"));
	TEST_HEXDATA_EQUAL("454652303132333435363738394142303132333435363738394142010000000001", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionFeatureEnable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F010000000F"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// TubeStatus
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0A"));
	TEST_HEXDATA_EQUAL("004000004137570000000000000000000000", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("0A"));
	TEST_HEXDATA_EQUAL("004000004137570000000000000000000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// CoinType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0C00000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	return true;
}

bool MdbSlaveCoinChangerTest::gotoStateEnabled() {
	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup
	TEST_NUMBER_EQUAL(true, tester->recvCommand("09"));
	TEST_HEXDATA_EQUAL("0316430A01005C00000102050A0AFF0000000000000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification (последние 2 байта - это вкомпилированная версия и она меняется)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F00"));
	TEST_HEXDATA_EQUAL("454652303132333435363738394142303132333435363738394142010000000001", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionFeatureEnable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F010000000F"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// TubeStatus
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0A"));
	TEST_HEXDATA_EQUAL("004000004137570000000000000000000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Coin type
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0C00FC005C"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	tester->clearSendData();
	return true;
}

bool MdbSlaveCoinChangerTest::testSelfReset() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll after reset
	slave->reset();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// TubeStatus
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0A"));
	TEST_HEXDATA_EQUAL("004000004137570000000000000000000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	return true;
}

bool MdbSlaveCoinChangerTest::testManualFilling() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Enable tube filling
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0C00FC0000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll deposite
	slave->deposite(0x42, 0x10);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("4210", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("4210", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Disable tube filling
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0C00000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Disable>", result->getString());
	result->clear();
	return true;
}

/**
 * Последовательность выдачи монет через меню торгового автомата
 * Автомат: Coffeemar G-250, 2013 года
 * Прошика: F02
 * Монетник: родной
 */
bool MdbSlaveCoinChangerTest::testCoffemarMenuDispense() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Disable tube filling and enable manual dispense
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0C0000FFFF"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Disable>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// TubeStatus
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0A"));
	TEST_HEXDATA_EQUAL("004000004137570000000000000000000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Dispense 1 coin with index 2
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0D12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,DispenseCoin>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("02", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("02", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	slave->dispenseComplete();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// TubeStatus
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0A"));
	TEST_HEXDATA_EQUAL("004000004137570000000000000000000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("08"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Disable tube filling and enable manual dispense
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0C0000FFFF"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// TubeStatus
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0A"));
	TEST_HEXDATA_EQUAL("004000004137570000000000000000000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	return true;
}

/**
 * Последовательность выдачи всех монет через меню торгового автомата
 * Автомат: Coffeemar G-250, 2013 года
 * Прошика: F02
 * Монетник: родной
 */
bool MdbSlaveCoinChangerTest::testCoffemarMenuDispenseBruteForce() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Disable tube filling and enable manual dispense
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0C0000FFFF"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Disable>", result->getString());
	result->clear();

	// Dispense 1 coin with index 2
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0D12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,DispenseCoin>", result->getString());
	result->clear();

	// Dispense 1 coin with index 2
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0D12"));
	TEST_STRING_EQUAL("<event=1,Error,1283,scc4*8*0500*0D12>", result->getString());
	result->clear();

	// Dispense 1 coin with index 2
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0D12"));
	TEST_STRING_EQUAL("<event=1,Error,1283,scc4*8*0500*0D12>", result->getString());
	result->clear();

	// Dispense 1 coin with index 2
	slave->dispenseComplete();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0D12"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,DispenseCoin>", result->getString());
	result->clear();

	// Dispense 1 coin with index 2
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0D12"));

	// Dispense 1 coin with index 2
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0D12"));

	// Poll
	slave->dispenseComplete();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// TubeStatus
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0A"));
	TEST_HEXDATA_EQUAL("004000004137570000000000000000000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

bool MdbSlaveCoinChangerTest::testEscrowRequest() {
	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup
	TEST_NUMBER_EQUAL(true, tester->recvCommand("09"));
	TEST_HEXDATA_EQUAL("0316430A01005C00000102050A0AFF0000000000000000", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("09"));
	TEST_HEXDATA_EQUAL("0316430A01005C00000102050A0AFF0000000000000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification (последние 2 байта - это вкомпилированная версия и она меняется)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F00"));
	TEST_HEXDATA_EQUAL("454652303132333435363738394142303132333435363738394142010000000001", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F00"));
	TEST_HEXDATA_EQUAL("454652303132333435363738394142303132333435363738394142010000000001", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionFeatureEnable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F010000000F"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// TubeStatus
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0A"));
	TEST_HEXDATA_EQUAL("004000004137570000000000000000000000", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("0A"));
	TEST_HEXDATA_EQUAL("004000004137570000000000000000000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// CoinType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0C00000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Escrow request
	slave->escrowRequest();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("01", tester->getSendData(), tester->getSendDataLen());

	return true;
}

bool MdbSlaveCoinChangerTest::testStateResetUnsupportedPacket() {
	// Unsupported packet
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0FAA"));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendDataLen());
	TEST_STRING_EQUAL("<event=1,Error,1283,scc1*8*07AA*>", result->getString());
	return true;
}

bool MdbSlaveCoinChangerTest::testStateResetUnwaitedPacket() {
	// FTL REQ TO RCV
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0FFA0102030405"));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendDataLen());
	TEST_STRING_EQUAL("<event=1,Error,1283,scc1*8*07FA*>", result->getString());
	return true;
}

/**
 * Неожиданная команда ExpansionPayout
 * Автомат: EVEND
 * Прошика:
 */
bool MdbSlaveCoinChangerTest::testStateDisabledExpansionPayout() {
	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("00"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	TEST_HEXDATA_EQUAL("0B", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup
	TEST_NUMBER_EQUAL(true, tester->recvCommand("09"));
	TEST_HEXDATA_EQUAL("0316430A01005C00000102050A0AFF0000000000000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification (последние 2 байта - это вкомпилированная версия и она меняется)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F00"));
	TEST_HEXDATA_EQUAL("454652303132333435363738394142303132333435363738394142010000000001", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionFeatureEnable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F010000000F"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// TubeStatus
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0A"));
	TEST_HEXDATA_EQUAL("004000004137570000000000000000000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	TEST_STRING_EQUAL("", result->getString());

	// Coin type
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0C00000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Payout
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F02C8"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// PayoutPoll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F04"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	// PayoutStatus
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F03"));
	TEST_HEXDATA_EQUAL("00", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));
	return true;
}

/**
 * Неожиданная команда ExpansionFeatureEnable
 * Автомат: CraneDixiNarco
 * Прошика:
 */
bool MdbSlaveCoinChangerTest::testStateEnabledExpansionFeatureEnable() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// ExpansionFeatureEnable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F010000000F"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	return true;
}
