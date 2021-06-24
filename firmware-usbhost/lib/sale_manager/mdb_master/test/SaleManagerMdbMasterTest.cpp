#include "lib/sale_manager/mdb_master/SaleManagerMdbMasterCore.h"
#include "lib/sale_manager/include/SaleManager.h"

#include "common/memory/include/RamMemory.h"
#include "common/timer/include/TestRealTime.h"
#include "common/event/include/TestEventEngine.h"
#include "common/fiscal_register/include/TestFiscalRegister.h"
#include "common/utils/include/TestLed.h"
#include "common/mdb/slave/bill_validator/TestMdbSlaveBillValidator.h"
#include "common/mdb/slave/coin_changer/TestMdbSlaveCoinChanger.h"
#include "common/mdb/slave/cashless/TestMdbSlaveCashless.h"
#include "common/mdb/master/bill_validator/TestMdbMasterBillValidator.h"
#include "common/mdb/master/coin_changer/TestMdbMasterCoinChanger.h"
#include "common/mdb/master/cashless/TestMdbMasterCashless.h"
#include "common/logger/include/Logger.h"
#include "common/test/include/Test.h"

class TestSaleManagerMdbMasterEventEngine : public TestEventEngine {
public:
	TestSaleManagerMdbMasterEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case SaleManager::Event_AutomatState: procEventUint8(envelope, SaleManager::Event_AutomatState, "AutomatState"); break;
		default: return TestEventEngine::transmit(envelope);
		}
		return true;
	}
};

class SaleManagerMdbMasterTest : public TestSet {
public:
	SaleManagerMdbMasterTest();
	bool init();
	void cleanup();
	bool initConfigModem();
	bool initConfigAutomatProductPrice(uint16_t index, const char *device, uint16_t number, uint16_t value);
	bool gotoStateSale();
	bool testBillValidatorSaleOnly();
	bool testBillValidatorAndCoinChangerSale();
	bool testCoinChangerResetAfterSale();
	bool testCoinChangerEscrowRequest();
	bool testStateCashCreditSessionBegin();
	bool testStateCashVendingDeposite();
	bool testStateCashVendingSessionBegin();
	bool testStateCashChangeDeposite();
	bool testStateCashChangeSessionBegin();
	bool testStateCashCheckPrintingDeposite();
	bool testStateCashCheckPrintingSessionBegin();
	bool testStateCashRefundDeposite();
	bool testStateCashRefundSessionBegin();
	bool testCashNotEnoughCredit();
	bool testCashCreditHolding();
	bool testCashMultiVend();
	bool testCashMultiVendZeroChange();
	bool testCashMultiVendAndCreditHolding();
	bool testCashlessMasterCancel();
	bool testCashlessSale();
	bool testCashlessHoldingCredit();
	bool testMixedCashlessSale();
	bool testMixedCashlessHoldingCredit();
	bool testCashlessSaleWrongState();
	bool testStateCashlessCreditSessionEnd();
	bool testStateCashlessCreditOtherSessionEnd();
	bool testCashlessSaleSlaveCancel();
	bool testStateCashlessCreditDeposite();
	bool testStateCashlessCreditSessionBegin();
	bool testCashlessRevalue();
	bool testCashlessRevalueWrongState();
	bool testStateCashlessApprovingDeposite();
	bool testStateCashlessApprovingSessionBegin();
	bool testStateCashlessApprovingSessionEnd();
	bool testStateCashlessVendingDeposite();
	bool testStateCashlessVendingSessionBegin();
	bool testStateCashlessCheckPrintingDeposite();
	bool testStateCashlessCheckPrintingSessionBegin();
	bool testStateCashlessClosingDeposite();
	bool testStateCashlessClosingSessionBegin();
	bool testStateCashlessClosingOtherSessionEnd();
	bool testExternCashlessSale();
	bool testAzkoyenPalmaH();
	bool testTokenSale();
	bool testStateTokenVendingEventCancel();
	bool testPriceHoldingCashSale();
	bool testPriceHoldingCashlessSale();
	bool testPriceHoldingTokenSale();

private:
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	ConfigModem *config;
	ConfigProductIterator *product;
	TimerEngine timers;
	TestSaleManagerMdbMasterEventEngine *eventEngine;
	StringBuilder *result;
	TestFiscalRegister *fiscalRegister;
	TestLed *leds;
	EventRegistrar *chronicler;
	TestMdbMasterCoinChanger *masterCoinChanger;
	TestMdbMasterBillValidator *masterBillValidator;
	TestMdbMasterCashless *masterCashless1;
	TestMdbMasterCashless *masterCashless2;
	TestMdbMasterCashless *externCashless;
	MdbMasterCashlessStack *masterCashlessStack;
	TestMdbSlaveCashless *slaveCashless1;
	TestMdbSlaveCoinChanger *slaveCoinChanger;
	TestMdbSlaveBillValidator *slaveBillValidator;
	SaleManagerMdbMasterCore *core;

	bool deliverEvent(EventInterface *event);
	bool deliverEvent(uint16_t type);
	bool deliverEvent(EventDeviceId deviceId, uint16_t type);
};

TEST_SET_REGISTER(SaleManagerMdbMasterTest);

SaleManagerMdbMasterTest::SaleManagerMdbMasterTest() {
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testBillValidatorSaleOnly);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testBillValidatorAndCoinChangerSale);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testCoinChangerResetAfterSale);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testCoinChangerEscrowRequest);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashCreditSessionBegin);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashVendingDeposite);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashVendingSessionBegin);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashChangeDeposite);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashChangeSessionBegin);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashCheckPrintingDeposite);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashCheckPrintingSessionBegin);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashRefundDeposite);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashRefundSessionBegin);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testCashNotEnoughCredit);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testCashCreditHolding);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testCashMultiVend);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testCashMultiVendZeroChange);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testCashMultiVendAndCreditHolding);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testCashlessMasterCancel);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testCashlessSale);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testCashlessHoldingCredit);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testMixedCashlessSale);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testMixedCashlessHoldingCredit);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testCashlessSaleWrongState);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashlessCreditSessionEnd);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashlessCreditOtherSessionEnd);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testCashlessSaleSlaveCancel);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashlessCreditDeposite);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashlessCreditSessionBegin);
#if 0
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testCashlessRevalue);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testCashlessRevalueWrongState);
#endif
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashlessApprovingDeposite);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashlessApprovingSessionBegin);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashlessApprovingSessionEnd);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashlessVendingDeposite);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashlessVendingSessionBegin);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashlessCheckPrintingDeposite);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashlessCheckPrintingSessionBegin);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashlessClosingDeposite);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashlessClosingSessionBegin);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateCashlessClosingOtherSessionEnd);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testExternCashlessSale);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testAzkoyenPalmaH);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testTokenSale);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testStateTokenVendingEventCancel);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testPriceHoldingCashSale);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testPriceHoldingCashlessSale);
	TEST_CASE_REGISTER(SaleManagerMdbMasterTest, testPriceHoldingTokenSale);
}

bool SaleManagerMdbMasterTest::init() {
	TEST_NUMBER_EQUAL(true, initConfigModem());

	result = new StringBuilder;
	eventEngine = new TestSaleManagerMdbMasterEventEngine(result);
	fiscalRegister = new TestFiscalRegister(1, result);
	leds = new TestLed(result);
	chronicler = new EventRegistrar(config, &timers, eventEngine, realtime);
	masterCoinChanger = new TestMdbMasterCoinChanger(2, result);
	masterBillValidator = new TestMdbMasterBillValidator(3, result);
	masterCashless1 = new TestMdbMasterCashless(4, result);
	masterCashless2 = new TestMdbMasterCashless(5, result);
	externCashless =  new TestMdbMasterCashless(6, result);
	masterCashlessStack = new MdbMasterCashlessStack;
	masterCashlessStack->push(masterCashless1);
	masterCashlessStack->push(masterCashless2);
	masterCashlessStack->push(externCashless);
	slaveCashless1 = new TestMdbSlaveCashless(7, result);
	slaveCoinChanger = new TestMdbSlaveCoinChanger(result);
	slaveBillValidator = new TestMdbSlaveBillValidator(result);

	SaleManagerMdbMasterParams params;
	params.config = config;
	params.timers = &timers;
	params.eventEngine = eventEngine;
	params.fiscalRegister = fiscalRegister;
	params.leds = leds;
	params.chronicler = chronicler;
	params.masterCoinChanger = masterCoinChanger;
	params.masterBillValidator = masterBillValidator;
	params.masterCashlessStack = masterCashlessStack;
	params.slaveCashless1 = slaveCashless1;
	params.slaveCoinChanger = slaveCoinChanger;
	params.slaveBillValidator = slaveBillValidator;

	core = new SaleManagerMdbMasterCore(&params);
	result->clear();
	return true;
}

void SaleManagerMdbMasterTest::cleanup() {
	delete core;
	delete slaveBillValidator;
	delete slaveCoinChanger;
	delete slaveCashless1;
	delete masterCashlessStack;
	delete externCashless;
	delete masterCashless2;
	delete masterCashless1;
	delete masterBillValidator;
	delete masterCoinChanger;
	delete chronicler;
	delete leds;
	delete fiscalRegister;
	delete eventEngine;
	delete result;
	delete config;
	delete stat;
	delete realtime;
	delete memory;
}

bool SaleManagerMdbMasterTest::initConfigModem() {
	memory = new RamMemory(128000),
	memory->setAddress(0);
	realtime = new TestRealTime;
	stat = new StatStorage;
	config = new ConfigModem(memory, realtime, stat);

	ConfigAutomat *configAutomat = config->getAutomat();
	configAutomat->shutdown();
	configAutomat->addProduct("1", 1);
	configAutomat->addProduct("2", 2);
	configAutomat->addProduct("3", 3);
	configAutomat->addProduct("4", 4);
	configAutomat->addPriceList("CA", 0, Config3PriceIndexType_Base);
	configAutomat->addPriceList("DA", 1, Config3PriceIndexType_Base);

	TEST_NUMBER_EQUAL(MemoryResult_Ok, config->init());

	configAutomat->setDecimalPoint(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, configAutomat->save());

	product = configAutomat->createIterator();
	TEST_NUMBER_EQUAL(true, initConfigAutomatProductPrice(0, "CA", 0, 10));
	TEST_NUMBER_EQUAL(true, initConfigAutomatProductPrice(0, "DA", 1, 11));
	TEST_NUMBER_EQUAL(true, initConfigAutomatProductPrice(1, "CA", 0, 50));
	TEST_NUMBER_EQUAL(true, initConfigAutomatProductPrice(1, "DA", 1, 51));
	TEST_NUMBER_EQUAL(true, initConfigAutomatProductPrice(2, "CA", 0, 70));
	TEST_NUMBER_EQUAL(true, initConfigAutomatProductPrice(2, "DA", 1, 71));
	TEST_NUMBER_EQUAL(true, initConfigAutomatProductPrice(3, "CA", 0, 120));
	TEST_NUMBER_EQUAL(true, initConfigAutomatProductPrice(3, "DA", 1, 121));
	return true;
}

bool SaleManagerMdbMasterTest::initConfigAutomatProductPrice(uint16_t index, const char *device, uint16_t number, uint16_t value) {
	product->findByIndex(index);
	ConfigPrice *price = product->getPrice(device, number);
	price->data.price = value;
	TEST_NUMBER_EQUAL(MemoryResult_Ok, price->save());
	return true;
}

bool SaleManagerMdbMasterTest::gotoStateSale() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<SCL::reset><MCC::reset><MBV::reset()><MCL(4)::reset><MCL(5)::reset><MCL(6)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init masterCoinChanger
	masterCoinChanger->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Ready));
	TEST_STRING_EQUAL("<SCC::reset>", result->getString());
	result->clear();

	// Init slaveCashless
	slaveCashless1->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Reset));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><ledPayment=2>", result->getString());
	result->clear();

	// Enable slaveCoinChanger
	slaveCoinChanger->setReseted(true);
	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCoinChanger::Event_Enable));
	TEST_STRING_EQUAL("<MCC::enable>", result->getString());
	result->clear();

	// Enable slaveCashless
	slaveCashless1->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Init slaveBillValidator
	masterBillValidator->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterBillValidator::Event_Ready));
	TEST_STRING_EQUAL("<SBV::reset>", result->getString());
	result->clear();

	// Enable slaveBillValidator
	slaveBillValidator->setReseted(true);
	slaveBillValidator->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveBillValidator::Event_Enable));
	TEST_STRING_EQUAL("<MBV::enable>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::deliverEvent(EventInterface *event) {
	EventEnvelope envelope(EVENT_DATA_SIZE);
	if(event->pack(&envelope) == false) { return false; }
	core->proc(&envelope);
	return true;
}

bool SaleManagerMdbMasterTest::deliverEvent(uint16_t type) {
	EventInterface event(type);
	return deliverEvent(&event);
}

bool SaleManagerMdbMasterTest::deliverEvent(EventDeviceId deviceId, uint16_t type) {
	EventInterface event(deviceId, type);
	return deliverEvent(&event);
}

bool SaleManagerMdbMasterTest::testBillValidatorSaleOnly() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<SCL::reset><MCC::reset><MBV::reset()><MCL(4)::reset><MCL(5)::reset><MCL(6)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init masterBillValidator
	masterBillValidator->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterBillValidator::Event_Ready));
	TEST_STRING_EQUAL("<SBV::reset>", result->getString());
	result->clear();

	// Init slaveCashless
	slaveCashless1->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Reset));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><ledPayment=2>", result->getString());
	result->clear();

	// Enable slaveBillValidator
	slaveBillValidator->setReseted(true);
	slaveBillValidator->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveBillValidator::Event_Enable));
	TEST_STRING_EQUAL("<MBV::enable>", result->getString());
	result->clear();

	// Enable slaveCashless
	slaveCashless1->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Unwaited disable slaveBillValidator
	slaveBillValidator->setEnabled(false);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveBillValidator::Event_Disable));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Insert bill2
	Mdb::EventDeposite bill2(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill2));
	TEST_STRING_EQUAL("<SCL::setCredit(150)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 3, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	slaveBillValidator->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,70,150,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(80)>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testBillValidatorAndCoinChangerSale() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Unwaited disable slaveBillValidator
	slaveBillValidator->setEnabled(false);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveBillValidator::Event_Disable));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Unwaited disble slaveCoinChanger
	slaveCoinChanger->setEnabled(false);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCoinChanger::Event_Disable));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Insert bill2
	MdbMasterCoinChanger::EventCoin coin2(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 50, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("<SCL::setCredit(150)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 3, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<MCC::dispense(80)>", result->getString());
	result->clear();

	// Change complete
	slaveBillValidator->setEnabled(true);
	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,70,150,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testCoinChangerResetAfterSale() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<SCL::reset><MCC::reset><MBV::reset()><MCL(4)::reset><MCL(5)::reset><MCL(6)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Reset slaveCashless
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Reset));
	result->clear();

	// Enable slaveCashless
	slaveCashless1->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Init masterBillValidator
	masterBillValidator->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterBillValidator::Event_Ready));
	TEST_STRING_EQUAL("<SBV::reset>", result->getString());
	result->clear();

	// Enable slaveBillValidator
	slaveBillValidator->setReseted(true);
	slaveBillValidator->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveBillValidator::Event_Enable));
	TEST_STRING_EQUAL("<MBV::enable>", result->getString());
	result->clear();

	// Insert bill1
	result->clear();
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Init masterCoinChanger
	masterCoinChanger->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Ready));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(),MdbSlaveCashlessInterface::Event_VendRequest, 3, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<MCC::dispense(30)>", result->getString());
	result->clear();

	// Change complete
	slaveBillValidator->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,70,100,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<SCC::reset><MCC::disable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Enable slaveCoinChanger
	slaveCoinChanger->setReseted(true);
	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCoinChanger::Event_Enable));
	TEST_STRING_EQUAL("<MCC::enable>", result->getString());
	result->clear();
	return true;
}

/*
 * Для корректной работы режима multivend в случае наличного кредиты, мы не прокидываем
 * escrowRequest, а сразу выдаем сдачу.
 */
bool SaleManagerMdbMasterTest::testCoinChangerEscrowRequest() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Escrow request
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_EscrowRequest));
	TEST_STRING_EQUAL("<SCC::escrowRequest>", result->getString());
	result->clear();

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Insert bill2
	MdbMasterCoinChanger::EventCoin coin2(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 50, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("<SCL::setCredit(150)>", result->getString());
	result->clear();

	// Escrow request
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_EscrowRequest));
	TEST_STRING_EQUAL("<SCL::cancelVend><MCC::dispense(150)>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashCreditSessionBegin() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Unwaited session begin
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCL(4)::closeSession>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 3, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashVendingDeposite() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 3, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Insert coin2
	MdbMasterCoinChanger::EventCoin coin2(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 50, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<MCC::dispense(80)>", result->getString());
	result->clear();

	// Change complete
	slaveBillValidator->setEnabled(true);
	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,70,150,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashVendingSessionBegin() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 3, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Unwaited session begin
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCL(4)::closeSession>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<MCC::dispense(30)>", result->getString());
	result->clear();

	// Change complete
	slaveBillValidator->setEnabled(true);
	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,70,100,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashChangeDeposite() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 3, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	slaveBillValidator->setEnabled(true);
	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<MCC::dispense(30)>", result->getString());
	result->clear();

	// Insert coin2
	MdbMasterCoinChanger::EventCoin coin2(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 50, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Change complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<MCC::dispense(50)>", result->getString());
	result->clear();

	// Change complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,70,100,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashChangeSessionBegin() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 3, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<MCC::dispense(30)>", result->getString());
	result->clear();

	// Unwaited session begin
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCL(4)::closeSession>", result->getString());
	result->clear();

	// Change complete
	slaveBillValidator->setEnabled(true);
	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,70,100,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashCheckPrintingDeposite() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 3, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<MCC::dispense(30)>", result->getString());
	result->clear();

	// Change complete
	slaveBillValidator->setEnabled(true);
	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,70,100,0)>", result->getString());
	result->clear();

	// Insert coin2
	MdbMasterCoinChanger::EventCoin coin2(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 50, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(50)>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashCheckPrintingSessionBegin() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 3, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<MCC::dispense(30)>", result->getString());
	result->clear();

	// Change complete
	slaveBillValidator->setEnabled(true);
	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,70,100,0)>", result->getString());
	result->clear();

	// Unwaited session begin
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCL(4)::closeSession>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashRefundDeposite() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Cancel by automat
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendCancel));
	TEST_STRING_EQUAL("<MCC::dispense(100)>", result->getString());
	result->clear();

	// Insert coin2
	MdbMasterCoinChanger::EventCoin coin2(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 50, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Refund complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<MCC::dispense(50)>", result->getString());
	result->clear();

	// Refund complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashRefundSessionBegin() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Cancel by automat
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendCancel));
	TEST_STRING_EQUAL("<MCC::dispense(100)>", result->getString());
	result->clear();

	// Unwaited session begin
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCL(4)::closeSession>", result->getString());
	result->clear();

	// Refund complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testCashNotEnoughCredit() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(50)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest1(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 3, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest1));
	TEST_STRING_EQUAL("<SCL::denyVend><MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(50)>", result->getString());
	result->clear();

	// Insert bill2
	Mdb::EventDeposite bill2(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill2));
	TEST_STRING_EQUAL("<SCL::setCredit(100)>", result->getString());
	result->clear();

	// Push product button
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest1));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	slaveBillValidator->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<MCC::dispense(30)>", result->getString());
	result->clear();

	// Change complete
	slaveBillValidator->setEnabled(true);
	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,70,100,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testCashCreditHolding() {
	config->getAutomat()->setCreditHolding(true);
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Escrow request
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_EscrowRequest));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Cancel by automat
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendCancel));
	TEST_STRING_EQUAL("<SCL::setCredit(100)>", result->getString());
	result->clear();

	// Push wrong product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest1(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 5, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest1));
	TEST_STRING_EQUAL("<SCL::denyVend><MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest2(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 3, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest2));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	slaveBillValidator->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<MCC::dispense(30)>", result->getString());
	result->clear();

	// Change complete
	slaveBillValidator->setEnabled(true);
	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,70,100,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testCashMultiVend() {
	config->getAutomat()->setMultiVend(true);
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Escrow request
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_EscrowRequest));
	TEST_STRING_EQUAL("<SCL::cancelVend><MCC::dispense(100)>", result->getString());
	result->clear();

	// Change complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Insert bill2
	Mdb::EventDeposite bill2(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill2));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest1(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 1, 10);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest1));
	TEST_STRING_EQUAL("<SCL::approveVend=10><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	slaveBillValidator->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,10,10,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(90)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest2(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 2, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest2));
	TEST_STRING_EQUAL("<SCL::approveVend=50><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	slaveBillValidator->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,50,50,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(40)>", result->getString());
	result->clear();

	// Escrow request
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_EscrowRequest));
	TEST_STRING_EQUAL("<SCL::cancelVend><MCC::dispense(40)>", result->getString());
	result->clear();

	// Change complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testCashMultiVendZeroChange() {
	config->getAutomat()->setMultiVend(true);
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest1(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 2, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest1));
	TEST_STRING_EQUAL("<SCL::approveVend=50><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	slaveBillValidator->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,50,50,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(50)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest2(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 2, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest2));
	TEST_STRING_EQUAL("<SCL::approveVend=50><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	slaveBillValidator->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,50,50,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testCashMultiVendAndCreditHolding() {
	config->getAutomat()->setMultiVend(true);
	config->getAutomat()->setCreditHolding(true);
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Escrow request
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_EscrowRequest));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest1(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 2, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest1));
	TEST_STRING_EQUAL("<SCL::approveVend=50><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	slaveBillValidator->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,50,50,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(50)>", result->getString());
	result->clear();

	// Escrow request
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_EscrowRequest));
	TEST_STRING_EQUAL("<SCL::cancelVend><MCC::dispense(50)>", result->getString());
	result->clear();

	// Change complete
	slaveBillValidator->setEnabled(true);
	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testCashlessMasterCancel() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Master Cancel
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<SCL::cancelVend><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testCashlessSale() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Approving timeout
	timers.tick(MDBMASTER_APPROVING_TIMEUT);
	timers.execute();
	TEST_STRING_EQUAL("", result->getString());

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testCashlessHoldingCredit() {
	config->getAutomat()->setCashless2Click(true);
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Approving timeout
	timers.tick(MDBMASTER_APPROVING_TIMEUT);
	timers.execute();
	TEST_STRING_EQUAL("<SCL::denyVend>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::setCredit(121)>", result->getString());
	result->clear();

	// Push product button
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testMixedCashlessSale() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Approving timeout
	timers.tick(MDBMASTER_APPROVING_TIMEUT);
	timers.execute();
	TEST_STRING_EQUAL("", result->getString());

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Ephor, 100, Fiscal::Payment_Sberbank, 21);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/3,100,100,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testMixedCashlessHoldingCredit() {
	config->getAutomat()->setCashless2Click(true);
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Approving timeout
	timers.tick(MDBMASTER_APPROVING_TIMEUT);
	timers.execute();
	TEST_STRING_EQUAL("<SCL::denyVend>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Ephor, 100, Fiscal::Payment_Sberbank, 21);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::setCredit(121)>", result->getString());
	result->clear();

	// Push product button
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/3,100,100,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testCashlessSaleWrongState() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Set result false
	masterCashless1->setResultSale(false);

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)><SCL::denyVend><MCL(4)::closeSession><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashlessCreditSessionEnd() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<SCL::cancelVend><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashlessCreditOtherSessionEnd() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless2->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testCashlessSaleSlaveCancel() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend cancel
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendCancel));
	TEST_STRING_EQUAL("<MCL(4)::saleFailed>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashlessCreditDeposite() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());
	masterCashless1->setRefundAble(false);

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashlessCreditSessionBegin() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());
	masterCashless1->setRefundAble(false);

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Unwaited session begin
	EventUint32Interface credit2(masterCashless2->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit2));
	TEST_STRING_EQUAL("<MCL(5)::closeSession>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testCashlessRevalue() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	masterCashless1->setRefundAble(true);
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCL(4)::revalue(100)>", result->getString());
	result->clear();

	// Revalue approved
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCashless::Event_RevalueApproved));
	TEST_STRING_EQUAL("<SCL::setCredit(300)>", result->getString());
	result->clear();

	// Insert bill2
	MdbMasterCoinChanger::EventCoin coin2(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 50, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("<MCL(4)::revalue(50)>", result->getString());
	result->clear();

	// Revalue denied
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCashless::Event_RevalueDenied));
	TEST_STRING_EQUAL("<MCC::dispense(50)>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<SCL::cancelVend><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testCashlessRevalueWrongState() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	masterCashless1->setRefundAble(true);
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCL(4)::revalue(100)>", result->getString());
	result->clear();

	// Revalue approved
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCashless::Event_RevalueApproved));
	TEST_STRING_EQUAL("<SCL::setCredit(300)>", result->getString());
	result->clear();

	// Cashless session cancel
	masterCashless1->setResultRevalue(false);

	// Insert bill2
	MdbMasterCoinChanger::EventCoin coin2(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 50, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("<MCL(4)::close>", result->getString());
	result->clear();

	// Result error
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCashless::Event_RevalueDenied));
	TEST_STRING_EQUAL("<MCC::dispense(50)>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashlessApprovingDeposite() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashlessApprovingSessionBegin() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());
	masterCashless1->setRefundAble(false);

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Unwaited session begin
	EventUint32Interface credit2(masterCashless2->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit2));
	TEST_STRING_EQUAL("<MCL(5)::closeSession>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashlessApprovingSessionEnd() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());
	masterCashless1->setRefundAble(false);

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<SCL::denyVend><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashlessVendingDeposite() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashlessVendingSessionBegin() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());
	masterCashless1->setRefundAble(false);

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Unwaited session begin
	EventUint32Interface credit2(masterCashless2->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit2));
	TEST_STRING_EQUAL("<MCL(5)::closeSession>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashlessCheckPrintingDeposite() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashlessCheckPrintingSessionBegin() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());
	masterCashless1->setRefundAble(false);

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Unwaited session begin
	EventUint32Interface credit2(masterCashless2->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit2));
	TEST_STRING_EQUAL("<MCL(5)::closeSession>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashlessClosingDeposite() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashlessClosingSessionBegin() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());
	masterCashless1->setRefundAble(false);

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Unwaited session begin
	EventUint32Interface credit2(masterCashless2->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit2));
	TEST_STRING_EQUAL("<MCL(5)::closeSession>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateCashlessClosingOtherSessionEnd() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());
	masterCashless1->setRefundAble(false);

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Other session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless2->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testExternCashlessSale() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	EventUint32Interface credit1(externCashless->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(6)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(6)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(externCashless->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testAzkoyenPalmaH() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<SCL::reset><MCC::reset><MBV::reset()><MCL(4)::reset><MCL(5)::reset><MCL(6)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init masterCoinChanger
	masterCoinChanger->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Ready));
	TEST_STRING_EQUAL("<SCC::reset>", result->getString());
	result->clear();

	// Enable slaveCoinChanger
	slaveCoinChanger->setReseted(true);
	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCoinChanger::Event_Enable));
	TEST_STRING_EQUAL("", result->getString());

	// Init masterBillValidator
	masterBillValidator->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterBillValidator::Event_Ready));
	TEST_STRING_EQUAL("<SBV::reset>", result->getString());
	result->clear();

	// Init slaveCashless
	slaveCashless1->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Reset));
	TEST_STRING_EQUAL("<MCC::enable><MBV::disable><ledPayment=2>", result->getString());
	result->clear();

	// Enable slaveBillValidator
	slaveBillValidator->setReseted(true);
	slaveBillValidator->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveBillValidator::Event_Enable));
	TEST_STRING_EQUAL("<MBV::enable>", result->getString());
	result->clear();

	// Enable slaveCashless
	slaveCashless1->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Unwaited disable slaveBillValidator
	slaveBillValidator->setEnabled(false);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveBillValidator::Event_Disable));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Insert bill2
	Mdb::EventDeposite bill2(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill2));
	TEST_STRING_EQUAL("<SCL::setCredit(150)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 3, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<MCC::dispense(80)>", result->getString());
	result->clear();

	// Change complete
	slaveBillValidator->setEnabled(true);
	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,70,150,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testTokenSale() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert token1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_DepositeToken, 0, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(50000)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 3, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,TA/2,70,70,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testStateTokenVendingEventCancel() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_DepositeToken, 0, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(50000)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 3, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend cancel
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendCancel));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testPriceHoldingCashSale() {
	config->getAutomat()->setPriceHolding(true);
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(100)>", result->getString());
	result->clear();

	// Insert bill2
	Mdb::EventDeposite bill2(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill2));
	TEST_STRING_EQUAL("<SCL::setCredit(150)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 0xFFFF, 3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=3><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	slaveBillValidator->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<MCC::dispense(80)>", result->getString());
	result->clear();

	// Change complete
	slaveBillValidator->setEnabled(true);
	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCoinChanger::Event_Dispense));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,70,150,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testPriceHoldingCashlessSale() {
	config->getAutomat()->setPriceHolding(true);
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Start cashless session
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 0xFFFF, 4);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(4)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=4>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbMasterTest::testPriceHoldingTokenSale() {
	config->getAutomat()->setPriceHolding(true);
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Insert token1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_DepositeToken, 0, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><SCL::setCredit(50000)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 0xFFFF, 3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=70><MCC::disable><MBV::disable>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,TA/2,70,70,0)>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}
