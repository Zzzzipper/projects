#include "test/include/Test.h"
#include "memory/include/RamMemory.h"
#include "config/v3/Config3AuditIniter.h"
#include "config/v3/Config3Modem.h"
#include "timer/include/TestRealTime.h"
#include "logger/include/Logger.h"

class Config3AuditIniterTest : public TestSet {
public:
	Config3AuditIniterTest();
	bool init();
	void cleanup();
	bool testCoffeemarG250();
	bool testRheaLuce();

private:
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	Config3Modem *config;
};

TEST_SET_REGISTER(Config3AuditIniterTest);

Config3AuditIniterTest::Config3AuditIniterTest() {
	TEST_CASE_REGISTER(Config3AuditIniterTest, testCoffeemarG250);
	TEST_CASE_REGISTER(Config3AuditIniterTest, testRheaLuce);
}

bool Config3AuditIniterTest::init() {
	memory = new RamMemory(64000);
	memory->clear();
	realtime = new TestRealTime;
	stat = new StatStorage;
	config = new Config3Modem(memory, realtime, stat);
	return true;
}

void Config3AuditIniterTest::cleanup() {
	delete config;
	delete stat;
	delete realtime;
	delete memory;
}

bool Config3AuditIniterTest::testCoffeemarG250() {
	Config3AuditIniter initer(config);

	const char data1[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"ID1******138*6*1\r\n"
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

	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();

	TEST_NUMBER_EQUAL(0, config->getAutomat()->getTotalCount());
	TEST_NUMBER_EQUAL(0, config->getAutomat()->getTotalMoney());
	TEST_NUMBER_EQUAL(0, config->getAutomat()->getCount());
	TEST_NUMBER_EQUAL(0, config->getAutomat()->getMoney());
	TEST_NUMBER_EQUAL(15, config->getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(4, config->getAutomat()->getPriceListNum());
	TEST_NUMBER_EQUAL(1, config->getAutomat()->getDecimalPoint());

	Config3ProductIterator *product = config->getAutomat()->createIterator();
	TEST_NUMBER_EQUAL(true, product->first());
	TEST_STRING_EQUAL("01", product->getId());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, product->getCashlessId());
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("02", product->getId());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, product->getCashlessId());
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("03", product->getId());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, product->getCashlessId());
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("04", product->getId());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, product->getCashlessId());
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("05", product->getId());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, product->getCashlessId());
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("06", product->getId());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, product->getCashlessId());
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("07", product->getId());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, product->getCashlessId());
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("08", product->getId());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, product->getCashlessId());
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("09", product->getId());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, product->getCashlessId());
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("10", product->getId());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, product->getCashlessId());
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("11", product->getId());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, product->getCashlessId());
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("12", product->getId());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, product->getCashlessId());
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("13", product->getId());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, product->getCashlessId());
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("14", product->getId());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, product->getCashlessId());
	TEST_NUMBER_EQUAL(true, product->next());
	TEST_STRING_EQUAL("15", product->getId());
	TEST_NUMBER_EQUAL(Config3ProductIndexList::UndefinedIndex, product->getCashlessId());
	TEST_NUMBER_EQUAL(false, product->next());

	Config3Price *price = product->getPrice("CA", 0);
	TEST_POINTER_NOT_NULL(price);
	price = product->getPrice("TA", 0);
	TEST_POINTER_NOT_NULL(price);
	price = product->getPrice("DA", 1);
	TEST_POINTER_NOT_NULL(price);
	price = product->getPrice("DB", 1);
	TEST_POINTER_NOT_NULL(price);

	return true;
}

bool Config3AuditIniterTest::testRheaLuce() {
	Config3AuditIniter initer(config);

	const char data1[] =
"DXS*RHV7654321*VA*V1/1*1\r\n"
"ST*001*0001\r\n"
"ID1*0*SAGOMAE*27*0*MDB*0\r\n"
"ID4*2**XTS\r\n"
"ID5*161026*0810\r\n"
"VA1*55046400*18312*55046400*18292\r\n"
"VA2*228400*79*228400*79\r\n"
"VA3*0*0*0*0\r\n"
"CA1***\r\n"
"CA2*55046400*18312*55046400*18312\r\n"
"CA3*84188500*6520100*28506400*49162000*84188500*6520100*28506400*49162000\r\n"
"CA4*28857200*0*28857200*0\r\n"
"CA7*0*0\r\n"
"CA8*102000*102000\r\n"
"CA9*1672100\r\n"
"CA10*178700*178700\r\n"
"TA2*0*0*0*0*0*0*0*0\r\n"
"DA1***\r\n"
"DA2*0*0*0*0\r\n"
"DA4*0*0\r\n"
"DA5*0**0\r\n"
"PA1*1*2500*FUNCTIONING\r\n"
"PA2*1554*3525000*1554*3525000\r\n"
"PA3*32*81100*32*81100\r\n"
"PA4*0*0*0*0\r\n"
"PA1*2*2500*FUNCTIONING\r\n"
"PA2*1288*3436800*1288*3436800\r\n"
"PA3*2*5500*2*5500\r\n"
"PA4*0*0*0*0\r\n"
"PA1*3*2600*FUNCTIONING\r\n"
"PA2*1986*4908600*1986*4908600\r\n"
"PA3*1*2500*1*2500\r\n"
"PA4*0*0*0*0\r\n"
"PA1*4*3000*FUNCTIONING\r\n"
"PA2*1151*3467700*1151*3467700\r\n"
"PA3*12*36100*12*36100\r\n"
"PA4*0*0*0*0\r\n"
"PA1*5*2800*FUNCTIONING\r\n"
"PA2*3790*12115600*3790*12115600\r\n"
"PA3*5*15500*5*15500\r\n"
"PA4*0*0*0*0\r\n"
"PA1*6*2800*FUNCTIONING\r\n"
"PA2*1547*5563000*1547*5563000\r\n"
"PA3*3*10500*3*10500\r\n"
"PA4*0*0*0*0\r\n"
"PA1*7*3000*FUNCTIONING\r\n"
"PA2*1572*5045500*1572*5045500\r\n"
"PA3*2*6500*2*6500\r\n"
"PA4*0*0*0*0\r\n"
"PA1*8*3000*FUNCTIONING\r\n"
"PA2*1184*3809200*1184*3809200\r\n"
"PA3*4*14000*4*14000\r\n"
"PA4*0*0*0*0\r\n"
"PA1*9*3200*FUNCTIONING\r\n"
"PA2*856*3177100*856*3177100\r\n"
"PA3*5*18700*5*18700\r\n"
"PA4*0*0*0*0\r\n"
"PA1*10*3000*FUNCTIONING\r\n"
"PA2*1166*3714000*1166*3714000\r\n"
"PA3*5*16000*5*16000\r\n"
"PA4*0*0*0*0\r\n"
"PA1*11*3000*FUNCTIONING\r\n"
"PA2*1281*4115400*1281*4115400\r\n"
"PA3*4*12500*4*12500\r\n"
"PA4*0*0*0*0\r\n"
"PA1*12*2500*FUNCTIONING\r\n"
"PA2*937*2168500*937*2168500\r\n"
"PA3*4*9500*4*9500\r\n"
"PA4*0*0*0*0\r\n"
"PA1*13*0*ESPRES\r\n"
"PA2*0*0*0*0\r\n"
"PA3*0*0*0*0\r\n"
"PA4*0*0*0*0\r\n"
"PA1*14*\r\n"
"PA2*0*0*0*0\r\n"
"PA3*0*0*0*0\r\n"
"PA4*0*0*0*0\r\n"
"CA15*0\r\n"
"EA1*6A*161024*1336*\r\n"
"EA1*6A*161024*0750*\r\n"
"EA1*6A*161023*2009*\r\n"
"EA1*6A*161023*0322*\r\n"
"EA1*6A*161023*0014*\r\n"
"EA1*6A*161022*0337*\r\n"
"EA1*6A*161022*0324*\r\n"
"EA1*6A*161022*0219*\r\n"
"EA1*6A*160721*1926*\r\n"
"EA1*6A*160720*1942*\r\n"
"EA1*6A*160720*1718*\r\n"
"EA1*6A*160711*1319*\r\n"
"EA1*6A*160708*0913*\r\n"
"EA1*6A*160708*0859*\r\n"
"EA1*6A*160708*0858*\r\n"
"EA1*3*160204*1537*\r\n"
"EA1*3*160117*1334*\r\n"
"EA1*3*151221*1700*\r\n"
"EA1*3*151221*1659*\r\n"
"EA1*3*151215*1117*\r\n"
"EA3*1*161026*0810**161026*0807\r\n"
"EA7*1957*1957\r\n"
"G85*01B8\r\n"
"SE*101*0001\r\n"
"DXE*1*1\r\n";

	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();

	TEST_NUMBER_EQUAL(0, config->getAutomat()->getTotalCount());
	TEST_NUMBER_EQUAL(0, config->getAutomat()->getTotalMoney());
	TEST_NUMBER_EQUAL(0, config->getAutomat()->getCount());
	TEST_NUMBER_EQUAL(0, config->getAutomat()->getMoney());
	TEST_NUMBER_EQUAL(14, config->getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(1, config->getAutomat()->getPriceListNum());
	TEST_NUMBER_EQUAL(2, config->getAutomat()->getDecimalPoint());

	return true;
}
