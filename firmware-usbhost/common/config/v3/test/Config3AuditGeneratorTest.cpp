#include "config/v3/Config3AuditGenerator.h"
#include "config/v3/Config3AuditIniter.h"
#include "config/v3/Config3AuditParser.h"
#include "timer/include/RealTime.h"
#include "test/include/Test.h"
#include "memory/include/RamMemory.h"
#include "timer/include/TestRealTime.h"
#include "logger/include/Logger.h"

class Config3AuditGeneratorTest : public TestSet {
public:
	Config3AuditGeneratorTest();
	bool init();
	void cleanup();
	bool testAuditGenerator();
	bool testAuditMaximumSize();
	bool testDisabledPriceLists();

private:
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	Config3Modem *config;
};

TEST_SET_REGISTER(Config3AuditGeneratorTest);

Config3AuditGeneratorTest::Config3AuditGeneratorTest() {
	TEST_CASE_REGISTER(Config3AuditGeneratorTest, testAuditGenerator);
	TEST_CASE_REGISTER(Config3AuditGeneratorTest, testAuditMaximumSize);
	TEST_CASE_REGISTER(Config3AuditGeneratorTest, testDisabledPriceLists);
}

bool Config3AuditGeneratorTest::init() {
	memory = new RamMemory(64000);
	memory->clear();
	realtime = new TestRealTime;
	stat = new StatStorage;
	config = new Config3Modem(memory, realtime, stat);

	MdbCoinChangerContext *context = config->getAutomat()->getCCContext();
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
	return true;
}

void Config3AuditGeneratorTest::cleanup() {
	delete config;
	delete stat;
	delete realtime;
	delete memory;
}

bool Config3AuditGeneratorTest::testAuditGenerator() {
	Config3AuditIniter initer(config);
	Config3AuditParser parser(config);
	Config3AuditGenerator generator(config);

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
"PA9*01**3\r\n"
"PA1*02*270*0\r\n"
"PA3*0**0\r\n"
"PA4*2**2\r\n"
"PA7*02*CA*0*270*1028*130879*1028*130879\r\n"
"PA7*02*TA*0*270*0*4294967295*0*4294967295\r\n"
"PA7*02*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*02*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA9*02**2\r\n"
"PA1*03*250*0\r\n"
"PA3*3**3\r\n"
"PA4*1**1\r\n"
"PA7*03*CA*0*250*1674*280299*1674*280299\r\n"
"PA7*03*TA*0*250*0*4294967295*0*4294967295\r\n"
"PA7*03*DA*1*100*0*4294967295*0*4294967295\r\n"
"PA7*03*DB*1*100*0*4294967295*0*4294967295\r\n"
"PA9*03**1\r\n"
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

	config->getAutomat()->sale("04", "CA", 0, 250);
	config->getAutomat()->sale("04", "CA", 0, 250);
	config->getAutomat()->sale("04", "CA", 0, 250);

	Buffer buf(4000);
	generator.reset();
	buf.add(generator.getData(), generator.getLen());
	while(generator.isLast() == false) {
		generator.next();
		buf.add(generator.getData(), generator.getLen());
	}

	const char expected1[] =
"DXS*EPHOR00001*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"AM2**internet*gdata*gdata*1*0*0*1*\r\n"
"AM3*0*1*192.168.1.210*0**0\r\n"
"ID1******0\r\n"
"ID4*1*7\r\n"
"CA15*12550\r\n"        // всего денег во всех тубах
"CA17*0*10*67***1\r\n"  // монеты в тубе 1
"CA17*1*20*54***0\r\n"  // монеты в тубе 2
"CA17*2*50*114***1\r\n" // монеты в тубе 3
"CA17*3*100*51***0\r\n" // монеты в тубе 4
"PA1*01*250*0\r\n"
"PA9*01**3\r\n"
"PA4*5\r\n"
"PA7*01*CA*0*250*0*0\r\n"
"PA7*01*TA*0*250*0*0\r\n"
"PA7*01*DA*1*100*0*0\r\n"
"PA7*01*DB*1*100*0*0\r\n"
"PA1*02*270*0\r\n"
"PA9*02**2\r\n"
"PA4*2\r\n"
"PA7*02*CA*0*270*0*0\r\n"
"PA7*02*TA*0*270*0*0\r\n"
"PA7*02*DA*1*100*0*0\r\n"
"PA7*02*DB*1*100*0*0\r\n"
"PA1*03*250*0\r\n"
"PA9*03**1\r\n"
"PA4*1\r\n"
"PA7*03*CA*0*250*0*0\r\n"
"PA7*03*TA*0*250*0*0\r\n"
"PA7*03*DA*1*100*0*0\r\n"
"PA7*03*DB*1*100*0*0\r\n"
"PA1*04*250*0\r\n"
"PA9*04**0\r\n"
"PA4*2\r\n"
"PA7*04*CA*0*250*3*750\r\n"
"PA7*04*TA*0*250*0*0\r\n"
"PA7*04*DA*1*100*0*0\r\n"
"PA7*04*DB*1*100*0*0\r\n"
"PA1*05*270*0\r\n"
"PA9*05**0\r\n"
"PA4*3\r\n"
"PA7*05*CA*0*270*0*0\r\n"
"PA7*05*TA*0*270*0*0\r\n"
"PA7*05*DA*1*100*0*0\r\n"
"PA7*05*DB*1*100*0*0\r\n"
"G85*5600\r\n"
"SE*47*0001\r\n"
"DXE*1*1\r\n";
	TEST_SUBSTR_EQUAL(expected1, (const char *)buf.getData(), buf.getLen());

	return true;
}

bool Config3AuditGeneratorTest::testAuditMaximumSize() {
	// price-lists
	Config3PriceIndexList list;
	list.add("CA", 0, Config3PriceIndexType_Base);
	list.add("CA", 1, Config3PriceIndexType_Base);
	list.add("CA", 2, Config3PriceIndexType_Base);
	list.add("CA", 3, Config3PriceIndexType_Base);
	list.add("DA", 0, Config3PriceIndexType_Base);
	list.add("DA", 1, Config3PriceIndexType_Base);
	list.add("DA", 2, Config3PriceIndexType_Base);
	list.add("DA", 3, Config3PriceIndexType_Base);

	// init config in memory
	Config3Automat *automat = config->getAutomat();
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config3PriceIndex *index = list.get(i);
		automat->addPriceList(index->device.get(), index->number, (Config3PriceIndexType)index->type);
	}
	StringBuilder selectId;
	for(uint16_t i = 0; i < 90; i++) {
		selectId.clear();
		selectId << i;
		automat->addProduct(selectId.getString(), i);
	}
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config->init());

	// init config data
	Config3ProductIterator *iterator = automat->createIterator();
	iterator->first();
	do {
		iterator->setName("абвгдеЄжзийклмнопрстуфхцчшщъыьэю€AЅ¬√ƒ≈®∆«»… ЋћЌќѕ");
		for(uint16_t i = 0; i < list.getSize(); i++) {
			Config3PriceIndex *index = list.get(i);
			iterator->setPriceCount(index->device.get(), index->number, 65000, 2500000000, 2500000000, 2500000000, 2500000000);
		}
	} while(iterator->next() == true);

	// generate audit
	Config3AuditGenerator generator(config);
	Buffer buf(64000);
	generator.reset();
	buf.add(generator.getData(), generator.getLen());
	while(generator.isLast() == false) {
		generator.next();
		buf.add(generator.getData(), generator.getLen());
	}
	LOG_STR(buf.getData(), buf.getLen());
	TEST_NUMBER_EQUAL(50533, buf.getLen());
	return true;
}

bool Config3AuditGeneratorTest::testDisabledPriceLists() {
	// price-lists
	Config3PriceIndexList list;
	list.add("CA", 0, Config3PriceIndexType_Base);
	list.add("CA", 1, Config3PriceIndexType_Time);
	list.add("CA", 2, Config3PriceIndexType_None);
	list.add("CA", 3, Config3PriceIndexType_None);
	list.add("DA", 0, Config3PriceIndexType_Base);
	list.add("DA", 1, Config3PriceIndexType_Time);
	list.add("DA", 2, Config3PriceIndexType_None);
	list.add("DA", 3, Config3PriceIndexType_None);

	// init config in memory
	Config3Automat *automat = config->getAutomat();
	for(uint16_t i = 0; i < list.getSize(); i++) {
		Config3PriceIndex *index = list.get(i);
		automat->addPriceList(index->device.get(), index->number, (Config3PriceIndexType)index->type);
	}
	StringBuilder selectId;
	for(uint16_t i = 0; i < 5; i++) {
		selectId.clear();
		selectId << i;
		automat->addProduct(selectId.getString(), i);
	}
	TEST_NUMBER_EQUAL(MemoryResult_Ok, config->init());

	// init config data
	Config3ProductIterator *iterator = automat->createIterator();
	iterator->first();
	do {
		iterator->setName("ячейка");
		for(uint16_t i = 0; i < list.getSize(); i++) {
			Config3PriceIndex *index = list.get(i);
			iterator->setPriceCount(index->device.get(), index->number, 65000, 2500000000, 2500000000, 2500000000, 2500000000);
		}
	} while(iterator->next() == true);

	// generate audit
	Config3AuditGenerator generator(config);
	Buffer buf(64000);
	generator.reset();
	buf.add(generator.getData(), generator.getLen());
	while(generator.isLast() == false) {
		generator.next();
		buf.add(generator.getData(), generator.getLen());
	}

	const char expected1[] =
"DXS*EPHOR00001*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"AM2**internet*gdata*gdata*1*0*0*1*\r\n"
"AM3*0*1*192.168.1.210*0**0\r\n"
"ID1******0\r\n"
"ID4*2*7\r\n"
"CA15*12550\r\n"        // всего денег во всех тубах
"CA17*0*10*67***1\r\n"  // монеты в тубе 1
"CA17*1*20*54***0\r\n"  // монеты в тубе 2
"CA17*2*50*114***1\r\n" // монеты в тубе 3
"CA17*3*100*51***0\r\n" // монеты в тубе 4
"PA1*0*65000*^DF^^F7^^E5^^E9^^EA^^E0^\r\n"
"PA9*0*0*0\r\n"
"PA4*0\r\n"
"PA7*0*CA*0*65000*2500000000*2500000000\r\n"
"PA7*0*CA*1*65000*2500000000*2500000000\r\n"
"PA7*0*DA*0*65000*2500000000*2500000000\r\n"
"PA7*0*DA*1*65000*2500000000*2500000000\r\n"
"PA1*1*65000*^DF^^F7^^E5^^E9^^EA^^E0^\r\n"
"PA9*1*1*0\r\n"
"PA4*0\r\n"
"PA7*1*CA*0*65000*2500000000*2500000000\r\n"
"PA7*1*CA*1*65000*2500000000*2500000000\r\n"
"PA7*1*DA*0*65000*2500000000*2500000000\r\n"
"PA7*1*DA*1*65000*2500000000*2500000000\r\n"
"PA1*2*65000*^DF^^F7^^E5^^E9^^EA^^E0^\r\n"
"PA9*2*2*0\r\n"
"PA4*0\r\n"
"PA7*2*CA*0*65000*2500000000*2500000000\r\n"
"PA7*2*CA*1*65000*2500000000*2500000000\r\n"
"PA7*2*DA*0*65000*2500000000*2500000000\r\n"
"PA7*2*DA*1*65000*2500000000*2500000000\r\n"
"PA1*3*65000*^DF^^F7^^E5^^E9^^EA^^E0^\r\n"
"PA9*3*3*0\r\n"
"PA4*0\r\n"
"PA7*3*CA*0*65000*2500000000*2500000000\r\n"
"PA7*3*CA*1*65000*2500000000*2500000000\r\n"
"PA7*3*DA*0*65000*2500000000*2500000000\r\n"
"PA7*3*DA*1*65000*2500000000*2500000000\r\n"
"PA1*4*65000*^DF^^F7^^E5^^E9^^EA^^E0^\r\n"
"PA9*4*4*0\r\n"
"PA4*0\r\n"
"PA7*4*CA*0*65000*2500000000*2500000000\r\n"
"PA7*4*CA*1*65000*2500000000*2500000000\r\n"
"PA7*4*DA*0*65000*2500000000*2500000000\r\n"
"PA7*4*DA*1*65000*2500000000*2500000000\r\n"
"G85*1AB4\r\n"
"SE*47*0001\r\n"
"DXE*1*1\r\n";
	TEST_SUBSTR_EQUAL(expected1, (const char *)buf.getData(), buf.getLen());
	return true;
}
