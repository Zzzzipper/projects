#include "lib/sale_manager/mdb_slave/SaleManagerMdbSlaveCore.h"
#include "lib/sale_manager/include/SaleManager.h"

#include "common/memory/include/RamMemory.h"
#include "common/timer/include/TestRealTime.h"
#include "common/event/include/TestEventEngine.h"
#include "common/fiscal_register/include/TestFiscalRegister.h"
#include "common/utils/include/TestLed.h"
#include "common/mdb/slave/cashless/TestMdbSlaveCashless.h"
#include "common/mdb/master/cashless/TestMdbMasterCashless.h"
#include "common/logger/include/Logger.h"
#include "common/test/include/Test.h"

//+++ выделить в отдельный модуль
#include "lib/utils/stm32/HardwareUartForwardController.h"

void HardwareUartForwardController::init() {}
void HardwareUartForwardController::start() {}
void HardwareUartForwardController::stop() {}
//+++

class TestSaleManagerMdbSlaveEventEngine : public TestEventEngine {
public:
	TestSaleManagerMdbSlaveEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case SaleManager::Event_AutomatState: procEventUint8(envelope, SaleManager::Event_AutomatState, "AutomatState"); break;
		default: return TestEventEngine::transmit(envelope);
		}
		return true;
	}
};

class TestSaleManagerSniffer : public MdbSniffer {
public:
	TestSaleManagerSniffer(uint16_t deviceId, StringBuilder *result) : MdbSniffer(Mdb::Device_CoinChanger, NULL), deviceId(deviceId), enabled(false), result(result) {}
	void setEnabled(bool value) { enabled = value; }
	virtual EventDeviceId getDeviceId() { return deviceId; }
	virtual void reset() { *result << "<Mdb::Sniffer(" <<  deviceId.getValue() << ")::reset>"; }
	virtual bool isEnable() { return enabled; }
	virtual void initSlave(MdbSlave::Sender *sender, MdbSlave::Receiver *receiver) { (void)sender; (void)receiver; }
	virtual void recvCommand(const uint8_t command) { (void)command; }
	virtual void recvSubcommand(const uint8_t subcommand) { (void)subcommand; }
	virtual void recvRequest(const uint8_t *data, uint16_t len) { (void)data; (void)len; }
	virtual void recvConfirm(uint8_t control) { (void)control; }
	virtual void procResponse(const uint8_t *data, uint16_t len, bool crc) { (void)data; (void)len; (void)crc; }

private:
	EventDeviceId deviceId;
	bool enabled;
	StringBuilder *result;
};

class TestMdbSlaveComGateway : public MdbSlaveComGatewayInterface {
public:
	TestMdbSlaveComGateway(uint16_t deviceId, StringBuilder *result) : deviceId(deviceId), result(result), inited(false) {}
	virtual EventDeviceId getDeviceId() { return deviceId; }
	void setInited(bool inited) { this->inited = inited; }
	virtual void reset() { *result << "<SCG(" <<  deviceId.getValue() << ")::reset>"; }
	virtual bool isInited() { return inited; }
	virtual bool isEnable() { return true; }

private:
	EventDeviceId deviceId;
	StringBuilder *result;
	bool inited;
};

class SaleManagerMdbSlaveTest : public TestSet {
public:
	SaleManagerMdbSlaveTest();
	bool init();
	void cleanup();
	bool initConfigModem();
	bool initConfigAutomatProductPrice(uint16_t index, const char *device, uint16_t number, uint16_t value);
	bool gotoStateSale();
	bool testSaleByCash();
	bool testSaleByCashless();
	bool testSaleByExternCashless();
	bool testSaleByExternCashlessAlwaysIdle();
	bool testSaleByExternMixedCashless();
	bool testGatewaySaleByCash();
	bool testGatewaySaleByCashless();
	bool testGatewaySaleByToken();
	bool testGatewayCashlessVendless();
	bool testGatewaySaleByExternCashless();
	bool testGatewayWaterOverflow();
	bool testEnableDisable();

private:
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	ConfigModem *config;
	ConfigProductIterator *product;
	TimerEngine *timerEngine;
	TestSaleManagerMdbSlaveEventEngine *eventEngine;
	StringBuilder *result;
	TestFiscalRegister *fiscalRegister;
	TestLed *leds;
	EventRegistrar *chronicler;
	TestSaleManagerSniffer *slaveCoinChanger;
	TestSaleManagerSniffer *slaveBillValidator;
	TestSaleManagerSniffer *slaveCashless1;
	TestMdbSlaveCashless *slaveCashless2;
	TestMdbMasterCashless *externCashless1;
	TestMdbMasterCashless *externCashless2;
	MdbMasterCashlessStack *masterCashlessStack;
	TestMdbSlaveComGateway *slaveComGateway;
	SaleManagerMdbSlaveCore *core;

	bool deliverEvent(EventInterface *event);
	bool deliverEvent(uint16_t type);
	bool deliverEvent(EventDeviceId deviceId, uint16_t type);
	bool deliverEventUint32(EventDeviceId deviceId, uint16_t type, uint32_t value);
};

TEST_SET_REGISTER(SaleManagerMdbSlaveTest);

SaleManagerMdbSlaveTest::SaleManagerMdbSlaveTest() {
	TEST_CASE_REGISTER(SaleManagerMdbSlaveTest, testSaleByCash);
	TEST_CASE_REGISTER(SaleManagerMdbSlaveTest, testSaleByCashless);
	TEST_CASE_REGISTER(SaleManagerMdbSlaveTest, testSaleByExternCashless);
	TEST_CASE_REGISTER(SaleManagerMdbSlaveTest, testSaleByExternCashlessAlwaysIdle);
	TEST_CASE_REGISTER(SaleManagerMdbSlaveTest, testSaleByExternMixedCashless);
	TEST_CASE_REGISTER(SaleManagerMdbSlaveTest, testGatewaySaleByCash);
	TEST_CASE_REGISTER(SaleManagerMdbSlaveTest, testGatewaySaleByCashless);
	TEST_CASE_REGISTER(SaleManagerMdbSlaveTest, testGatewaySaleByToken);
	TEST_CASE_REGISTER(SaleManagerMdbSlaveTest, testGatewayCashlessVendless);
	TEST_CASE_REGISTER(SaleManagerMdbSlaveTest, testGatewaySaleByExternCashless);
	TEST_CASE_REGISTER(SaleManagerMdbSlaveTest, testGatewayWaterOverflow);
	TEST_CASE_REGISTER(SaleManagerMdbSlaveTest, testEnableDisable);
}

bool SaleManagerMdbSlaveTest::init() {
	TEST_NUMBER_EQUAL(true, initConfigModem());

	result = new StringBuilder;
	timerEngine = new TimerEngine;
	eventEngine = new TestSaleManagerMdbSlaveEventEngine(result);
	fiscalRegister = new TestFiscalRegister(1, result);
	leds = new TestLed(result);
	chronicler = new EventRegistrar(config, timerEngine, eventEngine, realtime);
	slaveCoinChanger = new TestSaleManagerSniffer(2, result);
	slaveBillValidator = new TestSaleManagerSniffer(3, result);
	slaveCashless1 = new TestSaleManagerSniffer(4, result);
	slaveCashless2 = new TestMdbSlaveCashless(5, result);
	externCashless1 =  new TestMdbMasterCashless(6, result);
	externCashless2 =  new TestMdbMasterCashless(7, result);
	masterCashlessStack = new MdbMasterCashlessStack;
	masterCashlessStack->push(externCashless1);
	masterCashlessStack->push(externCashless2);
	slaveComGateway = new TestMdbSlaveComGateway(8, result);

	SaleManagerMdbSlaveParams params;
	params.config = config;
	params.timers = timerEngine;
	params.eventEngine = eventEngine;
	params.fiscalRegister = fiscalRegister;
	params.leds = leds;
	params.chronicler = chronicler;
	params.externCashless = masterCashlessStack;
	params.slaveCoinChanger = slaveCoinChanger;
	params.slaveBillValidator = slaveBillValidator;
	params.slaveCashless1 = slaveCashless1;
	params.slaveCashless2 = slaveCashless2;
	params.slaveGateway = slaveComGateway;
	params.rebootReason = Reboot::Reason_PowerDown;
	core = new SaleManagerMdbSlaveCore(&params);
	result->clear();
	return true;
}

void SaleManagerMdbSlaveTest::cleanup() {
	delete core;
	delete slaveComGateway;
	delete masterCashlessStack;
	delete externCashless2;
	delete externCashless1;
	delete slaveCashless2;
	delete slaveCashless1;
	delete slaveBillValidator;
	delete slaveCoinChanger;
	delete chronicler;
	delete leds;
	delete fiscalRegister;
	delete eventEngine;
	delete timerEngine;
	delete result;
	delete config;
	delete stat;
	delete realtime;
	delete memory;
}

bool SaleManagerMdbSlaveTest::initConfigModem() {
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

bool SaleManagerMdbSlaveTest::initConfigAutomatProductPrice(uint16_t index, const char *device, uint16_t number, uint16_t value) {
	product->findByIndex(index);
	ConfigPrice *price = product->getPrice(device, number);
	price->data.price = value;
	TEST_NUMBER_EQUAL(MemoryResult_Ok, price->save());
	return true;
}

bool SaleManagerMdbSlaveTest::gotoStateSale() {
	return true;
}

bool SaleManagerMdbSlaveTest::deliverEvent(EventInterface *event) {
	EventEnvelope envelope(EVENT_DATA_SIZE);
	if(event->pack(&envelope) == false) { return false; }
	core->proc(&envelope);
	return true;
}

bool SaleManagerMdbSlaveTest::deliverEvent(uint16_t type) {
	EventInterface event(type);
	return deliverEvent(&event);
}

bool SaleManagerMdbSlaveTest::deliverEvent(EventDeviceId deviceId, uint16_t type) {
	EventInterface event(deviceId, type);
	return deliverEvent(&event);
}

bool SaleManagerMdbSlaveTest::deliverEventUint32(EventDeviceId deviceId, uint16_t type, uint32_t value) {
	EventUint32Interface event(deviceId, type, value);
	return deliverEvent(&event);
}

bool SaleManagerMdbSlaveTest::testSaleByCash() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><Mdb::Sniffer(4)::reset><SCL::reset><SCG(8)::reset><MCL(6)::reset><MCL(7)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init slaveCashless
	slaveCashless2->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Enable slaveCashless
	slaveCashless2->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable><ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Event cash sale
	MdbSlaveCashlessInterface::EventVendRequest event1(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_CashSale, 1, 10000);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,10000,10000,0)>", result->getString());
	return true;
}

bool SaleManagerMdbSlaveTest::testSaleByCashless() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><Mdb::Sniffer(4)::reset><SCL::reset><SCG(8)::reset><MCL(6)::reset><MCL(7)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init slaveCashless
	slaveCashless2->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Enable slaveCashless
	slaveCashless2->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable><ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Event cashless sale
	MdbSnifferCashless::EventVend event1(slaveCashless1->getDeviceId(), 1, 30000);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,30000,30000,0)>", result->getString());
	return true;
}

bool SaleManagerMdbSlaveTest::testSaleByExternCashless() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><Mdb::Sniffer(4)::reset><SCL::reset><SCG(8)::reset><MCL(6)::reset><MCL(7)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init slaveCashless
	slaveCashless2->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Enable slaveCashless
	slaveCashless2->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable><ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Start cashless session
	EventUint32Interface credit1(externCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(6)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved event5(externCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)><MCL(6)::saleComplete>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("", result->getString());

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(externCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbSlaveTest::testSaleByExternCashlessAlwaysIdle() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><Mdb::Sniffer(4)::reset><SCL::reset><SCG(8)::reset><MCL(6)::reset><MCL(7)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init slaveCashless
	slaveCashless2->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Enable slaveCashless
	slaveCashless2->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable><ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(6)::sale(4,121)><MCL(7)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved event5(externCashless2->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<SCL::approveVend=121><MCL(6)::disable><MCL(7)::disable>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)><MCL(7)::saleComplete>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("", result->getString());

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(externCashless2->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbSlaveTest::testSaleByExternMixedCashless() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><Mdb::Sniffer(4)::reset><SCL::reset><SCG(8)::reset><MCL(6)::reset><MCL(7)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init slaveCashless
	slaveCashless2->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Enable slaveCashless
	slaveCashless2->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable><ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Start cashless session
	EventUint32Interface credit1(externCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(6)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved event5(externCashless1->getDeviceId(), Fiscal::Payment_Ephor, 100, Fiscal::Payment_Sberbank, 21);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/3,100,100,0)><MCL(6)::saleComplete>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("", result->getString());

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(externCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable>", result->getString());
	result->clear();
	return true;
}

bool SaleManagerMdbSlaveTest::testGatewaySaleByCash() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><Mdb::Sniffer(4)::reset><SCL::reset><SCG(8)::reset><MCL(6)::reset><MCL(7)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init slaveCashless
	slaveCashless2->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Enable slaveCashless
	slaveCashless2->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable><ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Init Gateway
	slaveComGateway->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveComGateway::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Event cash sale
	MdbSlaveCashlessInterface::EventVendRequest event1(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_CashSale, 1, 10000);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("", result->getString());

	// Report cash sale
	MdbSlaveComGateway::ReportTransaction event2(slaveComGateway->getDeviceId());
	event2.data.transactionType = Mdb::ComGateway::TransactionType_PaidVend;
	event2.data.itemNumber = 1;
	event2.data.price = 10000;
	event2.data.cashInCashbox = 15000;
	event2.data.cashOut = 5000;
	TEST_NUMBER_EQUAL(true, deliverEvent(&event2));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,CA/0,10000,15000,0)>", result->getString());
	return true;
}

bool SaleManagerMdbSlaveTest::testGatewaySaleByCashless() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><Mdb::Sniffer(4)::reset><SCL::reset><SCG(8)::reset><MCL(6)::reset><MCL(7)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init slaveCashless
	slaveCashless2->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Enable slaveCashless
	slaveCashless2->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable><ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Init Gateway
	slaveComGateway->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveComGateway::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Event cashless sale
	MdbSnifferCashless::EventVend event1(slaveCashless1->getDeviceId(), 1, 10000);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("", result->getString());

	// Report cashless sale
	MdbSlaveComGateway::ReportTransaction event2(slaveComGateway->getDeviceId());
	event2.data.transactionType = Mdb::ComGateway::TransactionType_PaidVend;
	event2.data.itemNumber = 1;
	event2.data.price = 20000;
	event2.data.valueInCashless1 = 20000;
	TEST_NUMBER_EQUAL(true, deliverEvent(&event2));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,20000,20000,0)>", result->getString());
	return true;
}

bool SaleManagerMdbSlaveTest::testGatewaySaleByToken() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><Mdb::Sniffer(4)::reset><SCL::reset><SCG(8)::reset><MCL(6)::reset><MCL(7)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init slaveCashless
	slaveCashless2->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Enable slaveCashless
	slaveCashless2->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable><ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Init Gateway
	slaveComGateway->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveComGateway::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Event cashless sale
	MdbSnifferCashless::EventVend event1(slaveCashless1->getDeviceId(), 1, 10000);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("", result->getString());

	// Report cashless sale
	MdbSlaveComGateway::ReportTransaction event2(slaveComGateway->getDeviceId());
	event2.data.transactionType = Mdb::ComGateway::TransactionType_TokenVend;
	event2.data.itemNumber = 1;
	TEST_NUMBER_EQUAL(true, deliverEvent(&event2));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,TA/2,10,10,0)>", result->getString());
	return true;
}

bool SaleManagerMdbSlaveTest::testGatewayCashlessVendless() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><Mdb::Sniffer(4)::reset><SCL::reset><SCG(8)::reset><MCL(6)::reset><MCL(7)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init slaveCashless
	slaveCashless2->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Enable slaveCashless
	slaveCashless2->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable><ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Init Gateway
	slaveComGateway->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveComGateway::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Event cashless sale
	MdbSnifferCashless::EventVend event1(slaveCashless1->getDeviceId(), 1, 10000);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("", result->getString());

	// Report cashless vendless
	MdbSlaveComGateway::ReportTransaction event2(slaveComGateway->getDeviceId());
	event2.data.transactionType = Mdb::ComGateway::TransactionType_Vendless;
	event2.data.itemNumber = 1;
	event2.data.price = 20000;
	TEST_NUMBER_EQUAL(true, deliverEvent(&event2));
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool SaleManagerMdbSlaveTest::testGatewaySaleByExternCashless() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><Mdb::Sniffer(4)::reset><SCL::reset><SCG(8)::reset><MCL(6)::reset><MCL(7)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init slaveCashless
	slaveCashless2->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Enable slaveCashless
	slaveCashless2->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable><ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Init Gateway
	slaveComGateway->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveComGateway::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Start cashless session
	EventUint32Interface credit1(externCashless1->getDeviceId(), MdbMasterCashless::Event_SessionBegin, 200);
	TEST_NUMBER_EQUAL(true, deliverEvent(&credit1));
	TEST_STRING_EQUAL("<SCL::setCredit(200)>", result->getString());
	result->clear();

	// Push product button
	MdbSlaveCashlessInterface::EventVendRequest vendRequest(slaveCashless1->getDeviceId(), MdbSlaveCashlessInterface::Event_VendRequest, 4, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&vendRequest));
	TEST_STRING_EQUAL("<MCL(6)::sale(4,121)>", result->getString());
	result->clear();

	// Vend approved
	MdbMasterCashlessInterface::EventApproved event5(externCashless1->getDeviceId(), Fiscal::Payment_Cashless, 121);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<SCL::approveVend=121>", result->getString());
	result->clear();

	// Vend complete
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_VendComplete));
	TEST_STRING_EQUAL("<MCL(6)::saleComplete>", result->getString());
	result->clear();

	// Print check
	TEST_NUMBER_EQUAL(true, deliverEvent(Fiscal::Register::Event_CommandOK));
	TEST_STRING_EQUAL("", result->getString());

	// Session closed
	TEST_NUMBER_EQUAL(true, deliverEvent(externCashless1->getDeviceId(), MdbMasterCashless::Event_SessionEnd));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable>", result->getString());
	result->clear();

	// Report cashless sale
	MdbSlaveComGateway::ReportTransaction event2(slaveComGateway->getDeviceId());
	event2.data.transactionType = Mdb::ComGateway::TransactionType_PaidVend;
	event2.data.itemNumber = 4;
	event2.data.price = 121;
	event2.data.valueInCashless1 = 121;
	TEST_NUMBER_EQUAL(true, deliverEvent(&event2));
	TEST_STRING_EQUAL("<FR::sale(Ячейка,DA/1,121,121,0)>", result->getString());
	return true;
}

bool SaleManagerMdbSlaveTest::testGatewayWaterOverflow() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><Mdb::Sniffer(4)::reset><SCL::reset><SCG(8)::reset><MCL(6)::reset><MCL(7)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init slaveCashless
	slaveCashless2->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Enable slaveCashless
	slaveCashless2->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable><ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();

	// Init Gateway
	slaveComGateway->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveComGateway::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Report water overflow detect
	MdbSlaveComGateway::ReportEvent event1(slaveComGateway->getDeviceId());
	event1.data.code.set("OBI");
	event1.data.duration = 0;
	event1.data.activity = 1;
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("", result->getString());
	TEST_NUMBER_NOT_EQUAL(0, (int)config->getAutomat()->getSMContext()->getErrors()->getByCode(ConfigEvent::Type_WaterOverflow));

	// Report water overflow off
	MdbSlaveComGateway::ReportEvent event2(slaveComGateway->getDeviceId());
	event2.data.code.set("OBI");
	event2.data.duration = 0;
	event2.data.activity = 0;
	TEST_NUMBER_EQUAL(true, deliverEvent(&event2));
	TEST_STRING_EQUAL("", result->getString());
	TEST_NUMBER_EQUAL(0, (int)config->getAutomat()->getSMContext()->getErrors()->getByCode(ConfigEvent::Type_WaterOverflow));
	return true;
}

bool SaleManagerMdbSlaveTest::testEnableDisable() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><Mdb::Sniffer(4)::reset><SCL::reset><SCG(8)::reset><MCL(6)::reset><MCL(7)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Init Gateway
	slaveComGateway->setInited(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(MdbSlaveComGateway::Event_Reset));
	TEST_STRING_EQUAL("", result->getString());

	// Disable all devices
	slaveBillValidator->setEnabled(false);
	TEST_NUMBER_EQUAL(true, deliverEvent(slaveBillValidator->getDeviceId(), MdbSnifferBillValidator::Event_Disable));
	TEST_STRING_EQUAL("", result->getString());

	slaveCoinChanger->setEnabled(false);
	TEST_NUMBER_EQUAL(true, deliverEvent(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Disable));
	TEST_STRING_EQUAL("", result->getString());

	slaveCashless1->setEnabled(false);
	TEST_NUMBER_EQUAL(true, deliverEvent(slaveCashless1->getDeviceId(), MdbSnifferCashless::Event_Disable));
	TEST_STRING_EQUAL("", result->getString());

	slaveCashless2->setEnabled(false);
	TEST_NUMBER_EQUAL(true, deliverEvent(slaveCashless2->getDeviceId(), MdbSlaveCashlessInterface::Event_Disable));
	TEST_STRING_EQUAL("<MCL(6)::disable><MCL(7)::disable>", result->getString());
	result->clear();

	// Enable all devices
	slaveBillValidator->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(slaveBillValidator->getDeviceId(), MdbSnifferBillValidator::Event_Enable));
	TEST_STRING_EQUAL("<ledPayment=2><event=1,AutomatState,1>", result->getString());
	result->clear();

	slaveCoinChanger->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Enable));
	TEST_STRING_EQUAL("", result->getString());

	slaveCashless1->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(slaveCashless1->getDeviceId(), MdbSnifferCashless::Event_Enable));
	TEST_STRING_EQUAL("", result->getString());

	slaveCashless2->setEnabled(true);
	TEST_NUMBER_EQUAL(true, deliverEvent(slaveCashless2->getDeviceId(), MdbSlaveCashlessInterface::Event_Enable));
	TEST_STRING_EQUAL("<MCL(6)::enable><MCL(7)::enable>", result->getString());
	result->clear();

	// Disable all devices
	slaveBillValidator->setEnabled(false);
	TEST_NUMBER_EQUAL(true, deliverEvent(slaveBillValidator->getDeviceId(), MdbSnifferBillValidator::Event_Disable));
	TEST_STRING_EQUAL("", result->getString());

	slaveCoinChanger->setEnabled(false);
	TEST_NUMBER_EQUAL(true, deliverEvent(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Disable));
	TEST_STRING_EQUAL("", result->getString());

	slaveCashless1->setEnabled(false);
	TEST_NUMBER_EQUAL(true, deliverEvent(slaveCashless1->getDeviceId(), MdbSnifferCashless::Event_Disable));
	TEST_STRING_EQUAL("", result->getString());

	slaveCashless2->setEnabled(false);
	TEST_NUMBER_EQUAL(true, deliverEvent(slaveCashless2->getDeviceId(), MdbSlaveCashlessInterface::Event_Disable));
	TEST_STRING_EQUAL("<MCL(6)::disable><MCL(7)::disable><ledPayment=2><event=1,AutomatState,0>", result->getString());
	result->clear();
	return true;
}
