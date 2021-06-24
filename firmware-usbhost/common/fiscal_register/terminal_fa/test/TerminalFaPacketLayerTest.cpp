#include "fiscal_register/terminal_fa/TerminalFaPacketLayer.h"
#include "fiscal_register/terminal_fa/TerminalFaProtocol.h"
#include "timer/include/TimerEngine.h"
#include "uart/include/TestUart.h"
#include "utils/include/TestEventObserver.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class TestTerminalFaPacketLayerObserver : public TerminalFa::PacketLayerInterface::Observer {
public:
	virtual ~TestTerminalFaPacketLayerObserver() {}
	void clear() { str.clear(); }
	const char *getResult() { return str.getString(); }

	virtual void procRecvData(uint8_t *data, uint16_t dataLen) {
		for(uint16_t i = 0; i < dataLen; i++) {
			str.addHex(data[i]);
		}
	}
	virtual void procRecvError(TerminalFa::PacketLayerInterface::Error error) { str << "<error=" << error << ">"; }

private:
	StringBuilder str;
};

class TerminalFaPacketLayerTest : public TestSet {
public:
	TerminalFaPacketLayerTest();
	bool init();
	void cleanup();
	bool testSendRecv();
	bool testResponseTimeout();

private:
	TestUart *uart;
	TimerEngine *timerEngine;
	TestTerminalFaPacketLayerObserver *observer;
	TerminalFa::PacketLayer *client;
};

TEST_SET_REGISTER(TerminalFaPacketLayerTest);

TerminalFaPacketLayerTest::TerminalFaPacketLayerTest() {
	TEST_CASE_REGISTER(TerminalFaPacketLayerTest, testSendRecv);
	TEST_CASE_REGISTER(TerminalFaPacketLayerTest, testResponseTimeout);
}

bool TerminalFaPacketLayerTest::init() {
	this->uart = new TestUart(256);
	this->timerEngine = new TimerEngine();
	this->observer = new TestTerminalFaPacketLayerObserver;
	this->client = new TerminalFa::PacketLayer(this->timerEngine, this->uart);
	this->client->setObserver(observer);
	return true;
}

void TerminalFaPacketLayerTest::cleanup() {
	delete this->client;
	delete this->observer;
	delete this->timerEngine;
	delete this->uart;
}

bool TerminalFaPacketLayerTest::testSendRecv() {
	// send data
	Buffer data(32);
	data.addUint8(0x30);
	data.addUint8(0x01);
	data.addUint8(0x00);
	data.addUint8(0x00);
	data.addUint8(0x00);
	client->sendPacket(&data);
	TEST_HEXDATA_EQUAL("B6290005300100000095C8", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());

	// first byte
	uart->addRecvData(TerminalFa::Control_FB);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// second byte
	uart->addRecvData(TerminalFa::Control_SB);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// len
	uart->addRecvData("0005");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// data
	uart->addRecvData("3001000000");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// crc
	uart->addRecvData("95C8");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("3001000000", observer->getResult());
	return true;
}

bool TerminalFaPacketLayerTest::testResponseTimeout() {
	// send data
	Buffer data(32);
	data.addUint8(0x01);
	data.addUint8(0x02);
	data.addUint8(0x03);
	data.addUint8(0x04);
	client->sendPacket(&data);
	TEST_HEXDATA_EQUAL("B629000401020304158A", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());

	// first byte
	uart->addRecvData(TerminalFa::Control_FB);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// second byte
	uart->addRecvData(TerminalFa::Control_SB);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// len
	uart->addRecvData("0005");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// timeout
	timerEngine->tick(TERMINALFA_RESPONSE_TIMER);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<error=0>", observer->getResult());
	return true;
}
