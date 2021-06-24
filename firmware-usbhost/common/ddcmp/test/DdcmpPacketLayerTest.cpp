#include "common/ddcmp/DdcmpPacketLayer.h"
#include "timer/include/TimerEngine.h"
#include "uart/include/TestUart.h"
#include "utils/include/Hex.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class TestDdcmpPacketLayerObserver : public Ddcmp::PacketLayerObserver {
public:
	TestDdcmpPacketLayerObserver(StringBuilder *result) : result(result) {}

	virtual void recvControl(const uint8_t *data, const uint16_t dataLen) {
		*result << "<control=";
		for(uint16_t i = 0; i < dataLen; i++) {
			result->addHex(data[i]);
		}
		*result << ">";
	}
	virtual void recvData(uint8_t *cmd, uint16_t cmdLen, uint8_t *data, uint16_t dataLen) {
		*result << "<command=";
		for(uint16_t i = 0; i < cmdLen; i++) {
			result->addHex(cmd[i]);
		}
		*result << ",data=";
		for(uint16_t i = 0; i < dataLen; i++) {
			result->addHex(data[i]);
		}
		*result << ">";
	}

private:
	StringBuilder *result;
};

class DdcmpPacketLayerTest : public TestSet {
public:
	DdcmpPacketLayerTest();
	bool init();
	void cleanup();
	bool test();

private:
	StringBuilder *result;
	TimerEngine *timerEngine;
	TestUart *uart;
	TestDdcmpPacketLayerObserver *observer;
	Ddcmp::PacketLayer *layer;
};

TEST_SET_REGISTER(DdcmpPacketLayerTest);

DdcmpPacketLayerTest::DdcmpPacketLayerTest() {
	TEST_CASE_REGISTER(DdcmpPacketLayerTest, test);
}

bool DdcmpPacketLayerTest::init() {
	result = new StringBuilder;
	timerEngine = new TimerEngine();
	uart = new TestUart(256);
	observer = new TestDdcmpPacketLayerObserver(result);
	layer = new Ddcmp::PacketLayer(timerEngine, uart);
	layer->setObserver(observer);
	return true;
}

void DdcmpPacketLayerTest::cleanup() {
	delete layer;
	delete uart;
	delete timerEngine;
}

bool DdcmpPacketLayerTest::test() {
	layer->reset();

	// send control
	Buffer data1(32);
	TEST_NUMBER_NOT_EQUAL(0, hexToData("050640000801", &data1));
	layer->sendControl(data1.getData(), data1.getLen());
	TEST_HEXDATA_EQUAL("0506400008015B95", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", result->getString());

	// recv control
	uart->addRecvData("050740020801C795");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<control=050740020801>", result->getString());
	result->clear();

	// send data
	Buffer data2(32);
	Buffer data3(32);
	TEST_NUMBER_NOT_EQUAL(0, hexToData("811000000101", &data2));
	TEST_NUMBER_NOT_EQUAL(0, hexToData("77E0000000000001031920191700000C", &data3));
	layer->sendData(data2.getData(), data2.getLen(), data3.getData(), data3.getLen());
	TEST_HEXDATA_EQUAL("8110000001011F8277E0000000000001031920191700000C5DC9", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", result->getString());

	// recv ack
	uart->addRecvData("050140010001B855");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<control=050140010001>", result->getString());
	result->clear();

	// recv data
	uart->addRecvData("811540010101978288E00100000000F5FD06050403020100FF000000A0D2D7");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<command=811540010101,data=88E00100000000F5FD06050403020100FF000000A0>", result->getString());
	result->clear();

	// recv ack
	uart->addRecvData("050140010001B855");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<control=050140010001>", result->getString());
	result->clear();

/*
	TEST_STRING_EQUAL(
		"<command=811000000101"
		",data=77E0000000000001031920191700000C>", packetLayer->getSendData());
	packetLayer->clearSendData();

	//<<<000276 auth request ack (tx=*,rx=1)
	packetLayer->recvControl("x05x01x40x01x00x01");

	//<<<000276 auth response (tx=1,rx=1)
	packetLayer->recvData("x81x15x40x01x01x01", "88E00100000000F5FD06050403020100FF000000A0");
 */
/*
	layer->reset();

	// send data
	Buffer data1(32);
	TEST_NUMBER_NOT_EQUAL(0, hexToData("0013c00c001d440a00d30000000000000001000000dbd5", &data1));
	layer->sendPacket(&data1);
	TEST_HEXDATA_EQUAL("0223414250414441416452416F41307741414141414141414142414141413239553D03", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", result->getString());

	// STX
	uart->addRecvData(Sberbank::Control_STX);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// MARK
	uart->addRecvData(Sberbank::Control_MARK);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// data
	uart->addRecvData("41417341424141645241714143534141414A6339");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", result->getString());

	// ETX
	uart->addRecvData(Sberbank::Control_ETX);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<frame=000B0004001D440A8009200000973D>", result->getString());
	result->clear();

	// EOT
	uart->addRecvData(Sberbank::Control_EOT);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<control=4>", result->getString());
 */
	return true;
}
