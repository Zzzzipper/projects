#include "lib/fiscal_register/orange_data/OrangeDataCommandLayer.h"
#include "lib/fiscal_register/orange_data/OrangeDataProtocol.h"

#include "memory/include/RamMemory.h"
#include "fiscal_register/TestFiscalRegisterEventEngine.h"
#include "timer/include/TimerEngine.h"
#include "timer/include/TestRealTime.h"
#include "sim900/include/GsmDriver.h"
#include "http/test/TestTcpIp.h"
#include "utils/include/TestLed.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

namespace OrangeData {

class TestRsaSign : public RsaSignInterface {
public:
	TestRsaSign(StringBuilder *result) : result(result) {}
	virtual bool init(uint8_t *pemPrivateKey, uint16_t pemPrivateKeyLen) {
		(void)pemPrivateKey;
		(void)pemPrivateKeyLen;
		*result << "<RS::init>";
		return true;
	}

	virtual bool sign(uint8_t *data, uint16_t dataLen) {
		(void)data;
		(void)dataLen;
		*result << "<RS::sign>";
		return true;
	}

	virtual uint16_t base64encode(uint8_t *buf, uint16_t bufSize) {
		(void)buf;
		(void)bufSize;
		*result << "<RS::base64encode>";
		const char testSign[] = "0123456789";
		uint16_t testSignLen = strlen(testSign);
		if(bufSize < testSignLen) {
			return 0;
		}
		memcpy(buf, testSign, testSignLen);
		return testSignLen;
	}

private:
	StringBuilder *result;
};

class CommandLayerTest : public TestSet {
public:
	CommandLayerTest();
	bool init();
	void cleanup();
	bool testRegister();
	bool testCheckReconnect();
	bool testCheckReconnectNetworkUp();
	bool testCheckAlreadyStarted();
	bool testPollReconnect();
	bool testPollReconnectNetworkUp();
	bool testStatusInQueue();
	bool testStatusUnknown();
	bool testStatusError();

private:
	StringBuilder *result;
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	ConfigModem *config;
	Fiscal::Context *context;
	TestRsaSign *sign;
	TestTcpIp *conn;
	TestLed *leds;
	TimerEngine *timerEngine;
	TestFiscalRegisterEventEngine *eventEngine;
	CommandLayer *layer;

	bool gotoStatePollDelay();
	bool deliverEvent(EventInterface *event);
	bool deliverEvent(uint16_t type);
	bool deliverEvent(EventDeviceId deviceId, uint16_t type);
};

TEST_SET_REGISTER(OrangeData::CommandLayerTest);

CommandLayerTest::CommandLayerTest() {
	TEST_CASE_REGISTER(CommandLayerTest, testRegister);
	TEST_CASE_REGISTER(CommandLayerTest, testCheckReconnect);
	TEST_CASE_REGISTER(CommandLayerTest, testCheckReconnectNetworkUp);
	TEST_CASE_REGISTER(CommandLayerTest, testCheckAlreadyStarted);
//	TEST_CASE_REGISTER(CommandLayerTest, testPollReconnect);
//	TEST_CASE_REGISTER(CommandLayerTest, testPollReconnectNetworkUp);
	TEST_CASE_REGISTER(CommandLayerTest, testStatusInQueue);
	TEST_CASE_REGISTER(CommandLayerTest, testStatusUnknown);
	TEST_CASE_REGISTER(CommandLayerTest, testStatusError);
}

bool CommandLayerTest::init() {
	this->result = new StringBuilder(2048, 2048);
	this->memory = new RamMemory(32000),
	this->realtime = new TestRealTime;
	this->stat = new StatStorage;
	this->config = new ConfigModem(memory, realtime, stat);
	this->config->getBoot()->setImei("888000888000888");
	this->config->getFiscal()->setINN("112233445566");
	this->config->getFiscal()->setGroup("Venda1");
	this->context = new Fiscal::Context(2, realtime);
	this->sign = new TestRsaSign(result);
	this->conn = new TestTcpIp(2048, result, true);
	this->timerEngine = new TimerEngine;
	this->eventEngine = new TestFiscalRegisterEventEngine(result);
	this->leds = new TestLed(result);
	this->layer = new CommandLayer(config, context, sign, conn, timerEngine, eventEngine, realtime, leds);
	return true;
}

void CommandLayerTest::cleanup() {
	delete this->layer;
	delete this->leds;
	delete this->eventEngine;
	delete this->timerEngine;
	delete this->conn;
	delete this->sign;
	delete this->context;
	delete this->stat;
	delete this->realtime;
	delete this->memory;
	delete this->result;
}

bool CommandLayerTest::deliverEvent(EventInterface *event) {
	EventEnvelope envelope(EVENT_DATA_SIZE);
	if(event->pack(&envelope) == false) { return false; }
	layer->proc(&envelope);
	return true;
}

bool CommandLayerTest::deliverEvent(uint16_t type) {
	EventInterface event(type);
	return deliverEvent(&event);
}

bool CommandLayerTest::deliverEvent(EventDeviceId deviceId, uint16_t type) {
	EventInterface event(deviceId, type);
	return deliverEvent(&event);
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
	TEST_STRING_EQUAL("<subscribe=3072><ledFiscal=1><RS::sign><RS::base64encode><connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect error
	realtime->setUnixTimeStamp(3444555666);
	Event event1(TcpIp::Event_ConnectOk);
	layer->proc(&event1);
	TEST_STRING_EQUAL("<send="
		"POST /api/v2/documents/ HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"X-Signature: 0123456789\r\n"
		"Content-Length: 364\r\n"
		"\r\n"
		"{\"id\": \"8880008880008882333444555\""
		",\"inn\": \"112233445566\""
		",\"group\": \"Venda1\""
		",\"content\": {\"type\": 1,\"positions\": ["
		"{\"quantity\": 1.000"
		",\"price\": 50.0"
		",\"tax\": 6"
		",\"text\": \"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		"}]"
		",\"checkClose\": {\"payments\": [{\"type\": 1,\"amount\": 50.0}],\"taxationSystem\": 3}"
		",\"automatNumber\": \"\""
		",\"settlementPlace\": \"\""
		",\"settlementAddress\": \"\""
		"}}"
		",len=516>",
		result->getString());
	result->clear();

	// send complete
	Event event2(TcpIp::Event_SendDataOk);
	layer->proc(&event2);
	TEST_STRING_EQUAL("<recv=2000>", result->getString());
	result->clear();

	// recv response
	const char resp1[] =
		"HTTP/1.1 201 OK\r\n"
		"Content-Length: 0\r\n"
		"\r\n";
	conn->addRecvString(resp1, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event4(TcpIp::Event_Close);
	layer->proc(&event4);
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	timerEngine->tick(ORANGEDATA_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect complete
	Event event5(TcpIp::Event_ConnectOk);
	layer->proc(&event5);
	TEST_STRING_EQUAL("<send="
		"GET /api/v2/documents/112233445566/status/8880008880008882333444555 HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 0\r\n"
		"\r\n"
		",len=169>",
		result->getString());
	result->clear();

	// send complete
	Event event6(TcpIp::Event_SendDataOk);
	layer->proc(&event6);
	TEST_STRING_EQUAL("<recv=2000>", result->getString());
	result->clear();

	// recv response
	const char resp2[] =
		"HTTP/1.1 202 Accepted\r\n"
		"Server: nginx/1.10.3\r\n"
		"Date: Thu, 29 Nov 2018 16:06:12 GMT\r\n"
		"Content-Length: 0\r\n"
		"Connection: keep-alive\r\n"
		"\r\n";
	conn->addRecvString(resp2, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event7(TcpIp::Event_Close);
	layer->proc(&event7);
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	timerEngine->tick(ORANGEDATA_TRY_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect complete
	Event event8(TcpIp::Event_ConnectOk);
	layer->proc(&event8);
	TEST_STRING_EQUAL("<send="
		"GET /api/v2/documents/112233445566/status/8880008880008882333444555 HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 0\r\n"
		"\r\n"
		",len=169>",
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
		"Transfer-Encoding: chunked\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"2c1\r\n"
		"{\"id\":\"8696960491890171543521995\","
		"\"deviceSN\":\"0390580038004444\","
		"\"deviceRN\":\"5892308424002406\","
		"\"fsNumber\":\"9999078900005419\","
		"\"ofdName\":\"ОФД-Я (тест)\","
		"\"ofdWebsite\":\"www.ofd-ya.ru\","
		"\"ofdinn\":\"7728699517\","
		"\"fnsWebsite\":\"www.nalog.ru\","
		"\"companyINN\":\"690209812752\","
		"\"companyName\":\"ИП Войткевич Алексей Олегович (ЭФОР)\","
		"\"documentNumber\":1866,"
		"\"shiftNumber\":264,"
		"\"documentIndex\":5,"
		"\"processedAt\":\"2018-11-29T20:06:00\","
		"\"content\":{"
		"\"type\":1,"
		"\"positions\":[{\"quantity\":1.000,\"price\":1.23,\"tax\":2,\"text\":\"Тесточино0\"}],"
		"\"checkClose\":{\"payments\":[{\"type\":1,\"amount\":1.23}],\"taxationSystem\":0},"
		"\"automatNumber\":\"159\","
		"\"settlementAddress\":\"ул. Афанасия Никитина, 90\","
		"\"settlementPlace\":\"Разработка3\"},"
		"\"change\":0.0,\"fp\":\"3326875305\""
		"}\r\n"
		"0";
	conn->addRecvString(resp3, true);
	TEST_STRING_EQUAL("<ledFiscal=2><close>", result->getString());
	result->clear();

	// send complete
	Event event10(TcpIp::Event_Close);
	layer->proc(&event10);
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	result->clear();
	TEST_NUMBER_EQUAL(5892308424002406, saleData.fiscalRegister);
	TEST_NUMBER_EQUAL(9999078900005419, saleData.fiscalStorage);
	TEST_NUMBER_EQUAL(1866, saleData.fiscalDocument);
	TEST_NUMBER_EQUAL(3326875305, saleData.fiscalSign);
	return true;
}

bool CommandLayerTest::testCheckReconnect() {
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
	TEST_STRING_EQUAL("<subscribe=3072><ledFiscal=1><RS::sign><RS::base64encode><connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect error
	realtime->setUnixTimeStamp(3444555666);
	EventError event2(TcpIp::Event_ConnectError);
	layer->proc(&event2);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event3(TcpIp::Event_Close);
	layer->proc(&event3);
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// timeout
	timerEngine->tick(ORANGEDATA_TRY_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect complete
	Event event4(TcpIp::Event_ConnectOk);
	layer->proc(&event4);
	TEST_STRING_EQUAL("<send="
		"POST /api/v2/documents/ HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"X-Signature: 0123456789\r\n"
		"Content-Length: 364\r\n"
		"\r\n"
		"{\"id\": \"8880008880008882333444555\""
		",\"inn\": \"112233445566\""
		",\"group\": \"Venda1\""
		",\"content\": {\"type\": 1,\"positions\": ["
		"{\"quantity\": 1.000"
		",\"price\": 50.0"
		",\"tax\": 6"
		",\"text\": \"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		"}]"
		",\"checkClose\": {\"payments\": [{\"type\": 1,\"amount\": 50.0}],\"taxationSystem\": 3}"
		",\"automatNumber\": \"\""
		",\"settlementPlace\": \"\""
		",\"settlementAddress\": \"\""
		"}}"
		",len=516>",
		result->getString());
	result->clear();

	// send complete
	Event event5(TcpIp::Event_SendDataError);
	layer->proc(&event5);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event6(TcpIp::Event_Close);
	layer->proc(&event6);
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	timerEngine->tick(ORANGEDATA_TRY_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect complete
	Event event7(TcpIp::Event_ConnectOk);
	layer->proc(&event7);
	TEST_STRING_EQUAL("<send="
		"POST /api/v2/documents/ HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"X-Signature: 0123456789\r\n"
		"Content-Length: 364\r\n"
		"\r\n"
		"{\"id\": \"8880008880008882333444555\""
		",\"inn\": \"112233445566\""
		",\"group\": \"Venda1\""
		",\"content\": {\"type\": 1,\"positions\": ["
		"{\"quantity\": 1.000"
		",\"price\": 50.0"
		",\"tax\": 6"
		",\"text\": \"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		"}]"
		",\"checkClose\": {\"payments\": [{\"type\": 1,\"amount\": 50.0}],\"taxationSystem\": 3}"
		",\"automatNumber\": \"\""
		",\"settlementPlace\": \"\""
		",\"settlementAddress\": \"\""
		"}}"
		",len=516>",
		result->getString());
	result->clear();

	// send complete
	Event event8(TcpIp::Event_SendDataOk);
	layer->proc(&event8);
	TEST_STRING_EQUAL("<recv=2000>", result->getString());
	result->clear();

	// recv response
	const char resp1[] =
		"HTTP/1.1 201 OK\r\n"
		"Content-Length: 0\r\n"
		"\r\n";
	conn->addRecvString(resp1, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event9(TcpIp::Event_Close);
	layer->proc(&event9);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool CommandLayerTest::testCheckReconnectNetworkUp() {
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
	TEST_STRING_EQUAL("<subscribe=3072><ledFiscal=1><RS::sign><RS::base64encode><connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect error
	realtime->setUnixTimeStamp(3444555666);
	EventError event1(TcpIp::Event_ConnectError);
	layer->proc(&event1);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event2(TcpIp::Event_Close);
	layer->proc(&event2);
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// network up
	deliverEvent(Gsm::Driver::Event_NetworkUp);
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect complete
	Event event3(TcpIp::Event_ConnectOk);
	layer->proc(&event3);
	TEST_STRING_EQUAL("<send="
		"POST /api/v2/documents/ HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"X-Signature: 0123456789\r\n"
		"Content-Length: 364\r\n"
		"\r\n"
		"{\"id\": \"8880008880008882333444555\""
		",\"inn\": \"112233445566\""
		",\"group\": \"Venda1\""
		",\"content\": {\"type\": 1,\"positions\": ["
		"{\"quantity\": 1.000"
		",\"price\": 50.0"
		",\"tax\": 6"
		",\"text\": \"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		"}]"
		",\"checkClose\": {\"payments\": [{\"type\": 1,\"amount\": 50.0}],\"taxationSystem\": 3}"
		",\"automatNumber\": \"\""
		",\"settlementPlace\": \"\""
		",\"settlementAddress\": \"\""
		"}}"
		",len=516>",
		result->getString());
	result->clear();

	// send complete
	Event event4(TcpIp::Event_SendDataError);
	layer->proc(&event4);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event6(TcpIp::Event_Close);
	layer->proc(&event6);
	TEST_STRING_EQUAL("", result->getString());

	// network up
	deliverEvent(Gsm::Driver::Event_NetworkUp);
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect complete
	Event event7(TcpIp::Event_ConnectOk);
	layer->proc(&event7);
	TEST_STRING_EQUAL("<send="
		"POST /api/v2/documents/ HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"X-Signature: 0123456789\r\n"
		"Content-Length: 364\r\n"
		"\r\n"
		"{\"id\": \"8880008880008882333444555\""
		",\"inn\": \"112233445566\""
		",\"group\": \"Venda1\""
		",\"content\": {\"type\": 1,\"positions\": ["
		"{\"quantity\": 1.000"
		",\"price\": 50.0"
		",\"tax\": 6"
		",\"text\": \"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		"}]"
		",\"checkClose\": {\"payments\": [{\"type\": 1,\"amount\": 50.0}],\"taxationSystem\": 3}"
		",\"automatNumber\": \"\""
		",\"settlementPlace\": \"\""
		",\"settlementAddress\": \"\""
		"}}"
		",len=516>",
		result->getString());
	result->clear();

	// send complete
	Event event8(TcpIp::Event_SendDataOk);
	layer->proc(&event8);
	TEST_STRING_EQUAL("<recv=2000>", result->getString());
	result->clear();

	// recv response
	const char resp1[] =
		"HTTP/1.1 201 OK\r\n"
		"Content-Length: 0\r\n"
		"\r\n";
	conn->addRecvString(resp1, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event9(TcpIp::Event_Close);
	layer->proc(&event9);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool CommandLayerTest::testCheckAlreadyStarted() {
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
	TEST_STRING_EQUAL("<subscribe=3072><ledFiscal=1><RS::sign><RS::base64encode><connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect error
	realtime->setUnixTimeStamp(3444555666);
	Event event1(TcpIp::Event_ConnectOk);
	layer->proc(&event1);
	TEST_STRING_EQUAL("<send="
		"POST /api/v2/documents/ HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"X-Signature: 0123456789\r\n"
		"Content-Length: 364\r\n"
		"\r\n"
		"{\"id\": \"8880008880008882333444555\""
		",\"inn\": \"112233445566\""
		",\"group\": \"Venda1\""
		",\"content\": {\"type\": 1,\"positions\": ["
		"{\"quantity\": 1.000"
		",\"price\": 50.0"
		",\"tax\": 6"
		",\"text\": \"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		"}]"
		",\"checkClose\": {\"payments\": [{\"type\": 1,\"amount\": 50.0}],\"taxationSystem\": 3}"
		",\"automatNumber\": \"\""
		",\"settlementPlace\": \"\""
		",\"settlementAddress\": \"\""
		"}}"
		",len=516>",
		result->getString());
	result->clear();

	// send complete
	Event event2(TcpIp::Event_SendDataError);
	layer->proc(&event2);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event3(TcpIp::Event_Close);
	layer->proc(&event3);
	TEST_STRING_EQUAL("", result->getString());

	// network up
	deliverEvent(Gsm::Driver::Event_NetworkUp);
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect error
	realtime->setUnixTimeStamp(3444555666);
	Event event4(TcpIp::Event_ConnectOk);
	layer->proc(&event4);
	TEST_STRING_EQUAL("<send="
		"POST /api/v2/documents/ HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"X-Signature: 0123456789\r\n"
		"Content-Length: 364\r\n"
		"\r\n"
		"{\"id\": \"8880008880008882333444555\""
		",\"inn\": \"112233445566\""
		",\"group\": \"Venda1\""
		",\"content\": {\"type\": 1,\"positions\": ["
		"{\"quantity\": 1.000"
		",\"price\": 50.0"
		",\"tax\": 6"
		",\"text\": \"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		"}]"
		",\"checkClose\": {\"payments\": [{\"type\": 1,\"amount\": 50.0}],\"taxationSystem\": 3}"
		",\"automatNumber\": \"\""
		",\"settlementPlace\": \"\""
		",\"settlementAddress\": \"\""
		"}}"
		",len=516>",
		result->getString());
	result->clear();

	// send complete
	Event event5(TcpIp::Event_SendDataOk);
	layer->proc(&event5);
	TEST_STRING_EQUAL("<recv=2000>", result->getString());
	result->clear();

	// recv response
	const char resp1[] =
		"HTTP/1.1 409 Conflict\r\n"
		"Content-Length: 0\r\n"
		"\r\n";
	conn->addRecvString(resp1, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event6(TcpIp::Event_Close);
	layer->proc(&event6);
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	timerEngine->tick(ORANGEDATA_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect complete
	Event event10(TcpIp::Event_ConnectOk);
	layer->proc(&event10);
	TEST_STRING_EQUAL("<send="
		"GET /api/v2/documents/112233445566/status/8880008880008882333444555 HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 0\r\n"
		"\r\n"
		",len=169>",
		result->getString());
	result->clear();

	// send complete
	Event event11(TcpIp::Event_SendDataOk);
	layer->proc(&event11);
	TEST_STRING_EQUAL("<recv=2000>", result->getString());
	result->clear();

	// recv response
	const char resp3[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.10.3\r\n"
		"Date: Thu, 29 Nov 2018 17:06:54 GMT\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Transfer-Encoding: chunked\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"2c1\r\n"
		"{\"id\":\"8696960491890171543521995\","
		"\"deviceSN\":\"0390580038004444\","
		"\"deviceRN\":\"5892308424002406\","
		"\"fsNumber\":\"9999078900005419\","
		"\"ofdName\":\"ОФД-Я (тест)\","
		"\"ofdWebsite\":\"www.ofd-ya.ru\","
		"\"ofdinn\":\"7728699517\","
		"\"fnsWebsite\":\"www.nalog.ru\","
		"\"companyINN\":\"690209812752\","
		"\"companyName\":\"ИП Войткевич Алексей Олегович (ЭФОР)\","
		"\"documentNumber\":1866,"
		"\"shiftNumber\":264,"
		"\"documentIndex\":5,"
		"\"processedAt\":\"2018-11-29T20:06:00\","
		"\"content\":{"
		"\"type\":1,"
		"\"positions\":[{\"quantity\":1.000,\"price\":1.23,\"tax\":2,\"text\":\"Тесточино0\"}],"
		"\"checkClose\":{\"payments\":[{\"type\":1,\"amount\":1.23}],\"taxationSystem\":0},"
		"\"automatNumber\":\"159\","
		"\"settlementAddress\":\"ул. Афанасия Никитина, 90\","
		"\"settlementPlace\":\"Разработка3\"},"
		"\"change\":0.0,\"fp\":\"3326875305\""
		"}\r\n"
		"0";
	conn->addRecvString(resp3, true);
	TEST_STRING_EQUAL("<ledFiscal=2><close>", result->getString());
	result->clear();

	// send complete
	Event event12(TcpIp::Event_Close);
	layer->proc(&event12);
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	result->clear();
	return true;
}

bool CommandLayerTest::gotoStatePollDelay() {
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
	TEST_STRING_EQUAL("<subscribe=3072><ledFiscal=1><RS::sign><RS::base64encode><connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect error
	realtime->setUnixTimeStamp(3444555666);
	Event event1(TcpIp::Event_ConnectOk);
	layer->proc(&event1);
	TEST_STRING_EQUAL("<send="
		"POST /api/v2/documents/ HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"X-Signature: 0123456789\r\n"
		"Content-Length: 364\r\n"
		"\r\n"
		"{\"id\": \"8880008880008882333444555\""
		",\"inn\": \"112233445566\""
		",\"group\": \"Venda1\""
		",\"content\": {\"type\": 1,\"positions\": ["
		"{\"quantity\": 1.000"
		",\"price\": 50.0"
		",\"tax\": 6"
		",\"text\": \"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		"}]"
		",\"checkClose\": {\"payments\": [{\"type\": 1,\"amount\": 50.0}],\"taxationSystem\": 3}"
		",\"automatNumber\": \"\""
		",\"settlementPlace\": \"\""
		",\"settlementAddress\": \"\""
		"}}"
		",len=516>",
		result->getString());
	result->clear();

	// send complete
	Event event2(TcpIp::Event_SendDataOk);
	layer->proc(&event2);
	TEST_STRING_EQUAL("<recv=2000>", result->getString());
	result->clear();

	// recv response
	const char resp1[] =
		"HTTP/1.1 201 OK\r\n"
		"Content-Length: 0\r\n"
		"\r\n";
	conn->addRecvString(resp1, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event4(TcpIp::Event_Close);
	layer->proc(&event4);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool CommandLayerTest::testPollReconnect() {
	gotoStatePollDelay();

	// timeout
	timerEngine->tick(ORANGEDATA_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect error
	realtime->setUnixTimeStamp(3444555666);
	Event event1(TcpIp::Event_ConnectError);
	layer->proc(&event1);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event2(TcpIp::Event_Close);
	layer->proc(&event2);
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// timeout
	timerEngine->tick(ORANGEDATA_TRY_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect complete
	Event event3(TcpIp::Event_ConnectOk);
	layer->proc(&event3);
	TEST_STRING_EQUAL("<send="
		"GET /api/v2/documents/112233445566/status/8880008880008882333444555 HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 0\r\n"
		"\r\n"
		",len=169>",
		result->getString());
	result->clear();

	// send complete
	Event event4(TcpIp::Event_SendDataError);
	layer->proc(&event4);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	Event event5(TcpIp::Event_Close);
	layer->proc(&event5);
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	timerEngine->tick(ORANGEDATA_TRY_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect complete
	Event event6(TcpIp::Event_ConnectOk);
	layer->proc(&event6);
	TEST_STRING_EQUAL("<send="
		"GET /api/v2/documents/112233445566/status/8880008880008882333444555 HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 0\r\n"
		"\r\n"
		",len=169>",
		result->getString());
	result->clear();

	// send complete
	Event event7(TcpIp::Event_SendDataOk);
	layer->proc(&event7);
	TEST_STRING_EQUAL("<recv=2000>", result->getString());
	result->clear();

	// recv response
	const char resp3[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.10.3\r\n"
		"Date: Thu, 29 Nov 2018 17:06:54 GMT\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Transfer-Encoding: chunked\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"2c1\r\n"
		"{\"id\":\"8696960491890171543521995\","
		"\"deviceSN\":\"0390580038004444\","
		"\"deviceRN\":\"5892308424002406\","
		"\"fsNumber\":\"9999078900005419\","
		"\"ofdName\":\"ОФД-Я (тест)\","
		"\"ofdWebsite\":\"www.ofd-ya.ru\","
		"\"ofdinn\":\"7728699517\","
		"\"fnsWebsite\":\"www.nalog.ru\","
		"\"companyINN\":\"690209812752\","
		"\"companyName\":\"ИП Войткевич Алексей Олегович (ЭФОР)\","
		"\"documentNumber\":1866,"
		"\"shiftNumber\":264,"
		"\"documentIndex\":5,"
		"\"processedAt\":\"2018-11-29T20:06:00\","
		"\"content\":{"
		"\"type\":1,"
		"\"positions\":[{\"quantity\":1.000,\"price\":1.23,\"tax\":2,\"text\":\"Тесточино0\"}],"
		"\"checkClose\":{\"payments\":[{\"type\":1,\"amount\":1.23}],\"taxationSystem\":0},"
		"\"automatNumber\":\"159\","
		"\"settlementAddress\":\"ул. Афанасия Никитина, 90\","
		"\"settlementPlace\":\"Разработка3\"},"
		"\"change\":0.0,\"fp\":\"3326875305\""
		"}\r\n"
		"0";
	conn->addRecvString(resp3, true);
	TEST_STRING_EQUAL("<ledFiscal=2><close>", result->getString());
	result->clear();
/*
	// send complete
	Event event8(TcpIp::Event_Close);
	layer->proc(&event8);
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	result->clear();*/
	return true;
}

bool CommandLayerTest::testPollReconnectNetworkUp() {
	gotoStatePollDelay();

	// timeout
	timerEngine->tick(ORANGEDATA_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect error
	realtime->setUnixTimeStamp(3444555666);
	EventError event1(TcpIp::Event_ConnectError);
	layer->proc(&event1);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event2(TcpIp::Event_Close);
	layer->proc(&event2);
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// network up
	deliverEvent(Gsm::Driver::Event_NetworkUp);
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect complete
	Event event3(TcpIp::Event_ConnectOk);
	layer->proc(&event3);
	TEST_STRING_EQUAL("<send="
		"GET /api/v2/documents/112233445566/status/8880008880008882333444555 HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 0\r\n"
		"\r\n"
		",len=169>",
		result->getString());
	result->clear();

	// send complete
	Event event4(TcpIp::Event_SendDataError);
	layer->proc(&event4);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event5(TcpIp::Event_Close);
	layer->proc(&event5);
	TEST_STRING_EQUAL("", result->getString());

	// network up
	deliverEvent(Gsm::Driver::Event_NetworkUp);
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect complete
	Event event6(TcpIp::Event_ConnectOk);
	layer->proc(&event6);
	TEST_STRING_EQUAL("<send="
		"GET /api/v2/documents/112233445566/status/8880008880008882333444555 HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: 0\r\n"
		"\r\n"
		",len=169>",
		result->getString());
	result->clear();

	// send complete
	Event event7(TcpIp::Event_SendDataOk);
	layer->proc(&event7);
	TEST_STRING_EQUAL("<recv=2000>", result->getString());
	result->clear();

	// recv response
	const char resp3[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.10.3\r\n"
		"Date: Thu, 29 Nov 2018 17:06:54 GMT\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Transfer-Encoding: chunked\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"2c1\r\n"
		"{\"id\":\"8696960491890171543521995\","
		"\"deviceSN\":\"0390580038004444\","
		"\"deviceRN\":\"5892308424002406\","
		"\"fsNumber\":\"9999078900005419\","
		"\"ofdName\":\"ОФД-Я (тест)\","
		"\"ofdWebsite\":\"www.ofd-ya.ru\","
		"\"ofdinn\":\"7728699517\","
		"\"fnsWebsite\":\"www.nalog.ru\","
		"\"companyINN\":\"690209812752\","
		"\"companyName\":\"ИП Войткевич Алексей Олегович (ЭФОР)\","
		"\"documentNumber\":1866,"
		"\"shiftNumber\":264,"
		"\"documentIndex\":5,"
		"\"processedAt\":\"2018-11-29T20:06:00\","
		"\"content\":{"
		"\"type\":1,"
		"\"positions\":[{\"quantity\":1.000,\"price\":1.23,\"tax\":2,\"text\":\"Тесточино0\"}],"
		"\"checkClose\":{\"payments\":[{\"type\":1,\"amount\":1.23}],\"taxationSystem\":0},"
		"\"automatNumber\":\"159\","
		"\"settlementAddress\":\"ул. Афанасия Никитина, 90\","
		"\"settlementPlace\":\"Разработка3\"},"
		"\"change\":0.0,\"fp\":\"3326875305\""
		"}\r\n"
		"0";
	conn->addRecvString(resp3, true);
	TEST_STRING_EQUAL("<ledFiscal=2><close>", result->getString());
	result->clear();

	// send complete
	Event event8(TcpIp::Event_Close);
	layer->proc(&event8);
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	result->clear();
	return true;
}

bool CommandLayerTest::testStatusInQueue() {
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
	TEST_STRING_EQUAL("<subscribe=3072><ledFiscal=1><RS::sign><RS::base64encode><connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect error
	realtime->setUnixTimeStamp(3444555666);
	Event event1(TcpIp::Event_ConnectOk);
	layer->proc(&event1);
	TEST_STRING_EQUAL("<send="
		"POST /api/v2/documents/ HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"X-Signature: 0123456789\r\n"
		"Content-Length: 364\r\n"
		"\r\n"
		"{\"id\": \"8880008880008882333444555\""
		",\"inn\": \"112233445566\""
		",\"group\": \"Venda1\""
		",\"content\": {\"type\": 1,\"positions\": ["
		"{\"quantity\": 1.000"
		",\"price\": 50.0"
		",\"tax\": 6"
		",\"text\": \"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		"}]"
		",\"checkClose\": {\"payments\": [{\"type\": 1,\"amount\": 50.0}],\"taxationSystem\": 3}"
		",\"automatNumber\": \"\""
		",\"settlementPlace\": \"\""
		",\"settlementAddress\": \"\""
		"}}"
		",len=516>",
		result->getString());
	result->clear();

	// send complete
	Event event2(TcpIp::Event_SendDataOk);
	layer->proc(&event2);
	TEST_STRING_EQUAL("<recv=2000>", result->getString());
	result->clear();

	// recv response
	const char resp1[] =
		"HTTP/1.1 201 OK\r\n"
		"Content-Length: 0\r\n"
		"\r\n";
	conn->addRecvString(resp1, true);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event4(TcpIp::Event_Close);
	layer->proc(&event4);
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	timerEngine->tick(ORANGEDATA_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	for(uint16_t i = 0; i < ORANGEDATA_TRY_MAX; i++) {
		// connect complete
		EventError event8(TcpIp::Event_ConnectError);
		layer->proc(&event8);
		TEST_STRING_EQUAL("<close>", result->getString());
		result->clear();

		// close
		Event event9(TcpIp::Event_Close);
		layer->proc(&event9);
		TEST_STRING_EQUAL("", result->getString());

		// timeout
		timerEngine->tick(ORANGEDATA_TRY_DELAY);
		timerEngine->execute();
		TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
		result->clear();
	}

	// connect complete
	EventError event10(TcpIp::Event_ConnectError);
	layer->proc(&event10);
	TEST_STRING_EQUAL("<ledFiscal=3><close>", result->getString());
	result->clear();

	// send complete
	Event event11(TcpIp::Event_Close);
	layer->proc(&event11);
	TEST_STRING_EQUAL("<event=CommandError,770>", result->getString());
	result->clear();
	TEST_NUMBER_EQUAL(0, saleData.fiscalRegister);
	TEST_NUMBER_EQUAL(2, saleData.fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleData.fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleData.fiscalSign);
	return true;
}

bool CommandLayerTest::testStatusUnknown() {
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
	TEST_STRING_EQUAL("<subscribe=3072><ledFiscal=1><RS::sign><RS::base64encode><connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// connect error
	realtime->setUnixTimeStamp(3444555666);
	Event event1(TcpIp::Event_ConnectOk);
	layer->proc(&event1);
	TEST_STRING_EQUAL("<send="
		"POST /api/v2/documents/ HTTP/1.1\r\n"
		"Host: 192.168.1.210:0\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"X-Signature: 0123456789\r\n"
		"Content-Length: 364\r\n"
		"\r\n"
		"{\"id\": \"8880008880008882333444555\""
		",\"inn\": \"112233445566\""
		",\"group\": \"Venda1\""
		",\"content\": {\"type\": 1,\"positions\": ["
		"{\"quantity\": 1.000"
		",\"price\": 50.0"
		",\"tax\": 6"
		",\"text\": \"\\u0422\\u0435\\u0441\\u0442\\u043E\\u0447\\u0438\\u043D\\u043E\""
		"}]"
		",\"checkClose\": {\"payments\": [{\"type\": 1,\"amount\": 50.0}],\"taxationSystem\": 3}"
		",\"automatNumber\": \"\""
		",\"settlementPlace\": \"\""
		",\"settlementAddress\": \"\""
		"}}"
		",len=516>",
		result->getString());
	result->clear();

	// send complete
	Event event2(TcpIp::Event_SendDataOk);
	layer->proc(&event2);
	TEST_STRING_EQUAL("<recv=2000>", result->getString());
	result->clear();

	// close
	Event event3(TcpIp::Event_Close);
	layer->proc(&event3);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event4(TcpIp::Event_Close);
	layer->proc(&event4);
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	timerEngine->tick(ORANGEDATA_TRY_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	for(uint16_t i = 0; i < (ORANGEDATA_TRY_MAX - 1); i++) {
		// connect error
		EventError event8(TcpIp::Event_ConnectError);
		layer->proc(&event8);
		TEST_STRING_EQUAL("<close>", result->getString());
		result->clear();

		// close
		Event event9(TcpIp::Event_Close);
		layer->proc(&event9);
		TEST_STRING_EQUAL("", result->getString());

		// timeout
		timerEngine->tick(ORANGEDATA_TRY_DELAY);
		timerEngine->execute();
		TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
		result->clear();
	}

	// connect complete
	EventError event10(TcpIp::Event_ConnectError);
	layer->proc(&event10);
	TEST_STRING_EQUAL("<ledFiscal=3><close>", result->getString());
	result->clear();

	// send complete
	Event event11(TcpIp::Event_Close);
	layer->proc(&event11);
	TEST_STRING_EQUAL("<event=CommandError,770>", result->getString());
	result->clear();
	TEST_NUMBER_EQUAL(0, saleData.fiscalRegister);
	TEST_NUMBER_EQUAL(3, saleData.fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleData.fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleData.fiscalSign);
	return true;
}

bool CommandLayerTest::testStatusError() {
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
	TEST_STRING_EQUAL("<subscribe=3072><ledFiscal=1><RS::sign><RS::base64encode><connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	// close
	EventError event3(TcpIp::Event_ConnectError);
	layer->proc(&event3);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// close
	Event event4(TcpIp::Event_Close);
	layer->proc(&event4);
	TEST_STRING_EQUAL("", result->getString());

	// timeout
	timerEngine->tick(ORANGEDATA_TRY_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
	result->clear();

	for(uint16_t i = 0; i < (ORANGEDATA_TRY_MAX - 1); i++) {
		// connect error
		EventError event8(TcpIp::Event_ConnectError);
		layer->proc(&event8);
		TEST_STRING_EQUAL("<close>", result->getString());
		result->clear();

		// close
		Event event9(TcpIp::Event_Close);
		layer->proc(&event9);
		TEST_STRING_EQUAL("", result->getString());

		// timeout
		timerEngine->tick(ORANGEDATA_TRY_DELAY);
		timerEngine->execute();
		TEST_STRING_EQUAL("<connect:192.168.1.210,0,1>", result->getString());
		result->clear();
	}

	// connect complete
	EventError event10(TcpIp::Event_ConnectError);
	layer->proc(&event10);
	TEST_STRING_EQUAL("<ledFiscal=3><close>", result->getString());
	result->clear();

	// send complete
	Event event11(TcpIp::Event_Close);
	layer->proc(&event11);
	TEST_STRING_EQUAL("<event=CommandError,770>", result->getString());
	result->clear();
	TEST_NUMBER_EQUAL(0, saleData.fiscalRegister);
	TEST_NUMBER_EQUAL(4, saleData.fiscalStorage);
	TEST_NUMBER_EQUAL(0, saleData.fiscalDocument);
	TEST_NUMBER_EQUAL(0, saleData.fiscalSign);
	return true;
}

}
