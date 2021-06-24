#include "ccicsi/CciCsiPacketLayer.h"
#include "ccicsi/CciCsiProtocol.h"
#include "timer/include/TimerEngine.h"
#include "uart/include/TestUart.h"
#include "utils/include/Hex.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

namespace CciCsi {

class TestCciCsiPacketLayerObserver : public CciCsi::PacketLayerObserver {
public:
	TestCciCsiPacketLayerObserver(StringBuilder *result) : result(result) {}
	virtual ~TestCciCsiPacketLayerObserver() {}

	virtual void procPacket(const uint8_t *data, const uint16_t dataLen) {
		*result << "<packet=";
		for(uint16_t i = 0; i < dataLen; i++) {
			result->addHex(data[i]);
		}
		*result << ">";
	}
	virtual void procControl(uint8_t control) {
		*result << "<control=" << control << ">";
	}
	virtual void procError(Error error) {
		*result << "<error=" << error << ">";
	}

private:
	StringBuilder *result;
};

class PacketLayerTest : public TestSet {
public:
	PacketLayerTest();
	bool init();
	void cleanup();
	bool testRecvSend();

private:
	StringBuilder *result;
	TestUart *uart;
	TimerEngine *timerEngine;
	TestCciCsiPacketLayerObserver *observer;
	CciCsi::PacketLayer *layer;
};

TEST_SET_REGISTER(CciCsi::PacketLayerTest);

PacketLayerTest::PacketLayerTest() {
	TEST_CASE_REGISTER(CciCsi::PacketLayerTest, testRecvSend);
}

bool PacketLayerTest::init() {
	this->result = new StringBuilder;
	this->uart = new TestUart(256);
	this->timerEngine = new TimerEngine();
	this->observer = new TestCciCsiPacketLayerObserver(result);
	this->layer = new CciCsi::PacketLayer(this->timerEngine, this->uart);
	this->layer->setObserver(observer);
	return true;
}

void PacketLayerTest::cleanup() {
	delete this->layer;
	delete this->observer;
	delete this->timerEngine;
	delete this->uart;
	delete this->result;
}

/*
025303353017
STX(x02)Type(x53)
ETX(x03)
CRC(3530),calc=50,recv=50
ETB(x17)
 */
bool PacketLayerTest::testRecvSend() {
	layer->reset();

	// STX
	uart->addRecvData(CciCsi::Control_STX);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Type
	uart->addRecvData(0x53);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// ETX
	uart->addRecvData(CciCsi::Control_ETX);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// Crc
	uart->addRecvData("3530");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// ETB
	uart->addRecvData(CciCsi::Control_ETB);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<packet=53>", result->getString());
	result->clear();

	// send ACK
	layer->sendControl(Control_ACK);
	TEST_HEXDATA_EQUAL("06", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", result->getString());

	// send DATA
	Buffer data1(32);
	TEST_NUMBER_NOT_EQUAL(0, hexToData("0102030405", &data1));
	layer->sendPacket(&data1);
	TEST_HEXDATA_EQUAL("02010203040503303217", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

}
