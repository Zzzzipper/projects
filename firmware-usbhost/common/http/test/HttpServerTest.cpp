#include "test/include/Test.h"
#include "http/include/HttpServer.h"
#include "http/test/TestTcpIp.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/StringParser.h"
#include "utils/include/Buffer.h"
#include "utils/include/TestEventObserver.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace Http {

class TestServerObserver : public TestEventObserver {
public:
	TestServerObserver(StringBuilder *result) : TestEventObserver(result) {}
	virtual void proc(Event *event) {
		switch(event->getType()) {
			case Http::ServerInterface::Event_RequestIncoming: *result << "<event=RequestIncoming>"; return;
			case Http::ServerInterface::Event_ResponseComplete: *result << "<event=ResponseComplete>"; return;
			case Http::ServerInterface::Event_ResponseError: *result << "<event=ResponseError>"; return;
			default: *result << "<event=" << event->getType() << ">";
		}
	}
};
/*
class Connection2 {
public:
	enum EventType {
		Event_RequestIncoming	= GlobalId_HttpClient | 0x03,
		Event_ResponseComplete	= GlobalId_HttpClient | 0x04,
		Event_ResponseError		= GlobalId_HttpClient | 0x05,
 */
class ServerTest : public TestSet {
public:
	ServerTest();
	bool init();
	void cleanup();
	bool testPost();

private:
	StringBuilder *result;
	TimerEngine *timers;
	TestTcpIp *tcpIp;
	TestServerObserver *observer;
	Server *server;
};

TEST_SET_REGISTER(Http::ServerTest);

ServerTest::ServerTest() {
	TEST_CASE_REGISTER(ServerTest, testPost);
}

bool ServerTest::init() {
	result = new StringBuilder;
	timers = new TimerEngine;
	tcpIp = new TestTcpIp(1024, result, true);
	observer = new TestServerObserver(result);
	server = new Server(timers, tcpIp);
	server->setObserver(observer);
	return true;
}

void ServerTest::cleanup() {
	delete server;
	delete observer;
	delete tcpIp;
	delete timers;
	delete result;
}

bool ServerTest::testPost() {
	Request2 req;
	StringBuilder serverName(1024, 1024);
	StringBuilder serverPath(1024, 1024);
	StringBuilder reqData(1024, 1024);
	req.serverName = &serverName;
	req.serverPort = 0;
	req.method = Http::Request::Method_Unknown;
	req.serverPath = &serverPath;
	req.data = &reqData;

	Response resp;
	StringBuilder respData("{\"success\":true}");
	resp.statusCode = Status_OK;
	resp.contentType = Http::ContentType_Json;
	resp.data = &respData;

	// recv request
	TEST_NUMBER_EQUAL(true, server->accept(&req));

	const char *req1part1 =
		"POST /server/json/Login.php?action=Login&_dc=1234567 HTTP/1.1\r\n"
		"Host: test.sambery.net\r\n"
		"Content-Type: application/text/plain; charset=windows-1251\r\n"
		"Content";
	tcpIp->addRecvString(req1part1, true);
	TEST_STRING_EQUAL("<recv=1024>", result->getString());
	result->clear();

	const char *req1part2 =
		"-Length: 69\r\n"
		"Cache-Control: no-cache\r\n\r\n"
		"{\"login\":\"pyroman\",\"hash\":\"cfc54c0fb8197c79defc7e8a5bc1d1d1c1becfbb\"}";
	tcpIp->addRecvString(req1part2, true);
	TEST_STRING_EQUAL("<event=RequestIncoming>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("test.sambery.net", req.serverName->getString());
	TEST_STRING_EQUAL("/server/json/Login.php?action=Login&_dc=1234567", req.serverPath->getString());
	TEST_STRING_EQUAL("{\"login\":\"pyroman\",\"hash\":\"cfc54c0fb8197c79defc7e8a5bc1d1d1c1becfbb\"}", req.data->getString());

	// send header
	TEST_NUMBER_EQUAL(true, server->sendResponse(&resp));
	TEST_STRING_EQUAL(
		"<send="
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: application/json; charset=windows-1251\r\n"
		"Content-Length: 16\r\n"
		"Cache-Control: no-cache\r\n"
		"\r\n"
		",len=118>",
		result->getString());
	result->clear();

	// send data
	Event event;
	event.set(TcpIp::Event_SendDataOk);
	server->proc(&event);
	TEST_STRING_EQUAL(
		"<send={\"success\":true},len=16>",
		result->getString());
	result->clear();

	// close
	event.set(TcpIp::Event_SendDataOk);
	server->proc(&event);
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// closed
	event.set(TcpIp::TcpIp::Event_Close);
	server->proc(&event);
	TEST_STRING_EQUAL("<event=ResponseComplete>", result->getString());
	return true;
}

}
