#include "fiscal_register/atol/AtolPacketLayer.h"
#include "fiscal_register/atol/AtolProtocol.h"
#include "timer/include/TimerEngine.h"
#include "http/test/TestTcpIp.h"
#include "utils/include/TestEventObserver.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class TestAtolPacketLayerObserver : public Atol::PacketLayerObserver {
public:
	TestAtolPacketLayerObserver(StringBuilder *str) : str(str) {}
	virtual void procRecvData(uint8_t packetId, const uint8_t *data, const uint16_t dataLen) {
		*str << "<event=RecvData,id=" << packetId << ",data=";
		for(uint16_t i = 0; i < dataLen; i++) {
			str->addHex(data[i]);
		}
		*str << ">";
	}
	virtual void procError(Atol::PacketLayerObserver::Error error) {
		switch(error) {
		case PacketLayerObserver::Error_OK: *str << "<event=OK>"; return;
		case PacketLayerObserver::Error_ConnectFailed: *str << "<event=ConnectFailed>"; return;
		case PacketLayerObserver::Error_RemoteClose: *str << "<event=RemoteClose>"; return;
		case PacketLayerObserver::Error_SendFailed: *str << "<event=SendFailed>"; return;
		case PacketLayerObserver::Error_RecvFailed: *str << "<event=RecvFailed>"; return;
		case PacketLayerObserver::Error_RecvTimeout: *str << "<event=RecvTimeout>"; return;
		default: *str << "<event=" << error << ">";
		}
	}

private:
	StringBuilder *str;
};

class AtolPacketLayerTest : public TestSet {
public:
	AtolPacketLayerTest();
	bool init();
	void cleanup();
	bool testSendRecv();
	bool testRecvTimeout();
	bool testRemoteCloseWhenWait();
	bool testRemoteCloseWhenSend();
	bool testRemoteCloseWhenRecv();

private:
	StringBuilder *result;
	TestTcpIp *conn;
	TimerEngine *timerEngine;
	TestAtolPacketLayerObserver *observer;
	Atol::PacketLayer *layer;
};

TEST_SET_REGISTER(AtolPacketLayerTest);

AtolPacketLayerTest::AtolPacketLayerTest() {
	TEST_CASE_REGISTER(AtolPacketLayerTest, testSendRecv);
	TEST_CASE_REGISTER(AtolPacketLayerTest, testRecvTimeout);
	TEST_CASE_REGISTER(AtolPacketLayerTest, testRemoteCloseWhenWait);
	TEST_CASE_REGISTER(AtolPacketLayerTest, testRemoteCloseWhenSend);
	TEST_CASE_REGISTER(AtolPacketLayerTest, testRemoteCloseWhenRecv);
}

bool AtolPacketLayerTest::init() {
	this->result = new StringBuilder(2048, 2048);
	this->timerEngine = new TimerEngine();
	this->conn = new TestTcpIp(256, result);
	this->observer = new TestAtolPacketLayerObserver(result);
	this->layer = new Atol::PacketLayer(this->timerEngine, this->conn);
	this->layer->setObserver(observer);
	return true;
}

void AtolPacketLayerTest::cleanup() {
	delete this->layer;
	delete this->observer;
	delete this->conn;
	delete this->timerEngine;
	delete this->result;
}

bool AtolPacketLayerTest::testSendRecv() {
	// connect
	layer->connect("test", 1234, TcpIp::Mode_TcpIp);
	TEST_STRING_EQUAL("<connect:test,1234,0>", result->getString());
	result->clear();

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
	TEST_STRING_EQUAL("<send=FE060002C101020000A593,len=11>", result->getString());
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
	result->clear();
	return true;
}

bool AtolPacketLayerTest::testRecvTimeout() {
	// connect
	layer->connect("test", 1234, TcpIp::Mode_TcpIp);
	TEST_STRING_EQUAL("<connect:test,1234,0>", result->getString());
	result->clear();

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
	TEST_STRING_EQUAL("<send=FE060002C101020000A593,len=11>", result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<recv=120>", result->getString());
	result->clear();

	// timeout
	timerEngine->tick(ATOL_PACKET_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=RecvTimeout>", result->getString());
	result->clear();
	return true;
}

bool AtolPacketLayerTest::testRemoteCloseWhenWait() {
	// connect
	layer->connect("test", 1234, TcpIp::Mode_TcpIp);
	TEST_STRING_EQUAL("<connect:test,1234,0>", result->getString());
	result->clear();

	// connect complete
	Event event1(TcpIp::Event_ConnectOk);
	layer->proc(&event1);
	TEST_STRING_EQUAL("<event=OK>", result->getString());
	result->clear();

	// close
	conn->remoteClose();
	TEST_STRING_EQUAL("<event=RemoteClose>", result->getString());
	result->clear();
	return true;
}

bool AtolPacketLayerTest::testRemoteCloseWhenSend() {
	// connect
	layer->connect("test", 1234, TcpIp::Mode_TcpIp);
	TEST_STRING_EQUAL("<connect:test,1234,0>", result->getString());
	result->clear();

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
	TEST_STRING_EQUAL("<send=FE060002C101020000A593,len=11>", result->getString());
	result->clear();

	// close
	conn->remoteClose();
	TEST_STRING_EQUAL("<event=RemoteClose>", result->getString());
	result->clear();
	return true;
}

bool AtolPacketLayerTest::testRemoteCloseWhenRecv() {
	// connect
	layer->connect("test", 1234, TcpIp::Mode_TcpIp);
	TEST_STRING_EQUAL("<connect:test,1234,0>", result->getString());
	result->clear();

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
	TEST_STRING_EQUAL("<send=FE060002C101020000A593,len=11>", result->getString());
	result->clear();

	// send complete
	conn->sendComplete();
	TEST_STRING_EQUAL("<recv=120>", result->getString());
	result->clear();

	// stx
	conn->addRecvData("FE");
	TEST_STRING_EQUAL("<recv=120>", result->getString());
	result->clear();

	// close
	conn->remoteClose();
	TEST_STRING_EQUAL("<event=RemoteClose>", result->getString());
	result->clear();
	return true;
}
