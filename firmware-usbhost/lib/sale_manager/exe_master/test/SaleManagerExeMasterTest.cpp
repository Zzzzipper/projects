#include "lib/sale_manager/exe_master/SaleManagerExeMasterCore.h"
#include "lib/sale_manager/include/SaleManager.h"

#include "common/memory/include/RamMemory.h"
#include "common/timer/include/TestRealTime.h"
#include "common/event/include/TestEventEngine.h"
#include "common/fiscal_register/include/TestFiscalRegister.h"
#include "common/utils/include/TestLed.h"
#include "common/mdb/master/bill_validator/TestMdbMasterBillValidator.h"
#include "common/mdb/master/coin_changer/TestMdbMasterCoinChanger.h"
#include "common/mdb/master/cashless/TestMdbMasterCashless.h"
#include "common/logger/include/Logger.h"
#include "common/test/include/Test.h"

class TestSaleManagerExeMasterEventEngine : public TestEventEngine {
public:
	TestSaleManagerExeMasterEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case SaleManager::Event_AutomatState: procEventUint8(envelope, SaleManager::Event_AutomatState, "AutomatState"); break;
		default: return TestEventEngine::transmit(envelope);
		}
		return true;
	}
};

class TestExeMaster : public ExeMasterInterface {
public:
	TestExeMaster(uint16_t deviceId, StringBuilder *str) : deviceId(deviceId), str(str) {}
	virtual ~TestExeMaster() {}
	virtual EventDeviceId getDeviceId() { return deviceId; }
	virtual void reset() { *str << "<EXE::reset>"; }
	virtual void setDicimalPoint(uint8_t decimalPoint) { *str << "<EXE::setDicimalPoint=" << decimalPoint << ">"; }
	virtual void setScalingFactor(uint8_t scalingFactor) { *str << "<EXE::setScalingFactor=" << scalingFactor << ">"; }
	virtual void showPrice(uint32_t price) { *str << "<EXE::showPrice=" << price << ">"; }
	virtual void setChange(bool change) { *str << "<EXE::setChange=" << change << ">"; }
	virtual void setCredit(uint32_t credit) { *str << "<EXE::setCredit=" << credit << ">"; }
	virtual void setPrice(uint32_t price) { *str << "<EXE::setPrice=" << price << ">"; }
	virtual void approveVend(uint32_t credit) { *str << "<EXE::approveVend=" << credit << ">"; }
	virtual void denyVend(uint32_t price) { *str << "<EXE::denyVend=" << price << ">"; }
	virtual bool isEnabled() { return true; }

private:
	EventDeviceId deviceId;
	StringBuilder *str;
};

class SaleManagerExeMasterTest : public TestSet {
public:
	SaleManagerExeMasterTest();
	bool init();
	void cleanup();
	bool initConfigModem();
	void initConfigAutomatProductPrice(uint16_t index, const char *device, uint16_t number, uint16_t value);
	bool gotoStateEnabled();
	bool testFreeVend();
	bool testBillValidatorSaleOnly();
	bool testBillValidatorAndCoinChangerSale();
	bool testCoinChangerEscrowRequest();
	bool testCashNotEnough();
	bool testCashCreditHolding();
	bool testCashMultiVend();
	bool testCashMultiVendAndCreditHolding();
	bool testCashlessSale();
	bool testCashlessSaleBySecondClickTwoClickSale();
	bool testCashlessStartByClickSaleByFirstClick();
	bool testCashlessStartByClickSaleBySecondClick();
	bool testMixedCashlessSale();
	bool testMixedCashlessSaleBySecondClickTwoClickSale();
	bool testMixedCashlessStartByClickSaleByFirstClick();
	bool testMixedCashlessStartByClickSaleBySecondClick();
	bool testExternCashlessSale();
	bool testCashlessSaleVendDeny();
	bool testCashlessSaleVendFailed();
	bool testCashlessSaleSessionCancel();
	bool testAutomatTemporaryError();
	bool testCashlessRevalue();
	bool testCashlessRevalueAtStart();
	bool testTokenSale();
	bool testStateCashCreditEventToken();

private:
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	ConfigModem *config;
	ConfigProductIterator *product;
	ClientContext *clientContext;
	TimerEngine timers;
	TestSaleManagerExeMasterEventEngine *eventEngine;
	StringBuilder *result;
	TestFiscalRegister *fiscalRegister;
	TestLed *leds;
	EventRegistrar *chronicler;
	TestExeMaster *executive;
	TestMdbMasterCoinChanger *masterCoinChanger;
	TestMdbMasterBillValidator *masterBillValidator;
	TestMdbMasterCashless *masterCashless1;
	TestMdbMasterCashless *masterCashless2;
	TestMdbMasterCashless *externCashless;
	MdbMasterCashlessStack *masterCashlessStack;
	SaleManagerExeMasterCore *core;

	bool deliverEvent(EventInterface *event);
	bool deliverEvent(uint16_t type);
};

TEST_SET_REGISTER(SaleManagerExeMasterTest);

SaleManagerExeMasterTest::SaleManagerExeMasterTest() {
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testFreeVend);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testBillValidatorSaleOnly);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testBillValidatorAndCoinChangerSale);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testCoinChangerEscrowRequest);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testCashNotEnough);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testCashCreditHolding);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testCashMultiVend);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testCashMultiVendAndCreditHolding);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testCashlessSale);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testCashlessSaleBySecondClickTwoClickSale);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testCashlessStartByClickSaleByFirstClick);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testCashlessStartByClickSaleBySecondClick);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testMixedCashlessSale);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testMixedCashlessSaleBySecondClickTwoClickSale);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testMixedCashlessStartByClickSaleByFirstClick);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testMixedCashlessStartByClickSaleBySecondClick);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testExternCashlessSale);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testCashlessSaleVendDeny);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testCashlessSaleVendFailed);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testCashlessSaleSessionCancel);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testAutomatTemporaryError);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testCashlessRevalue);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testCashlessRevalueAtStart);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testTokenSale);
	TEST_CASE_REGISTER(SaleManagerExeMasterTest, testStateCashCreditEventToken);
}

bool SaleManagerExeMasterTest::init() {
	TEST_NUMBER_EQUAL(true, initConfigModem());

	result = new StringBuilder;
	clientContext = new ClientContext;
	eventEngine = new TestSaleManagerExeMasterEventEngine(result);
	fiscalRegister = new TestFiscalRegister(1, result);
	leds = new TestLed(result);
	chronicler = new EventRegistrar(config, &timers, eventEngine, realtime);
	executive = new TestExeMaster(7, result);
	masterCoinChanger = new TestMdbMasterCoinChanger(2, result);
	masterBillValidator = new TestMdbMasterBillValidator(3, result);
	masterCashless1 = new TestMdbMasterCashless(4, result);
	masterCashless2 = new TestMdbMasterCashless(5, result);
	externCashless = new TestMdbMasterCashless(6, result);
	masterCashlessStack = new MdbMasterCashlessStack;
	masterCashlessStack->push(masterCashless1);
	masterCashlessStack->push(masterCashless2);
	masterCashlessStack->push(externCashless);

	SaleManagerExeMasterParams params;
	params.config = config;
	params.client = clientContext;
	params.timers = &timers;
	params.eventEngine = eventEngine;
	params.fiscalRegister = fiscalRegister;
	params.leds = leds;
	params.chronicler = chronicler;
	params.executive = executive;
	params.masterCoinChanger = masterCoinChanger;
	params.masterBillValidator = masterBillValidator;
	params.masterCashlessStack = masterCashlessStack;

	core = new SaleManagerExeMasterCore(&params);
	result->clear();
	return true;
}

void SaleManagerExeMasterTest::cleanup() {
	delete core;
	delete masterCashlessStack;
	delete externCashless;
	delete masterCashless2;
	delete masterCashless1;
	delete masterBillValidator;
	delete masterCoinChanger;
	delete executive;
	delete chronicler;
	delete leds;
	delete fiscalRegister;
	delete eventEngine;
	delete clientContext;
	delete result;
	delete config;
	delete stat;
	delete realtime;
	delete memory;
}

bool SaleManagerExeMasterTest::initConfigModem() {
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
	initConfigAutomatProductPrice(0, "CA", 0, 0);
	initConfigAutomatProductPrice(0, "DA", 1, 0);
	initConfigAutomatProductPrice(1, "CA", 0, 50);
	initConfigAutomatProductPrice(1, "DA", 1, 51);
	initConfigAutomatProductPrice(2, "CA", 0, 70);
	initConfigAutomatProductPrice(2, "DA", 1, 71);
	initConfigAutomatProductPrice(3, "CA", 0, 120);
	initConfigAutomatProductPrice(3, "DA", 1, 121);
	return true;
}

void SaleManagerExeMasterTest::initConfigAutomatProductPrice(uint16_t index, const char *device, uint16_t number, uint16_t value) {
	product->findByIndex(index);
	ConfigPrice *price = product->getPrice(device, number);
	price->data.price = value;
	price->save();
}

bool SaleManagerExeMasterTest::deliverEvent(EventInterface *event) {
	EventEnvelope envelope(EVENT_DATA_SIZE);
	if(event->pack(&envelope) == false) { return false; }
	core->proc(&envelope);
	return true;
}

bool SaleManagerExeMasterTest::deliverEvent(uint16_t type) {
	EventInterface event(type);
	return deliverEvent(&event);
}

bool SaleManagerExeMasterTest::gotoStateEnabled() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL(
		"<EXE::setDicimalPoint=0><EXE::setChange=0><EXE::setScalingFactor=1>"
		"<EXE::reset><MCC::reset><MBV::reset()><MCL(4)::reset><MCL(5)::reset><MCL(6)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init masterBillValidator
	masterBillValidator->setInited(true);
	EventInterface event1(MdbMasterBillValidator::Event_Ready);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Init masterCoinChanger
	masterCoinChanger->setInited(true);
	masterCoinChanger->setChange(true);
	EventInterface event2(MdbMasterCoinChanger::Event_Ready);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event2));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Found VMC
	EventInterface event3(ExeMasterInterface::Event_NotReady);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event3));
	TEST_STRING_EQUAL("", result->getString());

	// Enable VMC
	EventInterface event4(ExeMasterInterface::Event_Ready);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event4));
	TEST_STRING_EQUAL("<ledPayment=2><EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testFreeVend() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Push product button
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)1);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=0>", result->getString());
	result->clear();

	// Vend price
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<EXE::approveVend=0><MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event4(ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event4));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,0,0,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event5(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testBillValidatorSaleOnly() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL(
		"<EXE::setDicimalPoint=0><EXE::setChange=0><EXE::setScalingFactor=1>"
		"<EXE::reset><MCC::reset><MBV::reset()><MCL(4)::reset><MCL(5)::reset><MCL(6)::reset><ledPayment=1>", result->getString());

	// Init masterBillValidator
	result->clear();
	masterBillValidator->setInited(true);
	EventInterface event1(MdbMasterBillValidator::Event_Ready);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Found VMC
	EventInterface event2(ExeMasterInterface::Event_NotReady);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event2));
	TEST_STRING_EQUAL("", result->getString());

	// Enable VMC
	EventInterface event3(ExeMasterInterface::Event_Ready);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event3));
	TEST_STRING_EQUAL("<ledPayment=2><EXE::setChange=0><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=100>", result->getString());
	result->clear();

	// Insert bill2
	Mdb::EventDeposite bill2(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill2));
	TEST_STRING_EQUAL("<EXE::setCredit=150>", result->getString());
	result->clear();

	// Push product button
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=70>", result->getString());
	result->clear();

	// Vend price
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<EXE::approveVend=70><MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event4(executive->getDeviceId(), ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event4));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,70,150,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event5(fiscalRegister->getDeviceId(), Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=80>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testBillValidatorAndCoinChangerSale() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=100>", result->getString());
	result->clear();

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<EXE::setCredit=150>", result->getString());
	result->clear();

	// Push product button
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=70>", result->getString());
	result->clear();

	// Vend price
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<EXE::approveVend=70><MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event5(ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<MCC::dispense(80)>", result->getString());
	result->clear();

	// Change complete
	EventInterface event7(MdbMasterCoinChanger::Event_Dispense);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,70,150,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event6(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event6));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	return true;
}

bool SaleManagerExeMasterTest::testCoinChangerEscrowRequest() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=100>", result->getString());
	result->clear();

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<EXE::setCredit=150>", result->getString());
	result->clear();

	// Escrow request
	EventInterface event5(MdbMasterCoinChanger::Event_EscrowRequest);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<MCC::dispense(150)>", result->getString());
	result->clear();

	// Dispense complete
	EventInterface event6(MdbMasterCoinChanger::Event_Dispense);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event6));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	return true;
}

bool SaleManagerExeMasterTest::testCashNotEnough() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=50>", result->getString());
	result->clear();

	// Wrong product id
	EventUint8Interface wrongIdRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, 33);
	TEST_NUMBER_EQUAL(true, deliverEvent(&wrongIdRequest));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Push product button
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, 3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=70>", result->getString());
	result->clear();

	// Vend price
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<EXE::denyVend=70><MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=50>", result->getString());
	result->clear();

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<EXE::setCredit=150>", result->getString());
	result->clear();

	// Push product button
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=70>", result->getString());
	result->clear();

	// Vend price
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<EXE::approveVend=70><MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event5(ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<MCC::dispense(80)>", result->getString());
	result->clear();

	// Dispense complete
	EventInterface event7(MdbMasterCoinChanger::Event_Dispense);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,70,150,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event6(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event6));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	return true;
}

bool SaleManagerExeMasterTest::testCashCreditHolding() {
	config->getAutomat()->setCreditHolding(true);
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=50>", result->getString());
	result->clear();

	// Escrow request
	EventInterface escrowRequest(MdbMasterCoinChanger::Event_EscrowRequest);
	TEST_NUMBER_EQUAL(true, deliverEvent(&escrowRequest));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Wrong product id
	EventUint8Interface wrongIdRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, 33);
	TEST_NUMBER_EQUAL(true, deliverEvent(&wrongIdRequest));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Push product button
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, 3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=70>", result->getString());
	result->clear();

	// Vend price
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<EXE::denyVend=70><MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=50>", result->getString());
	result->clear();

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<EXE::setCredit=150>", result->getString());
	result->clear();

	// Push product button
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=70>", result->getString());
	result->clear();

	// Vend price
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<EXE::approveVend=70><MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event5(ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<MCC::dispense(80)>", result->getString());
	result->clear();

	// Dispense complete
	EventInterface event7(MdbMasterCoinChanger::Event_Dispense);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,70,150,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event6(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event6));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	return true;
}

bool SaleManagerExeMasterTest::testCashMultiVend() {
	config->getAutomat()->setMultiVend(true);
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Insert bill1	
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));	
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=50>", result->getString());
	result->clear();

	// Escrow request
	EventInterface escrowRequest(MdbMasterCoinChanger::Event_EscrowRequest);
	TEST_NUMBER_EQUAL(true, deliverEvent(&escrowRequest));
	TEST_STRING_EQUAL("<MCC::dispense(50)>", result->getString());
	result->clear();

	// Dispense complete
	EventInterface event1(MdbMasterCoinChanger::Event_Dispense);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Insert bill2
	Mdb::EventDeposite bill2(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill2));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=50>", result->getString());
	result->clear();

	// Wrong product id
	EventUint8Interface wrongIdRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, 33);
	TEST_NUMBER_EQUAL(true, deliverEvent(&wrongIdRequest));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Push product button
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, 3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=70>", result->getString());
	result->clear();

	// Vend price
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<EXE::denyVend=70><MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=50>", result->getString());
	result->clear();

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<EXE::setCredit=150>", result->getString());
	result->clear();

	// Push product button
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=70>", result->getString());
	result->clear();

	// Vend price
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<EXE::approveVend=70><MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event5(ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,70,150,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event6(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event6));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=80>", result->getString());
	result->clear();

	// Push product button
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=70>", result->getString());
	result->clear();

	// Vend price
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<EXE::approveVend=70><MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event7(ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,70,80,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event8(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event8));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=10>", result->getString());
	result->clear();

	// Escrow request
	EventInterface escrowRequest2(MdbMasterCoinChanger::Event_EscrowRequest);
	TEST_NUMBER_EQUAL(true, deliverEvent(&escrowRequest2));
	TEST_STRING_EQUAL("<MCC::dispense(10)>", result->getString());
	result->clear();

	// Dispense complete
	EventInterface event9(MdbMasterCoinChanger::Event_Dispense);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event9));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testCashMultiVendAndCreditHolding() {
	config->getAutomat()->setMultiVend(true);
	config->getAutomat()->setCreditHolding(true);
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Insert bill1
	Mdb::EventDeposite bill1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Deposite, Mdb::BillValidator::Route_Stacked, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&bill1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=50>", result->getString());
	result->clear();

	// Escrow request
	EventInterface escrowRequest(MdbMasterCoinChanger::Event_EscrowRequest);
	TEST_NUMBER_EQUAL(true, deliverEvent(&escrowRequest));
	TEST_STRING_EQUAL("", result->getString());

	// Wrong product id
	EventUint8Interface wrongIdRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, 33);
	TEST_NUMBER_EQUAL(true, deliverEvent(&wrongIdRequest));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Push product button
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, 3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=70>", result->getString());
	result->clear();

	// Vend price
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<EXE::denyVend=70><MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=50>", result->getString());
	result->clear();

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<EXE::setCredit=150>", result->getString());
	result->clear();

	// Push product button
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=70>", result->getString());
	result->clear();

	// Vend price
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<EXE::approveVend=70><MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event5(ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,70,150,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event6(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event6));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=80>", result->getString());
	result->clear();

	// Push product button
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=70>", result->getString());
	result->clear();

	// Vend price
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<EXE::approveVend=70><MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event7(ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,70,80,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event8(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event8));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=10>", result->getString());
	result->clear();

	// Escrow request
	EventInterface escrowRequest2(MdbMasterCoinChanger::Event_EscrowRequest);
	TEST_NUMBER_EQUAL(true, deliverEvent(&escrowRequest2));
	TEST_STRING_EQUAL("<MCC::dispense(10)>", result->getString());
	result->clear();

	// Dispense complete
	EventInterface event9(MdbMasterCoinChanger::Event_Dispense);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event9));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testCashlessSale() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Start cashless session
	result->clear();
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><EXE::setCredit=200>", result->getString());
	result->clear();

	// Push product button
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=71>", result->getString());
	result->clear();

	// Vend price
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<MCL(4)::sale(3,71)>", result->getString());
	result->clear();

	// Vend approving
	MdbMasterCashlessInterface::EventApproved event5(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 71);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<EXE::approveVend=71>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event6(executive->getDeviceId(), ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event6));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,DA/1,71,71,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event7(fiscalRegister->getDeviceId(), Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	EventInterface event8(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event8));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testCashlessSaleBySecondClickTwoClickSale() {
	config->getAutomat()->setCashless2Click(true);
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Start cashless session
	result->clear();
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><EXE::setCredit=200>", result->getString());
	result->clear();

	// Push product button
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=71>", result->getString());
	result->clear();

	// First click
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<MCL(4)::sale(3,71)>", result->getString());
	result->clear();

	// Approving timeout
	timers.tick(EXEMASTER_APPROVING_TIMEOUT);
	timers.execute();
	TEST_STRING_EQUAL("<EXE::denyVend=0>", result->getString());
	result->clear();

	// Vend approving
	MdbMasterCashlessInterface::EventApproved event5(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 71);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<EXE::setCredit=71>", result->getString());
	result->clear();

	// Second click
	EventUint8Interface vendRequest2(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest2));
	TEST_STRING_EQUAL("<EXE::approveVend=71>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event7(executive->getDeviceId(), ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,DA/1,71,71,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event8(fiscalRegister->getDeviceId(), Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event8));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	EventInterface event9(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event9));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testCashlessStartByClickSaleByFirstClick() {
	config->getAutomat()->setCashless2Click(true);
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// First click
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=71>", result->getString());
	result->clear();

	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<MCL(4)::sale(3,71)><MCL(5)::sale(3,71)><MCL(6)::sale(3,71)>", result->getString());
	result->clear();

	// Vend approving
	MdbMasterCashlessInterface::EventApproved event5(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 71);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<EXE::approveVend=71><MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event7(executive->getDeviceId(), ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,DA/1,71,71,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event8(fiscalRegister->getDeviceId(), Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event8));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	EventInterface event9(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event9));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testCashlessStartByClickSaleBySecondClick() {
	config->getAutomat()->setCashless2Click(true);
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// First click
	EventUint8Interface vendRequest1(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest1));
	TEST_STRING_EQUAL("<EXE::setPrice=71>", result->getString());
	result->clear();

	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<MCL(4)::sale(3,71)><MCL(5)::sale(3,71)><MCL(6)::sale(3,71)>", result->getString());
	result->clear();

	// Approving timeout
	timers.tick(EXEMASTER_APPROVING_TIMEOUT);
	timers.execute();
	TEST_STRING_EQUAL("<EXE::denyVend=0>", result->getString());
	result->clear();

	// Approved
	MdbMasterCashlessInterface::EventApproved event5(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 71);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=71>", result->getString());
	result->clear();

	// Second click
	EventUint8Interface vendRequest2(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest2));
	TEST_STRING_EQUAL("<EXE::approveVend=71>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event7(executive->getDeviceId(), ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,DA/1,71,71,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event8(fiscalRegister->getDeviceId(), Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event8));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	EventInterface event9(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event9));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testMixedCashlessSale() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Start cashless session
	result->clear();
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><EXE::setCredit=200>", result->getString());
	result->clear();

	// Push product button
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=71>", result->getString());
	result->clear();

	// Vend price
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<MCL(4)::sale(3,71)>", result->getString());
	result->clear();

	// Vend approving
	MdbMasterCashlessInterface::EventApproved event5(masterCashless1->getDeviceId(), Fiscal::Payment_Ephor, 1, Fiscal::Payment_Sberbank, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<EXE::approveVend=71>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event6(executive->getDeviceId(), ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event6));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,DA/3,1,1,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event7(fiscalRegister->getDeviceId(), Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	EventInterface event8(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event8));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}


bool SaleManagerExeMasterTest::testMixedCashlessSaleBySecondClickTwoClickSale() {
	config->getAutomat()->setCashless2Click(true);
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Start cashless session
	result->clear();
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><EXE::setCredit=200>", result->getString());
	result->clear();

	// Push product button
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=71>", result->getString());
	result->clear();

	// First click
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<MCL(4)::sale(3,71)>", result->getString());
	result->clear();

	// Approving timeout
	timers.tick(EXEMASTER_APPROVING_TIMEOUT);
	timers.execute();
	TEST_STRING_EQUAL("<EXE::denyVend=0>", result->getString());
	result->clear();

	// Vend approving
	MdbMasterCashlessInterface::EventApproved event5(masterCashless1->getDeviceId(), Fiscal::Payment_Ephor, 1, Fiscal::Payment_Sberbank, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<EXE::setCredit=71>", result->getString());
	result->clear();

	// Second click
	EventUint8Interface vendRequest2(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest2));
	TEST_STRING_EQUAL("<EXE::approveVend=71>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event7(executive->getDeviceId(), ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,DA/3,1,1,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event8(fiscalRegister->getDeviceId(), Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event8));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	EventInterface event9(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event9));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testMixedCashlessStartByClickSaleByFirstClick() {
	config->getAutomat()->setCashless2Click(true);
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// First click
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=71>", result->getString());
	result->clear();

	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<MCL(4)::sale(3,71)><MCL(5)::sale(3,71)><MCL(6)::sale(3,71)>", result->getString());
	result->clear();

	// Vend approving
	MdbMasterCashlessInterface::EventApproved event5(masterCashless1->getDeviceId(), Fiscal::Payment_Ephor, 1, Fiscal::Payment_Sberbank, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<EXE::approveVend=71><MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event7(executive->getDeviceId(), ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,DA/3,1,1,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event8(fiscalRegister->getDeviceId(), Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event8));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	EventInterface event9(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event9));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testMixedCashlessStartByClickSaleBySecondClick() {
	config->getAutomat()->setCashless2Click(true);
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// First click
	EventUint8Interface vendRequest1(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest1));
	TEST_STRING_EQUAL("<EXE::setPrice=71>", result->getString());
	result->clear();

	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<MCL(4)::sale(3,71)><MCL(5)::sale(3,71)><MCL(6)::sale(3,71)>", result->getString());
	result->clear();

	// Approving timeout
	timers.tick(EXEMASTER_APPROVING_TIMEOUT);
	timers.execute();
	TEST_STRING_EQUAL("<EXE::denyVend=0>", result->getString());
	result->clear();

	// Approved
	MdbMasterCashlessInterface::EventApproved event5(masterCashless1->getDeviceId(), Fiscal::Payment_Ephor, 1, Fiscal::Payment_Sberbank, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=71>", result->getString());
	result->clear();

	// Second click
	EventUint8Interface vendRequest2(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest2));
	TEST_STRING_EQUAL("<EXE::approveVend=71>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event7(executive->getDeviceId(), ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,DA/3,1,1,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event8(fiscalRegister->getDeviceId(), Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event8));
	TEST_STRING_EQUAL("<MCL(4)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	EventInterface event9(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event9));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testExternCashlessSale() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Start cashless session
	result->clear();
	EventUint32Interface credit1(externCashless->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><EXE::setCredit=200>", result->getString());
	result->clear();

	// Push product button
	EventUint8Interface vendRequest(externCashless->getDeviceId(), ExeMasterInterface::Event_VendRequest, 3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=71>", result->getString());
	result->clear();

	// Vend price
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<MCL(6)::sale(3,71)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved event5(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 71);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<EXE::approveVend=71>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event6(executive->getDeviceId(), ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event6));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,DA/1,71,71,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event7(fiscalRegister->getDeviceId(), Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<MCL(6)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	EventInterface event8(externCashless->getDeviceId(), MdbMasterCashless::Event_SessionEnd);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event8));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testCashlessSaleVendDeny() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Start cashless session
	result->clear();
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><EXE::setCredit=200>", result->getString());
	result->clear();

	// Push product button
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, 3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=71>", result->getString());
	result->clear();

	// Vend price
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<MCL(4)::sale(3,71)>", result->getString());
	result->clear();

	// Vend denied
	EventInterface event5(masterCashless1->getDeviceId(), MdbMasterCashless::Event_VendDenied);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<EXE::denyVend=71><MCL(4)::closeSession>", result->getString());
	result->clear();

	// Session closed
	EventInterface event6(executive->getDeviceId(), MdbMasterCashless::Event_SessionEnd);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event6));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testCashlessSaleVendFailed() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Start cashless session
	result->clear();
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><EXE::setCredit=200>", result->getString());
	result->clear();

	// Push product button
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, 3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=71>", result->getString());
	result->clear();

	// Vend price
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<MCL(4)::sale(3,71)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved event5(masterCashless1->getDeviceId(), Fiscal::Payment_Cashless, 71);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<EXE::approveVend=71>", result->getString());
	result->clear();

	// Vending timeout
	timers.tick(20000);
	timers.execute();
	TEST_STRING_EQUAL("", result->getString());

	// Vend failed
	EventInterface event6(executive->getDeviceId(), ExeMasterInterface::Event_VendFailed);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event6));
	TEST_STRING_EQUAL("<MCL(4)::saleFailed>", result->getString());
	result->clear();

	// Session closed
	EventInterface event7(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testCashlessSaleSessionCancel() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Start cashless session
	result->clear();
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, (uint32_t)200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><EXE::setCredit=200>", result->getString());
	result->clear();

	// Session cancelled
	EventInterface event5(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testAutomatTemporaryError() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL(
		"<EXE::setDicimalPoint=0><EXE::setChange=0><EXE::setScalingFactor=1>"
		"<EXE::reset><MCC::reset><MBV::reset()><MCL(4)::reset><MCL(5)::reset><MCL(6)::reset><ledPayment=1>", result->getString());

	// Init masterBillValidator
	result->clear();
	masterBillValidator->setInited(true);
	EventInterface event1(masterBillValidator->getDeviceId(), MdbMasterBillValidator::Event_Ready);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Found VMC
	EventInterface event2(executive->getDeviceId(), ExeMasterInterface::Event_NotReady);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event2));
	TEST_STRING_EQUAL("", result->getString());

	// Enable VMC
	EventInterface event3(executive->getDeviceId(), ExeMasterInterface::Event_Ready);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event3));
	TEST_STRING_EQUAL("<ledPayment=2><EXE::setChange=0><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Disable VMC
	EventInterface event4(executive->getDeviceId(), ExeMasterInterface::Event_NotReady);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event4));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><event=1,AutomatState,0>", result->getString());
	result->clear();

	// Enable VMC
	EventInterface event5(executive->getDeviceId(), ExeMasterInterface::Event_Ready);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<ledPayment=2><EXE::setChange=0><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testCashlessRevalue() {
	masterCashless1->setRefundAble(true);
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Start cashless session
	result->clear();
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><EXE::setCredit=200>", result->getString());
	result->clear();

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, 0, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCL(4)::revalue(100)>", result->getString());
	result->clear();

	// Revalue approved
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCashless::Event_RevalueApproved));
	TEST_STRING_EQUAL("<EXE::setCredit=300>", result->getString());
	result->clear();

	// Insert bill2
	MdbMasterCoinChanger::EventCoin coin2(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 50, 0, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("<MCL(4)::revalue(50)>", result->getString());
	result->clear();

	// Revalue denied
	EventInterface event6(masterCashless1->getDeviceId(), MdbMasterCashless::Event_RevalueDenied);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event6));
	TEST_STRING_EQUAL("<EXE::setCredit=300>", result->getString());
	result->clear();

	// Session closed
	EventInterface event7(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=50>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testCashlessRevalueAtStart() {
	masterCashless1->setRefundAble(true);
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=100>", result->getString());
	result->clear();

	// Insert bill2
	MdbMasterCoinChanger::EventCoin coin2(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 50, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("<EXE::setCredit=150>", result->getString());
	result->clear();

	// Start cashless session
	result->clear();
	EventUint32Interface credit1(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::revalue(150)>", result->getString());
	result->clear();

	// Revalue approved
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbMasterCashless::Event_RevalueApproved));
	TEST_STRING_EQUAL("<EXE::setCredit=350>", result->getString());
	result->clear();

	// Session closed
	EventInterface event7(masterCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testTokenSale() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Insert token1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_DepositeToken, 0, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=50000>", result->getString());
	result->clear();

	// Push product button
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=70>", result->getString());
	result->clear();

	// Vend price
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<EXE::approveVend=70>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event4(executive->getDeviceId(), ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event4));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,TA/2,70,70,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event5(fiscalRegister->getDeviceId(), Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<EXE::setChange=1><EXE::setCredit=0><MCC::enable><MBV::enable><MCL(4)::enable><MCL(5)::enable><MCL(6)::enable><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerExeMasterTest::testStateCashCreditEventToken() {
	TEST_NUMBER_EQUAL(true, gotoStateEnabled());

	// Insert coin1
	MdbMasterCoinChanger::EventCoin coin1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_Deposite, 100, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=100>", result->getString());
	result->clear();

	// Insert token1
	MdbMasterCoinChanger::EventCoin token1(masterCoinChanger->getDeviceId(), MdbMasterCoinChanger::Event_DepositeToken, 0, MdbMasterCoinChanger::Route_Cashbox, 0, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&token1));
	TEST_STRING_EQUAL("<MCC::disable><MBV::disable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=50000>", result->getString());
	result->clear();

	// Push product button
	EventUint8Interface vendRequest(executive->getDeviceId(), ExeMasterInterface::Event_VendRequest, (uint8_t)3);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<EXE::setPrice=70>", result->getString());
	result->clear();

	// Vend price
	EventInterface vendPrice(executive->getDeviceId(), ExeMasterInterface::Event_VendPrice);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendPrice));
	TEST_STRING_EQUAL("<EXE::approveVend=70>", result->getString());
	result->clear();

	// Vend complete
	EventInterface event5(executive->getDeviceId(), ExeMasterInterface::Event_VendComplete);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,TA/2,70,70,0)>", result->getString());
	result->clear();

	// Print check
	EventInterface event6(fiscalRegister->getDeviceId(), Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event6));
	TEST_STRING_EQUAL("<MCC::enable><MBV::enable><MCL(4)::disable><MCL(5)::disable><MCL(6)::disable><EXE::setCredit=100>", result->getString());
	result->clear();
	return true;
}
