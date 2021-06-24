#include "lib/sale_manager/cci_t3/SaleManagerCciT3Core.h"
#include "lib/sale_manager/include/SaleManager.h"
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

class TestOrder : public OrderInterface {
public:
	TestOrder(uint16_t deviceId, StringBuilder *str) : deviceId(deviceId), str(str) {}
	EventDeviceId getDeviceId() { return deviceId; }
	void reset() { *str << "<OM(" <<  deviceId.getValue() << ")::reset>"; }
	void disable() { *str << "<OM(" <<  deviceId.getValue() << ")::disable>"; }
	void enable() { *str << "<OM(" <<  deviceId.getValue() << ")::enable>"; }
	void setProductId(uint16_t cid) { { *str << "<OM(" <<  deviceId.getValue() << ")::setProductId(" << cid << ")>"; } }
	bool saleComplete() { *str << "<OM(" <<  deviceId.getValue() << ")::saleComplete>"; return true; }
	bool saleFailed() { *str << "<OM(" <<  deviceId.getValue() << ")::saleFailed>"; return true; }
	bool closeSession() { *str << "<OM(" <<  deviceId.getValue() << ")::closeSession>"; return true; }

private:
	EventDeviceId deviceId;
	StringBuilder *str;
};

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

class SaleManagerCciT3Test : public TestSet {
public:
	SaleManagerCciT3Test();
	bool init();
	void cleanup();
	bool initConfigModem();
	void initConfigAutomatProductPrice(uint16_t index, const char *device, uint16_t number, uint16_t value);
	bool gotoStateSale();
	bool testCashlessFreeSale();
	bool testCashlessSale();
	bool testOrderSale();

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
	TestOrder *orderMaster;
	TestMdbMasterCashless *externCashless;
	MdbMasterCashlessStack *masterCashlessStack;
	TestOrderDevice *orderDevice;
	TestFiscalRegister *fiscalRegister;
	TestLed *leds;
	EventRegistrar *chronicler;
	SaleManagerCciT3Core *core;

	bool deliverEvent(EventInterface *event);
	bool deliverEvent(uint16_t type);
	bool deliverEvent(EventDeviceId deviceId, uint16_t type);
};

TEST_SET_REGISTER(SaleManagerCciT3Test);

SaleManagerCciT3Test::SaleManagerCciT3Test() {
	TEST_CASE_REGISTER(SaleManagerCciT3Test, testCashlessFreeSale);
	TEST_CASE_REGISTER(SaleManagerCciT3Test, testCashlessSale);
	TEST_CASE_REGISTER(SaleManagerCciT3Test, testOrderSale);
}

bool SaleManagerCciT3Test::init() {
	TEST_NUMBER_EQUAL(true, initConfigModem());

	result = new StringBuilder;
	clientContext = new ClientContext;
	timerEngine = new TimerEngine;
	eventEngine = new TestSaleManagerOrderEventEngine(result);
	orderDevice = new TestOrderDevice(1, result);
	orderMaster = new TestOrder(2, result);
	externCashless =  new TestMdbMasterCashless(3, result);
	masterCashlessStack = new MdbMasterCashlessStack;
	masterCashlessStack->push(externCashless);
	fiscalRegister = new TestFiscalRegister(4, result);
	leds = new TestLed(result);
	chronicler = new EventRegistrar(config, timerEngine, eventEngine, realtime);
	core = new SaleManagerCciT3Core(config, clientContext, timerEngine, eventEngine, orderDevice, orderMaster, masterCashlessStack, fiscalRegister, leds, chronicler);
	result->clear();
	return true;
}

void SaleManagerCciT3Test::cleanup() {
	delete core;
	delete chronicler;
	delete leds;
	delete fiscalRegister;
	delete orderDevice;
	delete masterCashlessStack;
	delete externCashless;
	delete eventEngine;
	delete timerEngine;
	delete clientContext;
	delete result;
	delete config;
	delete stat;
	delete realtime;
	delete memory;
}

bool SaleManagerCciT3Test::initConfigModem() {
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

void SaleManagerCciT3Test::initConfigAutomatProductPrice(uint16_t index, const char *device, uint16_t number, uint16_t value) {
	product->findByIndex(index);
	ConfigPrice *price = product->getPrice(device, number);
	price->data.price = value;
	price->save();
}

bool SaleManagerCciT3Test::gotoStateSale() {
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

bool SaleManagerCciT3Test::deliverEvent(EventInterface *event) {
	EventEnvelope envelope(EVENT_DATA_SIZE);
	if(event->pack(&envelope) == false) { return false; }
	core->proc(&envelope);
	return true;
}

bool SaleManagerCciT3Test::deliverEvent(uint16_t type) {
	EventInterface event(type);
	return deliverEvent(&event);
}

bool SaleManagerCciT3Test::deliverEvent(EventDeviceId deviceId, uint16_t type) {
	EventInterface event(deviceId, type);
	return deliverEvent(&event);
}

bool SaleManagerCciT3Test::testCashlessFreeSale() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<OM(2)::reset><CSI(1)::reset><MCL(3)::reset><MCL(3)::enable><CSI(1)::enableProducts><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(orderDevice->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 2, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<CSI(1)::approveVend=0>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,DA/1,0,0,0)><MCL(3)::enable><CSI(1)::enableProducts>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(externCashless->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool SaleManagerCciT3Test::testCashlessSale() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<OM(2)::reset><CSI(1)::reset><MCL(3)::reset><MCL(3)::enable><CSI(1)::enableProducts><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(orderDevice->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 0);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(3)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved vendApproved(externCashless->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendApproved));
	TEST_STRING_EQUAL("<CSI(1)::approveVend=4><MCL(3)::disable>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,DA/1,121,121,0)><MCL(3)::closeSession><MCL(3)::saleComplete>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(externCashless->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCL(3)::enable><CSI(1)::enableProducts>", result->getString());
	return true;
}

bool SaleManagerCciT3Test::testOrderSale() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<OM(2)::reset><CSI(1)::reset><MCL(3)::reset><MCL(3)::enable><CSI(1)::enableProducts><event=1,AutomatState,1>", result->getString());
	result->clear();

	// QrCode
	EventInterface orderApprove(orderMaster->getDeviceId(), OrderInterface::Event_OrderApprove);
	TEST_NUMBER_EQUAL(true, deliverEvent(&orderApprove));
	TEST_STRING_EQUAL("<MCL(3)::disable><CSI(1)::disableProducts>", result->getString());
	result->clear();

	// Order request
	EventUint16Interface orderRequest(orderDevice->getDeviceId(), OrderInterface::Event_OrderRequest, 4);
	TEST_NUMBER_EQUAL(true, deliverEvent(&orderRequest));
	TEST_STRING_EQUAL("<CSI(1)::approveVend=4>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<OM(2)::saleComplete><CSI(1)::disableProducts>", result->getString());
	result->clear();

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(externCashless->getDeviceId(), OrderInterface::Event_OrderEnd));
	TEST_STRING_EQUAL("<MCL(3)::enable><CSI(1)::enableProducts>", result->getString());
	return true;
}
