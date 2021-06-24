#include "test/include/Test.h"
#include "http/include/HttpTransport.h"
#include "http/include/TestHttp.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/StringParser.h"
#include "utils/include/Buffer.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace Http {

class TestTransportObserver : public EventObserver  {
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
			case Http::ClientInterface::Event_RequestComplete: return "Http::Client::Event_RequestComplete";
			case Http::ClientInterface::Event_RequestError: return "Http::Client::Event_RequestError";
			default: return NULL;
		}
	}
};

class TransportTest : public TestSet {
public:
	TransportTest();
	bool testTcpIpError();
	bool testKeepALiveHttpError();
	bool testNoKeepALiveHttpError();
};

TEST_SET_REGISTER(Http::TransportTest);

TransportTest::TransportTest() {
	TEST_CASE_REGISTER(TransportTest, testTcpIpError);
	TEST_CASE_REGISTER(TransportTest, testKeepALiveHttpError);
	TEST_CASE_REGISTER(TransportTest, testNoKeepALiveHttpError);
}

bool TransportTest::testTcpIpError() {
	TestConnection client;
	TestTransportObserver observer;
	Transport transport(&client, 2);
	transport.setObserver(&observer);

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
	if(transport.sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_TEST, "Http::Transport::sendRequest() failed");
		return false;
	}

	// send request
	TEST_STRING_EQUAL("<setObserver><request>", client.getResult());
	client.clearResult();
	TEST_STRING_EQUAL("", observer.getResult());
	observer.clearResult();

	// send failed
	Event event(ClientInterface::Event_RequestError);
	transport.proc(&event);

	// resend request
	TEST_STRING_EQUAL("", client.getResult());
	client.clearResult();
	TEST_STRING_EQUAL("<Http::Client::Event_RequestError>", observer.getResult());
	observer.clearResult();

	return true;
}

bool TransportTest::testKeepALiveHttpError() {
	TestConnection client;
	TestTransportObserver observer;
	Transport transport(&client, 3);
	transport.setObserver(&observer);

	Request req;
	StringBuilder reqData("{\"login\":\"pyroman\",\"hash\":\"cfc54c0fb8197c79defc7e8a5bc1d1d1c1becfbb\"}");
	req.serverName = "test.sambery.net";
	req.serverPort = 81;
	req.method = Http::Request::Method_POST;
	req.keepAlive = true;
	req.serverPath = "/server/json/Login.php?action=Login&_dc=1234567";
	req.data = &reqData;

	Response resp;
	StringBuilder respData(1024, 1024);
	resp.data = &respData;

	// Send request
	if(transport.sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_TEST, "Http::Transport::sendRequest() failed");
		return false;
	}

	// send request, try 1
	TEST_STRING_EQUAL("<setObserver><request>", client.getResult());
	client.clearResult();
	TEST_STRING_EQUAL("", observer.getResult());
	observer.clearResult();

	// wrong status code
	resp.statusCode = Response::Status_ServerError;
	Event event1(ClientInterface::Event_RequestComplete);
	transport.proc(&event1);

	// send request
	TEST_STRING_EQUAL("<close>", client.getResult());
	client.clearResult();
	TEST_STRING_EQUAL("", observer.getResult());
	observer.clearResult();

	// close succeed
	Event event2(ClientInterface::Event_RequestComplete);
	transport.proc(&event2);

	// send request, try 2
	TEST_STRING_EQUAL("<request>", client.getResult());
	client.clearResult();
	TEST_STRING_EQUAL("", observer.getResult());
	observer.clearResult();

	// wrong status code
	resp.statusCode = Response::Status_ServerError;
	Event event3(ClientInterface::Event_RequestComplete);
	transport.proc(&event3);

	// send request
	TEST_STRING_EQUAL("<close>", client.getResult());
	client.clearResult();
	TEST_STRING_EQUAL("", observer.getResult());
	observer.clearResult();

	// close succeed
	Event event4(ClientInterface::Event_RequestComplete);
	transport.proc(&event4);

	// send request, try 3
	TEST_STRING_EQUAL("<request>", client.getResult());
	client.clearResult();
	TEST_STRING_EQUAL("", observer.getResult());
	observer.clearResult();

	// wrong status code
	resp.statusCode = Response::Status_ServerError;
	Event event5(ClientInterface::Event_RequestComplete);
	transport.proc(&event5);

	// too many tries, return error
	TEST_STRING_EQUAL("", client.getResult());
	client.clearResult();
	TEST_STRING_EQUAL("<Http::Client::Event_RequestError>", observer.getResult());
	observer.clearResult();

	return true;
}

bool TransportTest::testNoKeepALiveHttpError() {
	TestConnection client;
	TestTransportObserver observer;
	Transport transport(&client, 3);
	transport.setObserver(&observer);

	Request req;
	StringBuilder reqData("{\"login\":\"pyroman\",\"hash\":\"cfc54c0fb8197c79defc7e8a5bc1d1d1c1becfbb\"}");
	req.serverName = "test.sambery.net";
	req.serverPort = 81;
	req.method = Http::Request::Method_POST;
	req.keepAlive = false;
	req.serverPath = "/server/json/Login.php?action=Login&_dc=1234567";
	req.data = &reqData;

	Response resp;
	StringBuilder respData(1024, 1024);
	resp.data = &respData;

	// Send request
	if(transport.sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_TEST, "Http::Transport::sendRequest() failed");
		return false;
	}

	// send request, try 1
	TEST_STRING_EQUAL("<setObserver><request>", client.getResult());
	client.clearResult();
	TEST_STRING_EQUAL("", observer.getResult());
	observer.clearResult();

	// wrong status code
	client.setCloseReturn(false);
	resp.statusCode = Response::Status_ServerError;
	Event event1(ClientInterface::Event_RequestComplete);
	transport.proc(&event1);

	// send request, try 2
	TEST_STRING_EQUAL("<close><request>", client.getResult());
	client.clearResult();
	TEST_STRING_EQUAL("", observer.getResult());
	observer.clearResult();

	// wrong status code
	client.setCloseReturn(false);
	resp.statusCode = Response::Status_ServerError;
	Event event3(ClientInterface::Event_RequestComplete);
	transport.proc(&event3);

	// send request, try 3
	TEST_STRING_EQUAL("<close><request>", client.getResult());
	client.clearResult();
	TEST_STRING_EQUAL("", observer.getResult());
	observer.clearResult();

	// wrong status code
	client.setCloseReturn(false);
	resp.statusCode = Response::Status_ServerError;
	Event event5(ClientInterface::Event_RequestComplete);
	transport.proc(&event5);

	// too many tries, return error
	TEST_STRING_EQUAL("", client.getResult());
	client.clearResult();
	TEST_STRING_EQUAL("<Http::Client::Event_RequestError>", observer.getResult());
	observer.clearResult();

	return true;
}

}
