#include "lib/erp/ErpCommandProcessor.h"
#include "timer/include/TestRealTime.h"
#include "event/include/TestEventEngine.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class TestErpCommandProcessorEventEngine : public TestEventEngine {
public:
	TestErpCommandProcessorEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case MdbMasterCashless::Event_SessionBegin: {
			EventUint32Interface event(MdbMasterCashless::Event_SessionBegin);
			event.open(envelope);
			*result << "<deposite=" << event.getValue() << ">";
			break;
		}
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}
};

class TestHttpServer : public Http::ServerInterface {
public:
	TestHttpServer(StringBuilder *result) : result(result) {}
	void setObserver(EventObserver *observer) override { courier.setRecipient(observer); }
	bool accept(Http::Request2 *req) override {
		this->req = req;
		*result << "<accept>";
		return true;
	}
	bool sendResponse(const Http::Response *resp) override {
		*result << "<sendResponse:"
			"status=" << resp->statusCode << ","
			"data=" << *resp->data << ">";
		return true;
	}
	bool close() override {
		*result << "<close>";
		return true;
	}

	Http::Request2* getReq() { return req; }
	void incomintRequest() {
		Event event(Http::Server::Event_RequestIncoming);
		courier.deliver(&event);
	}
	void sendComplete() {
		Event event(Http::Server::Event_ResponseComplete);
		courier.deliver(&event);
	}
	void remoteClose() {
		Event event(Http::Server::Event_ResponseError);
		courier.deliver(&event);
	}

private:
	StringBuilder *result;
	Http::Request2 *req;
	EventCourier courier;
};

class ErpCommandProcessorTest : public TestSet {
public:
	ErpCommandProcessorTest();
	bool init();
	void cleanup();
	bool testRegister();

private:
	StringBuilder *result;
	TestRealTime *realtime;
	Mdb::DeviceContext *context;
	TestErpCommandProcessorEventEngine *eventEngine;
	TimerEngine timerEngine;
	TestHttpServer *server;
	ErpCommandProcessor *processor;

	bool gotoStatePollDelay();
};

TEST_SET_REGISTER(ErpCommandProcessorTest);

ErpCommandProcessorTest::ErpCommandProcessorTest() {
	TEST_CASE_REGISTER(ErpCommandProcessorTest, testRegister);
}

bool ErpCommandProcessorTest::init() {
	result = new StringBuilder;
	realtime = new TestRealTime;
	context = new Mdb::DeviceContext(2, realtime);
	eventEngine = new TestErpCommandProcessorEventEngine(result);
	server = new TestHttpServer(result);
	processor = new ErpCommandProcessor(context, &timerEngine, eventEngine);
	processor->bind(server);
	return true;
}

void ErpCommandProcessorTest::cleanup() {
	delete this->processor;
	delete this->server;
	delete this->eventEngine;
	delete this->context;
	delete this->realtime;
	delete this->result;
}

bool ErpCommandProcessorTest::testRegister() {
	// accept
	processor->accept();
	TEST_STRING_EQUAL("<accept>", result->getString());
	result->clear();

	// incomming request
	Http::Request2 *req = server->getReq();
	TEST_NUMBER_EQUAL(0, (int)req->serverName);
	req->serverPort = 80;
	TEST_NUMBER_EQUAL(0, (int)req->serverPath);
	req->contentType = Http::ContentType_Json;
	*req->data << "{\"command\":5,\"value\":3500}";
	server->incomintRequest();
	TEST_STRING_EQUAL("<deposite=3500><sendResponse:status=200,data={\"success\":true}>", result->getString());
	result->clear();

	// send complete
	server->sendComplete();
	TEST_STRING_EQUAL("", result->getString());
	return true;
}
