#include "lib/sale_manager/exe_master/ExeMaster.h"

#include "common/executive/ExeProtocol.h"
#include "common/uart/include/TestUart.h"
#include "common/event/include/TestEventEngine.h"
#include "common/test/include/Test.h"
#include "common/logger/include/Logger.h"

using namespace Exe;

class TestExeMasterEventEngine : public TestEventEngine {
public:
	TestExeMasterEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case ExeMasterInterface::Event_NotReady: procEvent(envelope, ExeMasterInterface::Event_NotReady, "NotReady"); break;
		case ExeMasterInterface::Event_Ready: procEvent(envelope, ExeMasterInterface::Event_Ready, "Ready"); break;
		case ExeMasterInterface::Event_VendRequest: procEventUint8(envelope, ExeMasterInterface::Event_VendRequest, "VendRequest"); break;
		case ExeMasterInterface::Event_VendPrice: procEvent(envelope, ExeMasterInterface::Event_VendPrice, "VendPrice"); break;
		case ExeMasterInterface::Event_VendComplete: procEvent(envelope, ExeMasterInterface::Event_VendComplete, "VendComplete"); break;
		case ExeMasterInterface::Event_VendFailed: procEvent(envelope, ExeMasterInterface::Event_VendFailed, "VendFailed"); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}
};

class ExeMasterTest : public TestSet {
public:
	ExeMasterTest();
	bool init();
	void cleanup();
	bool testParity();
	bool testPrice();
	bool testFreeVend();
	bool testVendSucceed();
	bool testVendNotEnoughMoney();
	bool testStateReady2StateNotReady();
	bool testStateNotFoundRecvTimeout();
	bool testStateNotReadyRecvTimeout();
	bool testStateReadyStatusRecvTimeout();
	bool testStateReadyCreditRecvTimeout();
	bool testStateCreditShowRecvTimeout();
	bool testStateCreditDelayRecvTimeout();
	bool testStatePriceShowRecvTimeout();
	bool testStateApproveTimeout();
	bool testStateVendingTimeout();
	bool testStatePriceDelayRecvTimeout();
	bool testApproveVendInMiddle();

private:
	StringBuilder *result;
	TestExeMasterEventEngine *eventEngine;
	TimerEngine *timers;
	TestUart *uart;
	StatStorage *stat;
	ExeMaster *exe;
};

TEST_SET_REGISTER(ExeMasterTest);

ExeMasterTest::ExeMasterTest() {
	TEST_CASE_REGISTER(ExeMasterTest, testParity);
	TEST_CASE_REGISTER(ExeMasterTest, testPrice);
	TEST_CASE_REGISTER(ExeMasterTest, testFreeVend);
	TEST_CASE_REGISTER(ExeMasterTest, testVendSucceed);
	TEST_CASE_REGISTER(ExeMasterTest, testVendNotEnoughMoney);
	TEST_CASE_REGISTER(ExeMasterTest, testStateReady2StateNotReady);
	TEST_CASE_REGISTER(ExeMasterTest, testStateNotFoundRecvTimeout);
	TEST_CASE_REGISTER(ExeMasterTest, testStateNotReadyRecvTimeout);
	TEST_CASE_REGISTER(ExeMasterTest, testStateReadyStatusRecvTimeout);
	TEST_CASE_REGISTER(ExeMasterTest, testStateReadyCreditRecvTimeout);
	TEST_CASE_REGISTER(ExeMasterTest, testStateCreditShowRecvTimeout);
	TEST_CASE_REGISTER(ExeMasterTest, testStateCreditDelayRecvTimeout);
	TEST_CASE_REGISTER(ExeMasterTest, testStatePriceShowRecvTimeout);
	TEST_CASE_REGISTER(ExeMasterTest, testStateApproveTimeout);
	TEST_CASE_REGISTER(ExeMasterTest, testStateVendingTimeout);
	TEST_CASE_REGISTER(ExeMasterTest, testStatePriceDelayRecvTimeout);
	TEST_CASE_REGISTER(ExeMasterTest, testApproveVendInMiddle);
}

bool ExeMasterTest::init() {
	result = new StringBuilder;
	eventEngine = new TestExeMasterEventEngine(result);
	timers = new TimerEngine;
	uart = new TestUart(256);
	stat = new StatStorage;
	exe = new ExeMaster(uart, timers, eventEngine, stat);
	return true;
}

void ExeMasterTest::cleanup() {
	delete exe;
	delete stat;
	delete uart;
	delete timers;
	delete eventEngine;
	delete result;
}

bool ExeMasterTest::testParity() {
	TEST_NUMBER_EQUAL(1, ExeMasterPacketLayer::calcParity(35));
	TEST_NUMBER_EQUAL(1, ExeMasterPacketLayer::calcParity(37));
	TEST_NUMBER_EQUAL(1, ExeMasterPacketLayer::calcParity(41));
	TEST_NUMBER_EQUAL(1, ExeMasterPacketLayer::calcParity(42));
	TEST_NUMBER_EQUAL(1, ExeMasterPacketLayer::calcParity(55));
	TEST_NUMBER_EQUAL(0, ExeMasterPacketLayer::calcParity(33));
	TEST_NUMBER_EQUAL(0, ExeMasterPacketLayer::calcParity(34));
	TEST_NUMBER_EQUAL(0, ExeMasterPacketLayer::calcParity(39));
	TEST_NUMBER_EQUAL(0, ExeMasterPacketLayer::calcParity(45));
	TEST_NUMBER_EQUAL(0, ExeMasterPacketLayer::calcParity(57));
	return true;
}

bool ExeMasterTest::testPrice() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// notready status
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// ready status
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// ready credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());

	// ready status
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// ready credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(0x07);
	TEST_STRING_EQUAL("<event=1,VendRequest,7>", result->getString());
	result->clear();

	// ----------
	// Show price
	// ----------
	exe->setPrice(2500);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C29202120242039", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);

	// vend price
	TEST_STRING_EQUAL("<event=1,VendPrice>", result->getString());
	result->clear();
	exe->denyVend(2500);

	// ready status
	for(uint16_t i = 0; i < EXE_SHOW_DELAY_COUNT; i++) {
		LOG(".");
		timers->tick(EXE_POLL_TIMEOUT);
		timers->execute();
		TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
		uart->clearSendBuffer();
		uart->addRecvData(Control_ACK);
		TEST_STRING_EQUAL("", result->getString());
	}

	// -------------
	// price timeout
	// -------------
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38202020202120242039", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);

	// ready status
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool ExeMasterTest::testFreeVend() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// status(READY)
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// ------------
	// Vend request
	// ------------
	// credit credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(0x07);
	TEST_STRING_EQUAL("<event=1,VendRequest,7>", result->getString());
	result->clear();

	// vend price
	exe->setPrice(0);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38202020202120242039", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<event=1,VendPrice>", result->getString());
	result->clear();

	// approve vend
	exe->approveVend(0);

	// vend
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("33", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,VendComplete>", result->getString());
	result->clear();

	// ---------------
	// Change returned
	// ---------------
	// credit status
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// credit credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());

	// update credit
	exe->setCredit(0);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38202020202120242039", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	uart->clearSendBuffer();

	for(uint16_t i = 0; i < EXE_SHOW_DELAY_COUNT; i++) {
		// credit status
		timers->tick(EXE_POLL_TIMEOUT);
		timers->execute();
		TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
		uart->clearSendBuffer();
		uart->addRecvData(Control_ACK);
		TEST_STRING_EQUAL("", result->getString());
	}

	// credit credit
	timers->tick(EXE_SHOW_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());

	// credit status
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool ExeMasterTest::testVendSucceed() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// init sequence
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	uart->addRecvData(Control_PNAK);
	TEST_STRING_EQUAL("", result->getString());

	// status(BUSY)
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	uart->addRecvData(CommandStatus_VendingInhibited);
	TEST_STRING_EQUAL("", result->getString());

	// status(READY)
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// ----------
	// Set credit
	// ----------
	// data
	exe->setCredit(2500);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("38", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("3824", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C29", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C2920", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C292021", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C29202120", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C2920212024", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C292021202420", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C29202120242039", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	uart->clearSendBuffer();

	// credit delay
	for(uint16_t i = 0; i < EXE_SHOW_DELAY_COUNT; i++) {
		// credit status
		timers->tick(EXE_POLL_TIMEOUT);
		timers->execute();
		TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
		uart->clearSendBuffer();
		uart->addRecvData(Control_ACK);
		TEST_STRING_EQUAL("", result->getString());
	}

	// credit credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());

	// credit status
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// ------------
	// Vend request
	// ------------
	// credit credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(0x07);
	TEST_STRING_EQUAL("<event=1,VendRequest,7>", result->getString());
	result->clear();

	// vend price
	exe->setPrice(1000);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38282E23202120242039", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<event=1,VendPrice>", result->getString());
	result->clear();

	// approve pause
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());

	// vend
	exe->approveVend(1000);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("33", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,VendComplete>", result->getString());
	result->clear();

	// ---------------
	// Change returned
	// ---------------
	// credit status
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// credit credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());

	// update credit
	exe->setCredit(0);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38202020202120242039", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	uart->clearSendBuffer();

	// credit delay
	for(uint16_t i = 0; i < EXE_SHOW_DELAY_COUNT; i++) {
		// credit status
		timers->tick(EXE_POLL_TIMEOUT);
		timers->execute();
		TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
		uart->clearSendBuffer();
		uart->addRecvData(Control_ACK);
		TEST_STRING_EQUAL("", result->getString());
	}

	// credit credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());

	// credit status
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool ExeMasterTest::testVendNotEnoughMoney() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// status(BUSY)
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	uart->addRecvData(CommandStatus_VendingInhibited);
	TEST_STRING_EQUAL("", result->getString());

	// status(READY)
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());

	// ----------
	// Set credit
	// ----------
	// data
	exe->setCredit(2500);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("38", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("3824", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C29", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C2920", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C292021", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C29202120", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C2920212024", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C292021202420", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C29202120242039", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	uart->clearSendBuffer();

	// credit delay
	for(uint16_t i = 0; i < EXE_SHOW_DELAY_COUNT; i++) {
		// credit status
		timers->tick(EXE_POLL_TIMEOUT);
		timers->execute();
		TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
		uart->clearSendBuffer();
		uart->addRecvData(Control_ACK);
		TEST_STRING_EQUAL("", result->getString());
	}

	// ------------
	// Vend request
	// ------------
	// credit credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(0x07);
	TEST_STRING_EQUAL("<event=1,VendRequest,7>", result->getString());
	result->clear();

	// vend price
	exe->setPrice(5000);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38282823212120242039", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<event=1,VendPrice>", result->getString());
	result->clear();

	// approve pause
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());

	// approve vend
	exe->denyVend(5000);

	// vend price delay
	for(uint16_t i = 0; i < EXE_SHOW_DELAY_COUNT; i++) {
		// credit status
		timers->tick(EXE_POLL_TIMEOUT);
		timers->execute();
		TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
		uart->clearSendBuffer();
		uart->addRecvData(Control_ACK);
		TEST_STRING_EQUAL("", result->getString());
	}

	// ---------------
	// Showtime is out
	// ---------------
	// update credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C29202120242039", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	uart->clearSendBuffer();

	// credit delay
	for(uint16_t i = 0; i < EXE_SHOW_DELAY_COUNT; i++) {
		// credit status
		timers->tick(EXE_POLL_TIMEOUT);
		timers->execute();
		TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
		uart->clearSendBuffer();
		uart->addRecvData(Control_ACK);
		TEST_STRING_EQUAL("", result->getString());
	}

	// ---------------
	// Showtime is out
	// ---------------
	// credit credit
	timers->tick(EXE_SHOW_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());

	// credit status
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// credit credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool ExeMasterTest::testStateReady2StateNotReady() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// status(BUSY)
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandStatus_VendingInhibited);
	TEST_STRING_EQUAL("", result->getString());

	// status(READY)
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());

	// status(BUSY)
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandStatus_VendingInhibited);
	TEST_STRING_EQUAL("<event=1,NotReady>", result->getString());
	result->clear();

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandStatus_VendingInhibited);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool ExeMasterTest::testStateNotFoundRecvTimeout() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// stateNotFound
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	exe->recvTimeout();
	TEST_STRING_EQUAL("", result->getString());

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	exe->recvTimeout();
	TEST_STRING_EQUAL("", result->getString());

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandStatus_VendingInhibited);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool ExeMasterTest::testStateNotReadyRecvTimeout() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// stateNotFound
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandStatus_VendingInhibited);
	TEST_STRING_EQUAL("", result->getString());

	// stateNotFound
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandStatus_VendingInhibited);
	TEST_STRING_EQUAL("", result->getString());

	// recv timeout
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	exe->recvTimeout();
	TEST_STRING_EQUAL("", result->getString());

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandStatus_VendingInhibited);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool ExeMasterTest::testStateReadyStatusRecvTimeout() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// stateNotFound
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// stateReadyStatus
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	exe->recvTimeout();
	TEST_STRING_EQUAL("", result->getString());

	// stateReadyStatus
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	exe->recvTimeout();
	TEST_STRING_EQUAL("", result->getString());

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool ExeMasterTest::testStateReadyCreditRecvTimeout() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// stateNotFound
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// stateReadyStatus
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// stateReadyCredit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	exe->recvTimeout();
	TEST_STRING_EQUAL("", result->getString());

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	exe->recvTimeout();
	TEST_STRING_EQUAL("", result->getString());

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool ExeMasterTest::testStateCreditShowRecvTimeout() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// stateNotFound
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// stateReadyStatus
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// stateReadyCredit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(0x07);
	TEST_STRING_EQUAL("<event=1,VendRequest,7>", result->getString());
	result->clear();

	// -----------------------
	// stateCreditShow timeout
	// -----------------------
	exe->setCredit(2500);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C292021", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	timers->tick(EXE_DATA_TIMEOUT);
	timers->execute();

	// ------------------------
	// stateCreditShow next try
	// ------------------------
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C29202120242039", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);

	// credit delay
	for(uint16_t i = 0; i < EXE_SHOW_DELAY_COUNT; i++) {
		// credit status
		timers->tick(EXE_POLL_TIMEOUT);
		timers->execute();
		TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
		uart->clearSendBuffer();
		uart->addRecvData(Control_ACK);
		TEST_STRING_EQUAL("", result->getString());
	}

	// credit credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());

	// credit status
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool ExeMasterTest::testStateCreditDelayRecvTimeout() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// stateNotFound
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// stateReadyStatus
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// stateReadyCredit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(0x07);
	TEST_STRING_EQUAL("<event=1,VendRequest,7>", result->getString());
	result->clear();

	// stateCreditShow
	exe->setCredit(2500);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C29202120242039", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	uart->clearSendBuffer();

	// credit delay
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// credit delay
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	exe->recvTimeout();
	TEST_STRING_EQUAL("", result->getString());

	// stateReadyStatus
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// stateReadyCredit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool ExeMasterTest::testStatePriceShowRecvTimeout() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// stateNotFound
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// stateReadyStatus
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// stateReadyCredit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(0x07);
	TEST_STRING_EQUAL("<event=1,VendRequest,7>", result->getString());
	result->clear();

	// ----------------------
	// statePriceShow timeout
	// ----------------------
	exe->setPrice(2500);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C292021", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	timers->tick(EXE_DATA_TIMEOUT);
	timers->execute();

	// -----------------------
	// statePriceShow next try
	// -----------------------
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C29202120242039", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());

	// vend price
	TEST_STRING_EQUAL("<event=1,VendPrice>", result->getString());
	result->clear();
	exe->denyVend(2500);

	// vend price delay
	for(uint16_t i = 0; i < EXE_SHOW_DELAY_COUNT; i++) {
		// credit status
		timers->tick(EXE_POLL_TIMEOUT);
		timers->execute();
		TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
		uart->clearSendBuffer();
		uart->addRecvData(Control_ACK);
		TEST_STRING_EQUAL("", result->getString());
	}

	// ---------------
	// Showtime is out
	// ---------------
	// update credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38202020202120242039", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	uart->clearSendBuffer();

	// credit delay
	for(uint16_t i = 0; i < EXE_SHOW_DELAY_COUNT; i++) {
		// credit status
		timers->tick(EXE_POLL_TIMEOUT);
		timers->execute();
		TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
		uart->clearSendBuffer();
		uart->addRecvData(Control_ACK);
		TEST_STRING_EQUAL("", result->getString());
	}

	// ---------------
	// Showtime is out
	// ---------------
	// credit credit
	timers->tick(EXE_SHOW_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());

	// credit status
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// credit credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool ExeMasterTest::testStateApproveTimeout() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// stateNotFound
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// stateReadyStatus
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// stateReadyCredit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(0x07);
	TEST_STRING_EQUAL("<event=1,VendRequest,7>", result->getString());
	result->clear();

	// statePriceShow
	exe->setPrice(2500);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C29202120242039", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);

	// vend price
	TEST_STRING_EQUAL("<event=1,VendPrice>", result->getString());
	result->clear();

	// approve timeout
	timers->tick(EXE_APPROVE_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());

	// stateReadyStatus
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// stateReadyCredit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool ExeMasterTest::testStateVendingTimeout() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// stateNotFound
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// stateReadyStatus
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// stateReadyCredit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(0x07);
	TEST_STRING_EQUAL("<event=1,VendRequest,7>", result->getString());
	result->clear();

	// vend price
	exe->setPrice(1000);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38282E23202120242039", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<event=1,VendPrice>", result->getString());
	result->clear();

	// approve pause
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());

	// vend failed
	exe->approveVend(1000);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("33", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	exe->recvTimeout();
	TEST_STRING_EQUAL("<event=1,VendFailed>", result->getString());
	result->clear();

	// stateReadyStatus
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// stateReadyCredit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool ExeMasterTest::testStatePriceDelayRecvTimeout() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// stateNotFound
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	// stateReadyStatus
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// stateReadyCredit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(0x07);
	TEST_STRING_EQUAL("<event=1,VendRequest,7>", result->getString());
	result->clear();

	// vend price
	exe->setPrice(1000);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38282E23202120242039", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<event=1,VendPrice>", result->getString());
	result->clear();

	// approve pause
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());

	// deny vend
	exe->denyVend(5000);

	// statePriceDelay
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// recvTimeout
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	exe->recvTimeout();
	TEST_STRING_EQUAL("", result->getString());

	// stateReadyStatus
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// stateReadyCredit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool ExeMasterTest::testApproveVendInMiddle() {
	exe->setDicimalPoint(2);
	exe->setChange(true);
	exe->reset();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// init sequence
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	uart->addRecvData(Control_PNAK);
	TEST_STRING_EQUAL("", result->getString());

	// status(BUSY)
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	uart->addRecvData(CommandStatus_VendingInhibited);
	TEST_STRING_EQUAL("", result->getString());

	// status(READY)
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,Ready>", result->getString());
	result->clear();

	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// ----------
	// Set credit
	// ----------
	// data
	exe->setCredit(2500);
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("38", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("3824", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C29", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C2920", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C292021", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C29202120", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C2920212024", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C292021202420", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	TEST_HEXDATA_EQUAL("38242C29202120242039", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ACK);
	uart->clearSendBuffer();

	// credit delay
	for(uint16_t i = 0; i < EXE_SHOW_DELAY_COUNT; i++) {
		// credit status
		timers->tick(EXE_POLL_TIMEOUT);
		timers->execute();
		TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
		uart->clearSendBuffer();
		uart->addRecvData(Control_ACK);
		TEST_STRING_EQUAL("", result->getString());
	}

	// credit credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());

	// credit status
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// ------------
	// Vend request
	// ------------
	// credit credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(0x07);
	TEST_STRING_EQUAL("<event=1,VendRequest,7>", result->getString());
	result->clear();

	// -----------------
	// Async approveVend
	// -----------------
	// credit status
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// approveVend command
	exe->approveVend(1000);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// credit status response
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// approveVend send
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("33", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("<event=1,VendComplete>", result->getString());
	result->clear();

	// -----------------------
	// Return to waiting state
	// -----------------------
	// credit status
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("31", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_ACK);
	TEST_STRING_EQUAL("", result->getString());

	// credit credit
	timers->tick(EXE_POLL_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("32", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(CommandCredit_NoVendRequest);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}
