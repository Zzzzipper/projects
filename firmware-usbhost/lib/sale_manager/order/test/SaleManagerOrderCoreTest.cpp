#include "lib/sale_manager/order/SaleManagerOrderCore.h"
#include "lib/sale_manager/include/SaleManager.h"
#include "lib/sale_manager/order/test/TestOrderMaster.h"
#include "lib/sale_manager/order/test/TestOrderDevice.h"

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

class TestSaleManagerOrderEventEngine : public TestEventEngine {
public:
	TestSaleManagerOrderEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case SaleManager::Event_AutomatState: procEventUint8(envelope, SaleManager::Event_AutomatState, "AutomatState"); break;
		default: return TestEventEngine::transmit(envelope);
		}
		return true;
	}
};

#if 0
class TestPinCodeDevice : public PinCodeDeviceInterface {
public:
	TestPinCodeDevice(uint16_t deviceId, StringBuilder *str) : deviceId(deviceId), str(str) {}
	EventDeviceId getDeviceId() { return deviceId; }
	void pageStart() override { *str << "<PC(" <<  deviceId.getValue() << ")::pageStart>"; }
	void pageProgress() override { *str << "<PC(" <<  deviceId.getValue() << ")::pageProgress>"; }
	void pagePinCode() override { *str << "<PC(" <<  deviceId.getValue() << ")::pagePinCode>"; }
	void pageSale() override { *str << "<PC(" <<  deviceId.getValue() << ")::pageSale>"; }
	void pageComplete() override { *str << "<PC(" <<  deviceId.getValue() << ")::pageComplete>"; }

private:
	EventDeviceId deviceId;
	StringBuilder *str;
};
#endif

class SaleManagerOrderTest : public TestSet {
public:
	SaleManagerOrderTest();
	bool init();
	void cleanup();
	bool initConfigModem();
	void initConfigAutomatProductPrice(uint16_t index, const char *device, uint16_t number, uint16_t value);
	bool gotoStateSale();
	bool testOrderSale();
	bool testPinCodeSale();

private:
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	ConfigModem *config;
	ConfigProductIterator *product;
	ClientContext *clientContext;
	TimerEngine *timerEngine;
	TestSaleManagerOrderEventEngine *eventEngine;
	StringBuilder *result;
	TestOrderMaster *orderMaster;
	TestOrderDevice *orderDevice;
	TestFiscalRegister *fiscalRegister;
	TestLed *leds;
	EventRegistrar *chronicler;
	SaleManagerOrderCore *core;

	bool deliverEvent(EventInterface *event);
	bool deliverEvent(uint16_t type);
	bool deliverEvent(EventDeviceId deviceId, uint16_t type);
};

TEST_SET_REGISTER(SaleManagerOrderTest);

SaleManagerOrderTest::SaleManagerOrderTest() {
	TEST_CASE_REGISTER(SaleManagerOrderTest, testOrderSale);
	TEST_CASE_REGISTER(SaleManagerOrderTest, testPinCodeSale);
}

bool SaleManagerOrderTest::init() {
	TEST_NUMBER_EQUAL(true, initConfigModem());

	result = new StringBuilder;
	clientContext = new ClientContext;
	timerEngine = new TimerEngine;
	eventEngine = new TestSaleManagerOrderEventEngine(result);
	orderMaster = new TestOrderMaster(1, result);
	orderDevice = new TestOrderDevice(2, result);
	fiscalRegister = new TestFiscalRegister(3, result);
	leds = new TestLed(result);
	chronicler = new EventRegistrar(config, timerEngine, eventEngine, realtime);
	core = new SaleManagerOrderCore(config, clientContext, timerEngine, eventEngine, orderMaster, orderDevice, fiscalRegister, leds, chronicler);
	result->clear();
	return true;
}

void SaleManagerOrderTest::cleanup() {
	delete core;
	delete chronicler;
	delete leds;
	delete fiscalRegister;
	delete orderDevice;
	delete orderMaster;
	delete eventEngine;
	delete timerEngine;
	delete clientContext;
	delete result;
	delete config;
	delete stat;
	delete realtime;
	delete memory;
}

bool SaleManagerOrderTest::initConfigModem() {
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

void SaleManagerOrderTest::initConfigAutomatProductPrice(uint16_t index, const char *device, uint16_t number, uint16_t value) {
	product->findByIndex(index);
	ConfigPrice *price = product->getPrice(device, number);
	price->data.price = value;
	price->save();
}

bool SaleManagerOrderTest::gotoStateSale() {
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

bool SaleManagerOrderTest::deliverEvent(EventInterface *event) {
	EventEnvelope envelope(EVENT_DATA_SIZE);
	if(event->pack(&envelope) == false) { return false; }
	core->proc(&envelope);
	return true;
}

bool SaleManagerOrderTest::deliverEvent(uint16_t type) {
	EventInterface event(type);
	return deliverEvent(&event);
}

bool SaleManagerOrderTest::deliverEvent(EventDeviceId deviceId, uint16_t type) {
	EventInterface event(deviceId, type);
	return deliverEvent(&event);
}

bool SaleManagerOrderTest::testOrderSale() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<OM(1)::reset><OD(2)::reset><OM(1)::connect><OD(2)::disable><ledPayment=1><event=1,AutomatState,0><event=1,AutomatState,0>", result->getString());
	result->clear();

	// Init master
	EventInterface masterInit(orderMaster->getDeviceId(), OrderMasterInterface::Event_Connected);
	TEST_NUMBER_EQUAL(true, deliverEvent(&masterInit));
	TEST_STRING_EQUAL("<OD(2)::enable><ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Vend request
	EventUint16Interface deviceRequest(orderDevice->getDeviceId(), OrderDeviceInterface::Event_VendRequest, 1);
	TEST_NUMBER_EQUAL(true, deliverEvent(&deviceRequest));
	TEST_STRING_EQUAL("<OM(1)::check>", result->getString());
	result->clear();

	// Order approved
	EventInterface masterApproved(orderMaster->getDeviceId(), OrderMasterInterface::Event_Approved);
	TEST_NUMBER_EQUAL(true, deliverEvent(&masterApproved));
	TEST_STRING_EQUAL("<OD(2)::approveVend>", result->getString());
	result->clear();

	// Vend complete
	EventUint16Interface deviceCompleted(orderDevice->getDeviceId(), OrderDeviceInterface::Event_VendCompleted, 1);
	TEST_NUMBER_EQUAL(true, deliverEvent(&deviceCompleted));
	TEST_STRING_EQUAL("<FR::sale(,CA/0,0,0,0)><OD(2)::disable><OM(1)::distribute>", result->getString());
	result->clear();

	// Save complete
	EventInterface masterDistributed(orderMaster->getDeviceId(), OrderMasterInterface::Event_Distributed);
	TEST_NUMBER_EQUAL(true, deliverEvent(&masterDistributed));
	TEST_STRING_EQUAL("<OM(1)::complete><OD(2)::enable><ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerOrderTest::testPinCodeSale() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<OM(1)::reset><OD(2)::reset><OM(1)::connect><OD(2)::disable><ledPayment=1><event=1,AutomatState,0><event=1,AutomatState,0>", result->getString());
	result->clear();

	// Init master
	EventInterface masterInit(orderMaster->getDeviceId(), OrderMasterInterface::Event_Connected);
	TEST_NUMBER_EQUAL(true, deliverEvent(&masterInit));
	TEST_STRING_EQUAL("<OD(2)::enable><ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Vend request
	EventUint16Interface deviceRequest(orderDevice->getDeviceId(), OrderDeviceInterface::Event_VendRequest, 1);
	TEST_NUMBER_EQUAL(true, deliverEvent(&deviceRequest));
	TEST_STRING_EQUAL("<OM(1)::check>", result->getString());
	result->clear();

	// PinCode request
	EventInterface masterPinCode(orderMaster->getDeviceId(), OrderMasterInterface::Event_PinCode);
	TEST_NUMBER_EQUAL(true, deliverEvent(&masterPinCode));
	TEST_STRING_EQUAL("<ledFiscal=1><OD(2)::requestPinCode>", result->getString());
	result->clear();

	// PinCode entered
	OrderDeviceInterface::EventPinCodeCompleted devicePinCode(orderDevice->getDeviceId(), "1234");
	TEST_NUMBER_EQUAL(true, deliverEvent(&devicePinCode));
	TEST_STRING_EQUAL("<ledFiscal=2><OM(1)::checkPinCode(1234>", result->getString());
	result->clear();

	// Order approved
	EventInterface masterApproved(orderMaster->getDeviceId(), OrderMasterInterface::Event_Approved);
	TEST_NUMBER_EQUAL(true, deliverEvent(&masterApproved));
	TEST_STRING_EQUAL("<OD(2)::approveVend>", result->getString());
	result->clear();

	// Vend complete
	EventUint16Interface deviceCompleted(orderDevice->getDeviceId(), OrderDeviceInterface::Event_VendCompleted, 1);
	TEST_NUMBER_EQUAL(true, deliverEvent(&deviceCompleted));
	TEST_STRING_EQUAL("<FR::sale(,CA/0,0,0,0)><OD(2)::disable><OM(1)::distribute>", result->getString());
	result->clear();

	// Save complete
	EventInterface masterDistributed(orderMaster->getDeviceId(), OrderMasterInterface::Event_Distributed);
	TEST_NUMBER_EQUAL(true, deliverEvent(&masterDistributed));
	TEST_STRING_EQUAL("<OM(1)::complete><OD(2)::enable><ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();
	return true;
}
