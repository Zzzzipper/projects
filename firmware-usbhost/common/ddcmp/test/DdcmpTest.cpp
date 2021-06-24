#include "ddcmp/DdcmpCommandLayer.h"
#include "ddcmp/DdcmpProtocol.h"
#include "timer/include/TimerEngine.h"
#include "timer/include/TestRealTime.h"
#include "event/include/TestEventEngine.h"
#include "dex/test/TestDataParser.h"
#include "utils/include/Hex.h"
#include "test/include/Test.h"

class TestDdcmpPacketLayer : public Ddcmp::PacketLayerInterface {
public:
	TestDdcmpPacketLayer() : recvBuf(256), recvBuf2(512) {}
	virtual ~TestDdcmpPacketLayer() {}
	void clearSendData() { sendBuf.clear(); }
	const char *getSendData() { return sendBuf.getString(); }
	void recvControl(const char *hex) {
		uint16_t len = hexToData(hex, strlen(hex), recvBuf.getData(), recvBuf.getSize());
		recvBuf.setLen(len);
		observer->recvControl(recvBuf.getData(), recvBuf.getLen());
	}
	void recvData(const char *cmdHex, const char *dataHex) {
		uint16_t cmdLen = hexToData(cmdHex, strlen(cmdHex), recvBuf.getData(), recvBuf.getSize());
		recvBuf.setLen(cmdLen);
		uint16_t dataLen = hexToData(dataHex, strlen(dataHex), recvBuf2.getData(), recvBuf2.getSize());
		recvBuf2.setLen(dataLen);
		observer->recvData(recvBuf.getData(), recvBuf.getLen(), recvBuf2.getData(), recvBuf2.getLen());
	}

	virtual void setObserver(Ddcmp::PacketLayerObserver *observer) { this->observer = observer; }
	virtual void reset() {}
	virtual void sendControl(uint8_t *cmd, uint16_t cmdLen) {
		sendBuf << "<command=";
		for(uint16_t i = 0; i < cmdLen; i++) {
			sendBuf.addHex(cmd[i]);
		}
		sendBuf << ">";
	}
	virtual void sendData(uint8_t *cmd, uint16_t cmdLen, uint8_t *data, uint16_t dataLen) {
		sendBuf << "<command=";
		for(uint16_t i = 0; i < cmdLen; i++) {
			sendBuf.addHex(cmd[i]);
		}
		sendBuf << ",data=";
		for(uint16_t j = 0; j < dataLen; j++) {
			sendBuf.addHex(data[j]);
		}
		sendBuf << ">";
	}

private:
	Ddcmp::PacketLayerObserver *observer;
	StringBuilder sendBuf;
	Buffer recvBuf;
	Buffer recvBuf2;
};

class DdcmpTest : public TestSet {
public:
	DdcmpTest();
	bool init();
	void cleanup();
	bool testCrc();
	bool testLoadAudit();

private:
	StringBuilder *result;
	TestRealTime *realtime;
	TestDdcmpPacketLayer *packetLayer;
	TimerEngine *timerEngine;
	Ddcmp::CommandLayer *commandLayer;
};

TEST_SET_REGISTER(DdcmpTest);

DdcmpTest::DdcmpTest() {
	TEST_CASE_REGISTER(DdcmpTest, testCrc);
	TEST_CASE_REGISTER(DdcmpTest, testLoadAudit);
}

bool DdcmpTest::init() {
	result = new StringBuilder;
	realtime = new TestRealTime;
	packetLayer = new TestDdcmpPacketLayer;
	timerEngine = new TimerEngine();
	commandLayer = new Ddcmp::CommandLayer(packetLayer, timerEngine);
	return true;
}

void DdcmpTest::cleanup() {
	delete commandLayer;
	delete timerEngine;
	delete packetLayer;
	delete realtime;
	delete result;
}

/*
>05 06 40 00 08 01 5B 95

<05 07 40 02 08 01 C7 95

>>000276:
>81 10 00 00 01 01 1F 82 77 E0 00 00 00 00 00 01
 03 19 20 19 17 00 00 0C 5D C9
<05 01 40 01 00 01 B8 55 81 15 40 01 01 01 97 82
 88 E0 01 00 00 00 00 F5 FD 06 05 04 03 02 01 00
 FF 00 00 00 A0 D2 D7
 */
bool DdcmpTest::testCrc() {
	Ddcmp::Crc crc1;
	uint8_t data1[] = { 0x05, 0x06, 0x40, 0x00, 0x08, 0x01 };
	crc1.start();
	crc1.add(data1, sizeof(data1));
	TEST_NUMBER_EQUAL(0x955B, crc1.getCrc());
	TEST_NUMBER_EQUAL(0x95, crc1.getHighByte());
	TEST_NUMBER_EQUAL(0x5B, crc1.getLowByte());

	uint8_t data2[] = { 0x81, 0x10, 0x00, 0x00, 0x01, 0x01 };
	crc1.start();
	crc1.add(data2, sizeof(data2));
	TEST_NUMBER_EQUAL(0x821F, crc1.getCrc());
	TEST_NUMBER_EQUAL(0x82, crc1.getHighByte());
	TEST_NUMBER_EQUAL(0x1F, crc1.getLowByte());
	return true;
}

bool DdcmpTest::testLoadAudit() {
	TestDataParser testDataParser(10240, false);

	//>>>000161 baudrate request
	commandLayer->recvAudit(&testDataParser, NULL);
	TEST_STRING_EQUAL("<command=050640000801>", packetLayer->getSendData());
	packetLayer->clearSendData();

	//<<<000170 baudrate request ack
	packetLayer->recvControl("x05x07x40x02x08x01");

	//>>>000267 auth request (tx=1,rx=0)
	TEST_STRING_EQUAL(
		"<command=811000000101"
		",data=77E0000000000001031920191700000C>", packetLayer->getSendData());
	packetLayer->clearSendData();

	//<<<000276 auth request ack (tx=*,rx=1)
	packetLayer->recvControl("x05x01x40x01x00x01");

	//<<<000276 auth response (tx=1,rx=1)
	packetLayer->recvData("x81x15x40x01x01x01", "88E00100000000F5FD06050403020100FF000000A0");

	//>>>000751 audit start request (tx=1,rx=2)
	TEST_STRING_EQUAL(
		"<command=050140010001><command=810900010201"
		",data=77E20001010000FFFF>", packetLayer->getSendData());
	packetLayer->clearSendData();

	//<<<000766 audit start request ack (tx=*,rx=2)
	packetLayer->recvControl("050140020001");

	//<<<000766 audit start response (tx=2,rx=2)
	packetLayer->recvData("x81x09x40x02x02x01", "88E20101010000FFFF");

	//>>>001095 audit start response ack (tx=*,rx=2)
	TEST_STRING_EQUAL("<command=050140020001>", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<start=65535>", testDataParser.getData());
	testDataParser.clearData();

	//<<<001104 audit data1 response (rx=2,tx=3)(001104-003001)
	packetLayer->recvData("x81x94x40x02x03x01",
		"x99x00x44x58x53x2Ax46x41"
		"x47x20x2Ax56x41x2Ax31x0Dx0Ax49x44x31x2Ax31x2Ax46"
		"x41x47x20x4Ax46x38x39x30x30x2Ax30x2Ax30x2Ax52x65"
		"x76x2Ex20x31x2Ex30x39x2Ex33x30x2Ax31x2Ax36x2Ax30"
		"x0Dx0Ax4Dx41x35x2Ax30x2Ax41x55x44x49x54x20x46x49"
		"x4Cx45x20x52x45x56x49x53x49x4Fx4Ex20x30x30x30x32"
		"x0Dx0Ax56x41x31x2Ax32x30x30x30x2Ax32x2Ax30x2Ax30"
		"x0Dx0Ax49x44x34x2Ax32x2Ax30x2Ax31x0Dx0Ax45x41x33"
		"x2Ax35x2Ax31x39x30x33x30x31x2Ax32x30x31x39x2Ax20"
		"x2Ax31x39x30x33x30x31x2Ax31x39x35x36");

	//>>>003001 audit data1 ack (rx=3,tx=*)
	TEST_STRING_EQUAL("<command=050140030001>", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL(
		"DXS*FAG *VA*1\r\n"
		"ID1*1*FAG JF8900*0*0*Rev. 1.09.30*1*6*0\r\n"
		"MA5*0*AUDIT FILE REVISION 0002\r\n"
		"VA1*2000*2*0*0\r\n"
		"ID4*2*0*1\r\n"
		"EA3*5*190301*2019* *190301*1956", testDataParser.getData());

	//<<<003010
	packetLayer->recvData("x81x94x40x02x04x01",
		"x99x01x0Dx0Ax45x41x34x2A"
		"x31x38x31x32x32x37x2Ax32x31x32x39x0Dx0Ax45x41x35"
		"x2Ax31x38x31x32x32x37x2Ax32x31x33x30x0Dx0Ax50x41"
		"x31x2Ax30x31x2Ax32x35x30x30x0Dx0Ax50x41x32x2Ax30"
		"x2Ax30x2Ax30x2Ax30x0Dx0Ax50x41x31x2Ax30x32x2Ax33"
		"x30x30x30x0Dx0Ax50x41x32x2Ax30x2Ax30x2Ax30x2Ax30"
		"x0Dx0Ax50x41x31x2Ax30x33x2Ax33x35x30x30x0Dx0Ax50"
		"x41x32x2Ax30x2Ax30x2Ax30x2Ax30x0Dx0Ax50x41x31x2A"
		"x30x34x2Ax34x30x30x30x0Dx0Ax50x41x32x2Ax30x2Ax30"
		"x2Ax30x2Ax30x0Dx0Ax50x41x31x2Ax30x35");

	//>>>004907 audit data1 ack (rx=4,tx=*)
	TEST_STRING_EQUAL("<command=050140040001>", packetLayer->getSendData());
	packetLayer->clearSendData();

	//<<<004916
	packetLayer->recvData("x81x95x40x02x05x01",
		"x99x02x2Ax35x30x30x30x0D"
		"x0Ax50x41x32x2Ax30x2Ax30x2Ax30x2Ax30x0Dx0Ax4Cx41"
		"x31x2Ax30x2Ax30x31x2Ax32x35x30x30x2Ax30x0Dx0Ax4C"
		"x41x31x2Ax30x2Ax30x32x2Ax33x30x30x30x2Ax30x0Dx0A"
		"x4Cx41x31x2Ax30x2Ax30x33x2Ax33x35x30x30x2Ax30x0D"
		"x0Ax4Cx41x31x2Ax30x2Ax30x34x2Ax34x30x30x30x2Ax30"
		"x0Dx0Ax4Cx41x31x2Ax30x2Ax30x35x2Ax35x30x30x30x2A"
		"x30x0Dx0Ax4Cx41x31x2Ax31x2Ax30x31x2Ax32x34x30x30"
		"x2Ax30x0Dx0Ax4Cx41x31x2Ax31x2Ax30x32x2Ax32x38x30"
		"x30x2Ax30x0Dx0Ax4Cx41x31x2Ax31x2Ax30x33");

	//>>>006825 audit data1 ack (rx=5,tx=*)
	TEST_STRING_EQUAL("<command=050140050001>", packetLayer->getSendData());
	packetLayer->clearSendData();

	//<<<006834
	packetLayer->recvData("x81x92x40x02x06x01",
		"x99x03x2Ax33x32x30x30x2A"
		"x30x0Dx0Ax4Cx41x31x2Ax31x2Ax30x34x2Ax33x36x30x30"
		"x2Ax30x0Dx0Ax4Cx41x31x2Ax31x2Ax30x35x2Ax34x35x30"
		"x30x2Ax30x0Dx0Ax43x41x32x2Ax30x2Ax30x2Ax30x2Ax30"
		"x0Dx0Ax43x41x33x2Ax30x2Ax30x2Ax30x2Ax30x2Ax30x2A"
		"x30x2Ax30x2Ax30x0Dx0Ax43x41x35x2Ax30x2Ax35x33x0D"
		"x0Ax43x41x37x2Ax30x2Ax30x0Dx0Ax43x41x38x2Ax30x2A"
		"x30x0Dx0Ax43x41x39x2Ax30x2Ax30x0Dx0Ax43x41x31x34"
		"x2Ax31x30x30x30x2Ax30x2Ax30x0Dx0Ax43x41x31x34x2A"
		"x35x30x30x30x2Ax30x2Ax30x0Dx0A");

	//>>>008707 audit data1 ack (rx=6,tx=*)
	TEST_STRING_EQUAL("<command=050140060001>", packetLayer->getSendData());
	packetLayer->clearSendData();

	//<<<008716
	packetLayer->recvData("x81x90x40x02x07x01",
		"x99x04x43x41x31x34x2Ax31"
		"x30x30x30x30x2Ax30x2Ax30x0Dx0Ax43x41x31x34x2Ax30"
		"x2Ax30x2Ax30x0Dx0Ax43x41x31x34x2Ax30x2Ax30x2Ax30"
		"x0Dx0Ax43x41x31x34x2Ax30x2Ax30x2Ax30x0Dx0Ax43x41"
		"x31x34x2Ax30x2Ax30x2Ax30x0Dx0Ax43x41x31x34x2Ax30"
		"x2Ax30x2Ax30x0Dx0Ax44x41x32x2Ax32x30x30x30x2Ax32"
		"x2Ax30x2Ax30x0Dx0Ax44x41x33x2Ax32x31x30x30x2Ax30"
		"x2Ax30x0Dx0Ax44x41x34x2Ax33x30x30x30x30x2Ax30x2A"
		"x30x0Dx0Ax44x41x35x2Ax30x2Ax30x2Ax30x0Dx0Ax44x41"
		"x31x30x2Ax31x30x30x2Ax30");

	//>>>010565 audit data1 ack (rx=7,tx=*)
	TEST_STRING_EQUAL("<command=050140070001>", packetLayer->getSendData());
	packetLayer->clearSendData();

	//<<<010574
	packetLayer->recvData("x81x94x40x02x08x01",
		"x99x05x2Ax33x30x30x30x30"
		"x2Ax30x0Dx0Ax43x41x31x31x2Ax31x30x30x2Ax30x2Ax30"
		"x2Ax30x0Dx0Ax43x41x31x31x2Ax32x30x30x2Ax30x2Ax30"
		"x2Ax30x0Dx0Ax43x41x31x31x2Ax35x30x30x2Ax30x2Ax30"
		"x2Ax30x0Dx0Ax43x41x31x31x2Ax31x30x30x30x2Ax30x2A"
		"x30x2Ax30x0Dx0Ax43x41x31x31x2Ax31x30x30x2Ax30x2A"
		"x30x2Ax30x0Dx0Ax43x41x31x31x2Ax32x30x30x2Ax30x2A"
		"x30x2Ax30x0Dx0Ax43x41x31x31x2Ax35x30x30x2Ax30x2A"
		"x30x2Ax30x0Dx0Ax43x41x31x31x2Ax31x30x30x30x2Ax30"
		"x2Ax30x2Ax30x0Dx0Ax43x41x31x31x2Ax30");

	//>>>012471 audit data1 ack (rx=8,tx=*)
	TEST_STRING_EQUAL("<command=050140080001>", packetLayer->getSendData());
	packetLayer->clearSendData();

	//<<<012480
	packetLayer->recvData("x81x94x40x02x09x01",
		"x99x06x2Ax30x2Ax30x2Ax30"
		"x0Dx0Ax43x41x31x31x2Ax30x2Ax30x2Ax30x2Ax30x0Dx0A"
		"x43x41x31x31x2Ax30x2Ax30x2Ax30x2Ax30x0Dx0Ax43x41"
		"x31x31x2Ax30x2Ax30x2Ax30x2Ax30x0Dx0Ax43x41x31x31"
		"x2Ax30x2Ax30x2Ax30x2Ax30x0Dx0Ax43x41x31x31x2Ax30"
		"x2Ax30x2Ax30x2Ax30x0Dx0Ax43x41x31x31x2Ax30x2Ax30"
		"x2Ax30x2Ax30x0Dx0Ax43x41x31x31x2Ax30x2Ax30x2Ax30"
		"x2Ax30x0Dx0Ax43x41x31x31x2Ax30x2Ax30x2Ax30x2Ax30"
		"x0Dx0Ax43x41x31x31x2Ax30x2Ax30x2Ax30x2Ax30x0Dx0A"
		"x43x41x31x31x2Ax30x2Ax30x2Ax30x2Ax30");

	//>>>014377 audit data1 ack (rx=9,tx=*)
	TEST_STRING_EQUAL("<command=050140090001>", packetLayer->getSendData());
	packetLayer->clearSendData();

	//<<<014386
	packetLayer->recvData("x81x94x40x02x0Ax01",
		"x99x07x0Dx0Ax43x41x31x31"
		"x2Ax30x2Ax30x2Ax30x2Ax30x0Dx0Ax43x41x31x31x2Ax30"
		"x2Ax30x2Ax30x2Ax30x0Dx0Ax43x41x31x31x2Ax30x2Ax30"
		"x2Ax30x2Ax30x0Dx0Ax43x41x31x31x2Ax30x2Ax30x2Ax30"
		"x2Ax30x0Dx0Ax43x41x31x31x2Ax30x2Ax30x2Ax30x2Ax30"
		"x0Dx0Ax43x41x31x31x2Ax30x2Ax30x2Ax30x2Ax30x0Dx0A"
		"x43x41x31x31x2Ax30x2Ax30x2Ax30x2Ax30x0Dx0Ax43x41"
		"x31x31x2Ax30x2Ax30x2Ax30x2Ax30x0Dx0Ax43x41x31x31"
		"x2Ax30x2Ax30x2Ax30x2Ax30x0Dx0Ax43x41x31x31x2Ax30"
		"x2Ax30x2Ax30x2Ax30x0Dx0Ax43x41x31x31");

	//>>>016283 audit data1 ack (rx=A,tx=*)
	TEST_STRING_EQUAL("<command=0501400A0001>", packetLayer->getSendData());
	packetLayer->clearSendData();

	//<<<016292
	packetLayer->recvData("x81x95x40x02x0Bx01",
		"x99x08x2Ax30x2Ax30x2Ax30"
		"x2Ax30x0Dx0Ax43x41x34x2Ax30x2Ax30x2Ax30x2Ax30x0D"
		"x0Ax43x41x31x30x2Ax30x2Ax30x0Dx0Ax54x41x32x2Ax30"
		"x2Ax30x2Ax30x2Ax30x2Ax30x2Ax30x2Ax30x2Ax30x0Dx0A"
		"x54x41x35x2Ax30x2Ax30x0Dx0Ax56x41x32x2Ax30x2Ax30"
		"x2Ax30x2Ax30x0Dx0Ax56x41x33x2Ax30x2Ax30x2Ax30x2A"
		"x30x0Dx0Ax45x41x32x2Ax30x30x31x20x53x65x6Cx65x63"
		"x74x2Ex20x75x6Ex6Bx6Ex6Fx77x6Ex20x2Ax30x2Ax32x2A"
		"x30x0Dx0Ax45x41x32x2Ax30x30x32x20x43x72x65x64x69"
		"x74x20x20x72x65x73x65x74x20x20x20x2Ax30");

	//>>>018201 audit data1 ack (rx=B,tx=*)
	TEST_STRING_EQUAL("<command=0501400B0001>", packetLayer->getSendData());
	packetLayer->clearSendData();

	//<<<018210
	packetLayer->recvData("x81x88x40x02x0Cx01",
		"x99x09x2Ax30x2Ax0Dx0Ax45"
		"x41x32x2Ax30x30x33x20x44x75x62x69x6Fx75x73x20x6C"
		"x6Fx61x64x69x6Ex67x20x2Ax30x2Ax30x2Ax30x0Dx0Ax45"
		"x41x32x2Ax30x30x34x20x41x64x64x69x74x69x6Fx6Ex61"
		"x6Cx20x63x61x73x68x20x2Ax30x2Ax30x2Ax30x0Dx0Ax45"
		"x41x32x2Ax30x30x38x20x4Cx6Fx61x64x65x64x20x69x6E"
		"x20x62x6Cx6Fx63x6Bx73x2Ax30x2Ax30x2Ax30x2Ax30x0D"
		"x0Ax45x41x32x2Ax30x39x39x20x46x72x65x65x56x65x6E"
		"x64x73x4Cx65x76x65x6Cx20x31x2Ax30x2Ax2Ax30x0Dx0A");

	//>>>019963 audit data1 ack (rx=C,tx=*)
	TEST_STRING_EQUAL("<command=0501400C0001>", packetLayer->getSendData());
	packetLayer->clearSendData();

	//<<<019972
	packetLayer->recvData("x81x7Ex40x02x0Dx01",
		"x99x0Ax45x41x32x2Ax30x39"
		"x38x20x46x72x65x65x56x65x6Ex64x73x4Cx65x76x65x6C"
		"x20x32x2Ax30x2Ax2Ax30x0Dx0Ax45x41x32x2Ax30x39x37"
		"x20x46x72x65x65x56x65x6Ex64x73x4Cx65x76x65x6Cx20"
		"x33x2Ax30x2Ax2Ax30x0Dx0Ax45x41x32x2Ax30x39x36x20"
		"x46x72x65x65x56x65x6Ex64x73x4Cx65x76x65x6Cx20x34"
		"x2Ax30x2Ax2Ax30x0Dx0Ax45x41x32x2Ax30x39x34x20x4E"
		"x65x67x61x74x69x76x65x20x76x65x6Ex64x20x20x20x2A"
		"x30x2Ax2Ax30x0Dx0A");

	//>>>021605 audit data1 ack (rx=D,tx=*)
	TEST_STRING_EQUAL("<command=0501400D0001>", packetLayer->getSendData());
	packetLayer->clearSendData();

	//<<<021614
	packetLayer->recvData("x81x48xC0x02x0Ex01",
		"x99x0Bx45x41x32x2Ax30x39"
		"x33x20x43x72x65x64x69x74x20x73x75x72x76x65x79x65"
		"x64x20x2Ax30x2Ax31x2Ax30x0Dx0Ax4Dx52x35x2Ax32x37"
		"x43x45x42x37x34x34x2Ax30x2Ax31x2Ax32x37x43x45x42"
		"x37x34x35x0Dx0Ax44x58x45x2Ax31x2Ax30x0Dx0Ax0Dx0A");

	//>>>022599 (rx=E,tx=*)
	TEST_STRING_EQUAL("<command=0501400E0001><command=8102000E0301,data=77FF>", packetLayer->getSendData());
	packetLayer->clearSendData();

	//<<<003001 audit data1 ack (rx=3,tx=*)
	packetLayer->recvControl("050140030001");

	TEST_STRING_EQUAL(
		"DXS*FAG *VA*1\r\n"
		"ID1*1*FAG JF8900*0*0*Rev. 1.09.30*1*6*0\r\n"
		"MA5*0*AUDIT FILE REVISION 0002\r\n"
		"VA1*2000*2*0*0\r\n"
		"ID4*2*0*1\r\n"
		"EA3*5*190301*2019* *190301*1956\r\n"
		"EA4*181227*2129\r\n"
		"EA5*181227*2130\r\n"
		"PA1*01*2500\r\n"
		"PA2*0*0*0*0\r\n"
		"PA1*02*3000\r\n"
		"PA2*0*0*0*0\r\n"
		"PA1*03*3500\r\n"
		"PA2*0*0*0*0\r\n"
		"PA1*04*4000\r\n"
		"PA2*0*0*0*0\r\n"
		"PA1*05*5000\r\n"
		"PA2*0*0*0*0\r\n"
		"LA1*0*01*2500*0\r\n"
		"LA1*0*02*3000*0\r\n"
		"LA1*0*03*3500*0\r\n"
		"LA1*0*04*4000*0\r\n"
		"LA1*0*05*5000*0\r\n"
		"LA1*1*01*2400*0\r\n"
		"LA1*1*02*2800*0\r\n"
		"LA1*1*03*3200*0\r\n"
		"LA1*1*04*3600*0\r\n"
		"LA1*1*05*4500*0\r\n"
		"CA2*0*0*0*0\r\n"
		"CA3*0*0*0*0*0*0*0*0\r\n"
		"CA5*0*53\r\n"
		"CA7*0*0\r\n"
		"CA8*0*0\r\n"
		"CA9*0*0\r\n"
		"CA14*1000*0*0\r\n"
		"CA14*5000*0*0\r\n"
		"CA14*10000*0*0\r\n"
		"CA14*0*0*0\r\n"
		"CA14*0*0*0\r\n"
		"CA14*0*0*0\r\n"
		"CA14*0*0*0\r\n"
		"CA14*0*0*0\r\n"
		"DA2*2000*2*0*0\r\n"
		"DA3*2100*0*0\r\n"
		"DA4*30000*0*0\r\n"
		"DA5*0*0*0\r\n"
		"DA10*100*0*30000*0\r\n"
		"CA11*100*0*0*0\r\n"
		"CA11*200*0*0*0\r\n"
		"CA11*500*0*0*0\r\n"
		"CA11*1000*0*0*0\r\n"
		"CA11*100*0*0*0\r\n"
		"CA11*200*0*0*0\r\n"
		"CA11*500*0*0*0\r\n"
		"CA11*1000*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA11*0*0*0*0\r\n"
		"CA4*0*0*0*0\r\n"
		"CA10*0*0\r\n"
		"TA2*0*0*0*0*0*0*0*0\r\n"
		"TA5*0*0\r\n"
		"VA2*0*0*0*0\r\n"
		"VA3*0*0*0*0\r\n"
		"EA2*001 Select. unknown *0*2*0\r\n"
		"EA2*002 Credit  reset   *0*0*\r\n"
		"EA2*003 Dubious loading *0*0*0\r\n"
		"EA2*004 Additional cash *0*0*0\r\n"
		"EA2*008 Loaded in blocks*0*0*0*0\r\n"
		"EA2*099 FreeVendsLevel 1*0**0\r\n"
		"EA2*098 FreeVendsLevel 2*0**0\r\n"
		"EA2*097 FreeVendsLevel 3*0**0\r\n"
		"EA2*096 FreeVendsLevel 4*0**0\r\n"
		"EA2*094 Negative vend   *0**0\r\n"
		"EA2*093 Credit surveyed *0*1*0\r\n"
		"MR5*27CEB744*0*1*27CEB745\r\n"
		"DXE*1*0\r\n\r\n<complete>", testDataParser.getData());
	return true;
}
