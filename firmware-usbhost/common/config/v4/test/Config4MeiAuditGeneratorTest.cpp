#include "config/v4/Config4MeiAuditGenerator.h"
#include "config/v4/Config4AuditIniter.h"
#include "config/v4/Config4AuditParser.h"
#include "timer/include/TestRealTime.h"
#include "test/include/Test.h"
#include "memory/include/RamMemory.h"
#include "logger/include/Logger.h"

class Config4MeiAuditGeneratorTest : public TestSet {
public:
	Config4MeiAuditGeneratorTest();
	bool test();
};

TEST_SET_REGISTER(Config4MeiAuditGeneratorTest);

Config4MeiAuditGeneratorTest::Config4MeiAuditGeneratorTest() {
	TEST_CASE_REGISTER(Config4MeiAuditGeneratorTest, test);
}

bool Config4MeiAuditGeneratorTest::test() {
	RamMemory memory(128000);
	memory.clear();
	TestRealTime realtime;
	StatStorage stat;
	Config4Modem config(&memory, &realtime, &stat);
	Config4AuditIniter initer(&config);
	Config4AuditParser parser(&config);
	Config4MeiAuditGenerator generator(&config);

	MdbCoinChangerContext *context = config.getAutomat()->getCCContext();
	context->init(1, 10);
	context->setInTubeValue(12550);
	context->get(0)->setNominal(10);
	context->get(0)->setInTube(true);
	context->get(0)->setNumber(67);
	context->get(0)->setFullTube(true);
	context->get(1)->setNominal(20);
	context->get(1)->setInTube(true);
	context->get(1)->setNumber(54);
	context->get(1)->setFullTube(false);
	context->get(2)->setNominal(50);
	context->get(2)->setInTube(true);
	context->get(2)->setNumber(114);
	context->get(2)->setFullTube(true);
	context->get(3)->setNominal(100);
	context->get(3)->setInTube(true);
	context->get(3)->setNumber(51);
	context->get(3)->setFullTube(false);

	const char data1[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"ID1******0*6*1\r\n"
"CB1***G250_F02**0\r\n"
"ID4*2*1643*\r\n"
"ID5*20160113*143005\r\n"
"VA1*5186480*21283*5186480*21283\r\n"
"VA2*14950*67*14950*67\r\n"
"VA3*5240*23*5240*23\r\n"
"CA1*JOF050287      *J2000MDB P32*9009\r\n"
"CA15*218100\r\n"
"CA17*0*100*96*0*0*1\r\n"
"CA17*2*500*153*0*0*0\r\n"
"CA17*3*1000*132*0*0*1\r\n"
"CA2*5186480*21283*5186480*21283\r\n"
"CA3*7754240*1502250*2556690**7754240*1502250*2556690**3695300*3695300\r\n"
"CA4*2544130*7100*2544130*7100\r\n"
"CA8*100*100\r\n"
"CA10*30630*30630\r\n"
"BA1*JOF61158       *BT10MDB     *0049\r\n"
"DA2*0*0*0*0\r\n"
"DA4*0*0\r\n"
"DA5*0**0\r\n"
"DB2*0*0*0*0\r\n"
"DB4*0*0\r\n"
"DB5*0**0\r\n"
"TA2*0*0*0*0\r\n"
"PA1*1*250*0\r\n"
"PA3*22**22\r\n"
"PA4*5**5\r\n"
"PA7*1*CA*0*4000*0*114399*983*114399\r\n"
"PA7*1*TA*0*4000*0*4294967295*0*4294967295\r\n"
"PA7*1*DA*1*4000*0*4294967295*0*4294967295\r\n"
"PA7*1*DB*1*4000*0*4294967295*0*4294967295\r\n"
"PA1*2*270*0\r\n"
"PA3*0**0\r\n"
"PA4*2**2\r\n"
"PA7*2*CA*0*4000*1*130879*1023*130879\r\n"
"PA7*2*TA*0*4000*0*4294967295*0*4294967295\r\n"
"PA7*2*DA*1*4000*0*4294967295*0*4294967295\r\n"
"PA7*2*DB*1*4000*0*4294967295*0*4294967295\r\n"
"PA1*3*250*0\r\n"
"PA3*3**3\r\n"
"PA4*1**1\r\n"
"PA7*3*CA*0*4000*0*280299*913*280299\r\n"
"PA7*3*TA*0*4000*0*4294967295*0*4294967295\r\n"
"PA7*3*DA*1*4000*0*4294967295*0*4294967295\r\n"
"PA7*3*DB*1*4000*0*4294967295*0*4294967295\r\n"
"PA1*4*250*0\r\n"
"PA3*0**0\r\n"
"PA4*2**2\r\n"
"PA7*4*CA*0*3000*0*41649*1166*41649\r\n"
"PA7*4*TA*0*3000*0*4294967295*0*4294967295\r\n"
"PA7*4*DA*1*3000*0*4294967295*0*4294967295\r\n"
"PA7*4*DB*1*3000*0*4294967295*0*4294967295\r\n"
"EA3**20160113*143005******1*0\r\n"
"MA5*ERROR*0*6\r\n"
"G85*A4CB\r\n"
"SE*136*0001\r\n"
"DXE*1*1\r\n";
	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());

	parser.start();
	parser.procData((const uint8_t*)data1, sizeof(data1));
	parser.complete();
	TEST_NUMBER_EQUAL(false, parser.hasError());

	config.getAutomat()->sale("04", "CA", 0, 3000);
	config.getAutomat()->sale("04", "CA", 0, 3000);
	config.getAutomat()->sale("04", "CA", 0, 3000);

	Config4PaymentStat *paymentStat = config.getAutomat()->getPaymentStat();
//CA3*32000*6900*10100*150*161183300*7617700*55528600*980370*15000*98037000
	paymentStat->data.cash_in_reset = 32000; // coins + bills
	paymentStat->data.coin_in_cashbox_reset = 6900;
	paymentStat->data.coin_in_tubes_reset = 10100;
	paymentStat->data.bill_in_reset = 15000;
	paymentStat->data.cash_in_total = 161183300; // coins + bills
	paymentStat->data.coin_in_cashbox_total = 7617700;
	paymentStat->data.coin_in_tubes_total = 55528600;
	paymentStat->data.bill_in_total = 98037000;
//CA4*8000*0*55307200*216500
	paymentStat->data.coin_dispense_reset = 8000;
	paymentStat->data.coin_manually_dispense_reset = 0;
	paymentStat->data.coin_dispense_total = 55307200;
	paymentStat->data.coin_manually_dispense_total = 216500;
//CA10*0*8872300
	paymentStat->data.coin_filled_reset = 0;
	paymentStat->data.coin_filled_total = 8872300;

	//CA2*97124812*27553*24000*8
	//VA1*97124812*27553*24000*8
	Config4ProductListStat *saleStat = config.getAutomat()->getProductList()->getStat();
	saleStat->data.vend_cash_num_total = 27553;
	saleStat->data.vend_cash_val_total = 97124812;
	saleStat->data.vend_cash_num_reset = 8;
	saleStat->data.vend_cash_val_reset = 24000;

	Config4ProductList *productList = config.getAutomat()->getProductList();
	Config3Product product1;
	Config3Price price1;
	productList->getByIndex(0, &product1);
	product1.data.totalCount = 983;
	product1.data.totalMoney = 3595500;
	product1.data.freeCount = 0;
	product1.data.freeTotalCount = 0;
	product1.save();
	product1.prices.getByIndex(0, &price1);
	price1.data.price = 250;
	price1.data.count = 0;
	price1.data.totalCount = 983;
	price1.save();

	productList->getByIndex(1, &product1);
	product1.data.count = 1;
	product1.data.money = 4000;
	product1.data.totalCount = 1023;
	product1.data.totalMoney = 4178000;
	product1.data.freeCount = 0;
	product1.data.freeTotalCount = 0;
	product1.save();
	product1.prices.getByIndex(0, &price1);
	price1.data.price = 300;
	price1.data.count = 1;
	price1.data.totalCount = 1023;
	price1.save();

	productList->getByIndex(2, &product1);
	product1.data.totalCount = 913;
	product1.data.totalMoney = 3402000;
	product1.data.freeCount = 0;
	product1.data.freeTotalCount = 0;
	product1.save();
	product1.prices.getByIndex(0, &price1);
	price1.data.price = 400;
	price1.data.count = 0;
	price1.data.totalCount = 913;
	price1.save();

	productList->getByIndex(3, &product1);
	product1.data.totalCount = 1166;
	product1.data.totalMoney = 3535000;
	product1.data.freeCount = 0;
	product1.data.freeTotalCount = 0;
	product1.save();
	product1.prices.getByIndex(0, &price1);
	price1.data.price = 300;
	price1.data.count = 0;
	price1.data.totalCount = 1166;
	price1.save();

	context->init(2, 10);
	context->setInTubeValue(218100);
	context->get(0)->setNominal(100);
	context->get(0)->setInTube(true);
	context->get(0)->setNumber(96);
	context->get(0)->setFullTube(true);
	context->get(2)->setNominal(500);
	context->get(2)->setInTube(true);
	context->get(2)->setNumber(153);
	context->get(2)->setFullTube(false);
	context->get(3)->setNominal(1000);
	context->get(3)->setInTube(true);
	context->get(3)->setNumber(132);
	context->get(3)->setFullTube(true);

	Buffer buf(10000);
	generator.reset();
	buf.add(generator.getData(), generator.getLen());
	for(uint16_t i = 0; i < 1000; i++) {
		if(generator.isLast() == true) { break; }
		generator.next();
		buf.add(generator.getData(), generator.getLen());
	}
	const char expected1[] =
#if 0
"DXS*9252131001*VA*V1/6*1\r\n"
"ST*001*0001\r\n"
"CA1*3029YE04086*CF7900EXEC*123*372935\r\n"
"CA2*97124812*27553*24000*8\r\n"
"CA3*32000*6900*10100*150*161183300*7617700*55528600*980370*15000*98037000\r\n"
"CA4*8000*0*55307200*216500\r\n"
"CA8*0*0\r\n"
"CA9*0*0\r\n"
"CA10*0*8872300\r\n"
"CA11*100*4*3*1*9438*7897*1541\r\n"
"CA11*200*3*3*0*6495*6495*0\r\n"
"CA11*500*2*0*2*15501*4048*11453\r\n"
"CA11*1000*0*0*0*48*48*0\r\n"
"CA11*1000*15*6*9*53105*3457*49648\r\n"
"CA13*100*0*278\r\n"
"CA13*500*0*1451\r\n"
"CA13*1000*0*8119\r\n"
"CA14*1000*0*0*277*267\r\n"
"CA14*5000*1*1*4845*4794\r\n"
"CA14*10000*1*1*11439*7380\r\n"
"CA15*218100\r\n"
"CA17*0*100*96*0*0*1\r\n"
"CA17*2*500*153*0*0*0\r\n"
"CA17*3*1000*132*0*0*1\r\n"
"EA3*1734*000000*0000*DEXTERM*000000*0000*DEXTERM***1734\r\n"
"EA4*000000*0000*\r\n"
"EA5*000000*0000***0\r\n"
"EA6*000000*0000*\r\n"
"EA7*0*2304\r\n"
"ID1*00000805***10811882**0***\r\n"
"ID4*2*1643\r\n"
"LA1*0*1*4000*0*983\r\n"
"LA1*0*2*4000*1*1023\r\n"
"LA1*0*3*4000*0*913\r\n"
"LA1*0*4*3000*0*1166\r\n"
"PA1*1*4000\r\n"
"PA2*983*3595500*0*0\r\n"
"PA4*0*0*0*0\r\n"
"PA1*2*4000\r\n"
"PA2*1023*4178000*1*4000\r\n"
"PA4*0*0*0*0\r\n"
"PA1*3*4000\r\n"
"PA2*913*3402000*0*0\r\n"
"PA4*0*0*0*0\r\n"
"PA1*4*3000\r\n"
"PA2*1166*3535000*0*0\r\n"
"PA4*0*0*0*0\r\n"
"TA2*0*0*0*0\r\n"
"TA3*0*0\r\n"
"TA4*0*0*0*0*0*0*0*0\r\n"
"TA5*0*0\r\n"
"TA6*0*0*0*0\r\n"
"VA1*97124812*27553*24000*8\r\n"
"VA3*0*0*0*0\r\n"
"AM1*3029YE04086*CF7900EXEC*V1.23\r\n"
"MA5*1*STATUS*CM0300\r\n"
"MA5*2*CAS*-1245184*-626304716\r\n"
"MA5*3*FIL*0*1*21992*500*77*77*100*96*96*1000*66*66*1000*66*66*500*77*76\r\n"
"MA5*LAST*CBS*0*0.00*39850256*8733*31536000*4562*0*0*0*48180\r\n"
"G85*8C18\r\n"
"SE*1*0001\r\n"
"DXE*1*1\r\n";
#else
//DXS01=
//ID104=
//DA201
//DA401
//DA301
//TA701
//TA705
//TA302
//TA202
//TA201
//VA101
"DXS*9252131001*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
//"CA1*3029YE04086*CF7900EXEC*123*372935\r\n"
"CA2*97124812*27553*24000*8\r\n"
"CA3*32000*6900*10100*150*161183300*7617700*55528600*980370*15000*98037000\r\n"
"CA4*8000*0*55307200*216500\r\n"
"CA8*0*0\r\n"
"CA9*0*0\r\n"
"CA10*0*8872300\r\n"
//"CA11*100*4*3*1*9438*7897*1541\r\n"
//"CA11*200*3*3*0*6495*6495*0\r\n"
//"CA11*500*2*0*2*15501*4048*11453\r\n"
//"CA11*1000*0*0*0*48*48*0\r\n"
//"CA11*1000*15*6*9*53105*3457*49648\r\n"
//"CA13*100*0*278\r\n"
//"CA13*500*0*1451\r\n"
//"CA13*1000*0*8119\r\n"
//"CA14*1000*0*0*277*267\r\n"
//"CA14*5000*1*1*4845*4794\r\n"
//"CA14*10000*1*1*11439*7380\r\n"
"CA15*218100\r\n"
"CA17*0*100*96*0*0*1\r\n"
"CA17*2*500*153*0*0*0\r\n"
"CA17*3*1000*132*0*0*1\r\n"
//"ID1*00000805***10811882**0***\r\n"
"ID1******0\r\n"
"ID4*2*1643\r\n"
"LA1*0*1*250*0*983\r\n"
"LA1*0*2*300*1*1023\r\n"
"LA1*0*3*400*0*913\r\n"
"LA1*0*4*300*0*1166\r\n"
"LA1*1*1*4000*0*0\r\n"
"LA1*1*2*4000*0*0\r\n"
"LA1*1*3*4000*0*0\r\n"
"LA1*1*4*3000*0*0\r\n"
"PA1*1*250\r\n"
"PA2*983*3595500*0*0\r\n"
"PA4*0*0*0*0\r\n"
"PA1*2*300\r\n"
"PA2*1023*4178000*1*4000\r\n"
"PA4*0*0*0*0\r\n"
"PA1*3*400\r\n"
"PA2*913*3402000*0*0\r\n"
"PA4*0*0*0*0\r\n"
"PA1*4*300\r\n"
"PA2*1166*3535000*0*0\r\n"
"PA4*0*0*0*0\r\n"
//"TA2*0*0*0*0\r\n"
//"TA3*0*0\r\n"
//"TA4*0*0*0*0*0*0*0*0\r\n"
//"TA5*0*0\r\n"
//"TA6*0*0*0*0\r\n"
"VA1*97124812*27553*24000*8\r\n"
"VA3*0*0*0*0\r\n"
//"AM1*3029YE04086*CF7900EXEC*V1.23\r\n"
"G85*21B3\r\n"
"SE*37*0001\r\n"
"DXE*1*1\r\n";
#endif
	TEST_SUBSTR_EQUAL(expected1, (const char *)buf.getData(), buf.getLen());
	return true;
}
