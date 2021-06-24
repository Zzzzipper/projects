#include "dex/DexCrc.h"
#include "evadts/EvadtsChecker.h"
#include "evadts/EvadtsGenerator.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class EvadtsCrcTest : public TestSet {
public:
	EvadtsCrcTest();
	bool testCrcCalculation();
	bool testEvadtsChecker();
	bool testEvadtsGenerator();
};

TEST_SET_REGISTER(EvadtsCrcTest);

EvadtsCrcTest::EvadtsCrcTest() {
	TEST_CASE_REGISTER(EvadtsCrcTest, testCrcCalculation);
	TEST_CASE_REGISTER(EvadtsCrcTest, testEvadtsChecker);
	TEST_CASE_REGISTER(EvadtsCrcTest, testEvadtsGenerator);
}

bool EvadtsCrcTest::testCrcCalculation() {
	const char data1[] =
//"DXS*JOF0000000*VA*V0/6*1\r\n" не входит в CRC
"ST*001*0001\r\n"
"ID1******0*6*1\r\n"
"CB1***G250_F02**0\r\n"
"ID4*1*7*   \r\n"
"ID5*20160113*154559\r\n"
"VA1*5186980*21285*0*0\r\n"
"VA2*14950*67*0*0\r\n"
"VA3*5760*25*0*0\r\n"
"CA1*JOF050287      *J2000MDB P32*9009\r\n"
"CA15*12450\r\n"
"CA17*2*10*67***1\r\n"
"CA17*3*20*54***0\r\n"
"CA17*4*50*114***1\r\n"
"CA17*6*100*50***0\r\n"
"CA2*5186980*21285*0*0\r\n"
"CA3*100*0*100**7755190*1502250*2557040**0*3695900\r\n"
"CA4*100*0*2544580*7100\r\n"
"CA8*0*100\r\n"
"CA10*0*30630\r\n"
"BA1*JOF61158       *BT10MDB     *0049\r\n"
"DA2*0*0*0*0\r\n"
"DA4*0*0\r\n"
"DA5*0**0\r\n"
"DB2*0*0*0*0\r\n"
"DB4*0*0\r\n"
"DB5*0**0\r\n"
"TA2*0*0*0*0\r\n"
"PA1*01*250*0\r\n"
"PA3*22**0\r\n"
"PA4*6**0\r\n"
"PA7*01*CA*0*250*880*114899*0*0\r\n"
"PA7*01*TA*0*250*0*4294967295*0*0\r\n"
"PA7*01*DA*1*100*0*4294967295*0*0\r\n"
"PA7*01*DB*1*100*0*4294967295*0*0\r\n"
"PA1*02*270*0\r\n"
"PA3*0**0\r\n"
"PA4*2**0\r\n"
"PA7*02*CA*0*270*1028*130879*0*0\r\n"
"PA7*02*TA*0*270*0*4294967295*0*0\r\n"
"PA7*02*DA*1*100*0*4294967295*0*0\r\n"
"PA7*02*DB*1*100*0*4294967295*0*0\r\n"
"PA1*03*250*0\r\n"
"PA3*3**0\r\n"
"PA4*1**0\r\n"
"PA7*03*CA*0*250*1674*280299*0*0\r\n"
"PA7*03*TA*0*250*0*4294967295*0*0\r\n"
"PA7*03*DA*1*100*0*4294967295*0*0\r\n"
"PA7*03*DB*1*100*0*4294967295*0*0\r\n"
"PA1*04*250*0\r\n"
"PA3*0**0\r\n"
"PA4*2**0\r\n"
"PA7*04*CA*0*250*383*41649*0*0\r\n"
"PA7*04*TA*0*250*0*4294967295*0*0\r\n"
"PA7*04*DA*1*100*0*4294967295*0*0\r\n"
"PA7*04*DB*1*100*0*4294967295*0*0\r\n"
"PA1*05*270*0\r\n"
"PA3*14**0\r\n"
"PA4*4**0\r\n"
"PA7*05*CA*0*270*2550*356069*0*0\r\n"
"PA7*05*TA*0*270*0*4294967295*0*0\r\n"
"PA7*05*DA*1*100*0*4294967295*0*0\r\n"
"PA7*05*DB*1*100*0*4294967295*0*0\r\n"
"PA1*06*300*0\r\n"
"PA3*2**0\r\n"
"PA4*1**0\r\n"
"PA7*06*CA*0*300*2790*670899*0*0\r\n"
"PA7*06*TA*0*300*0*4294967295*0*0\r\n"
"PA7*06*DA*1*100*0*4294967295*0*0\r\n"
"PA7*06*DB*1*100*0*4294967295*0*0\r\n"
"PA1*07*270*0\r\n"
"PA3*5**0\r\n"
"PA4*0**0\r\n"
"PA7*07*CA*0*270*1500*171119*0*0\r\n"
"PA7*07*TA*0*270*0*4294967295*0*0\r\n"
"PA7*07*DA*1*100*0*4294967295*0*0\r\n"
"PA7*07*DB*1*100*0*4294967295*0*0\r\n"
"PA1*08*300*0\r\n"
"PA3*3**0\r\n"
"PA4*0**0\r\n"
"PA7*08*CA*0*300*1601*318809*0*0\r\n"
"PA7*08*TA*0*300*0*4294967295*0*0\r\n"
"PA7*08*DA*1*100*0*4294967295*0*0\r\n"
"PA7*08*DB*1*100*0*4294967295*0*0\r\n"
"PA1*09*280*0\r\n"
"PA3*3**0\r\n"
"PA4*1**0\r\n"
"PA7*09*CA*0*280*1933*378249*0*0\r\n"
"PA7*09*TA*0*280*0*4294967295*0*0\r\n"
"PA7*09*DA*1*100*0*4294967295*0*0\r\n"
"PA7*09*DB*1*100*0*4294967295*0*0\r\n"
"PA1*10*320*0\r\n"
"PA3*1**0\r\n"
"PA4*2**0\r\n"
"PA7*10*CA*0*320*1594*205319*0*0\r\n"
"PA7*10*TA*0*320*0*4294967295*0*0\r\n"
"PA7*10*DA*1*100*0*4294967295*0*0\r\n"
"PA7*10*DB*1*100*0*4294967295*0*0\r\n"
"PA1*11*250*0\r\n"
"PA3*6**0\r\n"
"PA4*2**0\r\n"
"PA7*11*CA*0*250*1019*130859*0*0\r\n"
"PA7*11*TA*0*250*0*4294967295*0*0\r\n"
"PA7*11*DA*1*100*0*4294967295*0*0\r\n"
"PA7*11*DB*1*100*0*4294967295*0*0\r\n"
"PA1*12*260*0\r\n"
"PA3*4**0\r\n"
"PA4*2**0\r\n"
"PA7*12*CA*0*260*1478*250559*0*0\r\n"
"PA7*12*TA*0*260*0*4294967295*0*0\r\n"
"PA7*12*DA*1*100*0*4294967295*0*0\r\n"
"PA7*12*DB*1*100*0*4294967295*0*0\r\n"
"PA1*13*330*0\r\n"
"PA3*0**0\r\n"
"PA4*0**0\r\n"
"PA7*13*CA*0*330*1152*214349*0*0\r\n"
"PA7*13*TA*0*330*0*4294967295*0*0\r\n"
"PA7*13*DA*1*100*0*4294967295*0*0\r\n"
"PA7*13*DB*1*100*0*4294967295*0*0\r\n"
"PA1*14*280*0\r\n"
"PA3*4**0\r\n"
"PA4*1**0\r\n"
"PA7*14*CA*0*280*1284*235919*0*0\r\n"
"PA7*14*TA*0*280*0*4294967295*0*0\r\n"
"PA7*14*DA*1*100*0*4294967295*0*0\r\n"
"PA7*14*DB*1*100*0*4294967295*0*0\r\n"
"PA1*15*250*0\r\n"
"PA3*0**0\r\n"
"PA4*1**0\r\n"
"PA7*15*CA*0*250*419*52199*0*0\r\n"
"PA7*15*TA*0*250*0*4294967295*0*0\r\n"
"PA7*15*DA*1*100*0*4294967295*0*0\r\n"
"PA7*15*DB*1*100*0*4294967295*0*0\r\n"
"EA3**20160113*154559******6*5\r\n"
"MA5*ERROR*0*6\r\n";
//"G85*04E3\r\n" не входит в CRC
//"SE*136*0001\r\n"
//"DXE*1*1\r\n"

	Dex::Crc crc;
	crc.start();
	crc.add(data1);
	TEST_NUMBER_EQUAL(0x04, crc.getHighByte());
	TEST_NUMBER_EQUAL(0xE3, crc.getLowByte());
	return true;
}

bool EvadtsCrcTest::testEvadtsChecker() {
	const char data1[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"ID1******0*6*1\r\n"
"ID4*1*7\r\n"
"PA1*01*250*^DD^^F1^^EF^^F0^^E5^^F1^^F1^^EE^\r\n"
"PA7*01*CA*0*250*880*114899*0*0\r\n"
"PA7*01*TA*0*250*0*4294967295*0*0\r\n"
"PA7*01*DA*1*100*0*4294967295*0*0\r\n"
"PA7*01*DB*1*100*0*4294967295*0*0\r\n"
"PA1*02*250*^CA^^E0^^EF^^F3^^F7^^E8^^ED^^EE^\r\n"
"PA7*02*CA*0*250*1674*280299*0*0\r\n"
"PA7*02*TA*0*250*0*4294967295*0*0\r\n"
"PA7*02*DA*1*100*0*4294967295*0*0\r\n"
"PA7*02*DB*1*100*0*4294967295*0*0\r\n"
"G85*0675\r\n"
"SE*15*0001\r\n"
"DXE*1*1\r\n";
	Evadts::Checker checker1;
	checker1.start();
	checker1.procData((uint8_t*)data1, strlen(data1));
	checker1.complete();
	TEST_NUMBER_EQUAL(false, checker1.hasError());

	const char data2[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"ID1******0*6*1\r\n"
"ID4*1*7\r\n"
"PA1*01*250*^DD^^F1^^EF^^F0^^E5^^F1^^F1^^EE^\r\n"
"PA7*01*CA*0*250*880*114899*0*0\r\n"
"PA7*01*TA*0*250*0*4294967295*0*0\r\n"
"PA7*01*DA*1*100*0*4294967295*0*0\r\n"
"PA7*01*DB*1*100*0*4294967295*0*0\r\n"
"PA1*02*250*^CA^^E0^^EF^^F3^^F7^^E8^^ED^^EE^\r\n"
"PA7*02*CA*0*250*1674*280299*0*0\r\n"
"PA7*02*TA*0*250*0*4294967295*0*0\r\n"
"PA7*02*DA*1*100*0*4294967295*0*0\r\n"
"PA7*02*DB*1*100*0*4294967295*0*0\r\n"
"G85*0676\r\n"
"SE*15*0001\r\n"
"DXE*1*1\r\n";
	Evadts::Checker checker2;
	checker2.start();
	checker2.procData((uint8_t*)data2, strlen(data2));
	checker2.complete();
	TEST_NUMBER_EQUAL(true, checker2.hasError());
	return true;
}

class TestEvadtsGenerator : public EvadtsGenerator {
public:
	TestEvadtsGenerator() : EvadtsGenerator("JOF0000000") {}
	virtual void reset() {
		state = State_Header;
		next();
	}

	virtual void next() {
		switch (state) {
		case State_Header: {
			generateHeader();
			state = State_Main;
			break;
		}
		case State_Main: generateMain(); break;
		case State_Product1: generateProduct1(); break;
		case State_Product2: generateProduct2(); break;
		case State_Footer: {
			generateFooter();
			state = State_Complete;
			break;
		}
		case State_Complete: break;
		}
	}

	virtual bool isLast() {
		return state == State_Complete;
	}

private:
	enum State {
		State_Header = 0,
		State_Main,
		State_Product1,
		State_Product2,
		State_Footer,
		State_Complete
	};

	State state;

	void generateMain() {
		startBlock();
		*str << "ID1******0*6*1"; finishLine();
		*str << "ID4*1*7"; finishLine();
		finishBlock();
		state = State_Product1;
	}

	void generateProduct1() {
		startBlock();
		*str << "PA1*01*250*"; win1251ToLatin("Ёспрессо"); finishLine();
		*str << "PA7*01*CA*0*250*880*114899*0*0"; finishLine();
		*str << "PA7*01*TA*0*250*0*4294967295*0*0"; finishLine();
		*str << "PA7*01*DA*1*100*0*4294967295*0*0"; finishLine();
		*str << "PA7*01*DB*1*100*0*4294967295*0*0"; finishLine();
		finishBlock();
		state = State_Product2;
	}

	void generateProduct2() {
		startBlock();
		*str << "PA1*02*250*"; win1251ToLatin(" апучино"); finishLine();
		*str << "PA7*02*CA*0*250*1674*280299*0*0"; finishLine();
		*str << "PA7*02*TA*0*250*0*4294967295*0*0"; finishLine();
		*str << "PA7*02*DA*1*100*0*4294967295*0*0"; finishLine();
		*str << "PA7*02*DB*1*100*0*4294967295*0*0"; finishLine();
		finishBlock();
		state = State_Footer;
	}

};

bool EvadtsCrcTest::testEvadtsGenerator() {
	Buffer buf(4000);
	TestEvadtsGenerator generator;
	generator.reset();
	buf.add(generator.getData(), generator.getLen());
	while(generator.isLast() == false) {
		generator.next();
		buf.add(generator.getData(), generator.getLen());
	}

	const char expected1[] =
"DXS*JOF0000000*VA*V0/6*1\r\n"
"ST*001*0001\r\n"
"ID1******0*6*1\r\n"
"ID4*1*7\r\n"
"PA1*01*250*^DD^^F1^^EF^^F0^^E5^^F1^^F1^^EE^\r\n"
"PA7*01*CA*0*250*880*114899*0*0\r\n"
"PA7*01*TA*0*250*0*4294967295*0*0\r\n"
"PA7*01*DA*1*100*0*4294967295*0*0\r\n"
"PA7*01*DB*1*100*0*4294967295*0*0\r\n"
"PA1*02*250*^CA^^E0^^EF^^F3^^F7^^E8^^ED^^EE^\r\n"
"PA7*02*CA*0*250*1674*280299*0*0\r\n"
"PA7*02*TA*0*250*0*4294967295*0*0\r\n"
"PA7*02*DA*1*100*0*4294967295*0*0\r\n"
"PA7*02*DB*1*100*0*4294967295*0*0\r\n"
"G85*0675\r\n"
"SE*15*0001\r\n"
"DXE*1*1\r\n";
	TEST_SUBSTR_EQUAL(expected1, (const char *)buf.getData(), buf.getLen());
	return true;
}
