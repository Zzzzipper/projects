#include "ecp/EcpServerPacketLayer.h"
#include "ecp/EcpProtocol.h"
#include "timer/include/TimerEngine.h"
#include "uart/include/TestUart.h"
#include "utils/include/TestEventObserver.h"
#include "test/include/Test.h"

class TestEcpServerPacketLayerObserver : public Ecp::ServerPacketLayer::Observer {
public:
	virtual ~TestEcpServerPacketLayerObserver() {}
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

class EcpServerPacketLayerTest : public TestSet {
public:
	EcpServerPacketLayerTest();
	bool init();
	void cleanup();
	bool testCutPacket();
	bool testWrongCrc();
	bool testDisconnectKeepAlive();
	bool testDisconnectRequest();
	bool testDisconnectCorruptedRequest();

private:
	TestUart *uart;
	TimerEngine *timerEngine;
	TestEcpServerPacketLayerObserver *observer;
	Ecp::Crc *crc;
	Ecp::ServerPacketLayer *server;

	bool recvAnswer(uint8_t command, uint8_t resultCode);
};

TEST_SET_REGISTER(EcpServerPacketLayerTest);

EcpServerPacketLayerTest::EcpServerPacketLayerTest() {
	TEST_CASE_REGISTER(EcpServerPacketLayerTest, testCutPacket);
	TEST_CASE_REGISTER(EcpServerPacketLayerTest, testWrongCrc);
	TEST_CASE_REGISTER(EcpServerPacketLayerTest, testDisconnectKeepAlive);
	TEST_CASE_REGISTER(EcpServerPacketLayerTest, testDisconnectRequest);
	TEST_CASE_REGISTER(EcpServerPacketLayerTest, testDisconnectCorruptedRequest);
}

bool EcpServerPacketLayerTest::init() {
	this->uart = new TestUart(256);
	this->timerEngine = new TimerEngine();
	this->observer = new TestEcpServerPacketLayerObserver;
	this->crc = new Ecp::Crc;
	this->server = new Ecp::ServerPacketLayer(this->timerEngine, this->uart);
	this->server->setObserver(observer);
	return true;
}

void EcpServerPacketLayerTest::cleanup() {
	delete this->server;
	delete this->crc;
	delete this->observer;
	delete this->timerEngine;
	delete this->uart;
}

bool EcpServerPacketLayerTest::recvAnswer(uint8_t command, uint8_t resultCode) {
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

bool EcpServerPacketLayerTest::testCutPacket() {
	server->reset();

	// Recv ENQ
	uart->addRecvData(0x05);

	// Send ACK
	TEST_HEXDATA_EQUAL("06", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<connect>", observer->getResult());
	observer->clear();

	// Recv cutted packet
	uart->addRecvData(Ecp::Control_STX);
	uart->addRecvData(0x01);
	uart->addRecvData(0x01);

	// Send nothing
	timerEngine->tick(ECP_PACKET_TIMEOUT);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());
	observer->clear();

	// Recv Setup
	uart->addRecvData(Ecp::Control_STX);
	uart->addRecvData(0x01);
	uart->addRecvData(0x01);
	uart->addRecvData((uint8_t)0x00);

	// Send to master
	timerEngine->tick(10);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("01", observer->getResult());
	observer->clear();
	return true;
}

bool EcpServerPacketLayerTest::testWrongCrc() {
	server->reset();

	// Recv ENQ
	uart->addRecvData(0x05);

	// Send ACK
	TEST_HEXDATA_EQUAL("06", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<connect>", observer->getResult());
	observer->clear();

	// Recv packet with wrong CRC
	uart->addRecvData(Ecp::Control_STX);
	uart->addRecvData(2);
	uart->addRecvData(31);
	uart->addRecvData(32);
	uart->addRecvData(50);

	// Send NAK
	TEST_HEXDATA_EQUAL("15", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());
	observer->clear();

	// Recv Setup
	uart->addRecvData(Ecp::Control_STX);
	uart->addRecvData(0x01);
	uart->addRecvData(0x01);
	uart->addRecvData((uint8_t)0x00);

	// Send to master
	timerEngine->tick(10);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("01", observer->getResult());
	observer->clear();
	return true;
}

bool EcpServerPacketLayerTest::testDisconnectKeepAlive() {
	server->reset();

	// Recv ENQ
	uart->addRecvData(Ecp::Control_ENQ);

	// Send ACK
	TEST_HEXDATA_EQUAL("06", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<connect>", observer->getResult());
	observer->clear();

	// Recv Setup
	uart->addRecvData(Ecp::Control_STX);
	uart->addRecvData(0x01);
	uart->addRecvData(0x01);
	uart->addRecvData((uint8_t)0x00);

	// Send to master
	timerEngine->tick(10);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("01", observer->getResult());
	observer->clear();

	// Send Setup ACK
	Buffer setupResponse(16);
	setupResponse.addUint8(0x01);
	setupResponse.addUint8(0x00);
	server->sendData(&setupResponse);
	TEST_HEXDATA_EQUAL("0602010003", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());
	observer->clear();

	// Recv Keep-alive
	uart->addRecvData(Ecp::Control_ENQ);

	// Send Keep-alive ACK
	TEST_HEXDATA_EQUAL("06", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());

	// Disconnect
	server->disconnect();

	// Recv Keep-alive
	uart->addRecvData(Ecp::Control_ENQ);

	// Send disconnect response
	TEST_HEXDATA_EQUAL("04", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());

	// Recv EOT
	uart->addRecvData(Ecp::Control_EOT);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<disconnect>", observer->getResult());
	return true;
}

bool EcpServerPacketLayerTest::testDisconnectRequest() {
	server->reset();

	// Recv ENQ
	uart->addRecvData(Ecp::Control_ENQ);

	TEST_HEXDATA_EQUAL("06", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<connect>", observer->getResult());
	observer->clear();

	// Disconnect
	server->disconnect();

	// Recv Request
	uart->addRecvData("02");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	uart->addRecvData("01");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	uart->addRecvData("01");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	uart->addRecvData("00");

	TEST_HEXDATA_EQUAL("04", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// Recv EOT
	uart->addRecvData(Ecp::Control_EOT);
	TEST_STRING_EQUAL("<disconnect>", observer->getResult());
	return true;
}

bool EcpServerPacketLayerTest::testDisconnectCorruptedRequest() {
	server->reset();

	// Recv ENQ
	uart->addRecvData(Ecp::Control_ENQ);

	TEST_HEXDATA_EQUAL("06", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<connect>", observer->getResult());
	observer->clear();

	// Disconnect
	server->disconnect();

	// Recv Request
	uart->addRecvData("02");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	uart->addRecvData("01");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	uart->addRecvData("01");

	// Corrupted Request
	timerEngine->tick(ECP_PACKET_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("<disconnect>", observer->getResult());
	return true;
}
