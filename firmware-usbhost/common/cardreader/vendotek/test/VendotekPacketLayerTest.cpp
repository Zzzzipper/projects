#include "cardreader/vendotek/VendotekPacketLayer.h"
#include "cardreader/vendotek/VendotekProtocol.h"
#include "timer/include/TimerEngine.h"
#include "uart/include/TestUart.h"
#include "utils/include/Hex.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class TestVendotekPacketLayerObserver : public Vendotek::PacketLayerObserver {
public:
	TestVendotekPacketLayerObserver(StringBuilder *result) : result(result) {}
	virtual ~TestVendotekPacketLayerObserver() {}

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

class VendotekPacketLayerTest : public TestSet {
public:
	VendotekPacketLayerTest();
	bool init();
	void cleanup();
	bool testSendRecv();

private:
	StringBuilder *result;
	TestUart *uart;
	TimerEngine *timerEngine;
	TestVendotekPacketLayerObserver *observer;
	Vendotek::PacketLayer *layer;
};

TEST_SET_REGISTER(VendotekPacketLayerTest);

VendotekPacketLayerTest::VendotekPacketLayerTest() {
	TEST_CASE_REGISTER(VendotekPacketLayerTest, testSendRecv);
}

bool VendotekPacketLayerTest::init() {
	this->result = new StringBuilder;
	this->uart = new TestUart(256);
	this->timerEngine = new TimerEngine();
	this->observer = new TestVendotekPacketLayerObserver(result);
	this->layer = new Vendotek::PacketLayer(this->timerEngine, this->uart);
	this->layer->setObserver(observer);
	return true;
}

void VendotekPacketLayerTest::cleanup() {
	delete this->layer;
	delete this->observer;
	delete this->timerEngine;
	delete this->uart;
	delete this->result;
}

bool VendotekPacketLayerTest::testSendRecv() {
	layer->reset();

	// send data
	Buffer data1(32);
	TEST_NUMBER_NOT_EQUAL(0, hexToData("010349444C03083030303030303030", &data1));
	layer->sendPacket(&data1);
	TEST_HEXDATA_EQUAL("1F001196FB010349444C0308303030303030303010F8", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", result->getString());

	// recv data
	uart->addRecvData("1F001297FB010349444C0301300603313230080130");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// recv crc
	uart->addRecvData("67C6");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<packet=010349444C0301300603313230080130>", result->getString());
	result->clear();
	return true;
}
