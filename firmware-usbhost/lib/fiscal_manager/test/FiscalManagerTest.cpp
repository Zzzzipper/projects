#include "lib/fiscal_manager/include/FiscalManager.h"

#include "common/memory/include/RamMemory.h"
#include "common/config/include/ConfigModem.h"
#include "common/timer/include/TestRealTime.h"
#include "common/utils/include/TestEventObserver.h"
#include "common/event/include/TestEventEngine.h"
#include "common/fiscal_register/TestFiscalRegisterEventEngine.h"
#include "common/fiscal_register/include/TestFiscalRegister.h"
#include "common/test/include/Test.h"
#include "common/logger/include/Logger.h"

class TestQrCodePrinter : public QrCodeInterface {
public:
	TestQrCodePrinter(StringBuilder *result) : result(result) {}
	bool drawQrCode(const char *header, const char *footer, const char *text) override {
		(void)header;
		(void)footer;
		(void)text;
		*result << "<qrcode>";
		return true;
	}

private:
	StringBuilder *result;
};

class FiscalManagerTest : public TestSet {
public:
	FiscalManagerTest();
	bool init();
	void cleanup();
	bool testSingleCheck();
	bool testFreeSale();
	bool testOverflowCheck();
	bool testCheckError();
	bool testTokenCheck();
	bool testEphorOnline();

private:
	StringBuilder *result;
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	ConfigModem *config;
	TimerEngine *timerEngine;
	TestFiscalRegisterEventEngine *eventEngine;
	TestFiscalRegister *fiscalRegister;
	TestQrCodePrinter *qrCodePrinter;
	Fiscal::Manager *manager;
};

TEST_SET_REGISTER(FiscalManagerTest);

FiscalManagerTest::FiscalManagerTest() {
	TEST_CASE_REGISTER(FiscalManagerTest, testSingleCheck);
	TEST_CASE_REGISTER(FiscalManagerTest, testFreeSale);
	TEST_CASE_REGISTER(FiscalManagerTest, testOverflowCheck);
	TEST_CASE_REGISTER(FiscalManagerTest, testCheckError);
	TEST_CASE_REGISTER(FiscalManagerTest, testTokenCheck);
	TEST_CASE_REGISTER(FiscalManagerTest, testEphorOnline);
}

bool FiscalManagerTest::init() {
	result = new StringBuilder(1024, 1024);
	memory = new RamMemory(32000),
	realtime = new TestRealTime;
	stat = new StatStorage;
	config = new ConfigModem(memory, realtime, stat);
	config->init();
	timerEngine = new TimerEngine;
	eventEngine = new TestFiscalRegisterEventEngine(result);
	fiscalRegister = new TestFiscalRegister(1, result);
	qrCodePrinter = new TestQrCodePrinter(result);
	manager = new Fiscal::Manager(config, timerEngine, eventEngine, fiscalRegister, qrCodePrinter);
	return true;
}

void FiscalManagerTest::cleanup() {
	delete manager;
	delete qrCodePrinter;
	delete fiscalRegister;
	delete eventEngine;
	delete timerEngine;
	delete config;
	delete realtime;
	delete memory;
	delete result;
}

bool FiscalManagerTest::testSingleCheck() {
	// sale
	Fiscal::Sale saleData;
	saleData.selectId.set("7");
	saleData.name.set("Тесточино1");
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = 10000;
	saleData.price = 5000;
	saleData.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData.taxRate = Fiscal::TaxRate_NDSNone;
	manager->sale(&saleData, 2);
	TEST_STRING_EQUAL("<subscribe=3840><FR::sale(Тесточино1,CA/0,5000,10000,2)><event=CommandOK>", result->getString());
	result->clear();

	// fiscal complete
	fiscalRegister->saleComplete();
	EventEnvelope envelope(50);
	EventInterface event(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, event.pack(&envelope));
	manager->proc(&envelope);
	TEST_STRING_EQUAL("", result->getString());

	ConfigEventIterator iterator(config->getEvents());
	TEST_NUMBER_EQUAL(true, iterator.first());
	ConfigEventSale *saleEvent = iterator.getSale();
	TEST_STRING_EQUAL("Тесточино1", saleEvent->name.get());
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalRegister);
	TEST_NUMBER_EQUAL(Fiscal::Status_Complete, saleEvent->fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalSign);
	TEST_NUMBER_EQUAL(false, iterator.next());
	return true;
}

bool FiscalManagerTest::testFreeSale() {
	TEST_STRING_EQUAL("<subscribe=3840>", result->getString());
	result->clear();

	// sale1
	Fiscal::Sale saleData1;
	saleData1.selectId.set("7");
	saleData1.name.set("Тесточино1");
	saleData1.device.set("CA");
	saleData1.priceList = 0;
	saleData1.paymentType = Fiscal::Payment_Cash;
	saleData1.credit = 0;
	saleData1.price = 0;
	saleData1.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData1.taxRate = Fiscal::TaxRate_NDSNone;
	realtime->setDateTime("2019-06-11 11:30:00");
	manager->sale(&saleData1, 2);
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	result->clear();

	// sale2
	Fiscal::Sale saleData2;
	saleData2.selectId.set("7");
	saleData2.name.set("Тесточино2");
	saleData2.device.set("CA");
	saleData2.priceList = 0;
	saleData2.paymentType = Fiscal::Payment_Cash;
	saleData2.credit = 10000;
	saleData2.price = 4000;
	saleData2.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData2.taxRate = Fiscal::TaxRate_NDSNone;
	realtime->setDateTime("2019-06-11 11:40:00");
	manager->sale(&saleData2, 2);
	TEST_STRING_EQUAL("<FR::sale(Тесточино2,CA/0,4000,10000,2)><event=CommandOK>", result->getString());
	result->clear();

	// sale3
	Fiscal::Sale saleData3;
	saleData3.selectId.set("7");
	saleData3.name.set("Тесточино3");
	saleData3.device.set("CA");
	saleData3.priceList = 0;
	saleData3.paymentType = Fiscal::Payment_Cash;
	saleData3.credit = 0;
	saleData3.price = 0;
	saleData3.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData3.taxRate = Fiscal::TaxRate_NDSNone;
	realtime->setDateTime("2019-06-11 11:50:00");
	manager->sale(&saleData3, 2);
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	result->clear();

	// sale2 complete
	fiscalRegister->saleComplete(0, 9999078900005419, 1866, 3326875305);
	EventEnvelope envelope1(50);
	EventInterface event1(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, event1.pack(&envelope1));
	manager->proc(&envelope1);
	TEST_STRING_EQUAL("<qrcode>", result->getString());
	result->clear();

	ConfigEventIterator iterator(config->getEvents());
	TEST_NUMBER_EQUAL(true, iterator.first());
	ConfigEventSale *saleEvent = iterator.getSale();
	TEST_STRING_EQUAL("Тесточино1", saleEvent->name.get());
	TEST_DATETIME_EQUAL("2019-06-11 11:30:00", iterator.getDate());
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalRegister);
	TEST_NUMBER_EQUAL(Fiscal::Status_None, saleEvent->fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalSign);

	TEST_NUMBER_EQUAL(true, iterator.next());
	saleEvent = iterator.getSale();
	TEST_STRING_EQUAL("Тесточино3", saleEvent->name.get());
	TEST_DATETIME_EQUAL("2019-06-11 11:50:00", iterator.getDate())
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalRegister);
	TEST_NUMBER_EQUAL(Fiscal::Status_None, saleEvent->fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalSign);

	TEST_NUMBER_EQUAL(true, iterator.next());
	saleEvent = iterator.getSale();
	TEST_STRING_EQUAL("Тесточино2", saleEvent->name.get());
	TEST_DATETIME_EQUAL("2019-06-11 11:40:00", iterator.getDate())
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalRegister);
	TEST_NUMBER_EQUAL(9999078900005419, saleEvent->fiscalStorage);
	TEST_NUMBER_EQUAL(1866, saleEvent->fiscalDocument);
	TEST_NUMBER_EQUAL(3326875305, saleEvent->fiscalSign);

	TEST_NUMBER_EQUAL(false, iterator.next());
	return true;
}

bool FiscalManagerTest::testOverflowCheck() {
	// sale1
	Fiscal::Sale saleData1;
	saleData1.selectId.set("7");
	saleData1.name.set("Тесточино1");
	saleData1.device.set("CA");
	saleData1.priceList = 0;
	saleData1.paymentType = Fiscal::Payment_Cash;
	saleData1.credit = 10000;
	saleData1.price = 5000;
	saleData1.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData1.taxRate = Fiscal::TaxRate_NDSNone;
	manager->sale(&saleData1, 2);
	TEST_STRING_EQUAL("<subscribe=3840><FR::sale(Тесточино1,CA/0,5000,10000,2)><event=CommandOK>", result->getString());
	result->clear();

	// sale2
	Fiscal::Sale saleData2;
	saleData2.selectId.set("7");
	saleData2.name.set("Тесточино2");
	saleData2.device.set("CA");
	saleData2.priceList = 0;
	saleData2.paymentType = Fiscal::Payment_Cash;
	saleData2.credit = 10000;
	saleData2.price = 4000;
	saleData2.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData2.taxRate = Fiscal::TaxRate_NDSNone;
	manager->sale(&saleData2, 2);
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	result->clear();

	// sale3
	Fiscal::Sale saleData3;
	saleData3.selectId.set("7");
	saleData3.name.set("Тесточино3");
	saleData3.device.set("CA");
	saleData3.priceList = 0;
	saleData3.paymentType = Fiscal::Payment_Cash;
	saleData3.credit = 5000;
	saleData3.price = 3000;
	saleData3.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData3.taxRate = Fiscal::TaxRate_NDSNone;
	manager->sale(&saleData3, 2);
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	result->clear();

	// sale4
	Fiscal::Sale saleData4;
	saleData4.selectId.set("7");
	saleData4.name.set("Тесточино4");
	saleData4.device.set("CA");
	saleData4.priceList = 0;
	saleData4.paymentType = Fiscal::Payment_Cash;
	saleData4.credit = 5000;
	saleData4.price = 3000;
	saleData4.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData4.taxRate = Fiscal::TaxRate_NDSNone;
	manager->sale(&saleData4, 2);
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	result->clear();

	// sale5
	Fiscal::Sale saleData5;
	saleData5.selectId.set("7");
	saleData5.name.set("Тесточино5");
	saleData5.device.set("CA");
	saleData5.priceList = 0;
	saleData5.paymentType = Fiscal::Payment_Cash;
	saleData5.credit = 5000;
	saleData5.price = 3000;
	saleData5.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData5.taxRate = Fiscal::TaxRate_NDSNone;
	manager->sale(&saleData5, 2);
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	result->clear();

	// sale1 complete
	fiscalRegister->saleComplete();
	EventEnvelope envelope1(50);
	EventInterface event1(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, event1.pack(&envelope1));
	manager->proc(&envelope1);
	TEST_STRING_EQUAL("<FR::sale(Тесточино2,CA/0,4000,10000,2)>", result->getString());
	result->clear();

	// sale2 complete
	fiscalRegister->saleComplete();
	EventEnvelope envelope2(50);
	EventInterface event2(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, event2.pack(&envelope2));
	manager->proc(&envelope2);
	TEST_STRING_EQUAL("<FR::sale(Тесточино3,CA/0,3000,5000,2)>", result->getString());
	result->clear();

	// sale3 complete
	fiscalRegister->saleComplete();
	EventEnvelope envelope3(50);
	EventInterface event3(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, event3.pack(&envelope3));
	manager->proc(&envelope3);
	TEST_STRING_EQUAL("<FR::sale(Тесточино4,CA/0,3000,5000,2)>", result->getString());
	result->clear();

	// sale4 complete
	fiscalRegister->saleComplete();
	EventEnvelope envelope4(50);
	EventInterface event4(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, event4.pack(&envelope4));
	manager->proc(&envelope4);
	TEST_STRING_EQUAL("", result->getString());

	ConfigEventIterator iterator(config->getEvents());
	TEST_NUMBER_EQUAL(true, iterator.first());
	ConfigEventSale *saleEvent = iterator.getSale();
	TEST_STRING_EQUAL("Тесточино5", saleEvent->name.get());
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalRegister);
	TEST_NUMBER_EQUAL(Fiscal::Status_Overflow, saleEvent->fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalSign);

	TEST_NUMBER_EQUAL(true, iterator.next());
	saleEvent = iterator.getSale();
	TEST_STRING_EQUAL("Тесточино1", saleEvent->name.get());
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalRegister);
	TEST_NUMBER_EQUAL(Fiscal::Status_Complete, saleEvent->fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalSign);

	TEST_NUMBER_EQUAL(true, iterator.next());
	saleEvent = iterator.getSale();
	TEST_STRING_EQUAL("Тесточино2", saleEvent->name.get());
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalRegister);
	TEST_NUMBER_EQUAL(Fiscal::Status_Complete, saleEvent->fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalSign);

	TEST_NUMBER_EQUAL(true, iterator.next());
	saleEvent = iterator.getSale();
	TEST_STRING_EQUAL("Тесточино3", saleEvent->name.get());
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalRegister);
	TEST_NUMBER_EQUAL(Fiscal::Status_Complete, saleEvent->fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalSign);

	TEST_NUMBER_EQUAL(true, iterator.next());
	saleEvent = iterator.getSale();
	TEST_STRING_EQUAL("Тесточино4", saleEvent->name.get());
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalRegister);
	TEST_NUMBER_EQUAL(Fiscal::Status_Complete, saleEvent->fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalSign);

	TEST_NUMBER_EQUAL(false, iterator.next());
	return true;
}

bool FiscalManagerTest::testCheckError() {
	// sale
	realtime->setDateTime("2019-06-11 11:40:00");
	Fiscal::Sale saleData;
	saleData.selectId.set("7");
	saleData.name.set("Тесточино1");
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = 10000;
	saleData.price = 5000;
	saleData.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData.taxRate = Fiscal::TaxRate_NDSNone;
	manager->sale(&saleData, 2);
	TEST_STRING_EQUAL("<subscribe=3840><FR::sale(Тесточино1,CA/0,5000,10000,2)><event=CommandOK>", result->getString());
	result->clear();

	// fiscal complete
	realtime->setDateTime("2019-06-11 11:50:00");
	fiscalRegister->saleError();
	EventEnvelope envelope(50);
	Fiscal::EventError event;
	event.code = ConfigEvent::Type_FiscalUnknownError;
	event.data << 1234;
	TEST_NUMBER_EQUAL(true, event.pack(&envelope));
	manager->proc(&envelope);
	TEST_STRING_EQUAL("", result->getString());

	ConfigEventIterator iterator(config->getEvents());
	TEST_NUMBER_EQUAL(true, iterator.first());
	ConfigEventSale *saleEvent = iterator.getSale();
	TEST_STRING_EQUAL("Тесточино1", saleEvent->name.get());
	TEST_DATETIME_EQUAL("2019-06-11 11:40:00", iterator.getDate())
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalRegister);
	TEST_NUMBER_EQUAL(Fiscal::Status_Error, saleEvent->fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalSign);
	TEST_NUMBER_EQUAL(true, iterator.next());
	TEST_NUMBER_EQUAL(0x0300, iterator.getEvent()->getCode());
	TEST_STRING_EQUAL("1234", iterator.getEvent()->getString());
	TEST_NUMBER_EQUAL(false, iterator.next());
	return true;
}

bool FiscalManagerTest::testTokenCheck() {
	TEST_STRING_EQUAL("<subscribe=3840>", result->getString());
	result->clear();

	// sale1
	Fiscal::Sale saleData1;
	saleData1.selectId.set("7");
	saleData1.name.set("Тесточино1");
	saleData1.device.set("TA");
	saleData1.priceList = 0;
	saleData1.paymentType = Fiscal::Payment_Token;
	saleData1.credit = 10000;
	saleData1.price = 5000;
	saleData1.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData1.taxRate = Fiscal::TaxRate_NDSNone;
	realtime->setDateTime("2019-06-11 11:30:00");
	manager->sale(&saleData1, 2);
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	result->clear();

	// sale2
	Fiscal::Sale saleData2;
	saleData2.selectId.set("7");
	saleData2.name.set("Тесточино2");
	saleData2.device.set("CA");
	saleData2.priceList = 0;
	saleData2.paymentType = Fiscal::Payment_Cash;
	saleData2.credit = 10000;
	saleData2.price = 4000;
	saleData2.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData2.taxRate = Fiscal::TaxRate_NDSNone;
	realtime->setDateTime("2019-06-11 11:40:00");
	manager->sale(&saleData2, 2);
	TEST_STRING_EQUAL("<FR::sale(Тесточино2,CA/0,4000,10000,2)><event=CommandOK>", result->getString());
	result->clear();

	// sale3
	Fiscal::Sale saleData3;
	saleData3.selectId.set("7");
	saleData3.name.set("Тесточино3");
	saleData3.device.set("TA");
	saleData3.priceList = 0;
	saleData3.paymentType = Fiscal::Payment_Token;
	saleData3.credit = 5000;
	saleData3.price = 3000;
	saleData3.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData3.taxRate = Fiscal::TaxRate_NDSNone;
	realtime->setDateTime("2019-06-11 11:50:00");
	manager->sale(&saleData3, 2);
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	result->clear();

	// sale2 complete
	fiscalRegister->saleComplete(0, 9999078900005419, 1866, 3326875305);
	EventEnvelope envelope1(50);
	EventInterface event1(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, event1.pack(&envelope1));
	manager->proc(&envelope1);
	TEST_STRING_EQUAL("<qrcode>", result->getString());
	result->clear();

	ConfigEventIterator iterator(config->getEvents());
	TEST_NUMBER_EQUAL(true, iterator.first());
	ConfigEventSale *saleEvent = iterator.getSale();
	TEST_STRING_EQUAL("Тесточино1", saleEvent->name.get());
	TEST_DATETIME_EQUAL("2019-06-11 11:30:00", iterator.getDate());
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalRegister);
	TEST_NUMBER_EQUAL(Fiscal::Status_None, saleEvent->fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalSign);

	TEST_NUMBER_EQUAL(true, iterator.next());
	saleEvent = iterator.getSale();
	TEST_STRING_EQUAL("Тесточино3", saleEvent->name.get());
	TEST_DATETIME_EQUAL("2019-06-11 11:50:00", iterator.getDate())
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalRegister);
	TEST_NUMBER_EQUAL(Fiscal::Status_None, saleEvent->fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalSign);

	TEST_NUMBER_EQUAL(true, iterator.next());
	saleEvent = iterator.getSale();
	TEST_STRING_EQUAL("Тесточино2", saleEvent->name.get());
	TEST_DATETIME_EQUAL("2019-06-11 11:40:00", iterator.getDate())
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalRegister);
	TEST_NUMBER_EQUAL(9999078900005419, saleEvent->fiscalStorage);
	TEST_NUMBER_EQUAL(1866, saleEvent->fiscalDocument);
	TEST_NUMBER_EQUAL(3326875305, saleEvent->fiscalSign);

	TEST_NUMBER_EQUAL(false, iterator.next());
	return true;
}

bool FiscalManagerTest::testEphorOnline() {
	fiscalRegister->setRemoteFiscal(true);
	TEST_NUMBER_EQUAL(0, config->getEvents()->getLen());

	// sale
	Fiscal::Sale saleData;
	saleData.selectId.set("7");
	saleData.name.set("Тесточино1");
	saleData.device.set("DA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Ephor;
	saleData.credit = 5000;
	saleData.price = 5000;
	saleData.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData.taxRate = Fiscal::TaxRate_NDSNone;
	manager->sale(&saleData, 2);
	TEST_STRING_EQUAL("<subscribe=3840><event=CommandOK>", result->getString());
	result->clear();

	// fiscal complete
	fiscalRegister->saleComplete();
	EventEnvelope envelope(50);
	EventInterface event(Fiscal::Register::Event_CommandOK);
	TEST_NUMBER_EQUAL(true, event.pack(&envelope));
	manager->proc(&envelope);
	TEST_STRING_EQUAL("", result->getString());

	ConfigEventIterator iterator(config->getEvents());
	TEST_NUMBER_EQUAL(true, iterator.first());
	ConfigEventSale *saleEvent = iterator.getSale();
	TEST_STRING_EQUAL("Тесточино1", saleEvent->name.get());
	TEST_STRING_EQUAL("DA", saleEvent->device.get());
	TEST_NUMBER_EQUAL(0, saleEvent->priceList);
	TEST_NUMBER_EQUAL(5000, saleEvent->price);
	TEST_NUMBER_EQUAL(Fiscal::TaxSystem_None, saleEvent->taxSystem);
	TEST_NUMBER_EQUAL(Fiscal::TaxRate_NDSNone, saleEvent->taxRate);
	TEST_NUMBER_EQUAL(Fiscal::Status_None, saleEvent->fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleEvent->fiscalSign);
	TEST_NUMBER_EQUAL(false, iterator.next());
	return true;
}
