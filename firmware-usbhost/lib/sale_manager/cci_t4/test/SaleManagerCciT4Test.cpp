#include <lib/sale_manager/cci_t4/SaleManagerCciT4Core.h>
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

class SaleManagerCciT4Test : public TestSet {
public:
	SaleManagerCciT4Test();
	bool init();
	void cleanup();
	bool initConfigModem();
	void initConfigAutomatProductPrice(uint16_t index, const char *device, uint16_t number, uint16_t value);
	bool gotoStateSale();
	bool testSale();
	bool testFreeSale();

private:
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	ConfigModem *configModem;
	ClientContext *clientContext;
	ConfigProductIterator *product;
	TimerEngine *timerEngine;
	TestSaleManagerMdbMasterEventEngine *eventEngine;
	StringBuilder *result;
	TestMdbMasterCashless *externCashless;
	MdbMasterCashlessStack *masterCashlessStack;
	TestMdbSlaveCashless *slaveCashless;
	TestFiscalRegister *fiscalRegister;
	SaleManagerCciT4Core *core;

	bool deliverEvent(EventInterface *event);
	bool deliverEvent(uint16_t type);
	bool deliverEvent(EventDeviceId deviceId, uint16_t type);
};

TEST_SET_REGISTER(SaleManagerCciT4Test);

SaleManagerCciT4Test::SaleManagerCciT4Test() {
	TEST_CASE_REGISTER(SaleManagerCciT4Test, testSale);
	TEST_CASE_REGISTER(SaleManagerCciT4Test, testFreeSale);
}

bool SaleManagerCciT4Test::init() {
	TEST_NUMBER_EQUAL(true, initConfigModem());

	result = new StringBuilder;
	clientContext = new ClientContext;
	timerEngine = new TimerEngine;
	eventEngine = new TestSaleManagerMdbMasterEventEngine(result);
	slaveCashless = new TestMdbSlaveCashless(1, result);
	externCashless =  new TestMdbMasterCashless(2, result);
	masterCashlessStack = new MdbMasterCashlessStack;
	masterCashlessStack->push(externCashless);
	fiscalRegister = new TestFiscalRegister(3, result);
	core = new SaleManagerCciT4Core(configModem, clientContext, timerEngine, eventEngine, slaveCashless, masterCashlessStack, fiscalRegister);

	result->clear();
	return true;
}

void SaleManagerCciT4Test::cleanup() {
	delete core;
	delete fiscalRegister;
	delete slaveCashless;
	delete masterCashlessStack;
	delete externCashless;
	delete eventEngine;
	delete timerEngine;
	delete clientContext;
	delete result;
	delete configModem;
	delete stat;
	delete realtime;
	delete memory;
}

bool SaleManagerCciT4Test::initConfigModem() {
	memory = new RamMemory(128000),
	memory->setAddress(0);
	realtime = new TestRealTime;
	stat = new StatStorage;
	configModem = new ConfigModem(memory, realtime, stat);

	ConfigAutomat *configAutomat = configModem->getAutomat();
	configAutomat->shutdown();
	configAutomat->addProduct("1", 1);
	configAutomat->addProduct("2", 2);
	configAutomat->addProduct("3", 3);
	configAutomat->addProduct("4", 4);
	configAutomat->addPriceList("CA", 0, Config3PriceIndexType_Base);
	configAutomat->addPriceList("DA", 1, Config3PriceIndexType_Base);

	TEST_NUMBER_EQUAL(MemoryResult_Ok, configModem->init());

	configAutomat->setDecimalPoint(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, configAutomat->save());

	product = configAutomat->createIterator();
	initConfigAutomatProductPrice(0, "CA", 0, 10);
	initConfigAutomatProductPrice(0, "DA", 1, 11);
	initConfigAutomatProductPrice(1, "CA", 0, 0);
	initConfigAutomatProductPrice(1, "DA", 1, 0);
	initConfigAutomatProductPrice(2, "CA", 0, 70);
	initConfigAutomatProductPrice(2, "DA", 1, 71);
	initConfigAutomatProductPrice(3, "CA", 0, 120);
	initConfigAutomatProductPrice(3, "DA", 1, 121);
	return true;
}

void SaleManagerCciT4Test::initConfigAutomatProductPrice(uint16_t index, const char *device, uint16_t number, uint16_t value) {
	product->findByIndex(index);
	ConfigPrice *price = product->getPrice(device, number);
	price->data.price = value;
	price->save();
}

bool SaleManagerCciT4Test::gotoStateSale() {
#if 0
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
	slaveCashless->setInited(true);
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
	slaveCashless->setEnabled(true);
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
#endif
	return true;
}

bool SaleManagerCciT4Test::deliverEvent(EventInterface *event) {
	EventEnvelope envelope(EVENT_DATA_SIZE);
	if(event->pack(&envelope) == false) { return false; }
	core->proc(&envelope);
	return true;
}

bool SaleManagerCciT4Test::deliverEvent(uint16_t type) {
	EventInterface event(type);
	return deliverEvent(&event);
}

bool SaleManagerCciT4Test::deliverEvent(EventDeviceId deviceId, uint16_t type) {
	EventInterface event(deviceId, type);
	return deliverEvent(&event);
}

bool SaleManagerCciT4Test::testSale() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<SCL::reset><MCL(2)::reset><SCL::setCredit(0)><MCL(2)::disable><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(2)::sale(4,121)><MCL(2)::enable>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(externCashless->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<SCL::approveVend=121><MCL(2)::disable>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,DA/1,121,121,0)><MCL(2)::closeSession><MCL(2)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(externCashless->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<SCL::setCredit(0)><MCL(2)::disable>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerCciT4Test::testFreeSale() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<SCL::reset><MCL(2)::reset><SCL::setCredit(0)><MCL(2)::disable><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 2, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<SCL::approveVend=0>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,DA/1,0,0,0)><SCL::setCredit(0)><MCL(2)::disable>", result->getString());
	result->clear();
	return true;
}
