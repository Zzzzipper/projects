#include "test/include/Test.h"
#include "http/include/HttpClient.h"
#include "http/test/TestTcpIp.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/StringParser.h"
#include "utils/include/Buffer.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace Http {

class TestClientObserver : public EventObserver  {
public:
	void proc(Event *event) {
		const char *eventDesc = eventTypeToString(event->getType());
		if(eventDesc != NULL) {
			result << "<" << eventDesc << ">";
		} else {
			result << "<" << event->getType() << ">";
		}
	}
	const char *getResult() const { return result.getString(); }
	void clearResult() { result.clear(); }

private:
	StringBuilder result;

	const char *eventTypeToString(uint16_t eventType) {
		switch(eventType) {
			case Http::Client::Event_RequestComplete: return "Http::Client::Event_RequestComplete";
			case Http::Client::Event_RequestError: return "Http::Client::Event_RequestError";
			default: return NULL;
		}
	}
};

class ClientTest : public TestSet {
public:
	ClientTest();
	bool init();
	void cleanup();
	bool testPost();
	bool testPhpSessionId();
	bool testTcpIpConnectError();
	bool testTcpIpSendError();
	bool testTcpIpRecvError();
	bool testHttpParserError();

private:
	StringBuilder *result;
	TimerEngine *timers;
	TestTcpIp *tcpIp;
	TestClientObserver *observer;
	Client *client;
};

TEST_SET_REGISTER(Http::ClientTest);

ClientTest::ClientTest() {
	TEST_CASE_REGISTER(ClientTest, testPost);
	TEST_CASE_REGISTER(ClientTest, testPhpSessionId);
	TEST_CASE_REGISTER(ClientTest, testTcpIpConnectError);
	TEST_CASE_REGISTER(ClientTest, testTcpIpSendError);
	TEST_CASE_REGISTER(ClientTest, testTcpIpRecvError);
	TEST_CASE_REGISTER(ClientTest, testHttpParserError);
}

bool ClientTest::init() {
	result = new StringBuilder;
	timers = new TimerEngine;
	tcpIp = new TestTcpIp(1024, result, true);
	observer = new TestClientObserver;
	client = new Client(timers, tcpIp);
	client->setObserver(observer);
	return true;
}

void ClientTest::cleanup() {
	delete client;
	delete observer;
	delete tcpIp;
	delete timers;
	delete result;
}

bool ClientTest::testPost() {
	Request req;
	StringBuilder reqData("{\"login\":\"pyroman\",\"hash\":\"cfc54c0fb8197c79defc7e8a5bc1d1d1c1becfbb\"}");
	req.serverName = "test.sambery.net";
	req.serverPort = 81;
	req.method = Http::Request::Method_POST;
	req.serverPath = "/server/json/Login.php?action=Login&_dc=1234567";
	req.data = &reqData;

	Response resp;
	StringBuilder respData(1024, 1024);
	resp.data = &respData;

	// Send request
	if(client->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_TEST, "Http::Client::sendRequest() failed");
		return false;
	}

	// connect
	TEST_STRING_EQUAL("<connect:test.sambery.net,81,1>", result->getString());
	result->clear();
	Event event(TcpIp::Event_ConnectOk);
	client->proc(&event);

	// send data
	TEST_STRING_EQUAL(
		"<send="
		"POST /server/json/Login.php?action=Login&_dc=1234567 HTTP/1.1\r\n"
		"Host: test.sambery.net:81\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 69\r\n"
		"Cache-Control: no-cache\r\n\r\n"
		",len=191>",
		result->getString());
	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);

	// send data
	TEST_STRING_EQUAL(
		"<send={\"login\":\"pyroman\",\"hash\":\"cfc54c0fb8197c79defc7e8a5bc1d1d1c1becfbb\"},len=69>",
		result->getString());
	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);
	TEST_STRING_EQUAL("", result->getString());

	// incomming data event
	tcpIp->setRecvDataFlag(true);
	event.set(TcpIp::Event_IncomingData);
	client->proc(&event);

	// recv data
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();
	const char *resp1part1 =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.9.5\r\n"
		"Date: Sun, 24 Jan 2016 10:00:04 GMT\r\n"
		"Content-Ty";
	tcpIp->addRecvString(resp1part1, true);

	// recv data
	TEST_STRING_EQUAL("<recv=1014>", result->getString());
	result->clear();
	const char *resp1part2 =
		"pe: text/html\r\n"
		"Content-Length: 16\r\n"
		"Connection: keep-alive\r\n"
		"Keep-Alive: timeout=30\r\n"
		"X-Powered-By: PHP/5.5.31\r\n"
		"Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n"
		"Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"
		"Pragma: no-cache\r\n";
	tcpIp->addRecvString(resp1part2, true);

	// recv data
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();
	const char *resp1part3 =
		"Set-Cookie: PHPSESSID=8a4bb025730a7f81fec32d9358c0e005; expires=Sun, 24-Jan-2016 12:00:04 GMT; Max-Age=7200; path=/\r\n"
		"\r\n"
		"{\"success\":true}";
	tcpIp->addRecvString(resp1part3, false);

	// close
	TEST_STRING_EQUAL("<close>", result->getString());
	TEST_STRING_EQUAL("", observer->getResult());
	TEST_SUBSTR_EQUAL("{\"success\":true}", (const char*)resp.data->getData(), resp.data->getLen());

	// Завершение соединения
	result->clear();
	event.set(TcpIp::TcpIp::Event_Close);
	client->proc(&event);
	TEST_STRING_EQUAL("<Http::Client::Event_RequestComplete>", observer->getResult());
	return true;
}

bool ClientTest::testPhpSessionId() {
	Request req;
	StringBuilder reqData1("{\"login\":\"pyroman\",\"hash\":\"cfc54c0fb8197c79defc7e8a5bc1d1d1c1becfbb\"}");
	req.serverName = "test.sambery.net";
	req.serverPort = 81;
	req.keepAlive = true;
	req.method = Http::Request::Method_POST;
	req.serverPath = "/server/json/Login.php?action=Login&_dc=1234567";
	req.data = &reqData1;

	Response resp;
	StringBuilder phpSessionId(64, 64);
	StringBuilder respData(1024, 1024);
	resp.phpSessionId = &phpSessionId;
	resp.data = &respData;

	// Запрос на подключение к серверу
	if(client->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_TEST, "Http::Client::sendRequest() failed");
		return false;
	}
	TEST_STRING_EQUAL("<connect:test.sambery.net,81,1>", result->getString());

	// Посылка запроса к серверу
	result->clear();
	Event event(TcpIp::Event_ConnectOk);
	client->proc(&event);
	TEST_STRING_EQUAL(
		"<send="
		"POST /server/json/Login.php?action=Login&_dc=1234567 HTTP/1.1\r\n"
		"Host: test.sambery.net:81\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 69\r\n"
		"Cache-Control: no-cache\r\n"
		"Connection: keep-alive\r\n\r\n"
		",len=215>",
		result->getString());

	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);
	TEST_STRING_EQUAL(
		"<send={\"login\":\"pyroman\",\"hash\":\"cfc54c0fb8197c79defc7e8a5bc1d1d1c1becfbb\"},len=69>",
		result->getString());

	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);
	TEST_STRING_EQUAL("", result->getString());

	// Получение ответа от сервера
	result->clear();
	tcpIp->setRecvDataFlag(true);
	event.set(TcpIp::TcpIp::Event_IncomingData);
	client->proc(&event);
	TEST_STRING_EQUAL("<recv=1024>", result->getString());

	result->clear();
	const char *resp1part1 =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.9.5\r\n"
		"Date: Sun, 24 Jan 2016 10:00:04 GMT\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 16\r\n"
		"Connection: keep-alive\r\n"
		"Keep-Alive: timeout=30\r\n"
		"X-Powered-By: PHP/5.5.31\r\n"
		"Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n"
		"Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"
		"Pragma: no-cache\r\n"
		"Set-Cookie: PHPSESSID=8a4bb025730a7f81fec32d9358c0e005; expires=Sun, 24-Jan-2016 12:00:04 GMT; Max-Age=7200; path=/\r\n"
		"\r\n"
		"{\"success\":true}";
	tcpIp->addRecvString(resp1part1, false);
	TEST_STRING_EQUAL("<Http::Client::Event_RequestComplete>", observer->getResult());
	TEST_SUBSTR_EQUAL("{\"success\":true}", (const char*)resp.data->getData(), resp.data->getLen());
	TEST_SUBSTR_EQUAL("8a4bb025730a7f81fec32d9358c0e005", (const char*)phpSessionId.getData(), phpSessionId.getLen());

	// Второй запрос
	StringBuilder reqData2("data1234567890");
	req.serverPath = "/server/json/Audit.php?action=Import&_dc=1234567";
	req.keepAlive = false;
	req.phpSessionId = (const char*)phpSessionId.getData();
	req.data = &reqData2;

	respData.clear();

	// Посылка запроса к серверу
	observer->clearResult();
	result->clear();
	if(client->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_TEST, "Http::Client::sendRequest() failed");
		return false;
	}
	TEST_STRING_EQUAL(
		"<send="
		"POST /server/json/Audit.php?action=Import&_dc=1234567 HTTP/1.1\r\n"
		"Host: test.sambery.net:81\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 14\r\n"
		"Cache-Control: no-cache\r\n"
		"Cookie: PHPSESSID=8a4bb025730a7f81fec32d9358c0e005;\r\n\r\n"
		",len=245>",
		result->getString());

	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);
	TEST_STRING_EQUAL("<send=data1234567890,len=14>", result->getString());

	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);

	// Получение ответа от сервера
	result->clear();
	tcpIp->setRecvDataFlag(true);
	event.set(TcpIp::TcpIp::Event_IncomingData);
	client->proc(&event);
	TEST_STRING_EQUAL("<recv=1024>", result->getString());

	result->clear();
	const char *resp2part1 =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.9.5\r\n"
		"Date: Sun, 24 Jan 2016 10:00:04 GMT\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 16\r\n"
		"Connection: keep-alive\r\n"
		"Keep-Alive: timeout=30\r\n"
		"X-Powered-By: PHP/5.5.31\r\n"
		"Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n"
		"Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"
		"Pragma: no-cache\r\n"
		"Set-Cookie: PHPSESSID=8a4bb025730a7f81fec32d9358c0e005; expires=Sun, 24-Jan-2016 12:00:04 GMT; Max-Age=7200; path=/\r\n"
		"\r\n"
		"{\"success\":true}";
	tcpIp->addRecvString(resp2part1, false);
	TEST_STRING_EQUAL("<close>", result->getString());
	TEST_STRING_EQUAL("", observer->getResult());
	TEST_NUMBER_EQUAL(Response::Status_OK, resp.statusCode);
	TEST_SUBSTR_EQUAL("{\"success\":true}", (const char*)resp.data->getData(), resp.data->getLen());
	TEST_SUBSTR_EQUAL("8a4bb025730a7f81fec32d9358c0e005", (const char*)phpSessionId.getData(), phpSessionId.getLen());

	// Завершение соединения
	result->clear();
	event.set(TcpIp::TcpIp::Event_Close);
	client->proc(&event);
	TEST_STRING_EQUAL("<Http::Client::Event_RequestComplete>", observer->getResult());
	return true;
}

bool ClientTest::testTcpIpConnectError() {
	Request req;
	StringBuilder reqData("{\"login\":\"pyroman\",\"hash\":\"cfc54c0fb8197c79defc7e8a5bc1d1d1c1becfbb\"}");
	req.serverName = "test.sambery.net";
	req.serverPort = 81;
	req.method = Http::Request::Method_POST;
	req.serverPath = "/server/json/Login.php?action=Login&_dc=1234567";
	req.data = &reqData;

	Response resp;
	StringBuilder respData(1024, 1024);
	resp.data = &respData;

	// Send request
	if(client->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_TEST, "Http::Client::sendRequest() failed");
		return false;
	}
	TEST_STRING_EQUAL("<connect:test.sambery.net,81,1>", result->getString());
	result->clear();

	// Connect failed
	Event event(TcpIp::Event_ConnectError);
	client->proc(&event);
	TEST_STRING_EQUAL("", result->getString());
	result->clear();
	TEST_STRING_EQUAL("<Http::Client::Event_RequestError>", observer->getResult());
	return true;
}

bool ClientTest::testTcpIpSendError() {
	Request req;
	StringBuilder reqData("{\"login\":\"pyroman\",\"hash\":\"cfc54c0fb8197c79defc7e8a5bc1d1d1c1becfbb\"}");
	req.serverName = "test.sambery.net";
	req.serverPort = 81;
	req.method = Http::Request::Method_POST;
	req.serverPath = "/server/json/Login.php?action=Login&_dc=1234567";
	req.data = &reqData;

	Response resp;
	StringBuilder respData(1024, 1024);
	resp.data = &respData;

	// Send request
	if(client->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_TEST, "Http::Client::sendRequest() failed");
		return false;
	}

	// connect
	TEST_STRING_EQUAL("<connect:test.sambery.net,81,1>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", observer->getResult());
	Event event(TcpIp::Event_ConnectOk);
	client->proc(&event);

	// send data
	TEST_STRING_EQUAL(
		"<send="
		"POST /server/json/Login.php?action=Login&_dc=1234567 HTTP/1.1\r\n"
		"Host: test.sambery.net:81\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 69\r\n"
		"Cache-Control: no-cache\r\n\r\n"
		",len=191>",
		result->getString());
	result->clear();
	TEST_STRING_EQUAL("", observer->getResult());
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);

	// send data
	TEST_STRING_EQUAL(
		"<send={\"login\":\"pyroman\",\"hash\":\"cfc54c0fb8197c79defc7e8a5bc1d1d1c1becfbb\"},len=69>",
		result->getString());
	result->clear();
	event.set(TcpIp::Event_SendDataError);
	client->proc(&event);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("<Http::Client::Event_RequestError>", observer->getResult());
	return true;
}

bool ClientTest::testTcpIpRecvError() {
	Request req;
	StringBuilder reqData("{\"login\":\"pyroman\",\"hash\":\"cfc54c0fb8197c79defc7e8a5bc1d1d1c1becfbb\"}");
	req.serverName = "test.sambery.net";
	req.serverPort = 81;
	req.method = Http::Request::Method_POST;
	req.serverPath = "/server/json/Login.php?action=Login&_dc=1234567";
	req.data = &reqData;

	Response resp;
	StringBuilder respData(1024, 1024);
	resp.data = &respData;

	// Send request
	if(client->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_TEST, "Http::Client::sendRequest() failed");
		return false;
	}

	// connect
	TEST_STRING_EQUAL("<connect:test.sambery.net,81,1>", result->getString());
	result->clear();
	Event event(TcpIp::Event_ConnectOk);
	client->proc(&event);

	// send data
	TEST_STRING_EQUAL(
		"<send="
		"POST /server/json/Login.php?action=Login&_dc=1234567 HTTP/1.1\r\n"
		"Host: test.sambery.net:81\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 69\r\n"
		"Cache-Control: no-cache\r\n\r\n"
		",len=191>",
		result->getString());
	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);

	// send data
	TEST_STRING_EQUAL(
		"<send={\"login\":\"pyroman\",\"hash\":\"cfc54c0fb8197c79defc7e8a5bc1d1d1c1becfbb\"},len=69>",
		result->getString());
	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);
	TEST_STRING_EQUAL("", result->getString());

	// incomming data event
	tcpIp->setRecvDataFlag(true);
	event.set(TcpIp::TcpIp::Event_IncomingData);
	client->proc(&event);

	// recv data
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();
	const char *resp1part1 =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.9.5\r\n"
		"Date: Sun, 24 Jan 2016 10:00:04 GMT\r\n"
		"Content-Ty";
	tcpIp->addRecvString(resp1part1, true);
	TEST_STRING_EQUAL("", observer->getResult());

	// recv data
	TEST_STRING_EQUAL("<recv=1014>", result->getString());
	result->clear();
	event.setUint16(TcpIp::TcpIp::Event_RecvDataError, 0);
	client->proc(&event);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("<Http::Client::Event_RequestError>", observer->getResult());
	return true;
}

bool ClientTest::testHttpParserError() {
	Request req;
	StringBuilder reqData("{\"login\":\"pyroman\",\"hash\":\"cfc54c0fb8197c79defc7e8a5bc1d1d1c1becfbb\"}");
	req.serverName = "test.sambery.net";
	req.serverPort = 81;
	req.method = Http::Request::Method_POST;
	req.serverPath = "/server/json/Login.php?action=Login&_dc=1234567";
	req.data = &reqData;

	Response resp;
	StringBuilder respData(1024, 1024);
	resp.data = &respData;

	// Send request
	if(client->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_TEST, "Http::Client::sendRequest() failed");
		return false;
	}

	// connect
	TEST_STRING_EQUAL("<connect:test.sambery.net,81,1>", result->getString());
	result->clear();
	Event event(TcpIp::Event_ConnectOk);
	client->proc(&event);

	// send data
	TEST_STRING_EQUAL(
		"<send="
		"POST /server/json/Login.php?action=Login&_dc=1234567 HTTP/1.1\r\n"
		"Host: test.sambery.net:81\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 69\r\n"
		"Cache-Control: no-cache\r\n\r\n"
		",len=191>",
		result->getString());
	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);

	// send data
	TEST_STRING_EQUAL(
		"<send={\"login\":\"pyroman\",\"hash\":\"cfc54c0fb8197c79defc7e8a5bc1d1d1c1becfbb\"},len=69>",
		result->getString());
	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);
	TEST_STRING_EQUAL("", result->getString());

	// incomming data event
	tcpIp->setRecvDataFlag(true);
	event.set(TcpIp::TcpIp::Event_IncomingData);
	client->proc(&event);

	// recv broken data
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();
	const char *resp1part1 =
		"0123456789ABCDE\r\n"
		"Server: nginx/1.9.5\r\n"
		"Date: Sun, 24 Jan 2016 10:00:04 GMT\r\n"
		"Content-Ty";
	tcpIp->addRecvString(resp1part1, true);

	// recv data
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();
	const char *resp1part2 =
		"pe: text/html\r\n"
		"Content-Length: 16\r\n"
		"Connection: keep-alive\r\n"
		"Keep-Alive: timeout=30\r\n"
		"X-Powered-By: PHP/5.5.31\r\n"
		"Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n"
		"Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"
		"Pragma: no-cache\r\n";
	tcpIp->addRecvString(resp1part2, true);

	// recv data
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();
	const char *resp1part3 =
		"Set-Cookie: PHPSESSID=8a4bb025730a7f81fec32d9358c0e005; expires=Sun, 24-Jan-2016 12:00:04 GMT; Max-Age=7200; path=/\r\n"
		"\r\n"
		"{\"success\":true}";
	tcpIp->addRecvString(resp1part3, false);

	// wait data
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", observer->getResult());

	// recv timeout
	timers->tick(40000);
	timers->execute();
	TEST_STRING_EQUAL("<close>", result->getString());
	TEST_STRING_EQUAL("", observer->getResult());
	TEST_NUMBER_EQUAL(Response::Status_ParserError, resp.statusCode);

	// Завершение соединения
	result->clear();
	event.set(TcpIp::TcpIp::Event_Close);
	client->proc(&event);
	TEST_STRING_EQUAL("<Http::Client::Event_RequestError>", observer->getResult());
	return true;
}

}
