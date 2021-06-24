#include <lib/sale_manager/mdb_no_cashless/SaleManagerMdbNoCashlessCore.h>
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

#include "lib/utils/stm32/HardwareUartForwardController.h"

class TestSaleManagerMdbNoCashlessEventEngine : public TestEventEngine {
public:
	TestSaleManagerMdbNoCashlessEventEngine(StringBuilder *result) : TestEventEngine(result) {}
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

class SaleManagerMdbNoCashlessTest : public TestSet {
public:
	SaleManagerMdbNoCashlessTest();
	bool init();
	void cleanup();
	bool initConfigModem();
	bool initConfigAutomatProductPrice(uint16_t index, const char *device, uint16_t number, uint16_t value);
	bool gotoStateSale();
	bool testSaleByCash();
	bool testSaleByCashAndFastCash();
	bool testSaleByCashWithChange();
	bool testSaleByCashWithChangeBeforeEnable();
	bool testSaleByCashWithChangeAndFastCash();
	bool testReturnChange();
	bool testReturnChangeWithDisabling();

private:
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	ConfigModem *config;
	ConfigProductIterator *product;
	TimerEngine *timerEngine;
	TestSaleManagerMdbNoCashlessEventEngine *eventEngine;
	StringBuilder *result;
	TestFiscalRegister *fiscalRegister;
	TestLed *leds;
	EventRegistrar *chronicler;
	TestSaleManagerSniffer *slaveCoinChanger;
	TestSaleManagerSniffer *slaveBillValidator;
	SaleManagerMdbNoCashlessCore *core;

	bool deliverEvent(EventInterface *event);
	bool deliverEvent(uint16_t type);
	bool deliverEvent(EventDeviceId deviceId, uint16_t type);
	bool deliverEventUint32(EventDeviceId deviceId, uint16_t type, uint32_t value);
};

TEST_SET_REGISTER(SaleManagerMdbNoCashlessTest);

SaleManagerMdbNoCashlessTest::SaleManagerMdbNoCashlessTest() {
	TEST_CASE_REGISTER(SaleManagerMdbNoCashlessTest, testSaleByCash);
	TEST_CASE_REGISTER(SaleManagerMdbNoCashlessTest, testSaleByCashAndFastCash);
	TEST_CASE_REGISTER(SaleManagerMdbNoCashlessTest, testSaleByCashWithChange);
	TEST_CASE_REGISTER(SaleManagerMdbNoCashlessTest, testSaleByCashWithChangeBeforeEnable);
	TEST_CASE_REGISTER(SaleManagerMdbNoCashlessTest, testSaleByCashWithChangeAndFastCash);
	TEST_CASE_REGISTER(SaleManagerMdbNoCashlessTest, testReturnChange);
	TEST_CASE_REGISTER(SaleManagerMdbNoCashlessTest, testReturnChangeWithDisabling);
}

bool SaleManagerMdbNoCashlessTest::init() {
	TEST_NUMBER_EQUAL(true, initConfigModem());

	result = new StringBuilder;
	timerEngine = new TimerEngine;
	eventEngine = new TestSaleManagerMdbNoCashlessEventEngine(result);
	fiscalRegister = new TestFiscalRegister(1, result);
	leds = new TestLed(result);
	chronicler = new EventRegistrar(config, timerEngine, eventEngine, realtime);
	slaveCoinChanger = new TestSaleManagerSniffer(2, result);
	slaveBillValidator = new TestSaleManagerSniffer(3, result);

	SaleManagerMdbNoCashlessParams params;
	params.config = config;
	params.timers = timerEngine;
	params.eventEngine = eventEngine;
	params.fiscalRegister = fiscalRegister;
	params.leds = leds;
	params.chronicler = chronicler;
	params.slaveCoinChanger = slaveCoinChanger;
	params.slaveBillValidator = slaveBillValidator;
	params.rebootReason = Reboot::Reason_PowerDown;
	core = new SaleManagerMdbNoCashlessCore(&params);
	result->clear();
	return true;
}

void SaleManagerMdbNoCashlessTest::cleanup() {
	delete core;
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

bool SaleManagerMdbNoCashlessTest::initConfigModem() {
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
	TEST_NUMBER_EQUAL(true, initConfigAutomatProductPrice(3, "CA", 0, 110));
	TEST_NUMBER_EQUAL(true, initConfigAutomatProductPrice(3, "DA", 1, 121));
	return true;
}

bool SaleManagerMdbNoCashlessTest::initConfigAutomatProductPrice(uint16_t index, const char *device, uint16_t number, uint16_t value) {
	product->findByIndex(index);
	ConfigPrice *price = product->getPrice(device, number);
	price->data.price = value;
	TEST_NUMBER_EQUAL(MemoryResult_Ok, price->save());
	return true;
}

bool SaleManagerMdbNoCashlessTest::gotoStateSale() {
	return true;
}

bool SaleManagerMdbNoCashlessTest::deliverEvent(EventInterface *event) {
	EventEnvelope envelope(EVENT_DATA_SIZE);
	if(event->pack(&envelope) == false) { return false; }
	core->proc(&envelope);
	return true;
}

bool SaleManagerMdbNoCashlessTest::deliverEvent(uint16_t type) {
	EventInterface event(type);
	return deliverEvent(&event);
}

bool SaleManagerMdbNoCashlessTest::deliverEvent(EventDeviceId deviceId, uint16_t type) {
	EventInterface event(deviceId, type);
	return deliverEvent(&event);
}

bool SaleManagerMdbNoCashlessTest::deliverEventUint32(EventDeviceId deviceId, uint16_t type, uint32_t value) {
	EventUint32Interface event(deviceId, type, value);
	return deliverEvent(&event);
}

bool SaleManagerMdbNoCashlessTest::testSaleByCash() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Event coin
	Mdb::EventDeposite coin1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Tube, 10);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("", result->getString());

	// Event coin
	Mdb::EventDeposite coin2(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Cashbox, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("", result->getString());

	// Sale
	EventInterface disable1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Disable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&disable1));
	TEST_STRING_EQUAL("", result->getString());

	EventInterface enable1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Enable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&enable1));
	TEST_STRING_EQUAL("", result->getString());

	timerEngine->tick(5000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,110,110,0)>", result->getString());
	return true;
}

bool SaleManagerMdbNoCashlessTest::testSaleByCashAndFastCash() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Event coin
	Mdb::EventDeposite coin1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Tube, 10);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("", result->getString());

	// Event coin
	Mdb::EventDeposite coin2(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Cashbox, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("", result->getString());

	// Sale
	EventInterface disable1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Disable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&disable1));
	TEST_STRING_EQUAL("", result->getString());

	EventInterface enable1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Enable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&enable1));
	TEST_STRING_EQUAL("", result->getString());

	// Event coin
	Mdb::EventDeposite coin3(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Tube, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin3));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,110,110,0)>", result->getString());
	result->clear();

	// Sale
	EventInterface disable2(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Disable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&disable2));
	TEST_STRING_EQUAL("", result->getString());

	EventInterface enable2(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Enable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&enable2));
	TEST_STRING_EQUAL("", result->getString());

	timerEngine->tick(5000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,50,50,0)>", result->getString());
	return true;
}

bool SaleManagerMdbNoCashlessTest::testSaleByCashWithChange() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Event coin
	Mdb::EventDeposite coin1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Tube, 10);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("", result->getString());

	// Event coin
	Mdb::EventDeposite coin2(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Cashbox, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("", result->getString());

	// Sale
	EventInterface disable1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Disable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&disable1));
	TEST_STRING_EQUAL("", result->getString());

	EventInterface enable1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Enable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&enable1));
	TEST_STRING_EQUAL("", result->getString());

	// Change
	EventUint32Interface change1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DispenseCoin, 60);
	TEST_NUMBER_EQUAL(true, deliverEvent(&change1));
	TEST_STRING_EQUAL("", result->getString());

	timerEngine->tick(10000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,50,50,0)>", result->getString());
	return true;
}

bool SaleManagerMdbNoCashlessTest::testSaleByCashWithChangeBeforeEnable() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Event coin
	Mdb::EventDeposite coin1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Tube, 10);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("", result->getString());

	// Event coin
	Mdb::EventDeposite coin2(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Cashbox, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("", result->getString());

	// Sale
	EventInterface disable1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Disable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&disable1));
	TEST_STRING_EQUAL("", result->getString());

	// Change
	EventUint32Interface change1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DispenseCoin, 60);
	TEST_NUMBER_EQUAL(true, deliverEvent(&change1));
	TEST_STRING_EQUAL("", result->getString());

	// Enable
	EventInterface enable1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Enable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&enable1));
	TEST_STRING_EQUAL("", result->getString());

	timerEngine->tick(10000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,50,50,0)>", result->getString());
	return true;
}

bool SaleManagerMdbNoCashlessTest::testSaleByCashWithChangeAndFastCash() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Event coin
	Mdb::EventDeposite coin1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Tube, 10);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("", result->getString());

	// Event coin
	Mdb::EventDeposite coin2(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Cashbox, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("", result->getString());

	// Sale
	EventInterface disable1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Disable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&disable1));
	TEST_STRING_EQUAL("", result->getString());

	EventInterface enable1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Enable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&enable1));
	TEST_STRING_EQUAL("", result->getString());

	// Change
	EventUint32Interface change1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DispenseCoin, 60);
	TEST_NUMBER_EQUAL(true, deliverEvent(&change1));
	TEST_STRING_EQUAL("", result->getString());

	// Event coin
	Mdb::EventDeposite coin3(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Tube, 50);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin3));
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,50,50,0)>", result->getString());
	result->clear();

	// Sale
	EventInterface disable2(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Disable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&disable2));
	TEST_STRING_EQUAL("", result->getString());

	EventInterface enable2(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Enable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&enable2));
	TEST_STRING_EQUAL("", result->getString());

	timerEngine->tick(5000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<FR::sale(ίχεικΰ,CA/0,50,50,0)>", result->getString());
	return true;
}

bool SaleManagerMdbNoCashlessTest::testReturnChange() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Event coin
	Mdb::EventDeposite coin1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Tube, 10);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("", result->getString());

	// Event coin
	Mdb::EventDeposite coin2(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Cashbox, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("", result->getString());

	// Change
	EventUint32Interface change1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DispenseCoin, 110);
	TEST_NUMBER_EQUAL(true, deliverEvent(&change1));
	TEST_STRING_EQUAL("", result->getString());

	timerEngine->tick(10000);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool SaleManagerMdbNoCashlessTest::testReturnChangeWithDisabling() {
	// Reset core
	core->reset();
	TEST_STRING_EQUAL("<Mdb::Sniffer(2)::reset><Mdb::Sniffer(3)::reset><ledPayment=1>", result->getString());
	result->clear();

	// Event coin
	Mdb::EventDeposite coin1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Tube, 10);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin1));
	TEST_STRING_EQUAL("", result->getString());

	// Event coin
	Mdb::EventDeposite coin2(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DepositeCoin, Mdb::CoinChanger::Route_Cashbox, 100);
	TEST_NUMBER_EQUAL(true, deliverEvent(&coin2));
	TEST_STRING_EQUAL("", result->getString());

	// Disable(Max credit)
	EventInterface disable1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Disable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&disable1));
	TEST_STRING_EQUAL("", result->getString());

	EventInterface enable1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_Enable);
	TEST_NUMBER_EQUAL(true, deliverEvent(&enable1));
	TEST_STRING_EQUAL("", result->getString());

	// Change
	EventUint32Interface change1(slaveCoinChanger->getDeviceId(), MdbSnifferCoinChanger::Event_DispenseCoin, 110);
	TEST_NUMBER_EQUAL(true, deliverEvent(&change1));
	TEST_STRING_EQUAL("", result->getString());

	timerEngine->tick(10000);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	return true;
}
