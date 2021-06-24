#include "lib/fiscal_register/ephor/EphorCommandLayer.h"
#include "lib/fiscal_register/ephor/EphorProtocol.h"

#include "memory/include/RamMemory.h"
#include "fiscal_register/TestFiscalRegisterEventEngine.h"
#include "timer/include/TimerEngine.h"
#include "timer/include/TestRealTime.h"
#include "sim900/include/GsmDriver.h"
#include "http/test/TestTcpIp.h"
#include "utils/include/TestLed.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

namespace Ephor {

class CommandLayerTest : public TestSet {
public:
	CommandLayerTest();
	bool init();
	void cleanup();
	bool testRegister();
	bool testRegRepeat();
	bool testRegTooManyTries();
	bool testPollError();
	bool testPollTooManyTries();

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

TEST_SET_REGISTER(Ephor::CommandLayerTest);

CommandLayerTest::CommandLayerTest() {
	TEST_CASE_REGISTER(CommandLayerTest, testRegister);
	TEST_CASE_REGISTER(CommandLayerTest, testRegRepeat);
	TEST_CASE_REGISTER(CommandLayerTest, testRegTooManyTries);
	TEST_CASE_REGISTER(CommandLayerTest, testPollError);
	TEST_CASE_REGISTER(CommandLayerTest, testPollTooManyTries);
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
	this->config->getBoot()->setServerDomain("api.ephor.online");
	this->config->getBoot()->setServerPort(80);
	this->config->getBoot()->setServerPassword("1234567");
	this->config->getFiscal()->setKktAddr("apip.orangedata.ru");
	this->config->getFiscal()->setKktPort(12001);
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
	saleData.wareId = 5678;
	saleData.name.set("Тесточино");
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = 10000;
	saleData.price = 5000;
	saleData.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData.taxRate = Fiscal::TaxRate_NDSNone;
	layer->sale(&saleData);
	TEST_STRING_EQUAL("<subscribe=3072><ledFiscal=1><connect:api.ephor.online,80,1>", result->getString());
	result->clear();

	// connect error
	realtime->setUnixTimeStamp(3444555666);
	conn->connectComplete();
	TEST_STRING_EQUAL("<send="
		"POST /api/1.0/Fiscal.php?action=Reg&login=888000888000888&password=1234567&_dc=2333444555 HTTP/1.1\r\n"
		"Host: api.ephor.online:80\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 329\r\n"
		"Cache-Control: no-cache\r\n"
		"\r\n,len=229>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<send="
		"{\"date\":\"2000-01-02 10:20:30\""
		",\"point_addr\":\"PointAddr\""
		",\"point_name\":\"PointName\""
		",\"automat_number\":\"AutomatNumber\""
		",\"items\":["
		"{\"select_id\":\"7\""
		",\"ware_id\":5678"
		",\"name\":\"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		",\"device\":\"CA\""
		",\"price_list\":0"
		",\"price\":5000"
		",\"amount\":1000"
		",\"tax_rate\":0"
		"}]"
		",\"tax_system\":8"
		",\"cash\":5000"
		",\"cashless\":0"
		"},len=329>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("", result->getString());

	// recv data
	conn->incommingData();
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();

	// recv response
	const char resp1[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.12.0\r\n"
		"Date: Sun, 19 Aug 2018 13:43:42 GMT\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 36\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"{\"success\":true,\"id\":789,\"status\":7}";
	conn->addRecvString(resp1, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	conn->remoteClose();
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	realtime->setUnixTimeStamp(2333444666);
	timerEngine->tick(EPHOR_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:api.ephor.online,80,1>", result->getString());
	result->clear();

	// connect complete
	conn->connectComplete();
	TEST_STRING_EQUAL("<send="
		"GET /api/1.0/Fiscal.php?action=Get&login=888000888000888&password=1234567&_dc=2333444666 HTTP/1.1\r\n"
		"Host: api.ephor.online:80\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 12\r\n"
		"Cache-Control: no-cache\r\n"
		"\r\n,len=227>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<send="
		"{\"id\":\"789\"},len=12>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();

	// recv response
	const char resp2[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.10.3\r\n"
		"Date: Thu, 29 Nov 2018 16:06:12 GMT\r\n"
		"Content-Length: 38\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"{\"success\":true,\"id\":\"789\",\"status\":7}";
	conn->addRecvString(resp2, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	conn->remoteClose();
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	realtime->setUnixTimeStamp(2333444777);
	timerEngine->tick(EPHOR_TRY_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:api.ephor.online,80,1>", result->getString());
	result->clear();

	// connect complete
	conn->connectComplete();
	TEST_STRING_EQUAL("<send="
		"GET /api/1.0/Fiscal.php?action=Get&login=888000888000888&password=1234567&_dc=2333444777 HTTP/1.1\r\n"
		"Host: api.ephor.online:80\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 12\r\n"
		"Cache-Control: no-cache\r\n"
		"\r\n,len=227>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<send="
		"{\"id\":\"789\"},len=12>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();

	// recv response
	const char resp3[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.10.3\r\n"
		"Date: Thu, 29 Nov 2018 17:06:54 GMT\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 126\r\n"
		"\r\n"
		"{\"success\":true"
		",\"id\":789"
		",\"status\":1"
		",\"price\":5000"
		",\"date\":\"2018-07-20 16:38:00\""
		",\"fn\":9999078900005419"
		",\"fd\":1866"
		",\"fp\":3326875305"
		"}";
	conn->addRecvString(resp3, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	conn->remoteClose();
	TEST_STRING_EQUAL("<ledFiscal=2><event=CommandOK>", result->getString());
	result->clear();
	StringBuilder fiscalDatetime;
	datetime2string(&saleData.fiscalDatetime, &fiscalDatetime);
	TEST_STRING_EQUAL("2018-07-20 16:38:00", fiscalDatetime.getString());
	TEST_NUMBER_EQUAL(0, saleData.fiscalRegister);
	TEST_NUMBER_EQUAL(9999078900005419, saleData.fiscalStorage);
	TEST_NUMBER_EQUAL(1866, saleData.fiscalDocument);
	TEST_NUMBER_EQUAL(3326875305, saleData.fiscalSign);
	return true;
}

bool CommandLayerTest::testRegRepeat() {
	realtime->setUnixTimeStamp(2333444555);

	// connect
	Fiscal::Sale saleData;
	saleData.selectId.set("7");
	saleData.wareId = 5678;
	saleData.name.set("Тесточино");
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = 10000;
	saleData.price = 5000;
	saleData.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData.taxRate = Fiscal::TaxRate_NDSNone;
	layer->sale(&saleData);
	TEST_STRING_EQUAL("<subscribe=3072><ledFiscal=1><connect:api.ephor.online,80,1>", result->getString());
	result->clear();

	// connect complete
	realtime->setUnixTimeStamp(3444555666);
	conn->connectComplete();
	TEST_STRING_EQUAL("<send="
		"POST /api/1.0/Fiscal.php?action=Reg&login=888000888000888&password=1234567&_dc=2333444555 HTTP/1.1\r\n"
		"Host: api.ephor.online:80\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 329\r\n"
		"Cache-Control: no-cache\r\n"
		"\r\n,len=229>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<send="
		"{\"date\":\"2000-01-02 10:20:30\""
		",\"point_addr\":\"PointAddr\""
		",\"point_name\":\"PointName\""
		",\"automat_number\":\"AutomatNumber\""
		",\"items\":["
		"{\"select_id\":\"7\""
		",\"ware_id\":5678"
		",\"name\":\"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		",\"device\":\"CA\""
		",\"price_list\":0"
		",\"price\":5000"
		",\"amount\":1000"
		",\"tax_rate\":0"
		"}]"
		",\"tax_system\":8"
		",\"cash\":5000"
		",\"cashless\":0"
		"},len=329>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("", result->getString());

	// recv timeout
	conn->remoteClose();
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	realtime->setUnixTimeStamp(2333444777);
	timerEngine->tick(EPHOR_TRY_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:api.ephor.online,80,1>", result->getString());
	result->clear();

	// connect complete
	conn->connectComplete();
	TEST_STRING_EQUAL("<send="
		"POST /api/1.0/Fiscal.php?action=Reg&login=888000888000888&password=1234567&_dc=2333444555 HTTP/1.1\r\n"
		"Host: api.ephor.online:80\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 329\r\n"
		"Cache-Control: no-cache\r\n"
		"\r\n,len=229>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<send="
		"{\"date\":\"2000-01-02 10:20:30\""
		",\"point_addr\":\"PointAddr\""
		",\"point_name\":\"PointName\""
		",\"automat_number\":\"AutomatNumber\""
		",\"items\":["
		"{\"select_id\":\"7\""
		",\"ware_id\":5678"
		",\"name\":\"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		",\"device\":\"CA\""
		",\"price_list\":0"
		",\"price\":5000"
		",\"amount\":1000"
		",\"tax_rate\":0"
		"}]"
		",\"tax_system\":8"
		",\"cash\":5000"
		",\"cashless\":0"
		"},len=329>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("", result->getString());

	// recv data
	conn->incommingData();
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();

	// recv response
	const char resp1[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.12.0\r\n"
		"Date: Sun, 19 Aug 2018 13:43:42 GMT\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 36\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"{\"success\":true,\"id\":789,\"status\":7}";
	conn->addRecvString(resp1, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	conn->remoteClose();
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	realtime->setUnixTimeStamp(2333444888);
	timerEngine->tick(EPHOR_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:api.ephor.online,80,1>", result->getString());
	result->clear();

	// connect complete
	conn->connectComplete();
	TEST_STRING_EQUAL("<send="
		"GET /api/1.0/Fiscal.php?action=Get&login=888000888000888&password=1234567&_dc=2333444888 HTTP/1.1\r\n"
		"Host: api.ephor.online:80\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 12\r\n"
		"Cache-Control: no-cache\r\n"
		"\r\n,len=227>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<send="
		"{\"id\":\"789\"},len=12>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();

	// recv response
	const char resp3[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.10.3\r\n"
		"Date: Thu, 29 Nov 2018 17:06:54 GMT\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 126\r\n"
		"\r\n"
		"{\"success\":true"
		",\"id\":789"
		",\"status\":1"
		",\"price\":5000"
		",\"date\":\"2018-07-20 16:38:00\""
		",\"fn\":9999078900005419"
		",\"fd\":1866"
		",\"fp\":3326875305"
		"}";
	conn->addRecvString(resp3, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	conn->remoteClose();
	TEST_STRING_EQUAL("<ledFiscal=2><event=CommandOK>", result->getString());
	result->clear();
	StringBuilder fiscalDatetime;
	datetime2string(&saleData.fiscalDatetime, &fiscalDatetime);
	TEST_STRING_EQUAL("2018-07-20 16:38:00", fiscalDatetime.getString());
	TEST_NUMBER_EQUAL(0, saleData.fiscalRegister);
	TEST_NUMBER_EQUAL(9999078900005419, saleData.fiscalStorage);
	TEST_NUMBER_EQUAL(1866, saleData.fiscalDocument);
	TEST_NUMBER_EQUAL(3326875305, saleData.fiscalSign);
	return true;
}

bool CommandLayerTest::testRegTooManyTries() {
	realtime->setUnixTimeStamp(2333444555);

	// connect
	Fiscal::Sale saleData;
	saleData.selectId.set("7");
	saleData.wareId = 5678;
	saleData.name.set("Тесточино");
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = 10000;
	saleData.price = 5000;
	saleData.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData.taxRate = Fiscal::TaxRate_NDSNone;
	layer->sale(&saleData);
	TEST_STRING_EQUAL("<subscribe=3072><ledFiscal=1><connect:api.ephor.online,80,1>", result->getString());
	result->clear();

	// connect complete
	realtime->setUnixTimeStamp(3444555666);
	for(uint16_t i = 0; i < EPHOR_TRY_MAX; i++) {
		// connect complete
		conn->connectComplete();
		TEST_STRING_EQUAL("<send="
			"POST /api/1.0/Fiscal.php?action=Reg&login=888000888000888&password=1234567&_dc=2333444555 HTTP/1.1\r\n"
			"Host: api.ephor.online:80\r\n"
			"Content-Type: application/json; charset=windows-1251\r\n"
			"Content-Length: 329\r\n"
			"Cache-Control: no-cache\r\n"
			"\r\n,len=229>",
			result->getString());
		result->clear();

		// send complete
		conn->sendComplete();
		TEST_STRING_EQUAL("<send="
			"{\"date\":\"2000-01-02 10:20:30\""
			",\"point_addr\":\"PointAddr\""
			",\"point_name\":\"PointName\""
			",\"automat_number\":\"AutomatNumber\""
			",\"items\":["
			"{\"select_id\":\"7\""
			",\"ware_id\":5678"
			",\"name\":\"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
			",\"device\":\"CA\""
			",\"price_list\":0"
			",\"price\":5000"
			",\"amount\":1000"
			",\"tax_rate\":0"
			"}]"
			",\"tax_system\":8"
			",\"cash\":5000"
			",\"cashless\":0"
			"},len=329>",
			result->getString());
		result->clear();

		// send complete
		conn->sendComplete();
		TEST_STRING_EQUAL("", result->getString());

		// recv timeout
		conn->remoteClose();
		TEST_STRING_EQUAL("", result->getString());

		// timeout
		realtime->setUnixTimeStamp(2333444888);
		timerEngine->tick(EPHOR_TRY_DELAY);
		timerEngine->execute();
		TEST_STRING_EQUAL("<connect:api.ephor.online,80,1>", result->getString());
		result->clear();
	}

	// connect complete
	conn->connectComplete();
	TEST_STRING_EQUAL("<send="
		"POST /api/1.0/Fiscal.php?action=Reg&login=888000888000888&password=1234567&_dc=2333444555 HTTP/1.1\r\n"
		"Host: api.ephor.online:80\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 329\r\n"
		"Cache-Control: no-cache\r\n"
		"\r\n,len=229>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<send="
		"{\"date\":\"2000-01-02 10:20:30\""
		",\"point_addr\":\"PointAddr\""
		",\"point_name\":\"PointName\""
		",\"automat_number\":\"AutomatNumber\""
		",\"items\":["
		"{\"select_id\":\"7\""
		",\"ware_id\":5678"
		",\"name\":\"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		",\"device\":\"CA\""
		",\"price_list\":0"
		",\"price\":5000"
		",\"amount\":1000"
		",\"tax_rate\":0"
		"}]"
		",\"tax_system\":8"
		",\"cash\":5000"
		",\"cashless\":0"
		"},len=329>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("", result->getString());

	// recv timeout
	conn->remoteClose();
	TEST_STRING_EQUAL("<ledFiscal=3><event=CommandError,770>", result->getString());
	result->clear();
	StringBuilder fiscalDatetime;
	datetime2string(&saleData.fiscalDatetime, &fiscalDatetime);
	TEST_STRING_EQUAL("2001-01-01 00:00:00", fiscalDatetime.getString());
	TEST_NUMBER_EQUAL(0, saleData.fiscalRegister);
	TEST_NUMBER_EQUAL(Fiscal::Status_Error, saleData.fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleData.fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleData.fiscalSign);
	return true;
}

bool CommandLayerTest::testPollError() {
	realtime->setUnixTimeStamp(2333444555);

	// connect
	Fiscal::Sale saleData;
	saleData.selectId.set("7");
	saleData.wareId = 5678;
	saleData.name.set("Тесточино");
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = 10000;
	saleData.price = 5000;
	saleData.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData.taxRate = Fiscal::TaxRate_NDSNone;
	layer->sale(&saleData);
	TEST_STRING_EQUAL("<subscribe=3072><ledFiscal=1><connect:api.ephor.online,80,1>", result->getString());
	result->clear();

	// connect error
	realtime->setUnixTimeStamp(3444555666);
	conn->connectComplete();
	TEST_STRING_EQUAL("<send="
		"POST /api/1.0/Fiscal.php?action=Reg&login=888000888000888&password=1234567&_dc=2333444555 HTTP/1.1\r\n"
		"Host: api.ephor.online:80\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 329\r\n"
		"Cache-Control: no-cache\r\n"
		"\r\n,len=229>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<send="
		"{\"date\":\"2000-01-02 10:20:30\""
		",\"point_addr\":\"PointAddr\""
		",\"point_name\":\"PointName\""
		",\"automat_number\":\"AutomatNumber\""
		",\"items\":["
		"{\"select_id\":\"7\""
		",\"ware_id\":5678"
		",\"name\":\"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		",\"device\":\"CA\""
		",\"price_list\":0"
		",\"price\":5000"
		",\"amount\":1000"
		",\"tax_rate\":0"
		"}]"
		",\"tax_system\":8"
		",\"cash\":5000"
		",\"cashless\":0"
		"},len=329>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("", result->getString());

	// recv data
	conn->incommingData();
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();

	// recv response
	const char resp1[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.12.0\r\n"
		"Date: Sun, 19 Aug 2018 13:43:42 GMT\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 36\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"{\"success\":true,\"id\":789,\"status\":7}";
	conn->addRecvString(resp1, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	conn->remoteClose();
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	realtime->setUnixTimeStamp(2333444666);
	timerEngine->tick(EPHOR_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:api.ephor.online,80,1>", result->getString());
	result->clear();

	// connect complete
	conn->connectComplete();
	TEST_STRING_EQUAL("<send="
		"GET /api/1.0/Fiscal.php?action=Get&login=888000888000888&password=1234567&_dc=2333444666 HTTP/1.1\r\n"
		"Host: api.ephor.online:80\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 12\r\n"
		"Cache-Control: no-cache\r\n"
		"\r\n,len=227>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<send="
		"{\"id\":\"789\"},len=12>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();

	// recv response
	const char resp2[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.10.3\r\n"
		"Date: Thu, 29 Nov 2018 16:06:12 GMT\r\n"
		"Content-Length: 68\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"{\"success\":false,\"code\":\"1\",\"message\":\"Eshelbe meshelbe shaitanama\"}";
	conn->addRecvString(resp2, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	conn->remoteClose();
	TEST_STRING_EQUAL("<ledFiscal=2><event=CommandOK>", result->getString());
	result->clear();
	StringBuilder fiscalDatetime;
	datetime2string(&saleData.fiscalDatetime, &fiscalDatetime);
	TEST_STRING_EQUAL("2000-01-02 10:20:30", fiscalDatetime.getString());
	TEST_NUMBER_EQUAL(0, saleData.fiscalRegister);
	TEST_NUMBER_EQUAL(Fiscal::Status_InQueue, saleData.fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleData.fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleData.fiscalSign);
	return true;
}

bool CommandLayerTest::testPollTooManyTries() {
	realtime->setUnixTimeStamp(2333444555);

	// connect
	Fiscal::Sale saleData;
	saleData.selectId.set("7");
	saleData.wareId = 5678;
	saleData.name.set("Тесточино");
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = 10000;
	saleData.price = 5000;
	saleData.taxSystem = Fiscal::TaxSystem_ENVD;
	saleData.taxRate = Fiscal::TaxRate_NDSNone;
	layer->sale(&saleData);
	TEST_STRING_EQUAL("<subscribe=3072><ledFiscal=1><connect:api.ephor.online,80,1>", result->getString());
	result->clear();

	// connect complete
	conn->connectComplete();
	TEST_STRING_EQUAL("<send="
		"POST /api/1.0/Fiscal.php?action=Reg&login=888000888000888&password=1234567&_dc=2333444555 HTTP/1.1\r\n"
		"Host: api.ephor.online:80\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 329\r\n"
		"Cache-Control: no-cache\r\n"
		"\r\n,len=229>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<send="
		"{\"date\":\"2000-01-02 10:20:30\""
		",\"point_addr\":\"PointAddr\""
		",\"point_name\":\"PointName\""
		",\"automat_number\":\"AutomatNumber\""
		",\"items\":["
		"{\"select_id\":\"7\""
		",\"ware_id\":5678"
		",\"name\":\"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		",\"device\":\"CA\""
		",\"price_list\":0"
		",\"price\":5000"
		",\"amount\":1000"
		",\"tax_rate\":0"
		"}]"
		",\"tax_system\":8"
		",\"cash\":5000"
		",\"cashless\":0"
		"},len=329>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("", result->getString());

	// recv data
	conn->incommingData();
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();

	// recv response
	const char resp1[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.12.0\r\n"
		"Date: Sun, 19 Aug 2018 13:43:42 GMT\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 36\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"{\"success\":true,\"id\":789,\"status\":7}";
	conn->addRecvString(resp1, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	conn->remoteClose();
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	realtime->setUnixTimeStamp(2333444666);
	timerEngine->tick(EPHOR_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:api.ephor.online,80,1>", result->getString());
	result->clear();

	for(uint16_t i = 0; i < EPHOR_TRY_MAX; i++) {
		// connect complete
		conn->connectComplete();
		TEST_STRING_EQUAL("<send="
			"GET /api/1.0/Fiscal.php?action=Get&login=888000888000888&password=1234567&_dc=2333444666 HTTP/1.1\r\n"
			"Host: api.ephor.online:80\r\n"
			"Content-Type: application/json; charset=windows-1251\r\n"
			"Content-Length: 12\r\n"
			"Cache-Control: no-cache\r\n"
			"\r\n,len=227>",
			result->getString());
		result->clear();

		// send complete
		conn->sendComplete();
		TEST_STRING_EQUAL("<send="
			"{\"id\":\"789\"},len=12>",
			result->getString());
		result->clear();

		// send complete
		conn->sendComplete();
		TEST_STRING_EQUAL("<recv=1024>", result->getString());
		result->clear();

		// recv response
		const char resp2[] =
			"HTTP/1.1 200 OK\r\n"
			"Server: nginx/1.10.3\r\n"
			"Date: Thu, 29 Nov 2018 16:06:12 GMT\r\n"
			"Content-Length: 38\r\n"
			"Connection: keep-alive\r\n"
			"\r\n"
			"{\"success\":true,\"id\":\"789\",\"status\":7}";
		conn->addRecvString(resp2, true);
		TEST_STRING_EQUAL("<close>", result->getString());
		result->clear();

		// close
		conn->remoteClose();
		TEST_STRING_EQUAL("", result->getString());

		// timeout
		timerEngine->tick(EPHOR_TRY_DELAY);
		timerEngine->execute();
		TEST_STRING_EQUAL("<connect:api.ephor.online,80,1>", result->getString());
		result->clear();
	}

	// connect complete
	conn->connectComplete();
	TEST_STRING_EQUAL("<send="
		"GET /api/1.0/Fiscal.php?action=Get&login=888000888000888&password=1234567&_dc=2333444666 HTTP/1.1\r\n"
		"Host: api.ephor.online:80\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 12\r\n"
		"Cache-Control: no-cache\r\n"
		"\r\n,len=227>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<send="
		"{\"id\":\"789\"},len=12>",
		result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();

	// recv response
	const char resp3[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.10.3\r\n"
		"Date: Thu, 29 Nov 2018 16:06:12 GMT\r\n"
		"Content-Length: 38\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"{\"success\":true,\"id\":\"789\",\"status\":7}";
	conn->addRecvString(resp3, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	conn->remoteClose();
	TEST_STRING_EQUAL("<ledFiscal=2><event=CommandOK>", result->getString());
	result->clear();
	StringBuilder fiscalDatetime;
	datetime2string(&saleData.fiscalDatetime, &fiscalDatetime);
	TEST_STRING_EQUAL("2000-01-02 10:20:30", fiscalDatetime.getString());
	TEST_NUMBER_EQUAL(0, saleData.fiscalRegister);
	TEST_NUMBER_EQUAL(Fiscal::Status_InQueue, saleData.fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleData.fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleData.fiscalSign);
	return true;
}

}
