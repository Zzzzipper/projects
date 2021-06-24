#include "config/v3/Config3AuditIniter.h"
#include "config/v3/Config3AuditParser.h"
#include "test/include/Test.h"
#include "memory/include/RamMemory.h"
#include "timer/include/TestRealTime.h"
#include "logger/include/Logger.h"

class Config3AuditParserTest : public TestSet {
public:
	Config3AuditParserTest();
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

TEST_SET_REGISTER(Config3AuditParserTest);

Config3AuditParserTest::Config3AuditParserTest() {
	TEST_CASE_REGISTER(Config3AuditParserTest, testCoffeemarG250);
	TEST_CASE_REGISTER(Config3AuditParserTest, testRheaLuce);
}

bool Config3AuditParserTest::init() {
	memory = new RamMemory(32000);
	memory->clear();
	realtime = new TestRealTime;
	stat = new StatStorage;
	config = new Config3Modem(memory, realtime, stat);
	return true;
}

void Config3AuditParserTest::cleanup() {
	delete config;
	delete stat;
	delete realtime;
	delete memory;
}


bool Config3AuditParserTest::testCoffeemarG250() {
	Config3AuditIniter initer(config);
	Config3AuditParser parser(config);

	const char data1[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"ID1******138*6*1\r\n"
"CB1***G250_F02**138\r\n"
"ID4*0*0*\r\n"
"ID5*20170818*135220\r\n"
"VA1*9457690*30055*0*0\r\n"
"VA2*140750*459*0*0\r\n"
"VA3*408490*1317*0*0\r\n"
"CA2*9455290*30031*0*0\r\n"
"CA3*0*0*0**14643210*1512610*4878300**0*8252300\r\n"
"CA4*0*0*4852570*57770\r\n"
"CA8*0*0\r\n"
"CA10*0*390530\r\n"
"DA2*2400*24*0*0\r\n"
"DA4*0*0\r\n"
"DA5*7160**0\r\n"
"DB2*0*0*0*0\r\n"
"DB4*0*0\r\n"
"DB5*0**0\r\n"
"TA2*0*0*0*0\r\n"
"PA1*01*260*0\r\n"
"PA3*15**0\r\n"
"PA4*45**0\r\n"
"PA7*01*CA*0*260*1634*4294967295*0*0\r\n"
"PA7*01*TA*0*260*0*4294967295*0*0\r\n"
"PA7*01*DA*1*100*0*4294967295*0*0\r\n"
"PA7*01*DB*1*100*0*4294967295*0*0\r\n"
"PA1*02*350*0\r\n"
"PA3*17**0\r\n"
"PA4*724**0\r\n"
"PA7*02*CA*0*350*2924*4294967295*0*0\r\n"
"PA7*02*TA*0*350*0*4294967295*0*0\r\n"
"PA7*02*DA*1*100*0*4294967295*0*0\r\n"
"PA7*02*DB*1*100*0*4294967295*0*0\r\n"
"PA1*03*280*0\r\n"
"PA3*6**0\r\n"
"PA4*38**0\r\n"
"PA7*03*CA*0*280*1304*4294967295*0*0\r\n"
"PA7*03*TA*0*280*0*4294967295*0*0\r\n"
"PA7*03*DA*1*100*0*4294967295*0*0\r\n"
"PA7*03*DB*1*100*0*4294967295*0*0\r\n"
"PA1*04*280*0\r\n"
"PA3*2**0\r\n"
"PA4*89**0\r\n"
"PA7*04*CA*0*280*4733*4294967295*0*0\r\n"
"PA7*04*TA*0*280*0*4294967295*0*0\r\n"
"PA7*04*DA*1*100*0*4294967295*0*0\r\n"
"PA7*04*DB*1*100*0*4294967295*0*0\r\n"
"PA1*05*300*0\r\n"
"PA3*135**0\r\n"
"PA4*44**0\r\n"
"PA7*05*CA*0*300*3232*4294967295*0*0\r\n"
"PA7*05*TA*0*300*0*4294967295*0*0\r\n"
"PA7*05*DA*1*100*0*4294967295*0*0\r\n"
"PA7*05*DB*1*100*0*4294967295*0*0\r\n"
"PA1*06*320*0\r\n"
"PA3*41**0\r\n"
"PA4*8**0\r\n"
"PA7*06*CA*0*320*1537*4294967295*0*0\r\n"
"PA7*06*TA*0*320*0*4294967295*0*0\r\n"
"PA7*06*DA*1*100*0*4294967295*0*0\r\n"
"PA7*06*DB*1*100*0*4294967295*0*0\r\n"
"PA1*07*320*0\r\n"
"PA3*7**0\r\n"
"PA4*12**0\r\n"
"PA7*07*CA*0*320*1887*4294967295*0*0\r\n"
"PA7*07*TA*0*320*0*4294967295*0*0\r\n"
"PA7*07*DA*1*100*1*4294967295*0*0\r\n"
"PA7*07*DB*1*100*0*4294967295*0*0\r\n"
"PA1*08*360*0\r\n"
"PA3*66**0\r\n"
"PA4*37**0\r\n"
"PA7*08*CA*0*360*3001*4294967295*0*0\r\n"
"PA7*08*TA*0*360*0*4294967295*0*0\r\n"
"PA7*08*DA*1*100*0*4294967295*0*0\r\n"
"PA7*08*DB*1*100*0*4294967295*0*0\r\n"
"PA1*09*330*0\r\n"
"PA3*5**0\r\n"
"PA4*15**0\r\n"
"PA7*09*CA*0*330*624*4294967295*0*0\r\n"
"PA7*09*TA*0*330*0*4294967295*0*0\r\n"
"PA7*09*DA*1*100*0*4294967295*0*0\r\n"
"PA7*09*DB*1*100*0*4294967295*0*0\r\n"
"PA1*10*360*0\r\n"
"PA3*62**0\r\n"
"PA4*20**0\r\n"
"PA7*10*CA*0*360*1796*4294967295*0*0\r\n"
"PA7*10*TA*0*360*0*4294967295*0*0\r\n"
"PA7*10*DA*1*100*0*4294967295*0*0\r\n"
"PA7*10*DB*1*100*0*4294967295*0*0\r\n"
"PA1*11*260*0\r\n"
"PA3*33**0\r\n"
"PA4*9**0\r\n"
"PA7*11*CA*0*260*1834*4294967295*0*0\r\n"
"PA7*11*TA*0*260*0*4294967295*0*0\r\n"
"PA7*11*DA*1*100*1*4294967295*0*0\r\n"
"PA7*11*DB*1*100*0*4294967295*0*0\r\n"
"PA1*12*300*0\r\n"
"PA3*9**0\r\n"
"PA4*20**0\r\n"
"PA7*12*CA*0*300*1760*4294967295*0*0\r\n"
"PA7*12*TA*0*300*0*4294967295*0*0\r\n"
"PA7*12*DA*1*100*4*4294967295*0*0\r\n"
"PA7*12*DB*1*100*0*4294967295*0*0\r\n"
"PA1*13*350*0\r\n"
"PA3*34**0\r\n"
"PA4*63**0\r\n"
"PA7*13*CA*0*350*1718*4294967295*0*0\r\n"
"PA7*13*TA*0*350*0*4294967295*0*0\r\n"
"PA7*13*DA*1*100*14*299*0*0\r\n"
"PA7*13*DB*1*100*0*4294967295*0*0\r\n"
"PA1*14*300*0\r\n"
"PA3*11**0\r\n"
"PA4*183**0\r\n"
"PA7*14*CA*0*300*1387*4294967295*0*0\r\n"
"PA7*14*TA*0*300*0*4294967295*0*0\r\n"
"PA7*14*DA*1*100*4*4294967295*0*0\r\n"
"PA7*14*DB*1*100*0*4294967295*0*0\r\n"
"PA1*15*350*0\r\n"
"PA3*16**0\r\n"
"PA4*10**0\r\n"
"PA7*15*CA*0*350*660*4294967295*0*0\r\n"
"PA7*15*TA*0*350*0*4294967295*0*0\r\n"
"PA7*15*DA*1*100*0*4294967295*0*0\r\n"
"PA7*15*DB*1*100*0*4294967295*0*0\r\n"
"EA3**20170818*135220******22*175\r\n"
"MA5*ERROR*0*31\r\n"
"G85*5D88\r\n"
"SE*129*0001\r\n"
"DXE*1*1\r\n";
	initer.start();
	initer.procData((const uint8_t*)data1, sizeof(data1));
	initer.complete();
	TEST_NUMBER_EQUAL(false, initer.hasError());

	parser.start();
	parser.procData((const uint8_t*)data1, sizeof(data1));
	parser.complete();
	TEST_NUMBER_EQUAL(false, parser.hasError());

	TEST_NUMBER_EQUAL(0, config->getAutomat()->getTotalCount());
	TEST_NUMBER_EQUAL(0, config->getAutomat()->getTotalMoney());
	TEST_NUMBER_EQUAL(0, config->getAutomat()->getCount());
	TEST_NUMBER_EQUAL(0, config->getAutomat()->getMoney());
	TEST_NUMBER_EQUAL(15, config->getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(4, config->getAutomat()->getPriceListNum());

	// check main info
	TEST_NUMBER_EQUAL(138, config->getAutomat()->getAutomatId());
	TEST_NUMBER_EQUAL(0, config->getAutomat()->getDecimalPoint());

	// check product info
	Config3ProductIterator *product = config->getAutomat()->createIterator();
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
	TEST_NUMBER_EQUAL(260, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	price = product->getPrice("TA", 0);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(260, price->getPrice());
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

bool Config3AuditParserTest::testRheaLuce() {
	Config3AuditIniter initer(config);
	Config3AuditParser parser(config);

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
	TEST_NUMBER_EQUAL(false, initer.hasError());

	parser.start();
	parser.procData((const uint8_t*)data1, sizeof(data1));
	parser.complete();
	TEST_NUMBER_EQUAL(false, parser.hasError());

	TEST_NUMBER_EQUAL(0, config->getAutomat()->getTotalCount());
	TEST_NUMBER_EQUAL(0, config->getAutomat()->getTotalMoney());
	TEST_NUMBER_EQUAL(0, config->getAutomat()->getCount());
	TEST_NUMBER_EQUAL(0, config->getAutomat()->getMoney());
	TEST_NUMBER_EQUAL(14, config->getAutomat()->getProductNum());
	TEST_NUMBER_EQUAL(1, config->getAutomat()->getPriceListNum());

	// check product info
	Config3ProductIterator *product = config->getAutomat()->createIterator();
	TEST_NUMBER_EQUAL(true, product->first());
	TEST_STRING_EQUAL("1", product->getId());
	TEST_STRING_EQUAL("FUNCTIONING", product->getName());

	// price list info
	Config3Price *price = product->getPrice("CA", 0);
	TEST_POINTER_NOT_NULL(price);
	TEST_NUMBER_EQUAL(2500, price->getPrice());
	TEST_NUMBER_EQUAL(0, price->getTotalCount());
	TEST_NUMBER_EQUAL(0, price->getTotalMoney());
	TEST_NUMBER_EQUAL(0, price->getCount());
	TEST_NUMBER_EQUAL(0, price->getMoney());

	return true;
}
