#include "lib/fiscal_register/chekonline/ChekOnlineCommandLayer.h"
#include "lib/fiscal_register/chekonline/ChekOnlineProtocol.h"

#include "memory/include/RamMemory.h"
#include "fiscal_register/TestFiscalRegisterEventEngine.h"
#include "timer/include/TimerEngine.h"
#include "timer/include/TestRealTime.h"
#include "sim900/include/GsmDriver.h"
#include "http/test/TestTcpIp.h"
#include "utils/include/TestLed.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

namespace ChekOnline {

class CommandLayerTest : public TestSet {
public:
	CommandLayerTest();
	bool init();
	void cleanup();
	bool testRegister();

private:
	StringBuilder *result;
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	ConfigModem *config;
	Fiscal::Context *context;
	TestTcpIp *conn;
	TestLed *leds;
	TimerEngine *timerEngine;
	TestFiscalRegisterEventEngine *eventEngine;
	CommandLayer *layer;

	bool gotoStatePollDelay();
};

TEST_SET_REGISTER(ChekOnline::CommandLayerTest);

CommandLayerTest::CommandLayerTest() {
	TEST_CASE_REGISTER(CommandLayerTest, testRegister);
}

bool CommandLayerTest::init() {
	this->result = new StringBuilder(2048, 2048);
	this->memory = new RamMemory(32000),
	this->realtime = new TestRealTime;
	this->stat = new StatStorage;
	this->config = new ConfigModem(memory, realtime, stat);
	this->config->getFiscal()->setINN("112233445566");
	this->config->getBoot()->setImei("888000888000888");
	this->context = new Fiscal::Context(2, realtime);
	this->conn = new TestTcpIp(1024, result, true);
	this->timerEngine = new TimerEngine;
	this->eventEngine = new TestFiscalRegisterEventEngine(result);
	this->leds = new TestLed(result);
	this->layer = new CommandLayer(config, context, conn, timerEngine, eventEngine, realtime, leds);
	return true;
}

void CommandLayerTest::cleanup() {
	delete this->layer;
	delete this->leds;
	delete this->eventEngine;
	delete this->timerEngine;
	delete this->conn;
	delete this->context;
	delete this->stat;
	delete this->realtime;
	delete this->memory;
	delete this->result;
}

bool CommandLayerTest::testRegister() {
	realtime->setUnixTimeStamp(2333444555);

	// connect
	Fiscal::Sale saleData;
	saleData.selectId.set("7");
	saleData.name.set("Тесточино");
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = 10000;
	saleData.price = 5000;
	saleData.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData.taxRate = Fiscal::TaxRate_NDSNone;
	layer->sale(&saleData);
	TEST_STRING_EQUAL("<ledFiscal=1><connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect error
	realtime->setUnixTimeStamp(3444555666);
	Event event1(TcpIp::Event_ConnectOk);
	layer->proc(&event1);
	TEST_STRING_EQUAL("<send="
		"POST /fr/api/v2/Complex HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 317\r\n"
		"\r\n"
		"{\"Device\": \"auto\""
		",\"ClientId\": \"\""
		",\"Password\": 1"
		",\"RequestId\": \"8880008880008883444555666\""
		",\"Lines\": ["
		"{\"Qty\": 1000"
		",\"Price\": 5000"
		",\"PayAttribute\": 4"
		",\"TaxId\": 4"
		",\"Description\": \"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		"}]"
		",\"Cash\": 5000"
		",\"TaxMode\": 8"
		",\"Terminal\": \"\""
		",\"Place\": \"\""
		",\"Address\": \"\""
		",\"FullResponse\": false}"
		",len=444>",
		result->getString());
	result->clear();

	// send complete
	Event event2(TcpIp::Event_SendDataOk);
	layer->proc(&event2);
	TEST_STRING_EQUAL("<recv=2000>", result->getString());
	result->clear();

	// recv response
	const char resp3[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.12.0\r\n"
		"Date: Sun, 19 Aug 2018 13:43:42 GMT\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 594\r\n"
		"Connection: keep-alive\r\n"
		"X-Device-Address: 192.168.142.20:4051\r\n"
		"X-Device-Name: 10000000000000000051\r\n"
		"\r\n"
		"{\"ClientId\":\"690209812752\",\"Date\":{\"Date\":{\"Day\":19,\"Month\":8,\"Year\":18},\"Time\":{\"Hour\":16,\"Minute\":44,\"Second\":43}}"
		",\"Device\":{\"Name\":\"10000000000000000051\",\"Address\":\"192.168.142.20:4051\"}"
		",\"DeviceRegistrationNumber\":\"2505480089018141\""
		",\"DeviceSerialNumber\":\"10000000000000000051\""
		",\"DocNumber\":2"
		",\"DocumentType\":0"
		",\"FNSerialNumber\":\"9999999999999051\""
		",\"FiscalDocNumber\":4"
		",\"FiscalSign\":259844891"
		",\"GrandTotal\":123"
		",\"Path\":\"/fr/api/v2/Complex\""
		",\"QR\":\"t=20180819T1644\\u0026s=1.23\\u0026fn=9999999999999051\\u0026i=4\\u0026fp=259844891\\u0026n=1\""
		",\"RequestId\":\"8683450318545401534697027\""
		",\"Response\":{\"Error\":0}}";
	conn->addRecvString(resp3, true);
	TEST_STRING_EQUAL("<ledFiscal=2><close>", result->getString());
	result->clear();

	// send complete
	Event event10(TcpIp::Event_Close);
	layer->proc(&event10);
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	result->clear();
	TEST_NUMBER_EQUAL(2505480089018141, saleData.fiscalRegister);
	TEST_NUMBER_EQUAL(9999999999999051, saleData.fiscalStorage);
	TEST_NUMBER_EQUAL(4, saleData.fiscalDocument);
	TEST_NUMBER_EQUAL(259844891, saleData.fiscalSign);
	return true;
}

}
