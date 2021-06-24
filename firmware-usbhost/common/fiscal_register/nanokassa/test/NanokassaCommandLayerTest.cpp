#include "lib/fiscal_register/nanokassa/NanokassaCommandLayer.h"
#include "lib/fiscal_register/nanokassa/NanokassaProtocol.h"

#include "memory/include/RamMemory.h"
#include "fiscal_register/TestFiscalRegisterEventEngine.h"
#include "timer/include/TimerEngine.h"
#include "timer/include/TestRealTime.h"
#include "sim900/include/GsmDriver.h"
#include "http/test/TestTcpIp.h"
#include "utils/include/TestLed.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

namespace Nanokassa {

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

TEST_SET_REGISTER(Nanokassa::CommandLayerTest);

CommandLayerTest::CommandLayerTest() {
	TEST_CASE_REGISTER(CommandLayerTest, testRegister);
}

bool CommandLayerTest::init() {
	this->result = new StringBuilder(2048, 2048);
	this->memory = new RamMemory(32000),
	this->realtime = new TestRealTime;
	this->stat = new StatStorage;
	this->config = new ConfigModem(memory, realtime, stat);
	this->config->init();
	this->config->getFiscal()->setINN("112233445566");
	this->config->getBoot()->setImei("888000888000888");
	this->config->getFiscal()->setKktAddr("nanokassa.ru");
	this->config->getFiscal()->setKktPort(80);
	this->config->getFiscal()->setPointName("PointName");
	this->config->getFiscal()->setPointAddr("PointAddr");
	this->config->getFiscal()->setAutomatNumber("AutomatNumber");
	StringBuilder kassaid = "122597:1c8842acf20370f1823e434545b7c65d";
	this->config->getFiscal()->getSignPrivateKey()->save(&kassaid);
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
	TEST_STRING_EQUAL("<subscribe=3072><ledFiscal=1><connect:q.nanokassa.ru,80,0>", result->getString());
	result->clear();

	// connect error
	realtime->setUnixTimeStamp(3444555666);
	Event event1(TcpIp::Event_ConnectOk);
	layer->proc(&event1);
	TEST_STRING_EQUAL("<send="
		"POST /srv/gws/gw17.php HTTP/1.1\r\n"
		"Host: q.nanokassa.ru:80\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 330\r\n"
		"\r\n"
		"{\"id\": \"8880008880008882333444555\""
		",\"unit-id\": 1234"
		",\"token\": \"122597:1c8842acf20370f1823e434545b7c65d\""
		",\"address\": \"PointAddr\""
		",\"place\": \"PointName\""
		",\"vm-id\": \"AutomatNumber\""
		",\"items\": ["
		"{\"name\": \"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
	    ",\"price\": 5000"
		",\"amount\": 1000"
		",\"vat\": 6"
		"}]"
		",\"tax_scheme\": 8"
		",\"cash\": 5000"
		",\"cashless\": 0"
		"}"
		",len=458>",
		result->getString());
	result->clear();

	// send complete
	Event event2(TcpIp::Event_SendDataOk);
	layer->proc(&event2);
	TEST_STRING_EQUAL("<recv=2000>", result->getString());
	result->clear();

	// recv response
	const char resp1[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.12.0\r\n"
		"Date: Sun, 19 Aug 2018 13:43:42 GMT\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 0\r\n"
		"Connection: keep-alive\r\n"
		"\r\n";
	conn->addRecvString(resp1, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event4(TcpIp::Event_Close);
	layer->proc(&event4);
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	timerEngine->tick(NANOKASSA_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:fp.nanokassa.com,80,0>", result->getString());
	result->clear();

	// connect complete
	Event event5(TcpIp::Event_ConnectOk);
	layer->proc(&event5);
	TEST_STRING_EQUAL("<send="
		"GET /st_hshuihqhvzvav_ss/receipt/8880008880008882333444555 HTTP/1.1\r\n"
		"Host: fp.nanokassa.com:80\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 0\r\n"
		"\r\n"
		",len=164>",
		result->getString());
	result->clear();

	// send complete
	Event event6(TcpIp::Event_SendDataOk);
	layer->proc(&event6);
	TEST_STRING_EQUAL("<recv=2000>", result->getString());
	result->clear();

	// recv response
	const char resp2[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.10.3\r\n"
		"Date: Thu, 29 Nov 2018 16:06:12 GMT\r\n"
		"Content-Length: 12\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"{\"qr\": null}";
	conn->addRecvString(resp2, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event7(TcpIp::Event_Close);
	layer->proc(&event7);
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	timerEngine->tick(NANOKASSA_TRY_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:fp.nanokassa.com,80,0>", result->getString());
	result->clear();

	// connect complete
	Event event8(TcpIp::Event_ConnectOk);
	layer->proc(&event8);
	TEST_STRING_EQUAL("<send="
		"GET /st_hshuihqhvzvav_ss/receipt/8880008880008882333444555 HTTP/1.1\r\n"
		"Host: fp.nanokassa.com:80\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 0\r\n"
		"\r\n"
		",len=164>",
		result->getString());
	result->clear();

	// send complete
	Event event9(TcpIp::Event_SendDataOk);
	layer->proc(&event9);
	TEST_STRING_EQUAL("<recv=2000>", result->getString());
	result->clear();

	// recv response
	const char resp3[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.10.3\r\n"
		"Date: Thu, 29 Nov 2018 17:06:54 GMT\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 78\r\n"
		"\r\n"
		"{\"qr\": \"t=20180720T1638&s=50.00&fn=9999078900005419&i=1866&fp=3326875305&n=2\"}\r\n"
		"0";
	conn->addRecvString(resp3, true);
	TEST_STRING_EQUAL("<ledFiscal=2><close>", result->getString());
	result->clear();

	// send complete
	Event event10(TcpIp::Event_Close);
	layer->proc(&event10);
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	result->clear();
	TEST_NUMBER_EQUAL(0, saleData.fiscalRegister);
	StringBuilder fiscalDatetime;
	datetime2string(&saleData.fiscalDatetime, &fiscalDatetime);
	TEST_STRING_EQUAL("2018-07-20 16:38:00", fiscalDatetime.getString());
	TEST_NUMBER_EQUAL(9999078900005419, saleData.fiscalStorage);
	TEST_NUMBER_EQUAL(1866, saleData.fiscalDocument);
	TEST_NUMBER_EQUAL(3326875305, saleData.fiscalSign);
	return true;
}

}
