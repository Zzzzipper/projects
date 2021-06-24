#include "lib/modem/ConfigUpdater.h"

#include "common/memory/include/RamMemory.h"
#include "common/timer/include/TestRealTime.h"
#include "common/utils/include/TestEventObserver.h"
#include "common/test/include/Test.h"
#include "common/logger/include/Logger.h"

class TestConfigUpdaterObserver : public TestEventObserver {
public:
	TestConfigUpdaterObserver(StringBuilder *result) : TestEventObserver(result) {}
	virtual void proc(Event *event) {
		switch(event->getType()) {
		case Dex::DataParser::Event_AsyncOk: *result << "<event=AsyncOk>"; break;
		case Dex::DataParser::Event_AsyncError: *result << "<event=AsyncError>"; break;
		default: *result << "<event=" << event->getType() << ">";
		}
	}
};

class ConfigUpdaterTest : public TestSet {
public:
	ConfigUpdaterTest();
	bool init();
	void cleanup();
	bool testResize();
	bool testUpdate();

private:
	StringBuilder *result;
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	ConfigModem *config;
	TimerEngine *timers;
	TestConfigUpdaterObserver *observer;
	ConfigUpdater *updater;
};

TEST_SET_REGISTER(ConfigUpdaterTest);

ConfigUpdaterTest::ConfigUpdaterTest() {
	TEST_CASE_REGISTER(ConfigUpdaterTest, testResize);
	TEST_CASE_REGISTER(ConfigUpdaterTest, testUpdate);
}

bool ConfigUpdaterTest::init() {
	result = new StringBuilder(1024, 1024);
	memory = new RamMemory(32000),
	realtime = new TestRealTime;
	stat = new StatStorage;
	config = new ConfigModem(memory, realtime, stat);
	timers = new TimerEngine;
	observer = new TestConfigUpdaterObserver(result);
	updater = new ConfigUpdater(timers, config, observer);
	return true;
}

void ConfigUpdaterTest::cleanup() {
	delete updater;
	delete observer;
	delete timers;
	delete config;
	delete stat;
	delete realtime;
	delete memory;
	delete result;
}

bool ConfigUpdaterTest::testResize() {
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config->init());

	StringBuilder data1(
"DXS*EPHOR00001*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"AC2*868345031901523*internet*gdata*gdata*1*50462720*16777245*1*Revision:1418B08SIM800C24_BT\r\n"
"AC3*0*1*192.168.1.210*0**0\r\n"
"IC1******108\r\n"
"IC4*2*7\r\n"
"PC1*11*2200*^D7^^E8^^EF^^F1^^FB^ ^CB^^E5^^E9^^E7^(^E2^ ^E0^^F1^^F1^^EE^^F0^^F2^^E8^^EC^^E5^^ED^^F2^^E5^), 40^E3^^F5^28^F8^^F2^\r\n"
"PC9*11*1*3\r\n"
"PC4*0\r\n"
"PC7*11*CA*0*2200\r\n"
"PC7*11*DA*1*2200\r\n"
"PC1*12*1000*^DF^^F7^^E5^^E9^^EA^^E0^\r\n"
"PC9*12*2*0\r\n"
"PC4*0\r\n"
"PC7*12*CA*0*1000\r\n"
"PC7*12*DA*1*1000\r\n"
"PC1*13*3000*^CA^^F0^^F3^^E0^^F1^^F1^^E0^^ED^^FB^ 7 ^E4^^ED^^E5^^E9^ ^EC^^E8^^ED^^E8^ ^CA^^E0^^EA^^E0^^EE^ 65^E3^^F5^21^F8^^F2^.\r\n"
"PC9*13*3*2\r\n"
"PC4*0\r\n"
"PC7*13*CA*0*3000\r\n"
"PC7*13*DA*1*3000\r\n"
"PC1*14*1000*^DF^^F7^^E5^^E9^^EA^^E0^\r\n"
"PC9*14*4*0\r\n"
"PC4*0\r\n"
"PC7*14*CA*0*1000\r\n"
"PC7*14*DA*1*1000\r\n"
"G85*028D\r\n"
"SE*431*0001\r\n"
"DXE*1*1\r\n");

	updater->resize(&data1, false);
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Busy, updater->getResult());

	for(uint16_t i = 0; i < 1000; i++) {
		timers->tick(1);
		timers->execute();
		if(updater->getResult() != Dex::DataParser::Result_Busy) {
			break;
		}
	}

	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Ok, updater->getResult());
	TEST_NUMBER_EQUAL(4, config->getAutomat()->getPriceListNum());
	TEST_NUMBER_EQUAL(4, config->getAutomat()->getProductNum());
#if 0
	ConfigProductList *products = config->getAutomat()->getProductList();
	ConfigProduct product1;
	TEST_NUMBER_EQUAL(MemoryResult_Ok, products->getByIndex(0, &product1));
	TEST_STRING_EQUAL("Чипсы Лейз(в ассортименте), 40гх28шт", product1.data.name.get());
	ConfigPrice price1;
	TEST_NUMBER_EQUAL(MemoryResult_Ok, product1.prices.getByIndex(0, &price1));
	TEST_NUMBER_EQUAL(2200, price1.data.price);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, product1.prices.getByIndex(1, &price1));
	TEST_NUMBER_EQUAL(1000, price1.data.price);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, product1.prices.getByIndex(2, &price1));
	TEST_NUMBER_EQUAL(1000, price1.data.price);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, product1.prices.getByIndex(3, &price1));
	TEST_NUMBER_EQUAL(2200, price1.data.price);
#endif
	return true;
}

bool ConfigUpdaterTest::testUpdate() {
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config->init());

	StringBuilder data1(
"DXS*EPHOR00001*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"AC2*868345031901523*internet*gdata*gdata*1*50462720*16777245*1*Revision:1418B08SIM800C24_BT\r\n"
"AC3*0*1*192.168.1.210*0**0\r\n"
"IC1******108\r\n"
"IC4*2*7\r\n"
"PC1*11*2200*Test1\r\n"
"PC9*11*1*3\r\n"
"PC4*0\r\n"
"PC7*11*CA*0*2200\r\n"
"PC7*11*DA*1*2200\r\n"
"PC1*12*1000*Test2\r\n"
"PC9*12*2*0\r\n"
"PC4*0\r\n"
"PC7*12*CA*0*2700\r\n"
"PC7*12*DA*1*2700\r\n"
"PC1*13*3000*Test3\r\n"
"PC9*13*3*2\r\n"
"PC4*0\r\n"
"PC7*13*CA*0*3000\r\n"
"PC7*13*DA*1*3000\r\n"
"PC1*14*1000*Test4\r\n"
"PC9*14*4*0\r\n"
"PC4*0\r\n"
"PC7*14*CA*0*3500\r\n"
"PC7*14*DA*1*3500\r\n"
"G85*028D\r\n"
"SE*431*0001\r\n"
"DXE*1*1\r\n");
	updater->resize(&data1, false);
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Busy, updater->getResult());
	for(uint16_t i = 0; i < 1000; i++) {
		timers->tick(1);
		timers->execute();
		if(updater->getResult() != Dex::DataParser::Result_Busy) {
			break;
		}
	}

	StringBuilder data2(
"DXS*EPHOR00001*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"IC1******108\r\n"
"IC4*2***1*\r\n"
"AC2*****1\r\n"
"PC1*11*2200*Чипсы Лейз(в ассортименте), 40гх28шт\r\n"
"PC9*11**3*65\r\n"
"PC7*11*CA*0*2500\r\n"
"PC7*11*DA*1*2500\r\n"
"PC1*13*3000*Круассаны 7 дней мини Какао 65гх21шт.\r\n"
"PC9*13**2*66\r\n"
"PC7*13*CA*0*3200\r\n"
"PC7*13*DA*1*3200\r\n"
"G85*A8D1\r\n"
"SE*190*0001\r\n"
"DXE*1*1\r\n");
	updater->resize(&data2, false);
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Busy, updater->getResult());
	for(uint16_t i = 0; i < 1000; i++) {
		timers->tick(1);
		timers->execute();
		if(updater->getResult() != Dex::DataParser::Result_Busy) {
			break;
		}
	}

	ConfigModem config2(memory, realtime, stat);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.load());

#if 0
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getTotalCount());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getTotalMoney());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getCount());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getMoney());
#endif
	TEST_NUMBER_EQUAL(4, config2.getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(4, config2.getAutomat()->getPriceListNum());

	// check product info
	ConfigProductIterator *product = config2.getAutomat()->createIterator();
	TEST_NUMBER_EQUAL(true, product->first());
	TEST_STRING_EQUAL("11", product->getId());
	TEST_NUMBER_EQUAL(1, product->getCashlessId());
	TEST_STRING_EQUAL("Чипсы Лейз(в ассортименте), 40гх28шт", product->getName());
	TEST_NUMBER_EQUAL(3, product->getTaxRate());
	TEST_NUMBER_EQUAL(0, product->getTotalCount());
	TEST_NUMBER_EQUAL(0, product->getTotalMoney());
	TEST_NUMBER_EQUAL(0, product->getCount());
	TEST_NUMBER_EQUAL(0, product->getMoney());

	// check price list info
	ConfigPrice *price = product->getPrice("CA", 0);
	TEST_NUMBER_NOT_EQUAL((int)NULL, (int)price);
	TEST_NUMBER_EQUAL(2500, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("DA", 1);
	TEST_NUMBER_NOT_EQUAL((int)NULL, (int)price);
	TEST_NUMBER_EQUAL(2500, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	// product12
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("12", product->getId());
	TEST_NUMBER_EQUAL(2, product->getCashlessId());
	TEST_STRING_EQUAL("Test2", product->getName());
	price = product->getPrice("CA", 0);
	TEST_NUMBER_NOT_EQUAL((int)NULL, (int)price);
	TEST_NUMBER_EQUAL(2700, price->getPrice());
	price = product->getPrice("DA", 1);
	TEST_NUMBER_NOT_EQUAL((int)NULL, (int)price);
	TEST_NUMBER_EQUAL(2700, price->getPrice());

	// product13
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("13", product->getId());
	TEST_NUMBER_EQUAL(3, product->getCashlessId());
	TEST_STRING_EQUAL("Круассаны 7 дней мини Какао 65гх21шт.", product->getName());
	price = product->getPrice("CA", 0);
	TEST_NUMBER_NOT_EQUAL((int)NULL, (int)price);
	TEST_NUMBER_EQUAL(3200, price->getPrice());
	price = product->getPrice("DA", 1);
	TEST_NUMBER_NOT_EQUAL((int)NULL, (int)price);
	TEST_NUMBER_EQUAL(3200, price->getPrice());

	// product14
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("14", product->getId());
	TEST_NUMBER_EQUAL(4, product->getCashlessId());
	TEST_STRING_EQUAL("Test4", product->getName());
	price = product->getPrice("CA", 0);
	TEST_NUMBER_NOT_EQUAL((int)NULL, (int)price);
	TEST_NUMBER_EQUAL(3500, price->getPrice());
	price = product->getPrice("DA", 1);
	TEST_NUMBER_NOT_EQUAL((int)NULL, (int)price);
	TEST_NUMBER_EQUAL(3500, price->getPrice());
	return true;
}
