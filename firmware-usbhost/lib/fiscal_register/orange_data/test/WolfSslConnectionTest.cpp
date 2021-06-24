#include "lib/fiscal_register/orange_data/WolfSslConnection.h"
#include "lib/fiscal_register/orange_data/WolfSslAdapter.h"
#include "event/include/TestEventEngine.h"
#include "timer/include/TimerEngine.h"
#include "timer/include/TestRealTime.h"
#include "http/test/TestTcpIp.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class TestWolfSslConnectionObserver : public TestEventEngine {
public:
	TestWolfSslConnectionObserver(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventInterface *event) {
/*		switch(event->getType()) {
		case Fiscal::Register::Event_CommandOK: *result << "<event=CommandOK>"; break;
		case Fiscal::Register::Event_CommandError: procEventError(event); break;
		default: TestEventEngine::transmit(event);
		}*/
		return true;
	}

private:
	void procEventError(EventInterface *event) {
//		Fiscal::EventError event2;
//		if(convertEvent(event, &event2) == false) { return; }
//		*result << "<event=CommandError," << event2.code << "," << event2.data.getString() << ">";
	}
};

class WolfSslConnectionTest : public TestSet {
public:
	WolfSslConnectionTest();
	bool init();
	void cleanup();
	bool test();

private:
	StringBuilder *result;
	TimerEngine *timerEngine;
	TestTcpIp *conn;
	WolfSslAdapter *adapter;
	WolfSslConnection *layer;
};

TEST_SET_REGISTER(WolfSslConnectionTest);

WolfSslConnectionTest::WolfSslConnectionTest() {
	TEST_CASE_REGISTER(WolfSslConnectionTest, test);
}

bool WolfSslConnectionTest::init() {
	this->result = new StringBuilder(2048, 2048);
	this->timerEngine = new TimerEngine;
	this->conn = new TestTcpIp(1024, result);
	this->adapter = new WolfSslAdapter(conn);
	this->layer = new WolfSslConnection(adapter);
//	layer->init();
	return true;
}

void WolfSslConnectionTest::cleanup() {
	delete this->layer;
	delete this->adapter;
	delete this->conn;
	delete this->timerEngine;
	delete this->result;
}

bool WolfSslConnectionTest::test() {	
#if 0
	// connect
	TEST_NUMBER_EQUAL(true, layer->connect("test", 1234, TcpIp::Mode_TcpIpOverSsl));
	TEST_STRING_EQUAL("<connect:test,1234,0>", result->getString());
	result->clear();

	// connected
	Event event1(TcpIp::Event_ConnectOk);
	adapter->proc(&event1);
	TEST_SUBSTR_EQUAL("<send=68,data=160303003F0100003B030", result->getString(), 35);
	result->clear();
#endif

/*
	// connect complete
	Event event1(TcpIp::Event_ConnectOk);
	layer->proc(&event1);
	TEST_STRING_EQUAL("<event=OK>", result->getString());
	result->clear();

	// send data
	Buffer data(32);
	data.addUint8(0xC1);
	data.addUint8(0x01);
	data.addUint8(0x02);
	data.addUint8(0x00);
	data.addUint8(0x00);
	data.addUint8(0xA5);
	layer->sendPacket(data.getData(), data.getLen());
	TEST_STRING_EQUAL("<send=11,data=FE060002C101020000A593>", result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<recv=120>", result->getString());
	result->clear();

	// stx
	conn->addRecvData("FE");
	TEST_STRING_EQUAL("<recv=120>", result->getString());
	result->clear();

	// len
	conn->addRecvData("0600");
	TEST_STRING_EQUAL("<recv=120>", result->getString());
	result->clear();

	// data
	conn->addRecvData("01C101020000A5");
	TEST_STRING_EQUAL("<recv=120>", result->getString());
	result->clear();

	// crc
	conn->addRecvData("99");
	TEST_STRING_EQUAL("<event=RecvData,id=1,data=C101020000A5>", result->getString());
	result->clear();

	// disconnect
	layer->disconnect();
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// connect complete
	Event event2(TcpIp::Event_Close);
	layer->proc(&event2);
	TEST_STRING_EQUAL("<event=OK>", result->getString());
	result->clear();*/
	return true;
}
