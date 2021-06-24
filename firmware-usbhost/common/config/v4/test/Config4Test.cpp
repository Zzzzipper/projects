#include "config/v4/Config4Modem.h"
#include "config/v4/Config4Repairer.h"
#include "memory/include/RamMemory.h"
#include "timer/include/TestRealTime.h"
#include "utils/include/Buffer.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class Config4Test : public TestSet {
public:
	Config4Test();
	bool testInit();
	bool testRepair();
	bool testAutomatDecimalPoint();
	bool testRegisterSale();
	bool testMaximumSize();
	bool testCompareEqual();
	bool testCompareNotEqual();
};

TEST_SET_REGISTER(Config4Test);

Config4Test::Config4Test() {
#if 0
	TEST_CASE_REGISTER(Config4Test, testInit);
	TEST_CASE_REGISTER(Config4Test, testRepair);
	TEST_CASE_REGISTER(Config4Test, testRegisterSale);
	TEST_CASE_REGISTER(Config4Test, testMaximumSize);
	TEST_CASE_REGISTER(Config4Test, testCompareEqual);
	TEST_CASE_REGISTER(Config4Test, testCompareNotEqual);
#endif
}

bool Config4Test::testInit() {
	RamMemory memory(128000);
	TestRealTime realtime;
	StatStorage stat;
	Config4Modem config1(&memory, &realtime, &stat);
	memory.fill(0xFF);

	// init
	config1.getAutomat()->shutdown();
	config1.getAutomat()->addProduct("01", 1);
	config1.getAutomat()->addProduct("02", 2);
	config1.getAutomat()->addProduct("03", 3);
	config1.getAutomat()->addProduct("04", 4);
	config1.getAutomat()->addProduct("01", 1);
	config1.getAutomat()->addProduct("01", 1);
	config1.getAutomat()->addPriceList("CA", 0, Config3PriceIndexType_Base);
	config1.getAutomat()->addPriceList("DA", 1, Config3PriceIndexType_Base);
	config1.getAutomat()->addPriceList("DB", 1, Config3PriceIndexType_Base);
	config1.getAutomat()->addPriceList("DB", 1, Config3PriceIndexType_Base);
	config1.getAutomat()->addPriceList("DB", 1, Config3PriceIndexType_Base);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config1.init());

	// load
	Config4Modem config2(&memory, &realtime, &stat);
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.load());

	// boot check
	TEST_NUMBER_EQUAL(1, config2.getBoot()->getFirmwareRelease());

	// fiscal check
	TEST_NUMBER_EQUAL(0, config2.getFiscal()->getKkt());

	// automat check
	Config4ProductList *productList2 = config2.getAutomat()->getProductList();
#if 0
	TEST_NUMBER_EQUAL(0, productList2->getTotalCount());
	TEST_NUMBER_EQUAL(0, productList2->getTotalMoney());
	TEST_NUMBER_EQUAL(0, productList2->getCount());
	TEST_NUMBER_EQUAL(0, productList2->getMoney());
	TEST_NUMBER_EQUAL(4, productList2->getProductNum());
	TEST_NUMBER_EQUAL(3, productList2->getPriceListNum());
#endif
	Config4ProductIterator *product = config2.getAutomat()->createIterator();

	// first()
	TEST_NUMBER_EQUAL(true, product->first());
	TEST_STRING_EQUAL("Ячейка", product->getName());
	TEST_NUMBER_EQUAL(true, product->getEnable());
	TEST_NUMBER_EQUAL(0, product->getTotalCount());
	TEST_NUMBER_EQUAL(0, product->getTotalMoney());
	TEST_NUMBER_EQUAL(0, product->getCount());
	TEST_NUMBER_EQUAL(0, product->getMoney());

	Config3Price *price1 = product->getPriceByIndex(0);
	TEST_POINTER_NOT_NULL(price1);
	TEST_NUMBER_EQUAL(1000, price1->getPrice());
	TEST_NUMBER_EQUAL(0, price1->getTotalCount());
	TEST_NUMBER_EQUAL(0, price1->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price1->getCount());
	TEST_NUMBER_EQUAL(0, price1->getMoney());

	price1 = product->getPriceByIndex(1);
	TEST_POINTER_NOT_NULL(price1);
	TEST_NUMBER_EQUAL(1000, price1->getPrice());
	TEST_NUMBER_EQUAL(0, price1->getTotalCount());
	TEST_NUMBER_EQUAL(0, price1->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price1->getCount());
	TEST_NUMBER_EQUAL(0, price1->getMoney());

	price1 = product->getPriceByIndex(2);
	TEST_POINTER_NOT_NULL(price1);
	TEST_NUMBER_EQUAL(1000, price1->getPrice());
	TEST_NUMBER_EQUAL(0, price1->getTotalCount());
	TEST_NUMBER_EQUAL(0, price1->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price1->getCount());
	TEST_NUMBER_EQUAL(0, price1->getMoney());

	price1 = product->getPriceByIndex(3);
	TEST_POINTER_NOT_NULL(price1);

	// next()
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("Ячейка", product->getName());
	TEST_NUMBER_EQUAL(true, product->getEnable());
	TEST_NUMBER_EQUAL(0, product->getTotalCount());
	TEST_NUMBER_EQUAL(0, product->getTotalMoney());
	TEST_NUMBER_EQUAL(0, product->getCount());
	TEST_NUMBER_EQUAL(0, product->getMoney());

	// next()
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("Ячейка", product->getName());
	TEST_NUMBER_EQUAL(true, product->getEnable());
	TEST_NUMBER_EQUAL(0, product->getTotalCount());
	TEST_NUMBER_EQUAL(0, product->getTotalMoney());
	TEST_NUMBER_EQUAL(0, product->getCount());
	TEST_NUMBER_EQUAL(0, product->getMoney());

	// next()
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("Ячейка", product->getName());
	TEST_NUMBER_EQUAL(true, product->getEnable());
	TEST_NUMBER_EQUAL(0, product->getTotalCount());
	TEST_NUMBER_EQUAL(0, product->getTotalMoney());
	TEST_NUMBER_EQUAL(0, product->getCount());
	TEST_NUMBER_EQUAL(0, product->getMoney());

	// next()
	TEST_NUMBER_EQUAL(false, product->next());
	return true;
}

bool Config4Test::testRepair() {
	RamMemory memory(128000);
	TestRealTime realtime;
	StatStorage stat;
	Config4Modem config(&memory, &realtime, &stat);
	Config4Repairer repairer(&config, &memory);
	memory.clear();

	// init only boot
	memory.setAddress(0);
	config.getBoot()->init(&memory);
	TEST_NUMBER_EQUAL(Config1Boot::FirmwareState_None, config.getBoot()->getFirmwareState());
	config.getBoot()->setFirmwareState(Config1Boot::FirmwareState_UpdateRequired);
	config.getBoot()->save();
	TEST_NUMBER_EQUAL(Config1Boot::FirmwareState_UpdateRequired, config.getBoot()->getFirmwareState());

	// init all with null planogram
	TEST_NUMBER_EQUAL(MemoryResult_Ok, repairer.repair());
	TEST_NUMBER_EQUAL(Config1Boot::FirmwareState_UpdateRequired, config.getBoot()->getFirmwareState());
#if 0
	TEST_NUMBER_EQUAL(0, config.getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(1, config.getAutomat()->getPriceListNum());
#endif

	TEST_NUMBER_EQUAL(Config2Fiscal::Kkt_None, config.getFiscal()->getKkt());
	config.getFiscal()->setKkt(Config2Fiscal::Kkt_KaznachejFA);
	config.getFiscal()->save();

	TEST_NUMBER_EQUAL(Config2Fiscal::Kkt_KaznachejFA, config.getFiscal()->getKkt());
	TEST_NUMBER_EQUAL(0, config.getEvents()->getLen());
	config.getEvents()->add(Config4Event::Type_OnlineStart);
	TEST_NUMBER_EQUAL(1, config.getEvents()->getLen());

	// reinit only automat
	Config3PriceIndexList priceListIndexes1;
	priceListIndexes1.add("CA", 0, Config3PriceIndexType_Base);
	priceListIndexes1.add("DA", 1, Config3PriceIndexType_Base);
	priceListIndexes1.add("DB", 1, Config3PriceIndexType_Base);
	Config3ProductIndexList productIndexes1;
	productIndexes1.add("01", 1);
	productIndexes1.add("02", 2);
	productIndexes1.add("03", 3);
	productIndexes1.add("04", 4);
	TEST_NUMBER_EQUAL(Evadts::Result_OK, config.resizePlanogram(&priceListIndexes1, &productIndexes1));

	TEST_NUMBER_EQUAL(Config1Boot::FirmwareState_UpdateRequired, config.getBoot()->getFirmwareState());
	TEST_NUMBER_EQUAL(Config2Fiscal::Kkt_KaznachejFA, config.getFiscal()->getKkt());
	TEST_NUMBER_EQUAL(1, config.getEvents()->getLen());
#if 0
	TEST_NUMBER_EQUAL(4, config.getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(3, config.getAutomat()->getPriceListNum());
#endif

	// reinit only automat again
	Config3PriceIndexList priceListIndexes2;
	priceListIndexes2.add("CA", 0, Config3PriceIndexType_Base);
	priceListIndexes2.add("DA", 1, Config3PriceIndexType_Base);
	Config3ProductIndexList productIndexes2;
	productIndexes2.add("01", 1);
	productIndexes2.add("02", 2);
	productIndexes2.add("03", 3);
	productIndexes2.add("04", 4);
	productIndexes2.add("05", 5);
	productIndexes2.add("06", 6);
	TEST_NUMBER_EQUAL(Evadts::Result_OK, config.resizePlanogram(&priceListIndexes2, &productIndexes2));

	TEST_NUMBER_EQUAL(Config1Boot::FirmwareState_UpdateRequired, config.getBoot()->getFirmwareState());
	TEST_NUMBER_EQUAL(Config2Fiscal::Kkt_KaznachejFA, config.getFiscal()->getKkt());
	TEST_NUMBER_EQUAL(1, config.getEvents()->getLen());
#if 0
	TEST_NUMBER_EQUAL(6, config.getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(2, config.getAutomat()->getPriceListNum());
#endif

	// check params
	Config4Modem config2(&memory, &realtime, &stat);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.load());
	TEST_NUMBER_EQUAL(Config1Boot::FirmwareState_UpdateRequired, config2.getBoot()->getFirmwareState());
	TEST_NUMBER_EQUAL(Config2Fiscal::Kkt_KaznachejFA, config2.getFiscal()->getKkt());
	TEST_NUMBER_EQUAL(1, config2.getEvents()->getLen());
#if 0
	TEST_NUMBER_EQUAL(6, config2.getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(2, config2.getAutomat()->getPriceListNum());
#endif
	return true;
}

bool Config4Test::testRegisterSale() {
	RamMemory memory(128000);
	TestRealTime realtime;
	StatStorage stat;
	Config4Modem *config1 = new Config4Modem(&memory, &realtime, &stat);
	memory.clear();

	config1->getAutomat()->shutdown();
	config1->getAutomat()->addProduct("01", 1);
	config1->getAutomat()->addProduct("02", 2);
	config1->getAutomat()->addProduct("03", 3);
	config1->getAutomat()->addProduct("04", 4);
	config1->getAutomat()->addPriceList("CA", 0, Config3PriceIndexType_Base);
	config1->getAutomat()->addPriceList("DA", 1, Config3PriceIndexType_Base);
	config1->getAutomat()->addPriceList("DB", 1, Config3PriceIndexType_Base);

	config1->init();

//	settings1->getAutomat()->setDecimalPlaces(0);
//	TEST_NUMBER_EQUAL(0, settings1->getAutomat()->getDecimalPlaces());

	Config4ProductIterator *product1 = config1->getAutomat()->createIterator();
	TEST_NUMBER_EQUAL(true, product1->findBySelectId("02"));
	product1->setName("Вкусняшка");

	TEST_NUMBER_EQUAL(true, product1->findByCashlessId(2));
	product1->setPrice("CA", 0, 50);
	product1->setPrice("DA", 1, 30);

	TEST_NUMBER_EQUAL(MemoryResult_Ok, config1->getAutomat()->sale(2, "CA", 0, 50));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config1->getAutomat()->sale(2, "DA", 1, 30));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config1->getAutomat()->sale("02", "CA", 0, 50));
	config1->getAutomat()->reset();
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config1->getAutomat()->sale("02", "CA", 0, 50));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config1->getAutomat()->sale("02", "DA", 1, 30));

	Config4Modem *config2 = new Config4Modem(&memory, &realtime, &stat);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2->load());

	Config4ProductIterator *product2 = config2->getAutomat()->createIterator();
	TEST_NUMBER_EQUAL(true, product2->findBySelectId("02"));
	TEST_STRING_EQUAL("Вкусняшка", product2->getName());
	TEST_NUMBER_EQUAL(true, product2->getEnable());
	TEST_NUMBER_EQUAL(5, product2->getTotalCount());
	TEST_NUMBER_EQUAL(210, product2->getTotalMoney());
	TEST_NUMBER_EQUAL(2, product2->getCount());
	TEST_NUMBER_EQUAL(80, product2->getMoney());

	TEST_NUMBER_EQUAL(50, product2->getPriceByIndex(0)->getPrice());
	TEST_NUMBER_EQUAL(3, product2->getPriceByIndex(0)->getTotalCount());
	TEST_NUMBER_EQUAL(150, product2->getPriceByIndex(0)->getTotalMoney());
	TEST_NUMBER_EQUAL(1, product2->getPriceByIndex(0)->getCount());
	TEST_NUMBER_EQUAL(50, product2->getPriceByIndex(0)->getMoney());

	TEST_NUMBER_EQUAL(30, product2->getPriceByIndex(1)->getPrice());
	TEST_NUMBER_EQUAL(2, product2->getPriceByIndex(1)->getTotalCount());
	TEST_NUMBER_EQUAL(60, product2->getPriceByIndex(1)->getTotalMoney());
	TEST_NUMBER_EQUAL(1, product2->getPriceByIndex(1)->getCount());
	TEST_NUMBER_EQUAL(30, product2->getPriceByIndex(1)->getMoney());

	return true;
}

bool Config4Test::testMaximumSize() {
	RamMemory memory(128000);
	TestRealTime realtime;
	Config1Boot boot;
	Config2Fiscal fiscal;
	Config4EventList events(&realtime);
	Config4Automat automat(&realtime);

	memory.fill(0xFF);

	// Empty planogram
	memory.setAddress(0);
	boot.init(&memory);
	TEST_NUMBER_EQUAL(208, memory.getAddress());
	fiscal.init(&memory);
	TEST_NUMBER_EQUAL(6665, memory.getAddress());
	events.init(100, &memory);
	TEST_NUMBER_EQUAL(17385, memory.getAddress());
	automat.init(&memory);
	TEST_NUMBER_EQUAL(17464, memory.getAddress());

	// Maximum planogram
	automat.addPriceList("CA", 0, Config3PriceIndexType_Base);
	automat.addPriceList("CA", 1, Config3PriceIndexType_Base);
//	automat.addPriceList("CA", 2, Config3PriceIndexType_Base);
//	automat.addPriceList("CA", 3, Config3PriceIndexType_Base);
	automat.addPriceList("DA", 0, Config3PriceIndexType_Base);
	automat.addPriceList("DA", 1, Config3PriceIndexType_Base);
//	automat.addPriceList("DA", 2, Config3PriceIndexType_Base);
//	automat.addPriceList("DA", 3, Config3PriceIndexType_Base);
	StringBuilder selectId;
	for(uint16_t i = 0; i < 80; i++) {
		selectId.clear();
		selectId << i;
		automat.addProduct(selectId.getString(), i);
	}
	memory.setAddress(0);
	boot.init(&memory);
	TEST_NUMBER_EQUAL(208, memory.getAddress());
	fiscal.init(&memory);
	TEST_NUMBER_EQUAL(6665, memory.getAddress());
	events.init(100, &memory);
	TEST_NUMBER_EQUAL(17385, memory.getAddress());
	automat.init(&memory);
	TEST_NUMBER_EQUAL(32063, memory.getAddress());
	return true;
}

bool Config4Test::testCompareEqual() {
	RamMemory memory(128000);
	TestRealTime realtime;
	StatStorage stat;
	Config4Modem *config1 = new Config4Modem(&memory, &realtime, &stat);
	memory.clear();

	config1->getAutomat()->shutdown();
	config1->getAutomat()->addPriceList("CA", 0, Config3PriceIndexType_Base);
	config1->getAutomat()->addPriceList("DA", 1, Config3PriceIndexType_Base);
	config1->getAutomat()->addPriceList("DB", 1, Config3PriceIndexType_Base);
	config1->getAutomat()->addProduct("01", 1);
	config1->getAutomat()->addProduct("02", 2);
	config1->getAutomat()->addProduct("03", 3);
	config1->getAutomat()->addProduct("04", 4);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config1->init());

	Config3PriceIndexList priceIndexes1;
	priceIndexes1.add("CA", 0, Config3PriceIndexType_Base);
	priceIndexes1.add("DA", 1, Config3PriceIndexType_Base);
	priceIndexes1.add("DB", 1, Config3PriceIndexType_Base);
	Config3ProductIndexList productIndexes1;
	productIndexes1.add("01", 1);
	productIndexes1.add("02", 2);
	productIndexes1.add("03", 3);
	productIndexes1.add("04", 4);

	TEST_NUMBER_EQUAL(true, config1->asyncComparePlanogramStart(&priceIndexes1, &productIndexes1));
	TEST_NUMBER_EQUAL(Evadts::Result_Busy, config1->asyncComparePlanogramResult());
	TEST_NUMBER_EQUAL(true, config1->asyncComparePlanogramProc());
	TEST_NUMBER_EQUAL(Evadts::Result_Busy, config1->asyncComparePlanogramResult());
	TEST_NUMBER_EQUAL(true, config1->asyncComparePlanogramProc());
	TEST_NUMBER_EQUAL(Evadts::Result_Busy, config1->asyncComparePlanogramResult());
	TEST_NUMBER_EQUAL(true, config1->asyncComparePlanogramProc());
	TEST_NUMBER_EQUAL(Evadts::Result_Busy, config1->asyncComparePlanogramResult());
	TEST_NUMBER_EQUAL(false, config1->asyncComparePlanogramProc());
	TEST_NUMBER_EQUAL(Evadts::Result_OK, config1->asyncComparePlanogramResult());
	TEST_NUMBER_EQUAL(false, config1->asyncComparePlanogramProc());
	TEST_NUMBER_EQUAL(Evadts::Result_OK, config1->asyncComparePlanogramResult());
	TEST_NUMBER_EQUAL(Evadts::Result_OK, config1->comparePlanogram(&priceIndexes1, &productIndexes1));
	return true;
}

bool Config4Test::testCompareNotEqual() {
	RamMemory memory(128000);
	TestRealTime realtime;
	StatStorage stat;
	Config4Modem *config1 = new Config4Modem(&memory, &realtime, &stat);
	memory.clear();

	config1->getAutomat()->shutdown();
	config1->getAutomat()->addPriceList("CA", 0, Config3PriceIndexType_Base);
	config1->getAutomat()->addPriceList("DA", 1, Config3PriceIndexType_Base);
	config1->getAutomat()->addPriceList("DB", 1, Config3PriceIndexType_Base);
	config1->getAutomat()->addProduct("01", 1);
	config1->getAutomat()->addProduct("02", 2);
	config1->getAutomat()->addProduct("03", 3);
	config1->getAutomat()->addProduct("04", 4);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config1->init());

	// less prices
	Config3PriceIndexList priceIndexes1;
	priceIndexes1.add("CA", 0, Config3PriceIndexType_Base);
	priceIndexes1.add("DA", 1, Config3PriceIndexType_Base);
	Config3ProductIndexList productIndexes1;
	productIndexes1.add("01", 1);
	productIndexes1.add("02", 2);
	productIndexes1.add("03", 3);
	productIndexes1.add("04", 4);

	TEST_NUMBER_EQUAL(false, config1->asyncComparePlanogramStart(&priceIndexes1, &productIndexes1));
	TEST_NUMBER_EQUAL(Evadts::Result_PriceListNumberNotEqual, config1->asyncComparePlanogramResult());
	TEST_NUMBER_EQUAL(Evadts::Result_PriceListNumberNotEqual, config1->comparePlanogram(&priceIndexes1, &productIndexes1));

	// price id not equal
	Config3PriceIndexList priceIndexes2;
	priceIndexes2.add("CA", 0, Config3PriceIndexType_Base);
	priceIndexes2.add("CA", 1, Config3PriceIndexType_Base);
	priceIndexes2.add("DA", 1, Config3PriceIndexType_Base);
	Config3ProductIndexList productIndexes2;
	productIndexes2.add("01", 1);
	productIndexes2.add("02", 2);
	productIndexes2.add("03", 3);
	productIndexes2.add("04", 4);

	TEST_NUMBER_EQUAL(false, config1->asyncComparePlanogramStart(&priceIndexes2, &productIndexes2));
	TEST_NUMBER_EQUAL(Evadts::Result_PriceListNotFound, config1->asyncComparePlanogramResult());
	TEST_NUMBER_EQUAL(false, config1->asyncComparePlanogramProc());
	TEST_NUMBER_EQUAL(Evadts::Result_PriceListNotFound, config1->asyncComparePlanogramResult());
	TEST_NUMBER_EQUAL(Evadts::Result_PriceListNotFound, config1->comparePlanogram(&priceIndexes2, &productIndexes2));

	// less products
	Config3PriceIndexList priceIndexes3;
	priceIndexes3.add("CA", 0, Config3PriceIndexType_Base);
	priceIndexes3.add("DA", 1, Config3PriceIndexType_Base);
	priceIndexes3.add("DB", 1, Config3PriceIndexType_Base);
	Config3ProductIndexList productIndexes3;
	productIndexes3.add("01", 1);
	productIndexes3.add("02", 2);
	productIndexes3.add("04", 4);

	TEST_NUMBER_EQUAL(false, config1->asyncComparePlanogramStart(&priceIndexes3, &productIndexes3));
	TEST_NUMBER_EQUAL(Evadts::Result_ProductNumberNotEqual, config1->asyncComparePlanogramResult());
	TEST_NUMBER_EQUAL(Evadts::Result_ProductNumberNotEqual, config1->comparePlanogram(&priceIndexes3, &productIndexes3));

	// product id not equal
	Config3PriceIndexList priceIndexes4;
	priceIndexes4.add("CA", 0, Config3PriceIndexType_Base);
	priceIndexes4.add("DA", 1, Config3PriceIndexType_Base);
	priceIndexes4.add("DB", 1, Config3PriceIndexType_Base);
	Config3ProductIndexList productIndexes4;
	productIndexes4.add("01", 1);
	productIndexes4.add("02", 2);
	productIndexes4.add("11", 3);
	productIndexes4.add("12", 4);

	TEST_NUMBER_EQUAL(true, config1->asyncComparePlanogramStart(&priceIndexes4, &productIndexes4));
	TEST_NUMBER_EQUAL(Evadts::Result_Busy, config1->asyncComparePlanogramResult());
	TEST_NUMBER_EQUAL(true, config1->asyncComparePlanogramProc());
	TEST_NUMBER_EQUAL(Evadts::Result_Busy, config1->asyncComparePlanogramResult());
	TEST_NUMBER_EQUAL(true, config1->asyncComparePlanogramProc());
	TEST_NUMBER_EQUAL(Evadts::Result_Busy, config1->asyncComparePlanogramResult());
	TEST_NUMBER_EQUAL(false, config1->asyncComparePlanogramProc());
	TEST_NUMBER_EQUAL(Evadts::Result_ProductNotFound, config1->asyncComparePlanogramResult());
	TEST_NUMBER_EQUAL(Evadts::Result_ProductNotFound, config1->comparePlanogram(&priceIndexes4, &productIndexes4));
	return true;
}
