#include "config/v4/automat/Config4Automat.h"
#include "memory/include/RamMemory.h"
#include "timer/include/TestRealTime.h"
#include "utils/include/Buffer.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class Config4AutomatTest : public TestSet {
public:
	Config4AutomatTest();
	bool testInit();
	bool testRepair();
	bool testRegisterSale();
	bool testRegisterCash();
	bool testCompareEqual();
	bool testCompareNotEqual();
};

TEST_SET_REGISTER(Config4AutomatTest);

Config4AutomatTest::Config4AutomatTest() {
	TEST_CASE_REGISTER(Config4AutomatTest, testInit);
	TEST_CASE_REGISTER(Config4AutomatTest, testRegisterSale);
	TEST_CASE_REGISTER(Config4AutomatTest, testRegisterCash);
#if 0
	TEST_CASE_REGISTER(Config4AutomatTest, testRepair);
	TEST_CASE_REGISTER(Config4AutomatTest, testCompareEqual);
	TEST_CASE_REGISTER(Config4AutomatTest, testCompareNotEqual);
#endif
}

bool Config4AutomatTest::testInit() {
	RamMemory memory(128000);
	TestRealTime realtime;
	memory.fill(0xFF);

	// init
	Config4Automat automat1(&realtime);
	automat1.shutdown();
	automat1.addProduct("01", 1);
	automat1.addProduct("02", 2);
	automat1.addProduct("03", 3);
	automat1.addProduct("04", 4);
	automat1.addProduct("01", 1);
	automat1.addProduct("01", 1);
	automat1.addPriceList("CA", 0, Config3PriceIndexType_Base);
	automat1.addPriceList("DA", 1, Config3PriceIndexType_Base);
	automat1.addPriceList("DB", 1, Config3PriceIndexType_Base);
	automat1.addPriceList("DB", 1, Config3PriceIndexType_Base);
	automat1.addPriceList("DB", 1, Config3PriceIndexType_Base);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat1.init(&memory));

	// load
	Config4Automat automat2(&realtime);
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat2.load(&memory));
	TEST_NUMBER_EQUAL(4, automat2.getProductNum());
	TEST_NUMBER_EQUAL(3, automat2.getPriceListNum());

	Config4ProductIterator *product = automat2.createIterator();

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

bool Config4AutomatTest::testRegisterSale() {
	RamMemory memory(128000);
	memory.clear();

	TestRealTime realtime;
	Config4Automat automat1(&realtime);
	automat1.shutdown();
	automat1.addProduct("01", 1);
	automat1.addProduct("02", 2);
	automat1.addProduct("03", 3);
	automat1.addProduct("04", 4);
	automat1.addPriceList("CA", 0, Config3PriceIndexType_Base);
	automat1.addPriceList("DA", 1, Config3PriceIndexType_Base);
	automat1.addPriceList("DB", 1, Config3PriceIndexType_Base);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat1.init(&memory));

	Config4ProductIterator *product1 = automat1.createIterator();
	TEST_NUMBER_EQUAL(true, product1->findBySelectId("02"));
	product1->setName("Вкусняшка");

	TEST_NUMBER_EQUAL(true, product1->findByCashlessId(2));
	product1->setPrice("CA", 0, 50);
	product1->setPrice("DA", 1, 30);

	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat1.sale(2, "CA", 0, 50));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat1.sale(2, "DA", 1, 30));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat1.sale("02", "CA", 0, 50));
	automat1.reset();
	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat1.sale("02", "CA", 0, 50));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat1.sale("02", "DA", 1, 30));

	Config4Automat automat2(&realtime);
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat2.load(&memory));

	Config4ProductIterator *product2 = automat2.createIterator();
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

	Config4ProductListStatStruct *data = &automat2.getProductList()->getStat()->data;
	//TA2 token
	TEST_NUMBER_EQUAL(0, data->vend_token_num_reset);
	TEST_NUMBER_EQUAL(0, data->vend_token_val_reset);
	TEST_NUMBER_EQUAL(0, data->vend_token_num_total);
	TEST_NUMBER_EQUAL(0, data->vend_token_val_total);
	//CA2 cash sale
	TEST_NUMBER_EQUAL(1, data->vend_cash_num_reset);
	TEST_NUMBER_EQUAL(50, data->vend_cash_val_reset);
	TEST_NUMBER_EQUAL(3, data->vend_cash_num_total);
	TEST_NUMBER_EQUAL(150, data->vend_cash_val_total);
	//DA2 cashless1
	TEST_NUMBER_EQUAL(1, data->vend_cashless1_num_reset);
	TEST_NUMBER_EQUAL(30, data->vend_cashless1_val_reset);
	TEST_NUMBER_EQUAL(2, data->vend_cashless1_num_total);
	TEST_NUMBER_EQUAL(60, data->vend_cashless1_val_total);
	//DB2 cashless1
	TEST_NUMBER_EQUAL(0, data->vend_cashless2_num_reset);
	TEST_NUMBER_EQUAL(0, data->vend_cashless2_val_reset);
	TEST_NUMBER_EQUAL(0, data->vend_cashless2_num_total);
	TEST_NUMBER_EQUAL(0, data->vend_cashless2_val_total);
	return true;
}

bool Config4AutomatTest::testRegisterCash() {
	RamMemory memory(128000);
	memory.clear();

	TestRealTime realtime;
	Config4Automat automat1(&realtime);
	automat1.shutdown();
	automat1.addProduct("01", 1);
	automat1.addProduct("02", 2);
	automat1.addProduct("03", 3);
	automat1.addProduct("04", 4);
	automat1.addPriceList("CA", 0, Config3PriceIndexType_Base);
	automat1.addPriceList("DA", 1, Config3PriceIndexType_Base);
	automat1.addPriceList("DB", 1, Config3PriceIndexType_Base);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat1.init(&memory));

	Config4PaymentStat *stat1 = automat1.getPaymentStat();
	TEST_NUMBER_EQUAL(MemoryResult_Ok, stat1->registerCoinIn(100, MdbMasterCoinChanger::Route_Cashbox));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, stat1->registerCoinIn(1000, MdbMasterCoinChanger::Route_Tube));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, stat1->registerCoinDispense(7500));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, stat1->registerCoinFill(3500));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, stat1->registerBillIn(10000));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, stat1->registerBillIn(5000));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, stat1->registerCashOverpay(6500));
	automat1.reset();
	TEST_NUMBER_EQUAL(MemoryResult_Ok, stat1->registerCoinIn(1000, MdbMasterCoinChanger::Route_Cashbox));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, stat1->registerCoinIn(500, MdbMasterCoinChanger::Route_Tube));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, stat1->registerCoinDispense(1500));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, stat1->registerCoinFill(4500));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, stat1->registerBillIn(20000));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, stat1->registerCashOverpay(8500));

	Config4Automat automat2(&realtime);
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, automat2.load(&memory));

	Config4PaymentStatStruct *stat2 = &automat2.getPaymentStat()->data;
	//CA3 cash flow
	TEST_NUMBER_EQUAL(21500, stat2->cash_in_reset);
	TEST_NUMBER_EQUAL(37600, stat2->cash_in_total);
	TEST_NUMBER_EQUAL(1000, stat2->coin_in_cashbox_reset);
	TEST_NUMBER_EQUAL(1100, stat2->coin_in_cashbox_total);
	TEST_NUMBER_EQUAL(500, stat2->coin_in_tubes_reset);
	TEST_NUMBER_EQUAL(1500, stat2->coin_in_tubes_total);
	TEST_NUMBER_EQUAL(20000, stat2->bill_in_reset);
	TEST_NUMBER_EQUAL(35000, stat2->bill_in_total);
	//CA4 cash out
	TEST_NUMBER_EQUAL(1500, stat2->coin_dispense_reset);
	TEST_NUMBER_EQUAL(9000, stat2->coin_dispense_total);
	TEST_NUMBER_EQUAL(0, stat2->coin_manually_dispense_reset);
	TEST_NUMBER_EQUAL(0, stat2->coin_manually_dispense_total);
	//CA8 cash overpay
	TEST_NUMBER_EQUAL(8500, stat2->cash_overpay_reset);
	TEST_NUMBER_EQUAL(15000, stat2->cash_overpay_total);
	//CA10
	TEST_NUMBER_EQUAL(4500, stat2->coin_filled_reset);
	TEST_NUMBER_EQUAL(8000, stat2->coin_filled_total);
	return true;
}
#if 0
bool Config4AutomatTest::testRepair() {
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
	TEST_NUMBER_EQUAL(0, config.getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(1, config.getAutomat()->getPriceListNum());

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
	TEST_NUMBER_EQUAL(4, config.getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(3, config.getAutomat()->getPriceListNum());

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
	TEST_NUMBER_EQUAL(6, config.getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(2, config.getAutomat()->getPriceListNum());

	// check params
	Config4Modem config2(&memory, &realtime, &stat);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.load());
	TEST_NUMBER_EQUAL(Config1Boot::FirmwareState_UpdateRequired, config2.getBoot()->getFirmwareState());
	TEST_NUMBER_EQUAL(Config2Fiscal::Kkt_KaznachejFA, config2.getFiscal()->getKkt());
	TEST_NUMBER_EQUAL(1, config2.getEvents()->getLen());
	TEST_NUMBER_EQUAL(6, config2.getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(2, config2.getAutomat()->getPriceListNum());
	return true;
}

bool Config4AutomatTest::testCompareEqual() {
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

bool Config4AutomatTest::testCompareNotEqual() {
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
#endif
