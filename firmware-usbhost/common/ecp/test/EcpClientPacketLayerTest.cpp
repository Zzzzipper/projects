#if 1
#include "ecp/EcpClientPacketLayer.h"
#include "ecp/EcpProtocol.h"
#include "timer/include/TimerEngine.h"
#include "uart/include/TestUart.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class TestEcpPacketLayerObserver : public Ecp::ClientPacketLayerInterface::Observer {
public:
	virtual ~TestEcpPacketLayerObserver() {}
	void clear() { str.clear(); }
	const char *getResult() { return str.getString(); }

	virtual void procConnect() { str << "<connect>"; }
	virtual void procRecvData(const uint8_t *data, uint16_t dataLen) {
		for(uint16_t i = 0; i < dataLen; i++) {
			str.addHex(data[i]);
		}
	}
	virtual void procRecvError(Ecp::Error error) { str << "<error=" << error << ">"; }
	virtual void procDisconnect() { str << "<disconnect>"; }

private:
	StringBuilder str;
};

class EcpClientPacketLayerTest : public TestSet {
public:
	EcpClientPacketLayerTest();
	bool init();
	void cleanup();
	bool testKeepAlive();
	bool testSendRecv();
	bool testResponseTimeout();
	bool testDisconnectNotConnected();

private:
	TestUart *uart;
	TimerEngine *timerEngine;
	TestEcpPacketLayerObserver *observer;
	Ecp::ClientPacketLayer *client;
};

TEST_SET_REGISTER(EcpClientPacketLayerTest);

EcpClientPacketLayerTest::EcpClientPacketLayerTest() {
	TEST_CASE_REGISTER(EcpClientPacketLayerTest, testKeepAlive);
	TEST_CASE_REGISTER(EcpClientPacketLayerTest, testSendRecv);
	TEST_CASE_REGISTER(EcpClientPacketLayerTest, testResponseTimeout);
	TEST_CASE_REGISTER(EcpClientPacketLayerTest, testDisconnectNotConnected);
}

bool EcpClientPacketLayerTest::init() {
	this->uart = new TestUart(256);
	this->timerEngine = new TimerEngine();
	this->observer = new TestEcpPacketLayerObserver;
	this->client = new Ecp::ClientPacketLayer(this->timerEngine, this->uart);
	this->client->setObserver(observer);
	return true;
}

void EcpClientPacketLayerTest::cleanup() {
	delete this->client;
	delete this->observer;
	delete this->timerEngine;
	delete this->uart;
}

bool EcpClientPacketLayerTest::testKeepAlive() {
	// Start connecting/Send ENQ
	TEST_NUMBER_EQUAL(true, client->connect());
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv ACK
	uart->addRecvData(Ecp::Control_ACK);
	TEST_STRING_EQUAL("<connect>", observer->getResult());
	observer->clear();

	// Send Setup (STX,<len>,01,<crc>)
	Buffer data(32);
	data.addUint8(0x01);
	client->sendData(&data);
	TEST_HEXDATA_EQUAL("02010100", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv ACK
	uart->addRecvData("0602010003");
	timerEngine->tick(ECP_DELIVER_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("0100", observer->getResult());
	observer->clear();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());

	// No activity
	timerEngine->tick(ECP_KEEP_ALIVE_PERIOD);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// No answer
	timerEngine->tick(ECP_CONFIRM_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("<disconnect>", observer->getResult());
	return true;
}

bool EcpClientPacketLayerTest::testSendRecv() {
	// Start connecting/Send ENQ
	TEST_NUMBER_EQUAL(true, client->connect());
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv ACK
	uart->addRecvData(Ecp::Control_ACK);
	TEST_STRING_EQUAL("<connect>", observer->getResult());
	observer->clear();

	// send data
	Buffer data(32);
	data.addUint8(0x30);
	data.addUint8(0x01);
	data.addUint8(0x00);
	data.addUint8(0x00);
	data.addUint8(0x00);
	client->sendData(&data);
	TEST_HEXDATA_EQUAL("0205300100000034", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());

	// ACK
	uart->addRecvData(Ecp::Control_ACK);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// len
	uart->addRecvData("05");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// data
	uart->addRecvData("3001000000");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// crc
	uart->addRecvData("34");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	timerEngine->tick(ECP_CONFIRM_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("3001000000", observer->getResult());
	return true;
}

bool EcpClientPacketLayerTest::testResponseTimeout() {
	// Start connecting/Send ENQ
	TEST_NUMBER_EQUAL(true, client->connect());
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv ACK
	uart->addRecvData(Ecp::Control_ACK);
	TEST_STRING_EQUAL("<connect>", observer->getResult());
	observer->clear();

	// send data
	Buffer data(32);
	data.addUint8(0x01);
	data.addUint8(0x02);
	data.addUint8(0x03);
	data.addUint8(0x04);
	client->sendData(&data);
	TEST_HEXDATA_EQUAL("02040102030400", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());

	// ACK
	uart->addRecvData(Ecp::Control_ACK);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// len
	uart->addRecvData("0005");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// timeout
	timerEngine->tick(ECP_CONFIRM_TIMEOUT);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<error=4>", observer->getResult());
	return true;
}

bool EcpClientPacketLayerTest::testDisconnectNotConnected() {
	client->disconnect();
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("<disconnect>", observer->getResult());
	return true;
}
#else
#include "ecp/EcpClientPacketLayer.h"
#include "ecp/EcpProtocol.h"
#include "timer/include/Engine.h"
#include "uart/include/TestUart.h"
#include "utils/include/TestEvent.h"
#include "test/include/Test.h"

class TestEcpClientPacketLayerObserver : public Ecp::ClientPacketLayer::Observer {
public:
	virtual ~TestEcpClientPacketLayerObserver() {}
	void clear() { str.clear(); }
	const char *getResult() { return str.getString(); }

	virtual void procConnect() { str.addStr("<connect>"); }
	virtual void procRecvData(const uint8_t *data, uint16_t dataLen) {
		for(uint16_t i = 0; i < dataLen; i++) {
			str.addHex(data[i]);
		}
	}
	virtual void procError(uint8_t code) { str << "<error=" << code << ">"; }
	virtual void procDisconnect() { str.addStr("<disconnect>"); }

private:
	StringBuilder str;
};

class EcpClientPacketLayerTest : public TestSet {
public:
	EcpClientPacketLayerTest();
	void init();
	void cleanup();
	bool testKeepAlive();
	bool testDisconnect();
	bool testRemoteDisconnectKeepAlive();
	bool testRemoteDisconnectRequest();

private:
	TestUart *uart;
	TimerEngine *timerEngine;
	TestEcpClientPacketLayerObserver *observer;
	Ecp::Crc *crc;
	Ecp::ClientPacketLayer *client;

	bool recvAnswer(uint8_t command, uint8_t resultCode);
};

TEST_SET_REGISTER(EcpClientPacketLayerTest);

EcpClientPacketLayerTest::EcpClientPacketLayerTest() {
	TEST_CASE_REGISTER(EcpClientPacketLayerTest, testKeepAlive);
	TEST_CASE_REGISTER(EcpClientPacketLayerTest, testDisconnect);
	TEST_CASE_REGISTER(EcpClientPacketLayerTest, testRemoteDisconnectKeepAlive);
	TEST_CASE_REGISTER(EcpClientPacketLayerTest, testRemoteDisconnectRequest);
}

void EcpClientPacketLayerTest::init() {
	this->uart = new TestUart(256);
	this->timerEngine = new TimerEngine();
	this->observer = new TestEcpClientPacketLayerObserver;
	this->crc = new Ecp::Crc;
	this->client = new Ecp::ClientPacketLayer(this->uart, this->timerEngine, observer);
}

void EcpClientPacketLayerTest::cleanup() {
	delete this->client;
	delete this->crc;
	delete this->observer;
	delete this->timerEngine;
	delete this->uart;
}

bool EcpClientPacketLayerTest::recvAnswer(uint8_t command, uint8_t resultCode) {
	uart->addRecvData(Ecp::Control_ACK);
	uart->addRecvData(2);
	uart->addRecvData(command);
	uart->addRecvData(resultCode);
	crc->start(2);
	crc->add(command);
	crc->add(resultCode);
	uart->addRecvData(crc->getCrc());
	timerEngine->tick(10);
	timerEngine->execute();
	return true;
}

bool EcpClientPacketLayerTest::testKeepAlive() {
	// Start connecting/Send ENQ
	TEST_NUMBER_EQUAL(true, client->connect());
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv ACK
	uart->addRecvData(Ecp::Control_ACK);
	TEST_STRING_EQUAL("<connect>", observer->getResult());
	observer->clear();

	// Send Setup (STX,<len>,01,<crc>)
	Buffer data(20);
	data.addUint8(0x01);
	client->sendRequest(&data);
	TEST_HEXDATA_EQUAL("02010100", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv ACK
	TEST_NUMBER_EQUAL(true, recvAnswer(Ecp::Command_Setup, Ecp::Error_OK));
	TEST_STRING_EQUAL("0100", observer->getResult());
	observer->clear();

	// ------------------
	// Отложенная отсылка
	// ------------------
	// No activity
	timerEngine->tick(ECP_KEEP_ALIVE_PERIOD);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Send Setup (STX,<len>,01,<crc>)
	client->sendRequest(&data);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());

	// Recv ACK
	uart->addRecvData(Ecp::Control_ACK);
	TEST_STRING_EQUAL("", observer->getResult());
	observer->clear();

	// Send Setup
	TEST_HEXDATA_EQUAL("02010100", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv ACK
	TEST_NUMBER_EQUAL(true, recvAnswer(Ecp::Command_Setup, Ecp::Error_OK));
	TEST_STRING_EQUAL("0100", observer->getResult());
	observer->clear();

	// ------------------------------
	// Обнаружение разрыва соединения
	// ------------------------------
	// No activity
	timerEngine->tick(ECP_KEEP_ALIVE_PERIOD);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// No answer
	timerEngine->tick(ECP_CONFIRM_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("<disconnect>", observer->getResult());
	return true;
}

bool EcpClientPacketLayerTest::testDisconnect() {
	// Start connecting/Send ENQ
	TEST_NUMBER_EQUAL(true, client->connect());
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv ACK
	uart->addRecvData(Ecp::Control_ACK);
	TEST_STRING_EQUAL("<connect>", observer->getResult());
	observer->clear();

	// Send Setup (STX,<len>,01,<crc>)
	Buffer data(20);
	data.addUint8(0x01);
	client->sendRequest(&data);
	TEST_HEXDATA_EQUAL("02010100", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv ACK
	TEST_NUMBER_EQUAL(true, recvAnswer(Ecp::Command_Setup, Ecp::Error_OK));
	TEST_STRING_EQUAL("0100", observer->getResult());
	observer->clear();

	// Send Keep-alive
	timerEngine->tick(ECP_KEEP_ALIVE_PERIOD);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv ACK
	uart->addRecvData(Ecp::Control_ACK);
	TEST_STRING_EQUAL("", observer->getResult());
	observer->clear();

	// Disconnect
	client->disconnect();
	TEST_HEXDATA_EQUAL("04", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv ACK
	uart->addRecvData(Ecp::Control_EOT);
	TEST_STRING_EQUAL("<disconnect>", observer->getResult());
	return true;
}

bool EcpClientPacketLayerTest::testRemoteDisconnectKeepAlive() {
	// Start connecting/Send ENQ
	TEST_NUMBER_EQUAL(true, client->connect());
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv ACK
	uart->addRecvData(Ecp::Control_ACK);
	TEST_STRING_EQUAL("<connect>", observer->getResult());
	observer->clear();

	// Send Keep-alive
	timerEngine->tick(ECP_KEEP_ALIVE_PERIOD);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	uart->addRecvData(Ecp::Control_ACK);
	TEST_STRING_EQUAL("", observer->getResult());
	observer->clear();

	// Send Keep-alive
	timerEngine->tick(ECP_KEEP_ALIVE_PERIOD);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	uart->addRecvData(Ecp::Control_EOT);
	TEST_HEXDATA_EQUAL("04", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<disconnect>", observer->getResult());
	return true;
}

bool EcpClientPacketLayerTest::testRemoteDisconnectRequest() {
	// Start connecting/Send ENQ
	TEST_NUMBER_EQUAL(true, client->connect());
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv ACK
	uart->addRecvData(Ecp::Control_ACK);
	TEST_STRING_EQUAL("<connect>", observer->getResult());
	observer->clear();

	// Send Setup (STX,<len>,01,<crc>)
	Buffer data(20);
	data.addUint8(0x01);
	client->sendRequest(&data);
	TEST_HEXDATA_EQUAL("02010100", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	uart->addRecvData(Ecp::Control_EOT);
	TEST_HEXDATA_EQUAL("04", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<disconnect>", observer->getResult());
	return true;
}
#endif
