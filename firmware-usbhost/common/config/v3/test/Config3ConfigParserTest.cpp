#include "memory/include/RamMemory.h"
#include "config/v3/Config3AuditIniter.h"
#include "config/v3/Config3ConfigIniter.h"
#include "config/v3/Config3ConfigParser.h"
#include "config/v3/Config3Modem.h"
#include "timer/include/TestRealTime.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class Config3ParserTest : public TestSet {
public:
	Config3ParserTest();
	bool testMC5();
	bool testPriceListTime();
	bool testFixedDecimalPoint();
	bool testOrangeData();
	bool testFasCashlessIdError();
	bool testFiscalRegisterDisabled();
};

TEST_SET_REGISTER(Config3ParserTest);

Config3ParserTest::Config3ParserTest() {
	TEST_CASE_REGISTER(Config3ParserTest, testMC5);
	TEST_CASE_REGISTER(Config3ParserTest, testPriceListTime);
	TEST_CASE_REGISTER(Config3ParserTest, testFixedDecimalPoint);
	TEST_CASE_REGISTER(Config3ParserTest, testOrangeData);
	TEST_CASE_REGISTER(Config3ParserTest, testFasCashlessIdError);
	TEST_CASE_REGISTER(Config3ParserTest, testFiscalRegisterDisabled);
}

bool Config3ParserTest::testMC5() {
	RamMemory memory(64000);
	memory.clear();
	TestRealTime realtime;
	StatStorage stat;
	Config3Modem config1(&memory, &realtime, &stat);
	Config3ConfigIniter initer;
	Config3ConfigParser parser(&config1);

	const char data1[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"IC1******0*6*1\r\n"
"IC4*1*1810**1*4000*2*1*1*75*1*1*1*1*1*1*3*3\r\n"
"AC2*****3\r\n"
"LC2*CA*1*31*17:00:00*7200\r\n"
"LC2*DA*1*31*12:00:00*7200\r\n"
"PC1*01*250*0\r\n"
"PC7*01*CA*0*250\r\n"
"PC7*01*CA*1*150\r\n"
"PC7*01*DA*0*200\r\n"
"PC7*01*DA*1*100\r\n"
"PC9*01*1*3\r\n"
"PC1*02*270*0\r\n"
"PC7*02*CA*0*270\r\n"
"PC7*02*CA*1*170\r\n"
"PC7*02*DA*0*220\r\n"
"PC7*02*DA*1*120\r\n"
"PC9*02*2*3\r\n"
"PC1*03*250*0\r\n"
"PC7*03*CA*0*250\r\n"
"PC7*03*CA*1*150\r\n"
"PC7*03*DA*0*200\r\n"
"PC7*03*DA*1*150\r\n"
"PC9*03*3*3\r\n"
"PC1*04*250*0\r\n"
"PC7*04*CA*0*250\r\n"
"PC7*04*CA*1*150\r\n"
"PC7*04*DA*0*200\r\n"
"PC7*04*DA*1*150\r\n"
"PC9*04*4*3\r\n"
//"MC5*00*PAYMENT*3*1*4000\r\n"
//"MC5*00*MDBSCC*1*\r\n"
//"MC5*00*FISCAL*1*\r\n"
"MC5*0*INTERNET*128\r\n"
"MC5*1*EXT1*2\r\n"
"MC5*2*EXT2*5\r\n"
"MC5*3*USB1*4\r\n"
"MC5*4*QRTYPE*12\r\n"
"MC5*5*ETH1MAC*2086846a9601\r\n"
"MC5*6*ETH1ADDR*192.168.2.128\r\n"
"MC5*7*ETH1MASK*255.255.255.0\r\n"
"MC5*8*ETH1GW*192.168.2.1\r\n"
"MC5*9*FIDTYPE*1\r\n"
"MC5*10*FIDADDR*95.216.78.99*1883\r\n"
"MC5*11*FIDDEVICE*ephor1\r\n"
"MC5*12*FIDAUTH*ephor*2kDR8TMCu5dhiN2\r\n"
"MC5*13*BV*0\r\n"
"G85*A4CB\r\n"
"SE*136*0001\r\n"
"DXE*1*1\r\n";

	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());
	TEST_NUMBER_EQUAL(Evadts::Result_OK, config1.resizePlanogram(initer.getPrices(), initer.getProducts()));

	parser.start();
	parser.procData((const uint8_t*)data1, sizeof(data1));
	parser.complete();
	TEST_NUMBER_EQUAL(false, parser.hasError());

	Config3Modem config2(&memory, &realtime, &stat);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.load());

	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getTotalCount());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getTotalMoney());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getCount());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getMoney());
	TEST_NUMBER_EQUAL(4, config2.getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(4, config2.getAutomat()->getPriceListNum());

	// check main info
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getAutomatId());
	TEST_NUMBER_EQUAL(1, config2.getAutomat()->getTaxSystem());

	// check device info
	TEST_NUMBER_EQUAL(Config3Automat::InternetDevice_None, config2.getAutomat()->getInternetDevice());
	TEST_NUMBER_EQUAL(Config3Automat::Device_Inpas, config2.getAutomat()->getExt1Device());
	TEST_NUMBER_EQUAL(Config3Automat::Device_Screen, config2.getAutomat()->getExt2Device());
	TEST_NUMBER_EQUAL(Config3Automat::Device_Vendotek, config2.getAutomat()->getUsb1Device());
	TEST_NUMBER_EQUAL((Config3Automat::QrType_NefteMag|Config3Automat::QrType_EphorOrder), config2.getAutomat()->getQrType());
	TEST_NUMBER_EQUAL((Config3Automat::FiscalPrinter_Screen|Config3Automat::FiscalPrinter_Cashless), config2.getAutomat()->getFiscalPrinter());
	TEST_HEXDATA_EQUAL("2086846A9601", config2.getAutomat()->getEthMac(), 6);
	TEST_NUMBER_EQUAL(0x8002A8C0, config2.getAutomat()->getEthAddr());
	TEST_NUMBER_EQUAL(0x00FFFFFF, config2.getAutomat()->getEthMask());
	TEST_NUMBER_EQUAL(0x0102A8C0, config2.getAutomat()->getEthGateway());

	// check faceId
	TEST_NUMBER_EQUAL(1, config2.getAutomat()->getFidType());
	TEST_STRING_EQUAL("ephor1", config2.getAutomat()->getFidDevice());
	TEST_STRING_EQUAL("95.216.78.99", config2.getAutomat()->getFidAddr());
	TEST_NUMBER_EQUAL(1883, config2.getAutomat()->getFidPort());
	TEST_STRING_EQUAL("ephor", config2.getAutomat()->getFidUsername());
	TEST_STRING_EQUAL("2kDR8TMCu5dhiN2", config2.getAutomat()->getFidPassword());

	// check payment info
	TEST_NUMBER_EQUAL(3, config2.getAutomat()->getPaymentBus());
	TEST_NUMBER_EQUAL(true, config2.getAutomat()->getEvadts());
	TEST_NUMBER_EQUAL(1810, config2.getAutomat()->getCurrency());
	TEST_NUMBER_EQUAL(1, config2.getAutomat()->getDecimalPoint());
	TEST_NUMBER_EQUAL(4000, config2.getAutomat()->getMaxCredit());
	TEST_NUMBER_EQUAL(1, config2.getAutomat()->getCashlessNumber());
	TEST_NUMBER_EQUAL(3, config2.getAutomat()->getCashlessMaxLevel());
	TEST_NUMBER_EQUAL(75, config2.getAutomat()->getScaleFactor());
	TEST_NUMBER_EQUAL(true, config2.getAutomat()->getCategoryMoney());
	TEST_NUMBER_EQUAL(true, config2.getAutomat()->getShowChange());
	TEST_NUMBER_EQUAL(true, config2.getAutomat()->getPriceHolding());
	TEST_NUMBER_EQUAL(true, config2.getAutomat()->getCreditHolding());
	TEST_NUMBER_EQUAL(true, config2.getAutomat()->getMultiVend());
	TEST_NUMBER_EQUAL(true, config2.getAutomat()->getCashless2Click());

	// context
//	TEST_NUMBER_EQUAL(false, config2.getAutomat()->getBVContext()->getOn());

	// check price index info
	Config3PriceIndexList *priceIndexList = config2.getAutomat()->getPriceIndexList();
	Config3PriceIndex *priceIndex = priceIndexList->get(priceIndexList->getIndex("CA", 0));
	TEST_POINTER_NOT_NULL(priceIndex);
	TEST_NUMBER_EQUAL(0, priceIndex->timeTable.getWeek()->getValue());
	TEST_NUMBER_EQUAL(Config3PriceIndexType_Base, priceIndex->type);
	TimeInterval *interval = priceIndex->timeTable.getInterval();
	TEST_NUMBER_EQUAL(0, interval->getTime()->getHour());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getMinute());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getSecond());
	TEST_NUMBER_EQUAL(0, interval->getInterval());

	priceIndex = priceIndexList->get(priceIndexList->getIndex("CA", 1));
	TEST_POINTER_NOT_NULL(priceIndex);
	TEST_NUMBER_EQUAL(31, priceIndex->timeTable.getWeek()->getValue());
	TEST_NUMBER_EQUAL(Config3PriceIndexType_Time, priceIndex->type);
	interval = priceIndex->timeTable.getInterval();
	TEST_NUMBER_EQUAL(17, interval->getTime()->getHour());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getMinute());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getSecond());
	TEST_NUMBER_EQUAL(7200, interval->getInterval());

	// check product info
	Config3ProductIterator *product = config2.getAutomat()->createIterator();
	TEST_NUMBER_EQUAL(true, product->first());
	TEST_STRING_EQUAL("01", product->getId());
	TEST_STRING_EQUAL("0", product->getName());
	TEST_NUMBER_EQUAL(3, product->getTaxRate());
	TEST_NUMBER_EQUAL(0, product->getTotalCount());
	TEST_NUMBER_EQUAL(0, product->getTotalMoney());
	TEST_NUMBER_EQUAL(0, product->getCount());
	TEST_NUMBER_EQUAL(0, product->getMoney());

	// check price list info
	Config3Price *price = product->getPrice("CA", 0);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(250, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("CA", 1);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(150, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("DA", 0);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(200, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("DA", 1);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(100, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());
	return true;
}

bool Config3ParserTest::testPriceListTime() {
	RamMemory memory(64000);
	memory.clear();
	TestRealTime realtime;
	StatStorage stat;
	Config3Modem config1(&memory, &realtime, &stat);
	Config3ConfigIniter initer;
	Config3ConfigParser parser(&config1);

	const char data1[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"IC1******0*6*1\r\n"
"IC4*1*1810**1*4000*2*1*1*75*1*1*1*1*1*1*3*3\r\n"
"IC8*1*1*1"
"AC2*****3\r\n"
"LC2*CA*1*31*17:00:00*7200\r\n"
"LC2*DA*1*31*12:00:00*7200\r\n"
"PC1*01*250*0\r\n"
"PC7*01*CA*0*250\r\n"
"PC7*01*CA*1*150\r\n"
"PC7*01*DA*0*200\r\n"
"PC7*01*DA*1*100\r\n"
"PC9*01*1*3\r\n"
"PC1*02*270*0\r\n"
"PC7*02*CA*0*270\r\n"
"PC7*02*CA*1*170\r\n"
"PC7*02*DA*0*220\r\n"
"PC7*02*DA*1*120\r\n"
"PC9*02*2*3\r\n"
"PC1*03*250*0\r\n"
"PC7*03*CA*0*250\r\n"
"PC7*03*CA*1*150\r\n"
"PC7*03*DA*0*200\r\n"
"PC7*03*DA*1*150\r\n"
"PC9*03*3*3\r\n"
"PC1*04*250*0\r\n"
"PC7*04*CA*0*250\r\n"
"PC7*04*CA*1*150\r\n"
"PC7*04*DA*0*200\r\n"
"PC7*04*DA*1*150\r\n"
"PC9*04*4*3\r\n"
"G85*A4CB\r\n"
"SE*136*0001\r\n"
"DXE*1*1\r\n";
	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());
	TEST_NUMBER_EQUAL(Evadts::Result_OK, config1.resizePlanogram(initer.getPrices(), initer.getProducts()));

	parser.start();
	parser.procData((const uint8_t*)data1, sizeof(data1));
	parser.complete();
	TEST_NUMBER_EQUAL(false, parser.hasError());

	Config3Modem config2(&memory, &realtime, &stat);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.load());

	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getTotalCount());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getTotalMoney());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getCount());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getMoney());
	TEST_NUMBER_EQUAL(4, config2.getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(4, config2.getAutomat()->getPriceListNum());

	// check main info
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getAutomatId());
	TEST_NUMBER_EQUAL(1, config2.getAutomat()->getTaxSystem());

	// check device info
	TEST_NUMBER_EQUAL(Config3Automat::InternetDevice_Gsm, config2.getAutomat()->getInternetDevice());
	TEST_NUMBER_EQUAL(Config3Automat::Device_Inpas, config2.getAutomat()->getExt1Device());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getExt2Device());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getUsb1Device());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getQrType());
	TEST_NUMBER_EQUAL((Config3Automat::FiscalPrinter_Screen|Config3Automat::FiscalPrinter_Cashless), config2.getAutomat()->getFiscalPrinter());

	// check payment info
	TEST_NUMBER_EQUAL(1, config2.getAutomat()->getPaymentBus());
	TEST_NUMBER_EQUAL(true, config2.getAutomat()->getEvadts());
	TEST_NUMBER_EQUAL(1810, config2.getAutomat()->getCurrency());
	TEST_NUMBER_EQUAL(1, config2.getAutomat()->getDecimalPoint());
	TEST_NUMBER_EQUAL(4000, config2.getAutomat()->getMaxCredit());
	TEST_NUMBER_EQUAL(1, config2.getAutomat()->getCashlessNumber());
	TEST_NUMBER_EQUAL(3, config2.getAutomat()->getCashlessMaxLevel());
	TEST_NUMBER_EQUAL(75, config2.getAutomat()->getScaleFactor());
	TEST_NUMBER_EQUAL(true, config2.getAutomat()->getCategoryMoney());
	TEST_NUMBER_EQUAL(true, config2.getAutomat()->getShowChange());
	TEST_NUMBER_EQUAL(true, config2.getAutomat()->getPriceHolding());
	TEST_NUMBER_EQUAL(true, config2.getAutomat()->getCreditHolding());
	TEST_NUMBER_EQUAL(true, config2.getAutomat()->getMultiVend());
	TEST_NUMBER_EQUAL(true, config2.getAutomat()->getCashless2Click());

	// check price index info
	Config3PriceIndexList *priceIndexList = config2.getAutomat()->getPriceIndexList();
	Config3PriceIndex *priceIndex = priceIndexList->get(priceIndexList->getIndex("CA", 0));
	TEST_POINTER_NOT_NULL(priceIndex);
	TEST_NUMBER_EQUAL(0, priceIndex->timeTable.getWeek()->getValue());
	TEST_NUMBER_EQUAL(Config3PriceIndexType_Base, priceIndex->type);
	TimeInterval *interval = priceIndex->timeTable.getInterval();
	TEST_NUMBER_EQUAL(0, interval->getTime()->getHour());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getMinute());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getSecond());
	TEST_NUMBER_EQUAL(0, interval->getInterval());

	priceIndex = priceIndexList->get(priceIndexList->getIndex("CA", 1));
	TEST_POINTER_NOT_NULL(priceIndex);
	TEST_NUMBER_EQUAL(31, priceIndex->timeTable.getWeek()->getValue());
	TEST_NUMBER_EQUAL(Config3PriceIndexType_Time, priceIndex->type);
	interval = priceIndex->timeTable.getInterval();
	TEST_NUMBER_EQUAL(17, interval->getTime()->getHour());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getMinute());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getSecond());
	TEST_NUMBER_EQUAL(7200, interval->getInterval());

	// check product info
	Config3ProductIterator *product = config2.getAutomat()->createIterator();
	TEST_NUMBER_EQUAL(true, product->first());
	TEST_STRING_EQUAL("01", product->getId());
	TEST_STRING_EQUAL("0", product->getName());
	TEST_NUMBER_EQUAL(3, product->getTaxRate());
	TEST_NUMBER_EQUAL(0, product->getTotalCount());
	TEST_NUMBER_EQUAL(0, product->getTotalMoney());
	TEST_NUMBER_EQUAL(0, product->getCount());
	TEST_NUMBER_EQUAL(0, product->getMoney());

	// check price list info
	Config3Price *price = product->getPrice("CA", 0);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(250, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("CA", 1);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(150, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("DA", 0);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(200, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("DA", 1);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(100, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	return true;
}

bool Config3ParserTest::testFixedDecimalPoint() {
	RamMemory memory(64000);
	memory.clear();
	TestRealTime realtime;
	StatStorage stat;
	Config3Modem config(&memory, &realtime, &stat);
	const char data1[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"ID1******0*6*1\r\n"
"CB1***G250_F02**0\r\n"
"ID4*1*7*\r\n"
"ID5*20160113*143005\r\n"
"VA1*5186480*21283*5186480*21283\r\n"
"VA2*14950*67*14950*67\r\n"
"VA3*5240*23*5240*23\r\n"
"CA1*JOF050287      *J2000MDB P32*9009\r\n"
"CA15*12550\r\n"
"CA17*2*10*67***1\r\n"
"CA17*3*20*54***0\r\n"
"CA17*4*50*114***1\r\n"
"CA17*6*100*51***0\r\n"
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
"PA1*01*250*0\r\n"
"PA3*22**22\r\n"
"PA4*5**5\r\n"
"PA7*01*CA*0*250*878*114399*878*114399\r\n"
"PA7*01*TA*0*250*0*4294967295*0*4294967295\r\n"
"PA7*01*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*01*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA1*02*270*0\r\n"
"PA3*0**0\r\n"
"PA4*2**2\r\n"
"PA7*02*CA*0*270*1028*130879*1028*130879\r\n"
"PA7*02*TA*0*270*0*4294967295*0*4294967295\r\n"
"PA7*02*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*02*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA1*03*250*0\r\n"
"PA3*3**3\r\n"
"PA4*1**1\r\n"
"PA7*03*CA*0*250*1674*280299*1674*280299\r\n"
"PA7*03*TA*0*250*0*4294967295*0*4294967295\r\n"
"PA7*03*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*03*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA1*04*250*0\r\n"
"PA3*0**0\r\n"
"PA4*2**2\r\n"
"PA7*04*CA*0*250*383*41649*383*41649\r\n"
"PA7*04*TA*0*250*0*4294967295*0*4294967295\r\n"
"PA7*04*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*04*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA1*05*270*0\r\n"
"PA3*14**14\r\n"
"PA4*3**3\r\n"
"PA7*05*CA*0*270*2550*356069*2550*356069\r\n"
"PA7*05*TA*0*270*0*4294967295*0*4294967295\r\n"
"PA7*05*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*05*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA1*06*300*0\r\n"
"PA3*2**2\r\n"
"PA4*1**1\r\n"
"PA7*06*CA*0*300*2790*670899*2790*670899\r\n"
"PA7*06*TA*0*300*0*4294967295*0*4294967295\r\n"
"PA7*06*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*06*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA1*07*270*0\r\n"
"PA3*5**5\r\n"
"PA4*0**0\r\n"
"PA7*07*CA*0*270*1500*171119*1500*171119\r\n"
"PA7*07*TA*0*270*0*4294967295*0*4294967295\r\n"
"PA7*07*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*07*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA1*08*300*0\r\n"
"PA3*3**3\r\n"
"PA4*0**0\r\n"
"PA7*08*CA*0*300*1601*318809*1601*318809\r\n"
"PA7*08*TA*0*300*0*4294967295*0*4294967295\r\n"
"PA7*08*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*08*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA1*09*280*0\r\n"
"PA3*3**3\r\n"
"PA4*1**1\r\n"
"PA7*09*CA*0*280*1933*378249*1933*378249\r\n"
"PA7*09*TA*0*280*0*4294967295*0*4294967295\r\n"
"PA7*09*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*09*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA1*10*320*0\r\n"
"PA3*1**1\r\n"
"PA4*2**2\r\n"
"PA7*10*CA*0*320*1594*205319*1594*205319\r\n"
"PA7*10*TA*0*320*0*4294967295*0*4294967295\r\n"
"PA7*10*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*10*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA1*11*250*0\r\n"
"PA3*6**6\r\n"
"PA4*2**2\r\n"
"PA7*11*CA*0*250*1019*130859*1019*130859\r\n"
"PA7*11*TA*0*250*0*4294967295*0*4294967295\r\n"
"PA7*11*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*11*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA1*12*260*0\r\n"
"PA3*4**4\r\n"
"PA4*2**2\r\n"
"PA7*12*CA*0*260*1478*250559*1478*250559\r\n"
"PA7*12*TA*0*260*0*4294967295*0*4294967295\r\n"
"PA7*12*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*12*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA1*13*330*0\r\n"
"PA3*0**0\r\n"
"PA4*0**0\r\n"
"PA7*13*CA*0*330*1152*214349*1152*214349\r\n"
"PA7*13*TA*0*330*0*4294967295*0*4294967295\r\n"
"PA7*13*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*13*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA1*14*280*0\r\n"
"PA3*4**4\r\n"
"PA4*1**1\r\n"
"PA7*14*CA*0*280*1284*235919*1284*235919\r\n"
"PA7*14*TA*0*280*0*4294967295*0*4294967295\r\n"
"PA7*14*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*14*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA1*15*250*0\r\n"
"PA3*0**0\r\n"
"PA4*1**1\r\n"
"PA7*15*CA*0*250*419*52199*419*52199\r\n"
"PA7*15*TA*0*250*0*4294967295*0*4294967295\r\n"
"PA7*15*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*15*DB*1*100*0*4294967295*0*4294967295\r\n"
"EA3**20160113*143005******1*0\r\n"
"MA5*ERROR*0*6\r\n"
"G85*A4CB\r\n"
"SE*136*0001\r\n"
"DXE*1*1\r\n";
	Config3AuditIniter initer(&config);
	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());

	const char data2[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"IC1******0*6*1\r\n"
"IC4*2*7**1\r\n"
"PC1*01*2500*0\r\n"
"PC7*01*CA*0*2500\r\n"
"PC7*01*TA*0*2500\r\n"
"PC7*01*DA*1*1000\r\n"
"PC7*01*DB*1*1000\r\n"
"PC1*02*2700*0\r\n"
"PC7*02*CA*0*2700\r\n"
"PC7*02*TA*0*2700\r\n"
"PC7*02*DA*1*1000\r\n"
"PC7*02*DB*1*1000\r\n"
"PC1*03*2500*0\r\n"
"PC7*03*CA*0*2500\r\n"
"PC7*03*TA*0*2500\r\n"
"PC7*03*DA*1*1000\r\n"
"PC7*03*DB*1*1000\r\n"
"PC1*04*2500*0\r\n"
"PC7*04*CA*0*2500\r\n"
"PC7*04*TA*0*2500\r\n"
"PC7*04*DA*1*1000\r\n"
"PC7*04*DB*1*1000\r\n"
"PC1*05*2700*0\r\n"
"PC7*05*CA*0*2700\r\n"
"PC7*05*TA*0*2700\r\n"
"PC7*05*DA*1*1000\r\n"
"PC7*05*DB*1*1000\r\n"
"PC1*06*3000*0\r\n"
"PC7*06*CA*0*3000\r\n"
"PC7*06*TA*0*3000\r\n"
"PC7*06*DA*1*1000\r\n"
"PC7*06*DB*1*1000\r\n"
"PC1*07*2700*0\r\n"
"PC7*07*CA*0*2700\r\n"
"PC7*07*TA*0*2700\r\n"
"PC7*07*DA*1*1000\r\n"
"PC7*07*DB*1*1000\r\n"
"PC1*08*3000*0\r\n"
"PC7*08*CA*0*3000\r\n"
"PC7*08*TA*0*3000\r\n"
"PC7*08*DA*1*1000\r\n"
"PC7*08*DB*1*1000\r\n"
"PC1*09*2800*0\r\n"
"PC7*09*CA*0*2800\r\n"
"PC7*09*TA*0*2800\r\n"
"PC7*09*DA*1*1000\r\n"
"PC7*09*DB*1*1000\r\n"
"PC1*10*3200*0\r\n"
"PC7*10*CA*0*3200\r\n"
"PC7*10*TA*0*3200\r\n"
"PC7*10*DA*1*1000\r\n"
"PC7*10*DB*1*1000\r\n"
"PC1*11*2500*0\r\n"
"PC7*11*CA*0*2500\r\n"
"PC7*11*TA*0*2500\r\n"
"PC7*11*DA*1*1000\r\n"
"PC7*11*DB*1*1000\r\n"
"PC1*12*2600*0\r\n"
"PC7*12*CA*0*2600\r\n"
"PC7*12*TA*0*2600\r\n"
"PC7*12*DA*1*1000\r\n"
"PC7*12*DB*1*1000\r\n"
"PC1*13*3300*0\r\n"
"PC7*13*CA*0*3300\r\n"
"PC7*13*TA*0*3300\r\n"
"PC7*13*DA*1*1000\r\n"
"PC7*13*DB*1*1000\r\n"
"PC1*14*2800*0\r\n"
"PC7*14*CA*0*2800\r\n"
"PC7*14*TA*0*2800\r\n"
"PC7*14*DA*1*1000\r\n"
"PC7*14*DB*1*1000\r\n"
"PC1*15*2500*0\r\n"
"PC7*15*CA*0*2500\r\n"
"PC7*15*TA*0*2500\r\n"
"PC7*15*DA*1*1000\r\n"
"PC7*15*DB*1*1000\r\n"
"G85*A4CB\r\n"
"SE*136*0001\r\n"
"DXE*1*1\r\n";
	Config3ConfigParser parser(&config, true);
	parser.start();
	parser.procData((const uint8_t*)data2, sizeof(data2));
	parser.complete();
	TEST_NUMBER_EQUAL(false, parser.hasError());

	TEST_NUMBER_EQUAL(0, config.getAutomat()->getTotalCount());
	TEST_NUMBER_EQUAL(0, config.getAutomat()->getTotalMoney());
	TEST_NUMBER_EQUAL(0, config.getAutomat()->getCount());
	TEST_NUMBER_EQUAL(0, config.getAutomat()->getMoney());
	TEST_NUMBER_EQUAL(15, config.getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(4, config.getAutomat()->getPriceListNum());

	// check main info
	TEST_NUMBER_EQUAL(0, config.getAutomat()->getAutomatId());
	TEST_NUMBER_EQUAL(1643, config.getAutomat()->getCurrency());
	TEST_NUMBER_EQUAL(1, config.getAutomat()->getDecimalPoint());

	// check product info
	Config3ProductIterator *product = config.getAutomat()->createIterator();
	TEST_NUMBER_EQUAL(true, product->first());
	TEST_STRING_EQUAL("01", product->getId());
	TEST_STRING_EQUAL("0", product->getName());
	TEST_NUMBER_EQUAL(0, product->getTotalCount());
	TEST_NUMBER_EQUAL(0, product->getTotalMoney());
	TEST_NUMBER_EQUAL(0, product->getCount());
	TEST_NUMBER_EQUAL(0, product->getMoney());

	// price list info
	Config3Price *price = product->getPrice("CA", 0);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(250, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("TA", 0);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(250, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("DA", 1);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(100, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("DB", 1);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(100, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	return true;
}

bool Config3ParserTest::testOrangeData() {
	RamMemory memory(64000);
	memory.clear();
	TestRealTime realtime;
	StatStorage stat;
	Config3Modem config1(&memory, &realtime, &stat);
	Config3ConfigIniter initer;
	Config3ConfigParser parser(&config1);

	const char data1[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"IC1******0*6*1\r\n"
"IC4*1*1810**1*2***2*75*1\r\n"
"AC2*****3\r\n"
"FC1*7*2*apip.orangedata.ru*12001**\r\n"
"FC2*012345678901*0123456789*Тестовая точка*Тестовый адрес*GROUP1\r\n"
"FC3*-----BEGIN CERTIFICATE-----\r\n"
"FC3*MIIDYjCCAkoCAQAwDQYJKoZIhvcNAQELBQAwcTELMAkGA1UEBhMCUlUxDzANBgNV\r\n"
"FC3*BAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9zY293MRMwEQYDVQQKDApPcmFuZ2VkYXRh\r\n"
"FC3*MQ8wDQYDVQQLDAZOZWJ1bGExGjAYBgNVBAMMEXd3dy5vcmFuZ2VkYXRhLnJ1MB4X\r\n"
"FC3*DTE4MDMxNTE2NDYwMVoXDTI4MDMxMjE2NDYwMVowfTELMAkGA1UEBhMCUlUxDzAN\r\n"
"FC3*BgNVBAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9zY293MR8wHQYDVQQKDBZPcmFuZ2Vk\r\n"
"FC3*YXRhIHRlc3QgY2xpZW50MRMwEQYDVQQLDApFLWNvbW1lcmNlMRYwFAYDVQQDDA1v\r\n"
"FC3*cmFuZ2VkYXRhLnJ1MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAo7XZ\r\n"
"FC3*+VUUo9p+Q0zPmlt1eThA8NmVVAgNXkVDZoz3umyEnnm2d4R5Voxf4y6fuesW3Za8\r\n"
"FC3*/ImKWLbQ3/S/pHZKWiz75ElSfpnYJfMRuLAaqqs0eFfxmHbHi8Mgg9zjAMdILpR6\r\n"
"FC3*eEaP7qeCNRom3Zb6ziYoWEmDC2ZFFu9995rjkn7CtV3noWZveOCGExjM7WTkql8L\r\n"
"FC3*v1PX3ee3fXaEC7Kefxl4O/4w7agEceKRHlc0l3iwVJaKittQwAQd3ieUwoqsxzPH\r\n"
"FC3*dRwB4IU9aI6IjfqteyD51s7xd+ayM/O4j+aJ/HBhJajDHBcGWKytxv0f6YpqPUAc\r\n"
"FC3*25fRAXVa0Gsei6eY/QIDAQABMA0GCSqGSIb3DQEBCwUAA4IBAQCv/Vcxh2lMt8RV\r\n"
"FC3*Al0V9xIst0ZdjH22yTOUCOiH9PZgeagqrjTLT3ycWAdbZZUpzcFSdOmPUsgQ7Eqz\r\n"
"FC3*+TpcY5lmYFInLwJK/Afjqsb5LK2irGKT254p5qzD9rSRlM42wxRzQTA0BWX3mmhi\r\n"
"FC3*zwdrfLAvyCw1gHBbUZNf3eemBCY+8RRGPRAqD2XbyIya1bX0AHLXbx5dBe9EIOG/\r\n"
"FC3*F46WbTlrkR7kc06eiacTiGYwNdcywJ2KOcvmnXPup8Os6KOWe197CIathDHeiG2C\r\n"
"FC3*mQlsQDF/d7W4G/+l6Q66BhfRtuhp99gkT8P8j82X6ChrwbgQ5+vya3SytJ0wmIg2\r\n"
"FC3*67jOKmGK\r\n"
"FC3*-----END CERTIFICATE-----\r\n"
"FC4*-----BEGIN PRIVATE KEY-----\r\n"
"FC4*MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCjtdn5VRSj2n5D\r\n"
"FC4*TM+aW3V5OEDw2ZVUCA1eRUNmjPe6bISeebZ3hHlWjF/jLp+56xbdlrz8iYpYttDf\r\n"
"FC4*9L+kdkpaLPvkSVJ+mdgl8xG4sBqqqzR4V/GYdseLwyCD3OMAx0gulHp4Ro/up4I1\r\n"
"FC4*GibdlvrOJihYSYMLZkUW7333muOSfsK1XeehZm944IYTGMztZOSqXwu/U9fd57d9\r\n"
"FC4*doQLsp5/GXg7/jDtqARx4pEeVzSXeLBUloqK21DABB3eJ5TCiqzHM8d1HAHghT1o\r\n"
"FC4*joiN+q17IPnWzvF35rIz87iP5on8cGElqMMcFwZYrK3G/R/pimo9QBzbl9EBdVrQ\r\n"
"FC4*ax6Lp5j9AgMBAAECggEAL5qkrKT54H+bcZR3Vco8iag68g5DJvFEeeIoLDzXmGUP\r\n"
"FC4*10lLLsvdwLYG9/fJyHU86+h2QfT4vr1CVa1EwN0I19n20TYk/91ahgZ9Y7gJuREZ\r\n"
"FC4*q9jeztfTRKfT36Quej54ldrlFe5m0h3xdeGJ5auOeL2Nw8Z0ja8KbhXsCkEG5cTx\r\n"
"FC4*ZvXB0XlFoAJOp8AZvU3ZNBpmpItFlcl2aBXwRCb72DUjLkpnZf2kFDNorc1wFZ2e\r\n"
"FC4*DO/pujT6EtQ1r5qb2kUuj4GpCaHffOB/ukz3dg3bBhompTYdhax0RlZs2vNsUusm\r\n"
"FC4*6oYsUS5nWmJfnrh32Te03Fdzc2U8/XUflJzKL/0QvQKBgQDOpNQvCCxwvthZXART\r\n"
"FC4*q0fl9NY0fxlSqUpxd1BB4DYCg6Sg5kVvfwf7rdb5bbP4aNCC/9m4MgXTD0DGfEhM\r\n"
"FC4*FnYPVNKTzwLMBftBQdzDN6766j5lI49evwnh855EFAR5GyaIWh2n7tT3NUOstogp\r\n"
"FC4*kpwhzsPGH1WkEO1QLcBDyzPI3wKBgQDKz94V8au1EVKuRBR+c5gNJpF+zmUu2t2C\r\n"
"FC4*ZlPtYIuWaxMbqitmeCmNBQQZK+oLQdSUMkgMvYVpKriPk6AgnY7+1F+OOeg+ezPU\r\n"
"FC4*G+J4Vi8Yx/kZPhXoBuW745twux+q8WOBwEj2WeMy5p1F/V3qlu70HA3kbsrXdB+R\r\n"
"FC4*0bFVAxCtowKBgFTtq4M08cbYuORpDCIzGBarvMnQnuC5US43IlYgxzHbVvMGEO2V\r\n"
"FC4*IPvQY7UZ4EitE11zt9CbRoeLEk1BURlsddMxQmabQwQFRVF5tzjIjvLzCPfaWJdR\r\n"
"FC4*Hsetr5M9QuVfQkPx/ZRCdWawjoLSdj3X0rGWYCHySOloR5CXbRiv0DWzAoGAF3XW\r\n"
"FC4*Ldmn0Ckx1EDB0iLS+up0OCPt5m6g4v2tRa8+VmcKbc/Qd2j8/XgQEk1XJHg3+/CZ\r\n"
"FC4*Dwg5T4IGmW0tP7iaGvY8G3qtV9TumOGk3+CwUACJ2xaoeA+cMZDRoUe0ERUdOpwg\r\n"
"FC4*lIavVmsA1GDLpWBSQeCg5sS+KBAhur9z8O6K1lsCgYEAj7TLLE0jLNXRRfkfWzy5\r\n"
"FC4*RsJezMCQS9fjtJrLGB3BbYxqtebP2owp1qjmKMQioW5QjRxRCOyT2KrHjb31hRsp\r\n"
"FC4*Hk3Wi0OKOEuKNwmAZczbjcPH4caPZPeL6LMDtFFMsFX2BW7TnC8FcoVr2KPO/FG/\r\n"
"FC4*xs4KtXC9j5rrvBowJ0LbJ2U=\r\n"
"FC4*-----END PRIVATE KEY-----\r\n"
"FC5*-----BEGIN RSA PRIVATE KEY-----\r\n"
"FC5*MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC3ycL8S2HxRptB\r\n"
"FC5*te7yl2uje/s2pRqdXxj6D3ZiBPvPXGqQEtEddnWC6aXc/GuqM1f0C86a7xH6poo7\r\n"
"FC5*Id8lbQ9xEMvMKghRwc0DCkM78TmPpYBosi/uACNO3Kv2QkH2t8lqlqtWIk1m7dFJ\r\n"
"FC5*RgZO9XOc6Zcx/stM5MxHoc//kfVM/mfWDj4FsuYL0SGNR/Z40WrBkGo+3PJsFvqN\r\n"
"FC5*ocFFonRd0TeWHY54T384XQG0vCJg8MqxVPEh6Rs1/uX8NETL5htQ7FAtx54deu9t\r\n"
"FC5*guIZZ5w/RrsKocaP1k1jWglOErcDCtJ3jIdr1afH8ZplQ21a53UFo/2DexVf6xFX\r\n"
"FC5*3G2cj3p3AgMBAAECggEAPUfM+Aq6kZSVWAetsL3EajKAxOuwQCDhVx+ovW4j+DQ8\r\n"
"FC5*Y+WiTEyfShNV9qVD0PBltz3omch1GjpFhQn6OaRvraeIDH9HXttb3FOjr2zzYG4y\r\n"
"FC5*rrYbPSRWoYj63ZWiIP2O7zdl0caGQHezfNcYa2N0NTG99DGc3/q6EnhlvjWQsSbi\r\n"
"FC5*EjmxcPx8fmV1i4DoflMQ383nsixAFapgrROUAtCgMvhWn1kSeoojKd+e4eKZxa/S\r\n"
"FC5*NYulsBJWNFkmo1CZH4YTqlPM+IwYeDUOnOUGNxGurRZ3qQdWs2N2ZQhnrvlh+zpz\r\n"
"FC5*urD2hwAz6gQXP7mxxMR1xHtAD8XQ+w4OiJK6VWjoIQKBgQDdZJvvZrV6tvqNwuTJ\r\n"
"FC5*kDZjbVU0iKkbP61rVE/6JpyzfGeS0WzGBNiCpbK3pJZnatK2nS7i9v8gAfIqGAk8\r\n"
"FC5*1NRKLa7Qbjgw6xHEwL8VZMXzN3KsMXgGM8EziPzicCYT8VBi/kXyV0ORqRz3rMQ+\r\n"
"FC5*JOTkWRrcw943yYyTr84Dn0l0XQKBgQDUhFWJ3lKwOs7AlAAQqR1PjfpcRvSxVZ70\r\n"
"FC5*BxTwnJoIQQyPQ0/OjCc1sit5s+h8xh0MeKSilCmvZerFlgNtvsCd6geSERXbpN+k\r\n"
"FC5*9Vs3jAEkVeKHeUA/afmGqGCocanlarYu7uNRLfvpG7DduHBb4yJale/XGExNnwC0\r\n"
"FC5*N+dkUU284wKBgBaOSojQiQrQm6RXx+F1TOVCXVz102zQRwXZWDCfQHXU5eSCa7ed\r\n"
"FC5*BMYCxbuKDDzLGF68kutSyNlk+VwqiL5m3J4WG2pm4FizimLmVFGEq9pEuu0qORVA\r\n"
"FC5*rp1mhoU3cdm0S0FasJupIlwzw5zEQFYogh11qpP1bK14XlcpoS6jSuONAoGBAJqM\r\n"
"FC5*EljM4X1fhvPtrY5wLeyo56UrxM8h4RK+A7Bncm0GQUf+P4+JxQn7pDpBZ5U1zfI/\r\n"
"FC5*2hqRfS8dAvrl+WBaFGHCy/ahji/JWwrvk4J1wm7WNoMm3l4/h0MyN/jHkDJSxGKl\r\n"
"FC5*P5LNyiDgDmNvueZY66bM2zqlZPgd5bkp3pDJv6rZAoGAaP5e5F1j6s82Pm7dCpH3\r\n"
"FC5*mRZWnfZIKqoNQIq2BO8vA9/WrdFI2C27uNhxCp2ZDMulRdBZcoeHcwJjnyDzg4I4\r\n"
"FC5*gBZ2nSKkVdlN1REoTjLBBdlHi8XKiXzxvpItc2wjNC2AKHaJqj/dnh3bbTAQD1iU\r\n"
"FC5*AxPmmLJYYkhfZ2i1IrTVxZE=\r\n"
"FC5*-----END RSA PRIVATE KEY-----\r\n"
"LC2*CA*1*31*17:00:00*7200\r\n"
"LC2*DA*1*31*12:00:00*7200\r\n"
"PC1*01*250*0\r\n"
"PC7*01*CA*0*250\r\n"
"PC7*01*CA*1*150\r\n"
"PC7*01*DA*0*200\r\n"
"PC7*01*DA*1*100\r\n"
"PC9*01*1*3\r\n"
"PC1*02*270*0\r\n"
"PC7*02*CA*0*270\r\n"
"PC7*02*CA*1*170\r\n"
"PC7*02*DA*0*220\r\n"
"PC7*02*DA*1*120\r\n"
"PC9*02*2*3\r\n"
"PC1*03*250*0\r\n"
"PC7*03*CA*0*250\r\n"
"PC7*03*CA*1*150\r\n"
"PC7*03*DA*0*200\r\n"
"PC7*03*DA*1*150\r\n"
"PC9*03*3*3\r\n"
"PC1*04*250*0\r\n"
"PC7*04*CA*0*250\r\n"
"PC7*04*CA*1*150\r\n"
"PC7*04*DA*0*200\r\n"
"PC7*04*DA*1*150\r\n"
"PC9*04*4*3\r\n"
"G85*A4CB\r\n"
"SE*136*0001\r\n"
"DXE*1*1\r\n";
	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());
	TEST_NUMBER_EQUAL(Evadts::Result_OK, config1.resizePlanogram(initer.getPrices(), initer.getProducts()));

	parser.start();
	parser.procData((const uint8_t*)data1, sizeof(data1));
	parser.complete();
	TEST_NUMBER_EQUAL(false, parser.hasError());

	Config3Modem config2(&memory, &realtime, &stat);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.load());

	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getTotalCount());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getTotalMoney());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getCount());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getMoney());
	TEST_NUMBER_EQUAL(4, config2.getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(4, config2.getAutomat()->getPriceListNum());

	TEST_NUMBER_EQUAL(Config2Fiscal::Kkt_OrangeDataEphor, config2.getFiscal()->getKkt());
	TEST_NUMBER_EQUAL(2, config2.getFiscal()->getKktInterface());
	TEST_STRING_EQUAL("apip.orangedata.ru", config2.getFiscal()->getKktAddr());
	TEST_NUMBER_EQUAL(12001, config2.getFiscal()->getKktPort());
	TEST_STRING_EQUAL("", config2.getFiscal()->getOfdAddr());
	TEST_NUMBER_EQUAL(0, config2.getFiscal()->getOfdPort());
	TEST_STRING_EQUAL("012345678901", config2.getFiscal()->getINN());
	TEST_STRING_EQUAL("0123456789", config2.getFiscal()->getAutomatNumber());
	TEST_STRING_EQUAL("Тестовая точка", config2.getFiscal()->getPointName());
	TEST_STRING_EQUAL("Тестовый адрес", config2.getFiscal()->getPointAddr());
	TEST_STRING_EQUAL("GROUP1", config2.getFiscal()->getGroup());

	// check auth cert
	StringBuilder buf(2048, 2048);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.getFiscal()->getAuthPublicKey(&buf));
	TEST_STRING_EQUAL(
"-----BEGIN CERTIFICATE-----\n"
"MIIDYjCCAkoCAQAwDQYJKoZIhvcNAQELBQAwcTELMAkGA1UEBhMCUlUxDzANBgNV\n"
"BAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9zY293MRMwEQYDVQQKDApPcmFuZ2VkYXRh\n"
"MQ8wDQYDVQQLDAZOZWJ1bGExGjAYBgNVBAMMEXd3dy5vcmFuZ2VkYXRhLnJ1MB4X\n"
"DTE4MDMxNTE2NDYwMVoXDTI4MDMxMjE2NDYwMVowfTELMAkGA1UEBhMCUlUxDzAN\n"
"BgNVBAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9zY293MR8wHQYDVQQKDBZPcmFuZ2Vk\n"
"YXRhIHRlc3QgY2xpZW50MRMwEQYDVQQLDApFLWNvbW1lcmNlMRYwFAYDVQQDDA1v\n"
"cmFuZ2VkYXRhLnJ1MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAo7XZ\n"
"+VUUo9p+Q0zPmlt1eThA8NmVVAgNXkVDZoz3umyEnnm2d4R5Voxf4y6fuesW3Za8\n"
"/ImKWLbQ3/S/pHZKWiz75ElSfpnYJfMRuLAaqqs0eFfxmHbHi8Mgg9zjAMdILpR6\n"
"eEaP7qeCNRom3Zb6ziYoWEmDC2ZFFu9995rjkn7CtV3noWZveOCGExjM7WTkql8L\n"
"v1PX3ee3fXaEC7Kefxl4O/4w7agEceKRHlc0l3iwVJaKittQwAQd3ieUwoqsxzPH\n"
"dRwB4IU9aI6IjfqteyD51s7xd+ayM/O4j+aJ/HBhJajDHBcGWKytxv0f6YpqPUAc\n"
"25fRAXVa0Gsei6eY/QIDAQABMA0GCSqGSIb3DQEBCwUAA4IBAQCv/Vcxh2lMt8RV\n"
"Al0V9xIst0ZdjH22yTOUCOiH9PZgeagqrjTLT3ycWAdbZZUpzcFSdOmPUsgQ7Eqz\n"
"+TpcY5lmYFInLwJK/Afjqsb5LK2irGKT254p5qzD9rSRlM42wxRzQTA0BWX3mmhi\n"
"zwdrfLAvyCw1gHBbUZNf3eemBCY+8RRGPRAqD2XbyIya1bX0AHLXbx5dBe9EIOG/\n"
"F46WbTlrkR7kc06eiacTiGYwNdcywJ2KOcvmnXPup8Os6KOWe197CIathDHeiG2C\n"
"mQlsQDF/d7W4G/+l6Q66BhfRtuhp99gkT8P8j82X6ChrwbgQ5+vya3SytJ0wmIg2\n"
"67jOKmGK\n"
"-----END CERTIFICATE-----\n", buf.getString());

	// check auth cert
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.getFiscal()->getAuthPrivateKey(&buf));
	TEST_STRING_EQUAL(
"-----BEGIN PRIVATE KEY-----\n"
"MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCjtdn5VRSj2n5D\n"
"TM+aW3V5OEDw2ZVUCA1eRUNmjPe6bISeebZ3hHlWjF/jLp+56xbdlrz8iYpYttDf\n"
"9L+kdkpaLPvkSVJ+mdgl8xG4sBqqqzR4V/GYdseLwyCD3OMAx0gulHp4Ro/up4I1\n"
"GibdlvrOJihYSYMLZkUW7333muOSfsK1XeehZm944IYTGMztZOSqXwu/U9fd57d9\n"
"doQLsp5/GXg7/jDtqARx4pEeVzSXeLBUloqK21DABB3eJ5TCiqzHM8d1HAHghT1o\n"
"joiN+q17IPnWzvF35rIz87iP5on8cGElqMMcFwZYrK3G/R/pimo9QBzbl9EBdVrQ\n"
"ax6Lp5j9AgMBAAECggEAL5qkrKT54H+bcZR3Vco8iag68g5DJvFEeeIoLDzXmGUP\n"
"10lLLsvdwLYG9/fJyHU86+h2QfT4vr1CVa1EwN0I19n20TYk/91ahgZ9Y7gJuREZ\n"
"q9jeztfTRKfT36Quej54ldrlFe5m0h3xdeGJ5auOeL2Nw8Z0ja8KbhXsCkEG5cTx\n"
"ZvXB0XlFoAJOp8AZvU3ZNBpmpItFlcl2aBXwRCb72DUjLkpnZf2kFDNorc1wFZ2e\n"
"DO/pujT6EtQ1r5qb2kUuj4GpCaHffOB/ukz3dg3bBhompTYdhax0RlZs2vNsUusm\n"
"6oYsUS5nWmJfnrh32Te03Fdzc2U8/XUflJzKL/0QvQKBgQDOpNQvCCxwvthZXART\n"
"q0fl9NY0fxlSqUpxd1BB4DYCg6Sg5kVvfwf7rdb5bbP4aNCC/9m4MgXTD0DGfEhM\n"
"FnYPVNKTzwLMBftBQdzDN6766j5lI49evwnh855EFAR5GyaIWh2n7tT3NUOstogp\n"
"kpwhzsPGH1WkEO1QLcBDyzPI3wKBgQDKz94V8au1EVKuRBR+c5gNJpF+zmUu2t2C\n"
"ZlPtYIuWaxMbqitmeCmNBQQZK+oLQdSUMkgMvYVpKriPk6AgnY7+1F+OOeg+ezPU\n"
"G+J4Vi8Yx/kZPhXoBuW745twux+q8WOBwEj2WeMy5p1F/V3qlu70HA3kbsrXdB+R\n"
"0bFVAxCtowKBgFTtq4M08cbYuORpDCIzGBarvMnQnuC5US43IlYgxzHbVvMGEO2V\n"
"IPvQY7UZ4EitE11zt9CbRoeLEk1BURlsddMxQmabQwQFRVF5tzjIjvLzCPfaWJdR\n"
"Hsetr5M9QuVfQkPx/ZRCdWawjoLSdj3X0rGWYCHySOloR5CXbRiv0DWzAoGAF3XW\n"
"Ldmn0Ckx1EDB0iLS+up0OCPt5m6g4v2tRa8+VmcKbc/Qd2j8/XgQEk1XJHg3+/CZ\n"
"Dwg5T4IGmW0tP7iaGvY8G3qtV9TumOGk3+CwUACJ2xaoeA+cMZDRoUe0ERUdOpwg\n"
"lIavVmsA1GDLpWBSQeCg5sS+KBAhur9z8O6K1lsCgYEAj7TLLE0jLNXRRfkfWzy5\n"
"RsJezMCQS9fjtJrLGB3BbYxqtebP2owp1qjmKMQioW5QjRxRCOyT2KrHjb31hRsp\n"
"Hk3Wi0OKOEuKNwmAZczbjcPH4caPZPeL6LMDtFFMsFX2BW7TnC8FcoVr2KPO/FG/\n"
"xs4KtXC9j5rrvBowJ0LbJ2U=\n"
"-----END PRIVATE KEY-----\n", buf.getString());

	// check auth cert
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.getFiscal()->getSignPrivateKey(&buf));
	TEST_STRING_EQUAL(
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC3ycL8S2HxRptB\n"
"te7yl2uje/s2pRqdXxj6D3ZiBPvPXGqQEtEddnWC6aXc/GuqM1f0C86a7xH6poo7\n"
"Id8lbQ9xEMvMKghRwc0DCkM78TmPpYBosi/uACNO3Kv2QkH2t8lqlqtWIk1m7dFJ\n"
"RgZO9XOc6Zcx/stM5MxHoc//kfVM/mfWDj4FsuYL0SGNR/Z40WrBkGo+3PJsFvqN\n"
"ocFFonRd0TeWHY54T384XQG0vCJg8MqxVPEh6Rs1/uX8NETL5htQ7FAtx54deu9t\n"
"guIZZ5w/RrsKocaP1k1jWglOErcDCtJ3jIdr1afH8ZplQ21a53UFo/2DexVf6xFX\n"
"3G2cj3p3AgMBAAECggEAPUfM+Aq6kZSVWAetsL3EajKAxOuwQCDhVx+ovW4j+DQ8\n"
"Y+WiTEyfShNV9qVD0PBltz3omch1GjpFhQn6OaRvraeIDH9HXttb3FOjr2zzYG4y\n"
"rrYbPSRWoYj63ZWiIP2O7zdl0caGQHezfNcYa2N0NTG99DGc3/q6EnhlvjWQsSbi\n"
"EjmxcPx8fmV1i4DoflMQ383nsixAFapgrROUAtCgMvhWn1kSeoojKd+e4eKZxa/S\n"
"NYulsBJWNFkmo1CZH4YTqlPM+IwYeDUOnOUGNxGurRZ3qQdWs2N2ZQhnrvlh+zpz\n"
"urD2hwAz6gQXP7mxxMR1xHtAD8XQ+w4OiJK6VWjoIQKBgQDdZJvvZrV6tvqNwuTJ\n"
"kDZjbVU0iKkbP61rVE/6JpyzfGeS0WzGBNiCpbK3pJZnatK2nS7i9v8gAfIqGAk8\n"
"1NRKLa7Qbjgw6xHEwL8VZMXzN3KsMXgGM8EziPzicCYT8VBi/kXyV0ORqRz3rMQ+\n"
"JOTkWRrcw943yYyTr84Dn0l0XQKBgQDUhFWJ3lKwOs7AlAAQqR1PjfpcRvSxVZ70\n"
"BxTwnJoIQQyPQ0/OjCc1sit5s+h8xh0MeKSilCmvZerFlgNtvsCd6geSERXbpN+k\n"
"9Vs3jAEkVeKHeUA/afmGqGCocanlarYu7uNRLfvpG7DduHBb4yJale/XGExNnwC0\n"
"N+dkUU284wKBgBaOSojQiQrQm6RXx+F1TOVCXVz102zQRwXZWDCfQHXU5eSCa7ed\n"
"BMYCxbuKDDzLGF68kutSyNlk+VwqiL5m3J4WG2pm4FizimLmVFGEq9pEuu0qORVA\n"
"rp1mhoU3cdm0S0FasJupIlwzw5zEQFYogh11qpP1bK14XlcpoS6jSuONAoGBAJqM\n"
"EljM4X1fhvPtrY5wLeyo56UrxM8h4RK+A7Bncm0GQUf+P4+JxQn7pDpBZ5U1zfI/\n"
"2hqRfS8dAvrl+WBaFGHCy/ahji/JWwrvk4J1wm7WNoMm3l4/h0MyN/jHkDJSxGKl\n"
"P5LNyiDgDmNvueZY66bM2zqlZPgd5bkp3pDJv6rZAoGAaP5e5F1j6s82Pm7dCpH3\n"
"mRZWnfZIKqoNQIq2BO8vA9/WrdFI2C27uNhxCp2ZDMulRdBZcoeHcwJjnyDzg4I4\n"
"gBZ2nSKkVdlN1REoTjLBBdlHi8XKiXzxvpItc2wjNC2AKHaJqj/dnh3bbTAQD1iU\n"
"AxPmmLJYYkhfZ2i1IrTVxZE=\n"
"-----END RSA PRIVATE KEY-----\n", buf.getString());

	// check main info
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getAutomatId());
	TEST_NUMBER_EQUAL(1810, config2.getAutomat()->getCurrency());
	TEST_NUMBER_EQUAL(1, config2.getAutomat()->getDecimalPoint());
	TEST_NUMBER_EQUAL(3, config2.getAutomat()->getPaymentBus());
	TEST_NUMBER_EQUAL(1, config2.getAutomat()->getTaxSystem());
	TEST_NUMBER_EQUAL(2, config2.getAutomat()->getCashlessNumber());
	TEST_NUMBER_EQUAL(75, config2.getAutomat()->getScaleFactor());
	TEST_NUMBER_EQUAL(true, config2.getAutomat()->getCategoryMoney());

	// check price index info
	Config3PriceIndexList *priceIndexList = config2.getAutomat()->getPriceIndexList();
	Config3PriceIndex *priceIndex = priceIndexList->get(priceIndexList->getIndex("CA", 0));
	TEST_POINTER_NOT_NULL(priceIndex);
	TEST_NUMBER_EQUAL(0, priceIndex->timeTable.getWeek()->getValue());
	TEST_NUMBER_EQUAL(Config3PriceIndexType_Base, priceIndex->type);
	TimeInterval *interval = priceIndex->timeTable.getInterval();
	TEST_NUMBER_EQUAL(0, interval->getTime()->getHour());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getMinute());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getSecond());
	TEST_NUMBER_EQUAL(0, interval->getInterval());

	priceIndex = priceIndexList->get(priceIndexList->getIndex("CA", 1));
	TEST_POINTER_NOT_NULL(priceIndex);
	TEST_NUMBER_EQUAL(31, priceIndex->timeTable.getWeek()->getValue());
	TEST_NUMBER_EQUAL(Config3PriceIndexType_Time, priceIndex->type);
	interval = priceIndex->timeTable.getInterval();
	TEST_NUMBER_EQUAL(17, interval->getTime()->getHour());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getMinute());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getSecond());
	TEST_NUMBER_EQUAL(7200, interval->getInterval());

	// check product info
	Config3ProductIterator *product = config2.getAutomat()->createIterator();
	TEST_NUMBER_EQUAL(true, product->first());
	TEST_STRING_EQUAL("01", product->getId());
	TEST_STRING_EQUAL("0", product->getName());
	TEST_NUMBER_EQUAL(3, product->getTaxRate());
	TEST_NUMBER_EQUAL(0, product->getTotalCount());
	TEST_NUMBER_EQUAL(0, product->getTotalMoney());
	TEST_NUMBER_EQUAL(0, product->getCount());
	TEST_NUMBER_EQUAL(0, product->getMoney());

	// check price list info
	Config3Price *price = product->getPrice("CA", 0);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(250, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("CA", 1);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(150, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("DA", 0);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(200, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("DA", 1);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(100, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());
	return true;
}

bool Config3ParserTest::testFasCashlessIdError() {
	RamMemory memory(64000);
	memory.clear();
	TestRealTime realtime;
	StatStorage stat;
	Config3Modem config1(&memory, &realtime, &stat);
	Config3ConfigIniter initer;
	Config3ConfigParser parser(&config1);

	const char data1[] =
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
"DXE*1*1\r\n";
	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());
	TEST_NUMBER_EQUAL(Evadts::Result_PriceListNumberNotEqual, config1.comparePlanogram(initer.getPrices(), initer.getProducts()));
	TEST_NUMBER_EQUAL(Evadts::Result_OK, config1.resizePlanogram(initer.getPrices(), initer.getProducts()));

	parser.start();
	parser.procData((const uint8_t*)data1, sizeof(data1));
	parser.complete();
	TEST_NUMBER_EQUAL(false, parser.hasError());

	const char data2[] =
"DXS*EPHOR00001*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"IC1******108\r\n"
"IC4*2***1*\r\n"
"AC2*****1\r\n"
"PC1*11*2200*Чипсы Лейз(в ассортименте), 40гх28шт\r\n"
"PC9*11**3*65\r\n"
"PC7*11*CA*0*2200\r\n"
"PC7*11*DA*1*2200\r\n"
"PC1*13*3000*Круассаны 7 дней мини Какао 65гх21шт.\r\n"
"PC9*13**2*66\r\n"
"PC7*13*CA*0*3000\r\n"
"PC7*13*DA*1*3000\r\n"
"G85*A8D1\r\n"
"SE*190*0001\r\n"
"DXE*1*1\r\n";
	initer.start();
	initer.procData((const uint8_t*)data2, sizeof(data2));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());
	TEST_NUMBER_EQUAL(Evadts::Result_ProductNumberNotEqual, config1.comparePlanogram(initer.getPrices(), initer.getProducts()));

	parser.start();
	parser.procData((const uint8_t*)data2, sizeof(data2));
	parser.complete();
	TEST_NUMBER_EQUAL(false, parser.hasError());

	Config3Modem config2(&memory, &realtime, &stat);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.load());

	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getTotalCount());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getTotalMoney());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getCount());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getMoney());
	TEST_NUMBER_EQUAL(4, config2.getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(4, config2.getAutomat()->getPriceListNum());

	// check product info
	Config3ProductIterator *product = config2.getAutomat()->createIterator();
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
	Config3Price *price = product->getPrice("CA", 0);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(2200, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("DA", 1);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(2200, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());
	return true;
}

bool Config3ParserTest::testFiscalRegisterDisabled() {
	RamMemory memory(64000);
	memory.clear();
	TestRealTime realtime;
	StatStorage stat;
	Config3Modem config1(&memory, &realtime, &stat);
	Config3ConfigIniter initer;
	Config3ConfigParser parser(&config1);

	const char data1[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"IC1******0*6*1\r\n"
"IC4*1*1810**1\r\n"
"AC2*****3\r\n"
"FC1*4*2*apip.orangedata.ru*12001**\r\n"
"FC2*012345678901*0123456789*Тестовая точка*Тестовый адрес\r\n"
"FC3*-----BEGIN CERTIFICATE-----\r\n"
"FC3*MIIDYjCCAkoCAQAwDQYJKoZIhvcNAQELBQAwcTELMAkGA1UEBhMCUlUxDzANBgNV\r\n"
"FC3*BAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9zY293MRMwEQYDVQQKDApPcmFuZ2VkYXRh\r\n"
"FC3*MQ8wDQYDVQQLDAZOZWJ1bGExGjAYBgNVBAMMEXd3dy5vcmFuZ2VkYXRhLnJ1MB4X\r\n"
"FC3*DTE4MDMxNTE2NDYwMVoXDTI4MDMxMjE2NDYwMVowfTELMAkGA1UEBhMCUlUxDzAN\r\n"
"FC3*BgNVBAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9zY293MR8wHQYDVQQKDBZPcmFuZ2Vk\r\n"
"FC3*YXRhIHRlc3QgY2xpZW50MRMwEQYDVQQLDApFLWNvbW1lcmNlMRYwFAYDVQQDDA1v\r\n"
"FC3*cmFuZ2VkYXRhLnJ1MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAo7XZ\r\n"
"FC3*+VUUo9p+Q0zPmlt1eThA8NmVVAgNXkVDZoz3umyEnnm2d4R5Voxf4y6fuesW3Za8\r\n"
"FC3*/ImKWLbQ3/S/pHZKWiz75ElSfpnYJfMRuLAaqqs0eFfxmHbHi8Mgg9zjAMdILpR6\r\n"
"FC3*eEaP7qeCNRom3Zb6ziYoWEmDC2ZFFu9995rjkn7CtV3noWZveOCGExjM7WTkql8L\r\n"
"FC3*v1PX3ee3fXaEC7Kefxl4O/4w7agEceKRHlc0l3iwVJaKittQwAQd3ieUwoqsxzPH\r\n"
"FC3*dRwB4IU9aI6IjfqteyD51s7xd+ayM/O4j+aJ/HBhJajDHBcGWKytxv0f6YpqPUAc\r\n"
"FC3*25fRAXVa0Gsei6eY/QIDAQABMA0GCSqGSIb3DQEBCwUAA4IBAQCv/Vcxh2lMt8RV\r\n"
"FC3*Al0V9xIst0ZdjH22yTOUCOiH9PZgeagqrjTLT3ycWAdbZZUpzcFSdOmPUsgQ7Eqz\r\n"
"FC3*+TpcY5lmYFInLwJK/Afjqsb5LK2irGKT254p5qzD9rSRlM42wxRzQTA0BWX3mmhi\r\n"
"FC3*zwdrfLAvyCw1gHBbUZNf3eemBCY+8RRGPRAqD2XbyIya1bX0AHLXbx5dBe9EIOG/\r\n"
"FC3*F46WbTlrkR7kc06eiacTiGYwNdcywJ2KOcvmnXPup8Os6KOWe197CIathDHeiG2C\r\n"
"FC3*mQlsQDF/d7W4G/+l6Q66BhfRtuhp99gkT8P8j82X6ChrwbgQ5+vya3SytJ0wmIg2\r\n"
"FC3*67jOKmGK\r\n"
"FC3*-----END CERTIFICATE-----\r\n"
"FC4*-----BEGIN PRIVATE KEY-----\r\n"
"FC4*MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCjtdn5VRSj2n5D\r\n"
"FC4*TM+aW3V5OEDw2ZVUCA1eRUNmjPe6bISeebZ3hHlWjF/jLp+56xbdlrz8iYpYttDf\r\n"
"FC4*9L+kdkpaLPvkSVJ+mdgl8xG4sBqqqzR4V/GYdseLwyCD3OMAx0gulHp4Ro/up4I1\r\n"
"FC4*GibdlvrOJihYSYMLZkUW7333muOSfsK1XeehZm944IYTGMztZOSqXwu/U9fd57d9\r\n"
"FC4*doQLsp5/GXg7/jDtqARx4pEeVzSXeLBUloqK21DABB3eJ5TCiqzHM8d1HAHghT1o\r\n"
"FC4*joiN+q17IPnWzvF35rIz87iP5on8cGElqMMcFwZYrK3G/R/pimo9QBzbl9EBdVrQ\r\n"
"FC4*ax6Lp5j9AgMBAAECggEAL5qkrKT54H+bcZR3Vco8iag68g5DJvFEeeIoLDzXmGUP\r\n"
"FC4*10lLLsvdwLYG9/fJyHU86+h2QfT4vr1CVa1EwN0I19n20TYk/91ahgZ9Y7gJuREZ\r\n"
"FC4*q9jeztfTRKfT36Quej54ldrlFe5m0h3xdeGJ5auOeL2Nw8Z0ja8KbhXsCkEG5cTx\r\n"
"FC4*ZvXB0XlFoAJOp8AZvU3ZNBpmpItFlcl2aBXwRCb72DUjLkpnZf2kFDNorc1wFZ2e\r\n"
"FC4*DO/pujT6EtQ1r5qb2kUuj4GpCaHffOB/ukz3dg3bBhompTYdhax0RlZs2vNsUusm\r\n"
"FC4*6oYsUS5nWmJfnrh32Te03Fdzc2U8/XUflJzKL/0QvQKBgQDOpNQvCCxwvthZXART\r\n"
"FC4*q0fl9NY0fxlSqUpxd1BB4DYCg6Sg5kVvfwf7rdb5bbP4aNCC/9m4MgXTD0DGfEhM\r\n"
"FC4*FnYPVNKTzwLMBftBQdzDN6766j5lI49evwnh855EFAR5GyaIWh2n7tT3NUOstogp\r\n"
"FC4*kpwhzsPGH1WkEO1QLcBDyzPI3wKBgQDKz94V8au1EVKuRBR+c5gNJpF+zmUu2t2C\r\n"
"FC4*ZlPtYIuWaxMbqitmeCmNBQQZK+oLQdSUMkgMvYVpKriPk6AgnY7+1F+OOeg+ezPU\r\n"
"FC4*G+J4Vi8Yx/kZPhXoBuW745twux+q8WOBwEj2WeMy5p1F/V3qlu70HA3kbsrXdB+R\r\n"
"FC4*0bFVAxCtowKBgFTtq4M08cbYuORpDCIzGBarvMnQnuC5US43IlYgxzHbVvMGEO2V\r\n"
"FC4*IPvQY7UZ4EitE11zt9CbRoeLEk1BURlsddMxQmabQwQFRVF5tzjIjvLzCPfaWJdR\r\n"
"FC4*Hsetr5M9QuVfQkPx/ZRCdWawjoLSdj3X0rGWYCHySOloR5CXbRiv0DWzAoGAF3XW\r\n"
"FC4*Ldmn0Ckx1EDB0iLS+up0OCPt5m6g4v2tRa8+VmcKbc/Qd2j8/XgQEk1XJHg3+/CZ\r\n"
"FC4*Dwg5T4IGmW0tP7iaGvY8G3qtV9TumOGk3+CwUACJ2xaoeA+cMZDRoUe0ERUdOpwg\r\n"
"FC4*lIavVmsA1GDLpWBSQeCg5sS+KBAhur9z8O6K1lsCgYEAj7TLLE0jLNXRRfkfWzy5\r\n"
"FC4*RsJezMCQS9fjtJrLGB3BbYxqtebP2owp1qjmKMQioW5QjRxRCOyT2KrHjb31hRsp\r\n"
"FC4*Hk3Wi0OKOEuKNwmAZczbjcPH4caPZPeL6LMDtFFMsFX2BW7TnC8FcoVr2KPO/FG/\r\n"
"FC4*xs4KtXC9j5rrvBowJ0LbJ2U=\r\n"
"FC4*-----END PRIVATE KEY-----\r\n"
"FC5*-----BEGIN RSA PRIVATE KEY-----\r\n"
"FC5*MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC3ycL8S2HxRptB\r\n"
"FC5*te7yl2uje/s2pRqdXxj6D3ZiBPvPXGqQEtEddnWC6aXc/GuqM1f0C86a7xH6poo7\r\n"
"FC5*Id8lbQ9xEMvMKghRwc0DCkM78TmPpYBosi/uACNO3Kv2QkH2t8lqlqtWIk1m7dFJ\r\n"
"FC5*RgZO9XOc6Zcx/stM5MxHoc//kfVM/mfWDj4FsuYL0SGNR/Z40WrBkGo+3PJsFvqN\r\n"
"FC5*ocFFonRd0TeWHY54T384XQG0vCJg8MqxVPEh6Rs1/uX8NETL5htQ7FAtx54deu9t\r\n"
"FC5*guIZZ5w/RrsKocaP1k1jWglOErcDCtJ3jIdr1afH8ZplQ21a53UFo/2DexVf6xFX\r\n"
"FC5*3G2cj3p3AgMBAAECggEAPUfM+Aq6kZSVWAetsL3EajKAxOuwQCDhVx+ovW4j+DQ8\r\n"
"FC5*Y+WiTEyfShNV9qVD0PBltz3omch1GjpFhQn6OaRvraeIDH9HXttb3FOjr2zzYG4y\r\n"
"FC5*rrYbPSRWoYj63ZWiIP2O7zdl0caGQHezfNcYa2N0NTG99DGc3/q6EnhlvjWQsSbi\r\n"
"FC5*EjmxcPx8fmV1i4DoflMQ383nsixAFapgrROUAtCgMvhWn1kSeoojKd+e4eKZxa/S\r\n"
"FC5*NYulsBJWNFkmo1CZH4YTqlPM+IwYeDUOnOUGNxGurRZ3qQdWs2N2ZQhnrvlh+zpz\r\n"
"FC5*urD2hwAz6gQXP7mxxMR1xHtAD8XQ+w4OiJK6VWjoIQKBgQDdZJvvZrV6tvqNwuTJ\r\n"
"FC5*kDZjbVU0iKkbP61rVE/6JpyzfGeS0WzGBNiCpbK3pJZnatK2nS7i9v8gAfIqGAk8\r\n"
"FC5*1NRKLa7Qbjgw6xHEwL8VZMXzN3KsMXgGM8EziPzicCYT8VBi/kXyV0ORqRz3rMQ+\r\n"
"FC5*JOTkWRrcw943yYyTr84Dn0l0XQKBgQDUhFWJ3lKwOs7AlAAQqR1PjfpcRvSxVZ70\r\n"
"FC5*BxTwnJoIQQyPQ0/OjCc1sit5s+h8xh0MeKSilCmvZerFlgNtvsCd6geSERXbpN+k\r\n"
"FC5*9Vs3jAEkVeKHeUA/afmGqGCocanlarYu7uNRLfvpG7DduHBb4yJale/XGExNnwC0\r\n"
"FC5*N+dkUU284wKBgBaOSojQiQrQm6RXx+F1TOVCXVz102zQRwXZWDCfQHXU5eSCa7ed\r\n"
"FC5*BMYCxbuKDDzLGF68kutSyNlk+VwqiL5m3J4WG2pm4FizimLmVFGEq9pEuu0qORVA\r\n"
"FC5*rp1mhoU3cdm0S0FasJupIlwzw5zEQFYogh11qpP1bK14XlcpoS6jSuONAoGBAJqM\r\n"
"FC5*EljM4X1fhvPtrY5wLeyo56UrxM8h4RK+A7Bncm0GQUf+P4+JxQn7pDpBZ5U1zfI/\r\n"
"FC5*2hqRfS8dAvrl+WBaFGHCy/ahji/JWwrvk4J1wm7WNoMm3l4/h0MyN/jHkDJSxGKl\r\n"
"FC5*P5LNyiDgDmNvueZY66bM2zqlZPgd5bkp3pDJv6rZAoGAaP5e5F1j6s82Pm7dCpH3\r\n"
"FC5*mRZWnfZIKqoNQIq2BO8vA9/WrdFI2C27uNhxCp2ZDMulRdBZcoeHcwJjnyDzg4I4\r\n"
"FC5*gBZ2nSKkVdlN1REoTjLBBdlHi8XKiXzxvpItc2wjNC2AKHaJqj/dnh3bbTAQD1iU\r\n"
"FC5*AxPmmLJYYkhfZ2i1IrTVxZE=\r\n"
"FC5*-----END RSA PRIVATE KEY-----\r\n"
"LC2*CA*1*31*17:00:00*7200\r\n"
"LC2*DA*1*31*12:00:00*7200\r\n"
"PC1*01*250*0\r\n"
"PC7*01*CA*0*250\r\n"
"PC7*01*CA*1*150\r\n"
"PC7*01*DA*0*200\r\n"
"PC7*01*DA*1*100\r\n"
"PC9*01*1*3\r\n"
"PC1*02*270*0\r\n"
"PC7*02*CA*0*270\r\n"
"PC7*02*CA*1*170\r\n"
"PC7*02*DA*0*220\r\n"
"PC7*02*DA*1*120\r\n"
"PC9*02*2*3\r\n"
"PC1*03*250*0\r\n"
"PC7*03*CA*0*250\r\n"
"PC7*03*CA*1*150\r\n"
"PC7*03*DA*0*200\r\n"
"PC7*03*DA*1*150\r\n"
"PC9*03*3*3\r\n"
"PC1*04*250*0\r\n"
"PC7*04*CA*0*250\r\n"
"PC7*04*CA*1*150\r\n"
"PC7*04*DA*0*200\r\n"
"PC7*04*DA*1*150\r\n"
"PC9*04*4*3\r\n"
"G85*A4CB\r\n"
"SE*136*0001\r\n"
"DXE*1*1\r\n";
	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());
	TEST_NUMBER_EQUAL(Evadts::Result_PriceListNumberNotEqual, config1.comparePlanogram(initer.getPrices(), initer.getProducts()));
	TEST_NUMBER_EQUAL(Evadts::Result_OK, config1.resizePlanogram(initer.getPrices(), initer.getProducts()));

	parser.start();
	parser.procData((const uint8_t*)data1, sizeof(data1));
	parser.complete();
	TEST_NUMBER_EQUAL(false, parser.hasError());

	const char data2[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"IC1******0*6*1\r\n"
"IC4*1*1810**1\r\n"
"AC2*****3\r\n"
"FC1*0*****\r\n"
"FC2***Тестовая точка*Тестовый адрес\r\n"
"FC3*\r\n"
"FC4*\r\n"
"FC5*\r\n"
"LC2*CA*1*31*17:00:00*7200\r\n"
"LC2*DA*1*31*12:00:00*7200\r\n"
"PC1*01*250*0\r\n"
"PC7*01*CA*0*250\r\n"
"PC7*01*CA*1*150\r\n"
"PC7*01*DA*0*200\r\n"
"PC7*01*DA*1*100\r\n"
"PC9*01**3\r\n"
"PC1*02*270*0\r\n"
"PC7*02*CA*0*270\r\n"
"PC7*02*CA*1*170\r\n"
"PC7*02*DA*0*220\r\n"
"PC7*02*DA*1*120\r\n"
"PC9*02**3\r\n"
"PC1*03*250*0\r\n"
"PC7*03*CA*0*250\r\n"
"PC7*03*CA*1*150\r\n"
"PC7*03*DA*0*200\r\n"
"PC7*03*DA*1*150\r\n"
"PC9*03**3\r\n"
"PC1*04*250*0\r\n"
"PC7*04*CA*0*250\r\n"
"PC7*04*CA*1*150\r\n"
"PC7*04*DA*0*200\r\n"
"PC7*04*DA*1*150\r\n"
"PC9*04**3\r\n"
"G85*A4CB\r\n"
"SE*136*0001\r\n"
"DXE*1*1\r\n";
	initer.start();
	initer.procData((const uint8_t*)data2, sizeof(data2));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());
	TEST_NUMBER_EQUAL(Evadts::Result_OK, config1.comparePlanogram(initer.getPrices(), initer.getProducts()));

	parser.start();
	parser.procData((const uint8_t*)data2, sizeof(data2));
	parser.complete();
	TEST_NUMBER_EQUAL(false, parser.hasError());

	Config3Modem config2(&memory, &realtime, &stat);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.load());

	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getTotalCount());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getTotalMoney());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getCount());
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getMoney());
	TEST_NUMBER_EQUAL(4, config2.getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(4, config2.getAutomat()->getPriceListNum());

	TEST_NUMBER_EQUAL(0, config2.getFiscal()->getKkt());
	TEST_NUMBER_EQUAL(0, config2.getFiscal()->getKktInterface());
	TEST_STRING_EQUAL("", config2.getFiscal()->getKktAddr());
	TEST_NUMBER_EQUAL(0, config2.getFiscal()->getKktPort());
	TEST_STRING_EQUAL("", config2.getFiscal()->getOfdAddr());
	TEST_NUMBER_EQUAL(0, config2.getFiscal()->getOfdPort());
	TEST_STRING_EQUAL("", config2.getFiscal()->getINN());
	TEST_STRING_EQUAL("", config2.getFiscal()->getAutomatNumber());
	TEST_STRING_EQUAL("Тестовая точка", config2.getFiscal()->getPointName());
	TEST_STRING_EQUAL("Тестовый адрес", config2.getFiscal()->getPointAddr());

	// check auth cert
	StringBuilder buf(2048, 2048);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.getFiscal()->getAuthPublicKey(&buf));
	TEST_STRING_EQUAL("", buf.getString());

	// check auth cert
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.getFiscal()->getAuthPrivateKey(&buf));
	TEST_STRING_EQUAL("", buf.getString());

	// check auth cert
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config2.getFiscal()->getSignPrivateKey(&buf));
	TEST_STRING_EQUAL("", buf.getString());

	// check main info
	TEST_NUMBER_EQUAL(0, config2.getAutomat()->getAutomatId());
	TEST_NUMBER_EQUAL(1810, config2.getAutomat()->getCurrency());
	TEST_NUMBER_EQUAL(1, config2.getAutomat()->getDecimalPoint());
	TEST_NUMBER_EQUAL(3, config2.getAutomat()->getPaymentBus());
	TEST_NUMBER_EQUAL(1, config2.getAutomat()->getTaxSystem());

	// check price index info
	Config3PriceIndexList *priceIndexList = config2.getAutomat()->getPriceIndexList();
	Config3PriceIndex *priceIndex = priceIndexList->get(priceIndexList->getIndex("CA", 0));
	TEST_POINTER_NOT_NULL(priceIndex);
	TEST_NUMBER_EQUAL(0, priceIndex->timeTable.getWeek()->getValue());
	TEST_NUMBER_EQUAL(Config3PriceIndexType_Base, priceIndex->type);
	TimeInterval *interval = priceIndex->timeTable.getInterval();
	TEST_NUMBER_EQUAL(0, interval->getTime()->getHour());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getMinute());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getSecond());
	TEST_NUMBER_EQUAL(0, interval->getInterval());

	priceIndex = priceIndexList->get(priceIndexList->getIndex("CA", 1));
	TEST_POINTER_NOT_NULL(priceIndex);
	TEST_NUMBER_EQUAL(31, priceIndex->timeTable.getWeek()->getValue());
	TEST_NUMBER_EQUAL(Config3PriceIndexType_Time, priceIndex->type);
	interval = priceIndex->timeTable.getInterval();
	TEST_NUMBER_EQUAL(17, interval->getTime()->getHour());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getMinute());
	TEST_NUMBER_EQUAL(0, interval->getTime()->getSecond());
	TEST_NUMBER_EQUAL(7200, interval->getInterval());

	// check product info
	Config3ProductIterator *product = config2.getAutomat()->createIterator();
	TEST_NUMBER_EQUAL(true, product->first());
	TEST_STRING_EQUAL("01", product->getId());
	TEST_STRING_EQUAL("0", product->getName());
	TEST_NUMBER_EQUAL(3, product->getTaxRate());
	TEST_NUMBER_EQUAL(0, product->getTotalCount());
	TEST_NUMBER_EQUAL(0, product->getTotalMoney());
	TEST_NUMBER_EQUAL(0, product->getCount());
	TEST_NUMBER_EQUAL(0, product->getMoney());

	// check price list info
	Config3Price *price = product->getPrice("CA", 0);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(250, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("CA", 1);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(150, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("DA", 0);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(200, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("DA", 1);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(100, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());
	return true;
}

