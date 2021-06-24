#include "cardreader/vendotek/VendotekCommandLayer.h"
#include "cardreader/vendotek/VendotekProtocol.h"
#include "mdb/master/cashless/MdbMasterCashless.h"
#include "utils/include/Hex.h"
#include "timer/include/TimerEngine.h"
#include "timer/include/TestRealTime.h"
#include "mdb/master/cashless/test/TestCashlessEventEngine.h"
#include "http/test/TestTcpIp.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class TestVendotekPacketLayer : public Vendotek::PacketLayerInterface {
public:
//	TestVendotekPacketLayer() : sendBuf(VENDOTEK_PACKET_SIZE*2, VENDOTEK_PACKET_SIZE*2), recvBuf(VENDOTEK_PACKET_SIZE) {}
	TestVendotekPacketLayer() : sendBuf(VENDOTEK_PACKET_SIZE*10, VENDOTEK_PACKET_SIZE*10), recvBuf(VENDOTEK_PACKET_SIZE*10) {}
	virtual ~TestVendotekPacketLayer() {}
	void clearSendData() { sendBuf.clear(); }
	const char *getSendData() { return sendBuf.getString(); }
	void recvPacket(const char *hex) {
		uint16_t len = hexToData(hex, strlen(hex), recvBuf.getData(), recvBuf.getSize());
		recvBuf.setLen(len);
		observer->procPacket(recvBuf.getData(), recvBuf.getLen());
	}
	void recvError(Vendotek::PacketLayerObserver::Error error) { observer->procError(error); }

	virtual void setObserver(Vendotek::PacketLayerObserver *observer) { this->observer = observer; }
	virtual void reset() {}
	virtual bool sendPacket(Buffer *data) {
		for(uint16_t i = 0; i < data->getLen(); i++) {
			sendBuf.addHex((*data)[i]);
		}
		return true;
	}
	virtual bool sendControl(uint8_t control) {
		(void)control;
		return false;
	}

private:
	Vendotek::PacketLayerObserver *observer;
	StringBuilder sendBuf;
	Buffer recvBuf;
};

class VendotekCommandLayerTest : public TestSet {
public:
	VendotekCommandLayerTest();
	bool init();
	void cleanup();
	bool testPayment();
	bool testQrCode();
	bool testConnection();
	bool testConnection2();
	bool testConnectionSlowSender();
	bool testConfirmableDataBlock();

private:
	StringBuilder *result;
	TestRealTime *realtime;
	Mdb::DeviceContext *context;
	TestVendotekPacketLayer *packetLayer;
	TestTcpIp *tcpIp;
	TimerEngine *timerEngine;
	TestCashlessEventEngine *eventEngine;
	Vendotek::CommandLayer *commandLayer;
};

TEST_SET_REGISTER(VendotekCommandLayerTest);

VendotekCommandLayerTest::VendotekCommandLayerTest() {
	TEST_CASE_REGISTER(VendotekCommandLayerTest, testPayment);
	TEST_CASE_REGISTER(VendotekCommandLayerTest, testQrCode);
	TEST_CASE_REGISTER(VendotekCommandLayerTest, testConnection);
	TEST_CASE_REGISTER(VendotekCommandLayerTest, testConnection2);
	TEST_CASE_REGISTER(VendotekCommandLayerTest, testConnectionSlowSender);
	TEST_CASE_REGISTER(VendotekCommandLayerTest, testConfirmableDataBlock);
}

bool VendotekCommandLayerTest::init() {
	result = new StringBuilder;
	realtime = new TestRealTime;
	context = new Mdb::DeviceContext(2, realtime);
	packetLayer = new TestVendotekPacketLayer;
	tcpIp = new TestTcpIp(512, result);
	timerEngine = new TimerEngine();
	eventEngine = new TestCashlessEventEngine(result);
	commandLayer = new Vendotek::CommandLayer(context, packetLayer, tcpIp, timerEngine, eventEngine, realtime, 25000);
	return true;
}

void VendotekCommandLayerTest::cleanup() {
	delete commandLayer;
	delete eventEngine;
	delete timerEngine;
	delete tcpIp;
	delete packetLayer;
	delete context;
	delete realtime;
	delete result;
}

bool VendotekCommandLayerTest::testPayment() {
	commandLayer->reset();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// (INIT) IDL send
	timerEngine->tick(VENDOTEK_KEEPALIVE_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("010349444C0308303030303030303011143230303130313031543030303030302B30333030", packetLayer->getSendData());
	packetLayer->clearSendData();

	// (INIT) IDL recv
	// 010349444C MessageName=IDL
	// 030130 OperationNumber=0
	// 05023130 KeepaliveInterval=10
	// 0603313230 OperationTimeout=120
	packetLayer->recvPacket("010349444C030130050231300603313230");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// (DISABLED) DIS send
	timerEngine->tick(VENDOTEK_KEEPALIVE_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("01034449530308303030303030303011143230303130313031543030303030302B30333030", packetLayer->getSendData());
	packetLayer->clearSendData();

	// (DISABLED) DIS recv
	// 0103444953 MessageName=DIS
	// 03083030303030303032 EventNumber=2
	// 05023130 KeepaliveInterval=10
	// 0603313230 OperationTimeout=120
	packetLayer->recvPacket("010344495303083030303030303032050231300603313230");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	commandLayer->enable();
	// (ENABLED) IDL send
	timerEngine->tick(VENDOTEK_KEEPALIVE_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("010349444C0308303030303030303211143230303130313031543030303030302B30333030", packetLayer->getSendData());
	packetLayer->clearSendData();

	// (ENABLED) IDL recv
	packetLayer->recvPacket("010344495303023239080130");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// (ENABLED) STA
	packetLayer->recvPacket("0103535441");
	TEST_STRING_EQUAL("<event=1,SessionBegin,25000>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// (SESSION) VRP send
	commandLayer->sale(123, 7000);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("010356525003023330040437303030", packetLayer->getSendData());
	packetLayer->clearSendData();

	// (APPROVING) VRP recv
	packetLayer->recvPacket("010356525003023330040437303030");
	TEST_STRING_EQUAL("<event=1,VendApproved,1,7000,0,0>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	commandLayer->saleComplete();
	// (VENDING) FIN send
	timerEngine->tick(VENDOTEK_KEEPALIVE_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("010346494E0308303030303030333004083030303037303030", packetLayer->getSendData());
	packetLayer->clearSendData();

	// (VENDING) FIN recv
	packetLayer->recvPacket("010346494E03023330040437303030");
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// (ENABLED) IDL send
	timerEngine->tick(VENDOTEK_KEEPALIVE_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("010349444C0308303030303030333011143230303130313031543030303030302B30333030", packetLayer->getSendData());
	packetLayer->clearSendData();

	// (ENABLED) IDL recv
	packetLayer->recvPacket("010344495303023330080130");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
}

bool VendotekCommandLayerTest::testQrCode() {
	commandLayer->reset();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// (INIT) IDL send
	timerEngine->tick(VENDOTEK_KEEPALIVE_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("010349444C0308303030303030303011143230303130313031543030303030302B30333030", packetLayer->getSendData());
	packetLayer->clearSendData();

	// (INIT) IDL recv
	packetLayer->recvPacket("010349444C03023239080130");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// (DISABLED) DIS send
	timerEngine->tick(VENDOTEK_KEEPALIVE_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("01034449530308303030303030323911143230303130313031543030303030302B30333030", packetLayer->getSendData());
	packetLayer->clearSendData();

	// (DISABLED) DIS recv
	packetLayer->recvPacket("010344495303023239080130");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	commandLayer->printQrCode("", "", "12345678");
	// (DISABLED) DIS send
	timerEngine->tick(VENDOTEK_KEEPALIVE_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("0103444953030830303030303032390A08313233343536373814053230303030", packetLayer->getSendData());
	packetLayer->clearSendData();
	return true;
}

/*
14:42:52 << 1F 00 13 97 FB 01 03 43 4F 4E 0B 0A 00 B2 3E BE 8C CA 5B 00 00 00 3D 65
14:42:52 Connecting to 178.62.190.140:51803
14:42:52 >> 1F 00 13 96 FB 01 03 43 4F 4E 0B 0A 00 B2 3E BE 8C CA 5B 01 00 00 D4 4A
14:42:52 << 1F 00 0A 97 FB 01 03 44 53 43 0B 01 00 BF DD
14:42:52 >> 1F 00 0A 96 FB 01 03 44 53 43 0B 01 00 D0 98
14:42:52 << 1F 00 13 97 FB 01 03 43 4F 4E 0B 0A 00 FF FF FF FF 00 00 00 00 00 80 CE
14:42:52 Connecting to 255.255.255.255:0
14:42:52 Connect System.Net.Sockets.SocketException (0x80004005): “Â·ÛÂÏ˚È ‡‰ÂÒ ‰Îˇ Ò‚ÓÂ„Ó ÍÓÌÚÂÍÒÚ‡ ÌÂ‚ÂÂÌ 255.255.255.255:0
   ‚ System.Net.Sockets.Socket.DoConnect(EndPoint endPointSnapshot, SocketAddress socketAddress)
   ‚ System.Net.Sockets.Socket.Connect(EndPoint remoteEP)
   ‚ System.Net.Sockets.TcpClient.Connect(IPEndPoint remoteEP)
   ‚ System.Net.Sockets.TcpClient.Connect(IPAddress address, Int32 port)
   ‚ vtk_test_1.tcpSocket.connect()
14:42:52 >> 1F 00 13 96 FB 01 03 43 4F 4E 0B 0A 00 FF FF FF FF 00 00 06 00 00 EC 71
14:43:28 >> 1F 00 22 96 FB 01 03 49 44 4C 11 14 32 30 32 30 30 33 31 37 54 31 34 34 33 32 38 2B 30 33 30 30 0A 03 2E 2E 2E 75 84
14:43:28 << 1F 00 13 97 FB 01 03 49 44 4C 03 01 30 05 02 31 30 06 03 31 32 30 44 C5
14:43:30 >> 1F 00 0A 96 FB 01 03 44 49 53 03 01 30 16 39
14:43:30 << 1F 00 13 97 FB 01 03 44 49 53 03 01 30 05 02 31 30 06 03 31 32 30 EC 87
14:43:32 >> 1F 00 0F 96 FB 01 03 56 52 50 03 01 31 04 03 31 30 30 1F 46

1F 00 0F 96 FB
01 03 56 52 50
03 01 31
04 03 31 30 30
1F 46

14:43:36 << 1F 00 0D 97 FB 01 03 56 52 50 03 01 31 04 01 30 72 BD
 */
bool VendotekCommandLayerTest::testConnection() {
	commandLayer->reset();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// ----------------------------------------
	// Check conneciton to 178.62.190.140:51803
	// ----------------------------------------
	// CONN recv
	packetLayer->recvPacket("0103434F4E0B0A00B23EBE8CCA5B000000");
	TEST_STRING_EQUAL("<connect:178.62.190.140,51803,0>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// CONN send
	tcpIp->connectComplete();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("0103434F4E0B0A00B23EBE8CCA5B010200", packetLayer->getSendData());
	packetLayer->clearSendData();

	// DAT send
	packetLayer->recvPacket(
			"0103444154" // MessageName=DAT
			"0B0100" // TcpIpDestination = 0
			"0C03323938" // OutgoingByteCounter = 298
			"0E82012A" // SimpleDataBlock=298
			"16030101250100012103035E786B64FD"
			"AA1421C01D2FE623865B7F8EDDAA0231"
			"E054DE18C587623D169EC100009EC02C"
			"C030009FC0ADC09FC024C028006BC00A"
			"C0140039C0AFC0A3C02BC02F009EC0AC"
			"C09EC023C0270067C009C0130033C0AE"
			"C0A200ABC0A7C03800B3C0360091C0AB"
			"00AAC0A6C03700B2C0350090C0AA009D"
			"C09D003D0035C032C02AC00FC02EC026"
			"C005C0A1009CC09C003C002FC031C029"
			"C00EC02DC025C004C0A000AD00B70095"
			"00AC00B6009400A9C0A500AF008DC0A9"
			"00A8C0A400AE008CC0A800FF0100005A"
			"0000000E000C0000093132372E302E30"
			"2E31000D001600140603060105030501"
			"040304010303030102030201000A0018"
			"00160019001C0018001B00170016001A"
			"0015001400130012000B000201000016"
			"00000017000000230000");
	TEST_STRING_EQUAL("<send="
			"16030101250100012103035E786B64FD"
			"AA1421C01D2FE623865B7F8EDDAA0231"
			"E054DE18C587623D169EC100009EC02C"
			"C030009FC0ADC09FC024C028006BC00A"
			"C0140039C0AFC0A3C02BC02F009EC0AC"
			"C09EC023C0270067C009C0130033C0AE"
			"C0A200ABC0A7C03800B3C0360091C0AB"
			"00AAC0A6C03700B2C0350090C0AA009D"
			"C09D003D0035C032C02AC00FC02EC026"
			"C005C0A1009CC09C003C002FC031C029"
			"C00EC02DC025C004C0A000AD00B70095"
			"00AC00B6009400A9C0A500AF008DC0A9"
			"00A8C0A400AE008CC0A800FF0100005A"
			"0000000E000C0000093132372E302E30"
			"2E31000D001600140603060105030501"
			"040304010303030102030201000A0018"
			"00160019001C0018001B00170016001A"
			"0015001400130012000B000201000016"
			"00000017000000230000,len=298>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// DSC recv
	packetLayer->recvPacket("01034453430B0100");
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// DSC send
	tcpIp->remoteClose();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("01034453430B0100", packetLayer->getSendData());
	packetLayer->clearSendData();

	// -------------------------------------
	// Check conneciton to 255.255.255.255:0
	// -------------------------------------
	// CONN recv
	packetLayer->recvPacket("0103434F4E0B0A00FFFFFFFF0000000000");
	TEST_STRING_EQUAL("<connect:255.255.255.255,0,0>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// CONN send
	tcpIp->connectError();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("0103434F4E0B0A00FFFFFFFF0000030200", packetLayer->getSendData());
	packetLayer->clearSendData();

	// (INIT) IDL send
	timerEngine->tick(VENDOTEK_KEEPALIVE_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("010349444C0308303030303030303011143230303130313031543030303030302B30333030", packetLayer->getSendData());
	packetLayer->clearSendData();

	// (INIT) IDL recv
	packetLayer->recvPacket("010349444C03023239080130");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// (DISABLED) DIS send
	timerEngine->tick(VENDOTEK_KEEPALIVE_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("01034449530308303030303030323911143230303130313031543030303030302B30333030", packetLayer->getSendData());
	packetLayer->clearSendData();

	// (DISABLED) DIS recv
	packetLayer->recvPacket("010344495303023239080130");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// (DISABLED) CONN recv
	packetLayer->recvPacket("0103434F4E0B0A00B23EBE8CCA5BE00014");
	TEST_STRING_EQUAL("<connect:178.62.190.140,51803,0>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

/*
E 17:05:41 VendotekCommandLayer.cpp#309 Unsupported message name CON
x01;x03;x43;x4F;x4E;x0B;x0A;x00;xB2;x3E;xBE;x8C;xCA;x5B;xE0;x00;x14;
17:05:41 Tlv.cpp#193 type=1,len=3,value=
x43;x4F;x4E;
17:05:41 Tlv.cpp#193 type=11,len=10,value=
x00;xB2;x3E;xBE;x8C;xCA;x5B;xD0;x00;x98;
 */
	return true;
}

bool VendotekCommandLayerTest::testConnection2() {
	commandLayer->reset();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// ----------------------------------------
	// Check conneciton to 178.62.190.140:51803
	// ----------------------------------------
	// CONN recv
	packetLayer->recvPacket("0103434F4E0B0A00C3972F8415AB000000");
	TEST_STRING_EQUAL("<connect:195.151.47.132,5547,0>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// CONN send
	tcpIp->connectComplete();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("0103434F4E0B0A00C3972F8415AB010200", packetLayer->getSendData());
	packetLayer->clearSendData();

	// DAT send
	packetLayer->recvPacket(
			"0103444154" // MessageName=DAT
			"0B0100" // TcpIpDestination = 0
			"0C03323938" // OutgoingByteCounter = 298
			"0E82012A160301012501000121030300" // SimpleDataBlock=298
			"0000574DA9332C56DCB15B5871A02483"
			"8F7D4D02D554BEF49F8C8072CFEF2B00"
			"009EC02CC030009FC0ADC09FC024C028"
			"006BC00AC0140039C0AFC0A3C02BC02F"
			"009EC0ACC09EC023C0270067C009C013"
			"0033C0AEC0A200ABC0A7C03800B3C036"
			"0091C0AB00AAC0A6C03700B2C0350090"
			"C0AA009DC09D003D0035C032C02AC00F"
			"C02EC026C005C0A1009CC09C003C002F"
			"C031C029C00EC02DC025C004C0A000AD"
			"00B7009500AC00B6009400A9C0A500AF"
			"008DC0A900A8C0A400AE008CC0A800FF"
			"0100005A0000000E000C000009313237"
			"2E302E302E31000D0016001406030601"
			"05030501040304010303030102030201"
			"000A001800160019001C0018001B0017"
			"0016001A0015001400130012000B0002"
			"0100001600000017000000230000"
			);

	TEST_STRING_EQUAL(
			"<send="
			"1603010125010001210303000000574D"
			"A9332C56DCB15B5871A024838F7D4D02"
			"D554BEF49F8C8072CFEF2B00009EC02C"
			"C030009FC0ADC09FC024C028006BC00A"
			"C0140039C0AFC0A3C02BC02F009EC0AC"
			"C09EC023C0270067C009C0130033C0AE"
			"C0A200ABC0A7C03800B3C0360091C0AB"
			"00AAC0A6C03700B2C0350090C0AA009D"
			"C09D003D0035C032C02AC00FC02EC026"
			"C005C0A1009CC09C003C002FC031C029"
			"C00EC02DC025C004C0A000AD00B70095"
			"00AC00B6009400A9C0A500AF008DC0A9"
			"00A8C0A400AE008CC0A800FF0100005A"
			"0000000E000C0000093132372E302E30"
			"2E31000D001600140603060105030501"
			"040304010303030102030201000A0018"
			"00160019001C0018001B00170016001A"
			"0015001400130012000B000201000016"
			"00000017000000230000"
			",len=298>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// data sended
	tcpIp->sendComplete();
	TEST_STRING_EQUAL("01034441540B01000C03323938", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

/*
>>>>001429: 2020-04-13 11:50:03,1701873 +0,0919668
 1F 02 0E 96 FB 01 03 44 41 54 0B 01 00 0D 82 02   ...ñ˚..DAT....Ç.
 00 16 03 03 00 41 02 00 00 3D 03 03 5E 94 27 BC   .....A...=..^î'º
 F2 8E 7E 8A C2 7F 85 85 5D 2C 3E 1A C9 36 57 46   Úé~ä¬ÖÖ],>.…6WF
 45 5D E7 3C 7F 31 F1 DE BE E8 FB D2 00 C0 2F 00   E]Á<1ÒﬁæË˚“.¿/.
 00 15 00 00 00 00 FF 01 00 01 00 00 0B 00 04 03   ......ˇ.........
 00 01 02 00 23 00 00 16 03 03 0D 44 0B 00 0D 40   ....#......D...@
 00 0D 3D 00 06 05 30 82 06 01 30 82 03 E9 A0 03   ..=...0Ç..0Ç.È .
 02 01 02 02 02 10 00 30 0D 06 09 2A 86 48 86 F7   .......0...*ÜHÜ˜
 0D 01 01 0B 05 00 30 81 A4 31 0B 30 09 06 03 55   ......0Å§1.0...U
 04 06 13 02 52 55 31 0F 30 0D 06 03 55 04 08 13   ....RU1.0...U...
 06 4D 6F 73 63 6F 77 31 0F 30 0D 06 03 55 04 07   .Moscow1.0...U..
 13 06 4D 6F 73 63 6F 77 31 1B 30 19 06 03 55 04   ..Moscow1.0...U.
 0A 13 12 42 50 43 20 50 72 6F 63 65 73 73 69 6E   ...BPC Processin
 67 20 4C 4C 43 31 1A 30 18 06 03 55 04 0B 13 11   g LLC1.0...U....
 50 72 6F 63 65 73 73 69 6E 67 20 43 65 6E 74 72   Processing Centr
 65 31 1B 30 19 06 03 55 04 03 13 12 54 45 53 54   e1.0...U....TEST
 20 50 4F 53 20 43 41 20 53 48 41 32 35 36 31 1D    POS CA SHA2561.
 30 1B 06 09 2A 86 48 86 F7 0D 01 09 01 16 0E 69   0...*ÜHÜ˜......i
 6E 66 6F 40 62 70 63 62 74 2E 63 6F 6D 30 1E 17   nfo@bpcbt.com0..
 0D 31 39 31 31 30 37 31 33 31 38 35 35 5A 17 0D   .191107131855Z..
 32 39 31 31 30 34 31 33 31 38 35 35 5A 30 81 8C   291104131855Z0Åå
 31 0B 30 09 06 03 55 04 06 13 02 52 55 31 0F 30   1.0...U....RU1.0
 0D 06 03 55 04 08 0C 06 4D 6F 73 63 6F 77 31 0F   ...U....Moscow1.
 30 0D 06 03 55 04 07 0C 06 4D 6F 73 63 6F 77 31   0...U....Moscow1
 1B 30 19 06 03 55 04 0A 0C 12 4C 4C 43 20 42 50   .0...U....LLC BP
 43 20 50 72 6F 63 65 73 73 69 6E 67 31 1A 30 18   C Processing1.0.
 06 03 55 04 0B 0C 11 50 72 6F 63 65 73 73 69 6E   ..U....Processin
 67 20 43 65 6E 74 65 72 31 22 30 20 06 03 55 04   g Center1"0 ..U.
 03 0C 19 70 31 2D 74 65 73 74 2E 62 70 63 70 72   ...p1-test.bpcpr
 6F 63 65 73 73 69 6E 67 2E 63 6F 6D 30 82 02 22   ocessing.com0Ç."
 30 0D 06 09 2A 86 48 86 F7 0D 01 01 01 05 00 03   0...*ÜHÜ˜.......
 82 02 0F 00 30 82 02 0A 02 82 02 01 00 EE C4 0F   Ç...0Ç...Ç...Óƒ.
 83 70 90 52 B2 4A BE 6A BA 8B 34 8B 31 35 AB 04   ÉpêR≤Jæj∫ã4ã15´.
 4C A4 2E

 1F 02 0E 96 FB 01 03 44 41 54 0B 01 00   L§....ñ˚..DAT...
 0D 82 02 00 62 D2 71 1D 7F 02 92 CE 54 54 58 C5   .Ç..b“q..íŒTTX≈
 0F 44 39 0C 63 93 BD B4 7F A3 97 1C 8B 93 EE E8   .D9.cìΩ¥£ó.ãìÓË
 B9 8F 01 CB 87 AD EF F9 6D CB E5 03 C7 43 AD 76   πè.Àá≠Ô˘mÀÂ.«C≠v
 A0 D3 53 56 E9 D1 77 40 4F F5 59 D8 23 CE B8 11    ”SVÈ—w@OıYÿ#Œ∏.
 5E 09 4F A9 A9 6C 97 04 4F 4F 55 15 E7 72 BB B8   ^.O©©ló.OOU.Árª∏
 BC 4F D3 A6 71 28 07 86 1B 0A 6C 07 D6 29 40 85   ºO”¶q(.Ü..l.÷)@Ö
 CA E3 5C 1D 95 01 42 3F 02 8E F0 1A 3E 91 32 13    „\.ï.B?.é.>ë2.
 0D D8 D8 98 F5 64 5B 6A 74 FB 32 11 68 0F B8 AE   .ÿÿ?ıd[jt˚2.h.∏Æ
 D0 93 78 33 59 64 66 06 34 E7 2E B4 6D 24 A5 E1   –ìx3Ydf.4Á.¥m$•·
 4F 55 62 A5 51 E9 87 F1 3D A2 EA 6F EE 68 F0 4B   OUb•QÈáÒ=¢ÍoÓhK
 71 8F ED F8 85 27 71 B8 66 59 23 62 F2 C5 EA 4D   qèÌ¯Ö'q∏fY#bÚ≈ÍM
 2D 20 7C 9B 28 EA 44 75 F9 BC 17 7A 08 F0 18 E4   - |õ(ÍDu˘º.z..‰
 F6 AB 5D 58 EA 2E A9 11 BE D6 E9 F5 28 F5 51 CA   ˆ´]XÍ.©.æ÷Èı(ıQ 
 B3 51 5D 26 07 4C 6D D1 88 56 0B F6 86 93 3E C4   ≥Q]&.Lm—àV.ˆÜì>ƒ
 50 B2 D4 14 2F EB A9 D4 87 5E 90 80 33 95 31 2F   P≤‘./Î©‘á^êÄ3ï1/
 FB A8 CA C9 D5 C1 CF 82 C3 9D 36 8A A4 6D E8 D1   ˚® …’¡œÇ√ù6ä§mË—
 37 67 8B A7 C0 2E 30 7E E3 40 10 BE 7D 57 C1 1E   7gãß¿.0~„@.æ}W¡.
 A9 D7 05 C1 30 A3 68 B3 F6 F8 04 48 19 39 D6 B2   ©◊.¡0£h≥ˆ¯.H.9÷≤
 19 F5 AE 0F 5B BD 56 35 18 3C 93 10 2C C4 95 8F   .ıÆ.[ΩV5.<ì.,ƒïè
 46 64 AF 16 E3 2F 1E 6F 12 04 99 5C 27 B4 88 8A   FdØ.„/.o..ô\'¥àä
 4A FA 22 B4 08 AD B5 2C 6B FE 24 3C 1E 20 59 B8   J˙"¥.≠µ,k˛$<. Y∏
 84 92 30 60 3E FF 04 5C 2E 7F 33 67 51 2C 37 13   Ñí0`>ˇ.\.3gQ,7.
 95 8A 81 8B 72 FC 7F FE 28 B5 A8 F4 82 98 6A 10   ïäÅãr¸˛(µ®ÙÇ?j.
 6B 70 AA 92 26 F4 FD D1 C4 2C F2 98 E0 90 97 7F   kp™í&Ù˝—ƒ,Ú?‡êó
 45 93 32 04 C6 45 46 66 A4 B6 73 1B C5 36 CF 03   Eì2.∆EFf§∂s.≈6œ.
 C5 BA 0A 69 10 E7 C1 12 A8 A2 3D 9D F1 43 15 38   ≈∫.i.Á¡.®¢=ùÒC.8
 39 58 0D 98 BB 07 AE 06 7C 39 E0 11 76 EE 90 36   9X.?ª.Æ.|9‡.vÓê6
 71 69 48 C7 74 44 F9 8A 18 4A 5D B8 48 75 51 0F   qiH«tD˘ä.J]∏HuQ.
 E7 9A 98 4C 3F 4A 23 99 C6 31 50 64 D1 52 27 E0   Áö?L?J#ô∆1Pd—R'‡
 F7 54 C1 9F 5F 2F 80 15 5F 9C 09 93 FD DD 7A 11   ˜T¡ü_/Ä._ú.ì˝›z.
 C8 0A 3C 28 7A 02 D1 1B 49 63 52 47 65 43 34 AD   ».<(z.—.IcRGeC4≠
 02 03 01 00 01 A3 53 30 51 30 09 06 03 55 1D 13   .....£S0Q0...U..
 04 02 30 00 3D 2E

 1F 02 0E 96 FB 01 03 44 41 54   ..0.=....ñ˚..DAT
 0B 01 00 0D 82 02 00 30 0B 06 03 55 1D 0F 04 04   ....Ç..0...U....
 03 02 05 E0 30 37 06 03 55 1D 25 01 01 FF 04 2D   ...‡07..U.%..ˇ.-
 30 2B 06 08 2B 06 01 05 05 07 03 01 06 08 2B 06   0+..+.........+.
 01 05 05 07 03 02 06 09 60 86 48 01 86 F8 42 04   ........`ÜH.Ü¯B.
 01 06 0A 2B 06 01 04 01 82 37 0A 03 03 30 0D 06   ...+....Ç7...0..
 09 2A 86 48 86 F7 0D 01 01 0B 05 00 03 82 02 01   .*ÜHÜ˜.......Ç..
 00 B1 AF B0 46 99 3C D7 1C DA C2 75 3A F4 74 AB   .±Ø∞Fô<◊.⁄¬u:Ùt´
 7E 40 02 4D A0 39 F1 73 26 54 25 D3 F2 CD 4E 58   ~@.M 9Òs&T%”ÚÕNX
 12 83 A1 50 42 AA 69 A9 0F 29 70 E4 F5 83 92 C6   .É°PB™i©.)p‰ıÉí∆
 4C 14 BF 93 BF 02 9C 09 DF 4B D4 B6 41 31 26 82   L.øìø.ú.ﬂK‘∂A1&Ç
 F9 7F 92 CD 52 55 A9 F0 72 23 B9 0C 8E 5E FD 21   ˘íÕRU©r#π.é^˝!
 9E 37 E7 B7 D7 DE 04 A8 E6 05 9B 14 42 50 9A 1A   û7Á∑◊ﬁ.®Ê.õ.BPö.
 AD EA 29 35 0E 58 72 76 E6 55 20 14 6D A2 3A 97   ≠Í)5.XrvÊU .m¢:ó
 BF 6C 1C 72 B6 1F C9 F4 5F 72 21 DB 16 24 FE 2C   øl.r∂.…Ù_r!€.$˛,
 9D BE 54 7D FE F3 A7 57 2B 9E 5D D3 4F 9F D5 64   ùæT}˛ÛßW+û]”Oü’d
 D7 B9 47 9A 7C F2 C0 41 4D 17 F3 83 D1 17 9D DC   ◊πGö|Ú¿AM.ÛÉ—.ù‹
 B9 B1 F8 60 0C 0B B2 EA A8 77 AF 25 D8 D9 9E 47   π±¯`..≤Í®wØ%ÿŸûG
 3C 03 44 C6 8F 95 82 0D D5 2F 01 91 07 E1 19 10   <.D∆èïÇ.’/.ë.·..
 64 61 FB FE FF FD 25 78 A8 E2 72 F6 4F 02 62 7B   da˚˛ˇ˝%x®‚rˆO.b{
 92 2E 52 A0 2B E4 A8 F0 C2 0E 7B FE EF 62 51 8A   í.R +‰®¬.{˛ÔbQä
 E4 66 09 10 A3 4F CC BF 30 BB 50 84 11 6F 56 49   ‰f..£OÃø0ªPÑ.oVI
 80 8A 81 82 19 EC 29 C3 2C 2D 02 30 11 9C BE 51   ÄäÅÇ.Ï)√,-.0.úæQ
 C9 CB 7E 52 92 F6 13 A0 C9 10 98 BC 37 10 D2 DC   …À~Ríˆ. ….?º7.“‹
 C3 DD 12 6F EC 60 43 F9 C5 8A 98 B6 8D D5 76 B2   √›.oÏ`C˘≈ä?∂ç’v≤
 E3 21 3A FE 70 E9 94 61 01 6F 92 11 E2 FE 40 CD   „!:˛pÈîa.oí.‚˛@Õ
 36 E5 09 A7 36 1B 66 AC 23 00 28 29 8B 0F 03 50   6Â.ß6.f¨#.()ã..P
 25 6B 0F 26 33 7F 6A 3C 06 81 BC 28 CA 57 30 4C   %k.&3j<.Åº( W0L
 54 C6 E6 0C 56 2E 17 98 73 A5 A0 42 77 A2 57 30   T∆Ê.V..?s• Bw¢W0
 64 FB 38 17 05 84 4C 9D FE EE EC 83 67 8F 0B 6B   d˚8..ÑLù˛ÓÏÉgè.k
 56 03 8A 05 6D E7 02 57 81 8D 39 35 10 33 B8 D1   V.ä.mÁ.WÅç95.3∏—
 E5 24 1D 36 CB 19 80 9B 22 E8 15 F3 EB A6 3B 70   Â$.6À.Äõ"Ë.ÛÎ¶;p
 96 EB 35 23 24 0C 44 1A 7E 21 30 92 44 D0 FE 4C   ñÎ5#$.D.~!0íD–˛L
 92 EE 35 0B 49 95 46 18 C9

 1F 02 0E 96 FB 01 03   íÓ5.IïF.…...ñ˚..
 44 41 54 0B 01 00 0D 82 02 00 1D 1F E0 84 F6 C0   DAT....Ç....‡Ñˆ¿
 A5 E8 14 93 28 9F 1A CA 6D 71 99 22 89 8A 8A 1E   •Ë.ì(ü. mqô"âää.
 22 43 FA 66 03 18 60 F7 B9 18 91 A5 E3 64 67 57   "C˙f..`˜π.ë•„dgW
 A4 40 66 E3 AB 4E 86 1D 13 8E 09 86 64 59 BA 06   §@f„´NÜ..é.ÜdY∫.
 AF 8F 53 FF B1 7B E8 17 46 CA BD EB 39 3F 24 28   ØèSˇ±{Ë.F ΩÎ9?$(
 EC C4 E9 A2 3F 4E 60 64 77 E1 1F 7C F9 62 D6 29   ÏƒÈ¢?N`dw·.|˘b÷)
 3E 5C 90 02 00 07 32 30 82 07 2E 30 82 05 16 A0   >\ê...20Ç..0Ç..
 03 02 01 02 02 09 00 A0 D3 00 39 16 38 DC D4 30   ....... ”.9.8‹‘0
 0D 06 09 2A 86 48 86 F7 0D 01 01 05 05 00 30 81   ...*ÜHÜ˜......0Å
 A4 31 0B 30 09 06 03 55 04 06 13 02 52 55 31 0F   §1.0...U....RU1.
 30 0D 06 03 55 04 08 13 06 4D 6F 73 63 6F 77 31   0...U....Moscow1
 0F 30 0D 06 03 55 04 07 13 06 4D 6F 73 63 6F 77   .0...U....Moscow
 31 1B 30 19 06 03 55 04 0A 13 12 42 50 43 20 50   1.0...U....BPC P
 72 6F 63 65 73 73 69 6E 67 20 4C 4C 43 31 1A 30   rocessing LLC1.0
 18 06 03 55 04 0B 13 11 50 72 6F 63 65 73 73 69   ...U....Processi
 6E 67 20 43 65 6E 74 72 65 31 1B 30 19 06 03 55   ng Centre1.0...U
 04 03 13 12 54 45 53 54 20 50 4F 53 20 43 41 20   ....TEST POS CA
 53 48 41 32 35 36 31 1D 30 1B 06 09 2A 86 48 86   SHA2561.0...*ÜHÜ
 F7 0D 01 09 01 16 0E 69 6E 66 6F 40 62 70 63 62   ˜......info@bpcb
 74 2E 63 6F 6D 30 1E 17 0D 31 39 31 31 30 37 31   t.com0...1911071
 32 31 39 35 33 5A 17 0D 33 39 31 31 30 32 31 32   21953Z..39110212
 31 39 35 33 5A 30 81 A4 31 0B 30 09 06 03 55 04   1953Z0Å§1.0...U.
 06 13 02 52 55 31 0F 30 0D 06 03 55 04 08 13 06   ...RU1.0...U....
 4D 6F 73 63 6F 77 31 0F 30 0D 06 03 55 04 07 13   Moscow1.0...U...
 06 4D 6F 73 63 6F 77 31 1B 30 19 06 03 55 04 0A   .Moscow1.0...U..
 13 12 42 50 43 20 50 72 6F 63 65 73 73 69 6E 67   ..BPC Processing
 20 4C 4C 43 31 1A 30 18 06 03 55 04 0B 13 11 50    LLC1.0...U....P
 72 6F 63 65 73 73 69 6E 67 20 43 65 6E 74 72 65   rocessing Centre
 31 1B 30 19 06 03 55 04 03 13 12 54 45 53 54 20   1.0...U....TEST
 50 4F 53 20 43 41 20 53 48 41 32 35 36 31 1D 30   POS CA SHA2561.0
 1B 06 09 2A 86 48 86 F7 0D 01 09 01 16 0E 69 6E   ...*ÜHÜ˜......in
 66 6F 40 62 70 63 62 74 2E 63 6F 6D 30 82 02 22   fo@bpcbt.com0Ç."
 30 0D 06 09 2A 86 48 86 F7 0D E1 9C

 1F 02 0E 96   0...*ÜHÜ˜.·ú...ñ
 FB 01 03 44 41 54 0B 01 00 0D 82 02 00 01 01 01   ˚..DAT....Ç.....
 05 00 03 82 02 0F 00 30 82 02 0A 02 82 02 01 00   ...Ç...0Ç...Ç...
 D5 D9 D2 B5 38 E4 0C EB BB B3 2E 26 7D E7 64 00   ’Ÿ“µ8‰.Îª≥.&}Ád.
 5B 17 60 5C 1A BC 42 0C D4 AD D6 E5 B7 A1 54 24   [.`\.ºB.‘≠÷Â∑°T$
 A9 E5 B5 CE AB 57 2F 58 18 00 A2 50 F9 46 B6 AF   ©ÂµŒ´W/X..¢P˘F∂Ø
 83 C2 51 FA BE 0E 7C 66 DB D3 1F BA 24 00 09 89   É¬Q˙æ.|f€”.∫$..â
 18 9F 49 8B E5 A3 81 A6 E0 02 A8 0A 44 DC 26 CA   .üIãÂ£Å¶‡.®.D‹& 
 5E F7 0A 4A 87 BC 47 A7 9F A3 57 4F F6 D9 1C CD   ^˜.JáºGßü£WOˆŸ.Õ
 08 EE 5B EC 6B A5 AB C7 AE E3 59 7B EF 1B 9D 75   .Ó[Ïk•´«Æ„Y{Ô.ùu
 1E A1 40 67 45 AF D1 C3 48 EF 0A E0 52 FC 4A 37   .°@gEØ—√HÔ.‡R¸J7
 BF DA 8C 68 8B F8 16 E3 AC 44 50 B5 D8 6D 3A 50   ø⁄åhã¯.„¨DPµÿm:P
 22 40 BA D5 C8 A2 B9 73 36 86 8D D8 78 A4 13 07   "@∫’»¢πs6Üçÿx§..
 93 22 8C 9A AA E2 3B 86 53 7B 50 8C 87 F4 81 8E   ì"åö™‚;ÜS{PåáÙÅé
 45 14 3D 59 63 D7 A4 00 15 46 A6 27 24 45 DC 2F   E.=Yc◊§..F¶'$E‹/
 75 3B 43 8F 9D 1B 13 31 CB 14 1D BB 83 E6 5D 42   u;Cèù..1À..ªÉÊ]B
 F7 E2 A0 93 13 D8 B4 99 07 8D E9 35 69 C1 C8 2B   ˜‚ ì.ÿ¥ô.çÈ5i¡»+
 79 6E 5D 50 CB 9B E0 5F B4 FE AE 29 13 EE 0E 66   yn]PÀõ‡_¥˛Æ).Ó.f
 9B 59 6A 9D E7 A9 6B 18 16 DA 55 C2 C4 10 99 0D   õYjùÁ©k..⁄U¬ƒ.ô.
 DA 76 41 17 1E 63 21 D6 64 21 C5 34 40 28 1B E1   ⁄vA..c!÷d!≈4@(.·
 1A 02 FC 84 17 3D F6 EE 9E 46 C2 5F A7 56 7A 6E   ..¸Ñ.=ˆÓûF¬_ßVzn
 1C 53 CF D6 DB BE 15 CE 32 A7 0B 67 3C 55 52 40   .Sœ÷€æ.Œ2ß.g<UR@
 D6 C0 33 73 0B 4F 68 70 A1 2A 1F 1B 7B BC D9 45   ÷¿3s.Ohp°*..{ºŸE
 32 C6 A3 63 CA 8F 97 C5 12 0A D7 0A EF 6F D5 91   2∆£c èó≈..◊.Ôo’ë
 AE A8 76 27 C8 6D 7F 60 FE 73 B7 65 2A C1 59 10   Æ®v'»m`˛s∑e*¡Y.
 5B AB A1 29 37 B5 08 6F 10 78 B2 8B 4F 10 34 DB   [´°)7µ.o.x≤ãO.4€
 75 15 53 60 60 12 FE 9A 99 50 C1 66 45 CB 33 25   u.S``.˛öôP¡fEÀ3%
 D7 96 22 E2 99 E9 17 96 7F 43 9F 68 48 EC 6E 9D   ◊ñ"‚ôÈ.ñCühHÏnù
 32 93 D4 CB 63 62 08 4D 27 D2 5D 57 65 35 5E 60   2ì‘Àcb.M'“]We5^`
 BF 30 2A C0 A6 AA D6 61 E7 27 D6 B7 64 70 C3 83   ø0*¿¶™÷aÁ'÷∑dp√É
 5A D4 A3 2D 89 C7 25 E0 AE 9D 4B E5 69 51 03 DA   Z‘£-â«%‡ÆùKÂiQ.⁄
 45 55 A7 A7 6B BE 75 6C 78 DC F8 02 68 CF 70 AC   EUßßkæulx‹¯.hœp¨
 11 F9 F9 28 56 D2 A4 5D FB E8 05 1D F9 E8 36 5E   .˘˘(V“§]˚Ë..˘Ë6^
 03 C1 C4 B1 B3 55 08 02 51 6A 64 0C F1 49 83

 1F   .¡ƒ±≥U..Qjd.ÒIÉ.
 02 0E 96 FB 01 03 44 41 54 0B 01 00 0D 82 02 00   ..ñ˚..DAT....Ç..
 5E EC 5E 5B 25 56 3F 7E CE C4 9D 21 14 20 D0 60   ^Ï^[%V?~Œƒù!. –`
 C7 99 71 02 03 01 00 01 A3 82 01 5F 30 82 01 5B   «ôq.....£Ç._0Ç.[
 30 1D 06 03 55 1D 0E 04 16 04 14 C3 16 AF F1 24   0...U......√.ØÒ$
 52 1C 7D 61 96 15 DA 8E 04 3E 2D 65 3F 64 8A 30   R.}añ.⁄é.>-e?dä0
 81 D9 06 03 55 1D 23 04 81 D1 30 81 CE 80 14 C3   ÅŸ..U.#.Å—0ÅŒÄ.√
 16 AF F1 24 52 1C 7D 61 96 15 DA 8E 04 3E 2D 65   .ØÒ$R.}añ.⁄é.>-e
 3F 64 8A A1 81 AA A4 81 A7 30 81 A4 31 0B 30 09   ?dä°Å™§Åß0Å§1.0.
 06 03 55 04 06 13 02 52 55 31 0F 30 0D 06 03 55   ..U....RU1.0...U
 04 08 13 06 4D 6F 73 63 6F 77 31 0F 30 0D 06 03   ....Moscow1.0...
 55 04 07 13 06 4D 6F 73 63 6F 77 31 1B 30 19 06   U....Moscow1.0..
 03 55 04 0A 13 12 42 50 43 20 50 72 6F 63 65 73   .U....BPC Proces
 73 69 6E 67 20 4C 4C 43 31 1A 30 18 06 03 55 04   sing LLC1.0...U.
 0B 13 11 50 72 6F 63 65 73 73 69 6E 67 20 43 65   ...Processing Ce
 6E 74 72 65 31 1B 30 19 06 03 55 04 03 13 12 54   ntre1.0...U....T
 45 53 54 20 50 4F 53 20 43 41 20 53 48 41 32 35   EST POS CA SHA25
 36 31 1D 30 1B 06 09 2A 86 48 86 F7 0D 01 09 01   61.0...*ÜHÜ˜....
 16 0E 69 6E 66 6F 40 62 70 63 62 74 2E 63 6F 6D   ..info@bpcbt.com
 82 09 00 A0 D3 00 39 16 38 DC D4 30 12 06 03 55   Ç.. ”.9.8‹‘0...U
 1D 13 01 01 FF 04 08 30 06 01 01 FF 02 01 01 30   ....ˇ..0...ˇ...0
 11 06 09 60 86 48 01 86 F8 42 01 01 04 04 03 02   ...`ÜH.Ü¯B......
 02 04 30 37 06 03 55 1D 25 01 01 FF 04 2D 30 2B   ..07..U.%..ˇ.-0+
 06 08 2B 06 01 05 05 07 03 01 06 08 2B 06 01 05   ..+.........+...
 05 07 03 02 06 09 60 86 48 01 86 F8 42 04 01 06   ......`ÜH.Ü¯B...
 0A 2B 06 01 04 01 82 37 0A 03 03 30 0D 06 09 2A   .+....Ç7...0...*
 86 48 86 F7 0D 01 01 05 05 00 03 82 02 01 00 BB   ÜHÜ˜.......Ç...ª
 36 59 C0 22 20 80 D8 27 27 3F 81 E8 A0 53 20 E1   6Y¿" Äÿ''?ÅË S ·
 AB E3 68 D8 8F 3C AC 2B B6 43 54 72 49 21 7E D2   ´„hÿè<¨+∂CTrI!~“
 D2 D6 72 B1 35 63 04 FE 1E 5A 91 D7 B6 EE DA 94   “÷r±5c.˛.Zë◊∂Ó⁄î
 06 ED A1 84 53 6A D5 DA B3 E8 E6 C5 93 CC B4 35   .Ì°ÑSj’⁄≥ËÊ≈ìÃ¥5
 EB 0A 9C 94 0F A7 28 9C 2A F5 50 3D ED 77 B6 C8   Î.úî.ß(ú*ıP=Ìw∂»
 A0 B6 B3 9F 2D 29 14 91 42 78 A7 F7 3B B6 8C BE    ∂≥ü-).ëBxß˜;∂åæ
 C0 A7 50 EE 6E 21 81 F6 01 A3 BA B7 6E 5A DB A4   ¿ßPÓn!Åˆ.£∫∑nZ€§
 A9 92

 1F 02 0E 96 FB 01 03 44 41 54 0B 01 00 0D   ©í...ñ˚..DAT....
 82 02 00 6D FA 7F 30 4B B9 15 BA 66 41 AA 28 FC   Ç..m˙0Kπ.∫fA™(¸
 56 D1 A6 E2 9F 77 58 60 E7 4A 45 65 B0 FB F7 9D   V—¶‚üwX`ÁJEe∞˚˜ù
 70 05 44 D2 0B 0C 7D 13 C5 69 47 14 75 77 75 F6   p.D“..}.≈iG.uwuˆ
 AF 7A 92 E1 2C 11 E2 E2 B3 50 89 24 6F A5 D6 3A   Øzí·,.‚‚≥Pâ$o•÷:
 C9 6D 00 A5 B7 BB DB B2 CB 74 9B 12 D9 D4 70 3A   …m.•∑ª€≤Àtõ.Ÿ‘p:
 F1 14 68 25 79 DA 8A F6 6B C5 82 E2 0C DA EC C0   Ò.h%y⁄äˆk≈Ç‚.⁄Ï¿
 4A FE CF EC FF 38 83 89 32 78 87 E8 31 E4 30 83   J˛œÏˇ8Éâ2xáË1‰0É
 81 87 38 1B 0D 02 7E 00 D8 42 92 B5 0E A5 D1 3E   Åá8...~.ÿBíµ.•—>
 46 60 56 50 A0 22 A3 97 AB 87 74 19 09 A7 4C DD   F`VP "£ó´át..ßL›
 E4 E9 32 16 CA 4F 62 AB 09 C2 95 21 A5 49 5A 40   ‰È2. Ob´.¬ï!•IZ@
 6A 8D 2D E3 BD 46 70 F1 A9 6E 8C 1E 56 CC 3B 26   jç-„ΩFpÒ©nå.VÃ;&
 1F E9 29 EC CB A5 B6 80 A0 25 73 6E 84 CE 76 12   .È)ÏÀ•∂Ä %snÑŒv.
 17 20 B2 72 CD F4 3F EF 51 83 73 DA A8 8C 94 BD   . ≤rÕÙ?ÔQÉs⁄®åîΩ
 A2 D9 FC 58 DC A5 8A 73 2B 79 3C 96 F1 BD B0 2B   ¢Ÿ¸X‹•äs+y<ñÒΩ∞+
 63 D8 EE 92 AA 86 5F 1E 42 53 A9 3C 72 DB 1A CF   cÿÓí™Ü_.BS©<r€.œ
 57 B2 A5 91 D0 61 B3 3A 6D 40 BA 85 81 EB BA EC   W≤•ë–a≥:m@∫ÖÅÎ∫Ï
 B3 A5 FE 54 B1 BF BC F0 BB 3C 30 1A DD 23 A7 A6   ≥•˛T±øºª<0.›#ß¶
 FC AF 78 AD C0 6F C5 33 A7 78 88 1A 11 F4 89 CD   ¸Øx≠¿o≈3ßxà..ÙâÕ
 6F 1A 43 E4 58 62 F4 CF 5E DB 07 63 0E AE D8 77   o.C‰XbÙœ^€.c.Æÿw
 58 A2 83 2A 57 FA C3 00 74 70 75 84 C0 E6 60 13   X¢É*W˙√.tpuÑ¿Ê`.
 65 08 71 B8 72 A8 7D BF B6 C6 79 BF 0C A6 EC 1D   e.q∏r®}ø∂∆yø.¶Ï.
 EF B8 52 52 17 77 3D 96 04 62 A0 F3 A3 75 FD 6C   Ô∏RR.w=ñ.b Û£u˝l
 79 FE B0 53 74 08 53 30 E3 38 BB ED 23 37 B2 44   y˛∞St.S0„8ªÌ#7≤D
 65 21 05 5D E4 18 BE 60 D5 64 FA E2 40 80 76 BB   e!.]‰.æ`’d˙‚@Ävª
 89 78 58 6D D3 13 A7 3C 57 BD 36 1C 73 DF 79 46   âxXm”.ß<WΩ6.sﬂyF
 2B 12 16 03 03 02 4D 0C 00 02 49 03 00 17 41 04   +.....M...I...A.
 C6 4E 0A 97 6A 95 45 64 C3 13 3C CF F1 2B 93 E4   ∆N.ójïEd√.<œÒ+ì‰
 86 D0 E0 2D 3F 6F EB A5 67 D3 40 16 64 C7 E7 BF   Ü–‡-?oÎ•g”@.d«Áø
 B1 AA 60 41 45 6B E2 D8 D7 39 E6 0E 39 50 DE 4E   ±™`AEk‚ÿ◊9Ê.9PﬁN
 9F 0F AF 6D A9 54 A1 29 A5 9F 14 9E CF 21 76 1E   ü.Øm©T°)•ü.ûœ!v.
 06 01 02 00 9B 04 47 23 3D E9 68 A8 46 EF 20 93   ....õ.G#=Èh®FÔ ì
 4C E0 10 1A 97 B3 15 2F D5 4A 8F 4C 27 92 CA 65   L‡..ó≥./’JèL'í e
 91 70 E5 EB 62

 1F 02 0E 96 FB 01 03 44 41 54 0B   ëpÂÎb...ñ˚..DAT.
 01 00 0D 82 02 00 8D D1 D8 AB 84 CD 56 E1 F5 6C   ...Ç..ç—ÿ´ÑÕV·ıl
 E0 86 7C 10 F8 F1 CF F5 85 B5 30 B7 EE 49 2C A0   ‡Ü|.¯ÒœıÖµ0∑ÓI,
 C5 F1 E3 65 76 72 16 25 5E 06 62 5B 14 F1 2D 0A   ≈Ò„evr.%^.b[.Ò-.
 45 95 08 C8 4B F6 E0 80 1B 1F 4F 3A E8 65 AF CA   Eï.»Kˆ‡Ä..O:ËeØ 
 60 CD 4E F1 EB ED 24 86 88 CA F5 E9 28 53 3D 08   `ÕNÒÎÌ$Üà ıÈ(S=.
 8E 66 92 6E 14 A3 C6 DB E0 49 9A FD 69 A2 27 7C   éfín.£∆€‡Iö˝i¢'|
 06 B6 45 5D CC 55 95 CD 13 48 F3 52 62 F9 D8 7C   .∂E]ÃUïÕ.HÛRb˘ÿ|
 AA F3 0A E6 E8 5E 0C 3E CD 73 64 87 0D DF C9 F9   ™Û.ÊË^.>Õsdá.ﬂ…˘
 4C 7A CD D3 20 EA C0 2C A9 54 F7 A3 AE 06 F4 8D   LzÕ” Í¿,©T˜£Æ.Ùç
 9D DF D3 9F 16 87 48 97 34 23 65 BE D8 8C D1 B5   ùﬂ”ü.áHó4#eæÿå—µ
 5C 3D FA 41 57 C3 3F B5 4C 23 62 32 E3 94 C9 84   \=˙AW√?µL#b2„î…Ñ
 E1 EA 31 53 B9 7A BF EB 74 61 1C B7 49 30 A8 29   ·Í1SπzøÎta.∑I0®)
 CE 80 14 32 ED B6 42 C3 E0 57 4E DF D7 2B D2 A5   ŒÄ.2Ì∂B√‡WNﬂ◊+“•
 51 F0 82 D7 D5 11 FD 14 0D 23 A1 92 0F 20 FB 2A   QÇ◊’.˝..#°í. ˚*
 29 66 D5 46 21 DF E1 F8 6B 9E 2E 6F 6E DE 67 19   )f’F!ﬂ·¯kû.onﬁg.
 2D DC 78 E5 6A 4B 45 81 59 99 A1 90 EE 7B 9B D5   -‹xÂjKEÅYô°êÓ{õ’
 76 11 18 E7 17 F8 2C 93 A3 60 E2 CA B6 2A 12 44   v..Á.¯,ì£`‚ ∂*.D
 2D DD CB 2D 18 A4 49 64 9E D3 F4 AE 0D 6E 0A 39   -›À-.§Idû”ÙÆ.n.9
 80 4A 03 AE BB 96 D1 38 BE F5 5A 59 C2 4A FA E9   ÄJ.Æªñ—8æıZY¬J˙È
 5E 4B 5A 1E 25 5F 0E FB DB CC A9 6D BA 6C CD 0D   ^KZ.%_.˚€Ã©m∫lÕ.
 DD 4C 5B 94 37 FC EA F1 F4 E5 48 3C E3 26 7B 9D   ›L[î7¸ÍÒÙÂH<„&{ù
 5A 99 90 E5 19 8A 8E 3A FA 03 9A 84 BF 31 FF 0B   ZôêÂ.äé:˙.öÑø1ˇ.
 83 F5 C1 A1 C2 17 DB 08 35 10 09 B3 00 8D 71 A0   Éı¡°¬.€.5..≥.çq
 AE 96 71 73 CA E9 18 8F 76 CE A7 2E 81 AB CF EA   Æñqs È.èvŒß.Å´œÍ
 8A FC 38 B7 2D 25 7B 38 C3 EC 74 4D B1 BE 3D CD   ä¸8∑-%{8√ÏtM±æ=Õ
 CE 9F 1C EF 81 AE 27 3E B7 53 06 D8 D4 60 D1 79   Œü.ÔÅÆ'>∑S.ÿ‘`—y
 9E 78 36 5D 8A D0 18 BB BC 5F 17 BA 74 08 82 0A   ûx6]ä–.ªº_.∫t.Ç.
 B9 59 0B 4E 0C 1A 26 30 A5 97 BF 95 1E 93 BD 37   πY.N..&0•óøï.ìΩ7
 0D 24 E7 CE 9F D8 13 F0 BF FB 79 7C 83 4C FF 5B   .$ÁŒüÿ.ø˚y|ÉLˇ[
 8D AE 56 20 B5 36 7D A3 E3 93 EE 8D ED 62 02 7F   çÆV µ6}£„ìÓçÌb.
 88 FD E6 F3 82 92 F8 16 03 03 00 D7 0D 00 00 CF   à˝ÊÛÇí¯....◊...œ
 03 01 02 40 00 1E 06 01 06 02 06 03 05 01 05 02   ...@............
 05 03 04 01 04 02 3C 4C

 1F 00 CA 96 FB 01 03 44   ......<L.. ñ˚..D
 41 54 0B 01 00 0D 81 BD 04 03 03 01 03 02 03 03   AT....ÅΩ........
 02 01 02 02 02 03 00 A9 00 A7 30 81 A4 31 0B 30   .......©.ß0Å§1.0
 09 06 03 55 04 06 13 02 52 55 31 0F 30 0D 06 03   ...U....RU1.0...
 55 04 08 13 06 4D 6F 73 63 6F 77 31 0F 30 0D 06   U....Moscow1.0..
 03 55 04 07 13 06 4D 6F 73 63 6F 77 31 1B 30 19   .U....Moscow1.0.
 06 03 55 04 0A 13 12 42 50 43 20 50 72 6F 63 65   ..U....BPC Proce
 73 73 69 6E 67 20 4C 4C 43 31 1A 30 18 06 03 55   ssing LLC1.0...U
 04 0B 13 11 50 72 6F 63 65 73 73 69 6E 67 20 43   ....Processing C
 65 6E 74 72 65 31 1B 30 19 06 03 55 04 03 13 12   entre1.0...U....
 54 45 53 54 20 50 4F 53 20 43 41 20 53 48 41 32   TEST POS CA SHA2
 35 36 31 1D 30 1B 06 09 2A 86 48 86 F7 0D 01 09   561.0...*ÜHÜ˜...
 01 16 0E 69 6E 66 6F 40 62 70 63 62 74 2E 63 6F   ...info@bpcbt.co
 6D 0E 00 00 00 9B 8D                              m....õç
 */
	// incoming data1
	tcpIp->addRecvData("");
	tcpIp->addRecvData(
		"16030300410200003D03035E9427BCF2"
		"8E7E8AC27F85855D2C3E1AC936574645"
		"5DE73C7F31F1DEBEE8FBD200C02F0000"
		"1500000000FF01000100000B00040300"
		"0102002300001603030D440B000D4000"
		"0D3D00060530820601308203E9A00302"
		"010202021000300D06092A864886F70D"
		"01010B05003081A4310B300906035504"
		"0613025255310F300D06035504081306"
		"4D6F73636F77310F300D060355040713"
		"064D6F73636F77311B3019060355040A"
		"13124250432050726F63657373696E67"
		"204C4C43311A3018060355040B131150"
		"726F63657373696E672043656E747265"
		"311B3019060355040313125445535420"
		"504F5320434120534841323536311D30"
		"1B06092A864886F70D010901160E696E"
		"666F4062706362742E636F6D301E170D"
		"3139313130373133313835355A170D32"
		"39313130343133313835355A30818C31"
		"0B3009060355040613025255310F300D"
		"06035504080C064D6F73636F77310F30"
		"0D06035504070C064D6F73636F77311B"
		"3019060355040A0C124C4C4320425043"
		"2050726F63657373696E67311A301806"
		"0355040B0C1150726F63657373696E67"
		"2043656E746572312230200603550403"
		"0C1970312D746573742E62706370726F"
		"63657373696E672E636F6D3082022230"
		"0D06092A864886F70D01010105000382"
		"020F003082020A0282020100EEC40F83"
		"709052B24ABE6ABA8B348B3135AB044C"
		);
	TEST_STRING_EQUAL(
		"0103444154"// MessageName=DAT
		"0B0100" // TcpIpDestination = 0
		"0D820200" // SimpleDataBlock=298
		"16030300410200003D03035E9427BCF2"
		"8E7E8AC27F85855D2C3E1AC936574645"
		"5DE73C7F31F1DEBEE8FBD200C02F0000"
		"1500000000FF01000100000B00040300"
		"0102002300001603030D440B000D4000"
		"0D3D00060530820601308203E9A00302"
		"010202021000300D06092A864886F70D"
		"01010B05003081A4310B300906035504"
		"0613025255310F300D06035504081306"
		"4D6F73636F77310F300D060355040713"
		"064D6F73636F77311B3019060355040A"
		"13124250432050726F63657373696E67"
		"204C4C43311A3018060355040B131150"
		"726F63657373696E672043656E747265"
		"311B3019060355040313125445535420"
		"504F5320434120534841323536311D30"
		"1B06092A864886F70D010901160E696E"
		"666F4062706362742E636F6D301E170D"
		"3139313130373133313835355A170D32"
		"39313130343133313835355A30818C31"
		"0B3009060355040613025255310F300D"
		"06035504080C064D6F73636F77310F30"
		"0D06035504070C064D6F73636F77311B"
		"3019060355040A0C124C4C4320425043"
		"2050726F63657373696E67311A301806"
		"0355040B0C1150726F63657373696E67"
		"2043656E746572312230200603550403"
		"0C1970312D746573742E62706370726F"
		"63657373696E672E636F6D3082022230"
		"0D06092A864886F70D01010105000382"
		"020F003082020A0282020100EEC40F83"
		"709052B24ABE6ABA8B348B3135AB044C"
		, packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<recv=512><recv=512>", result->getString());
	result->clear();

	// incoming data2
	tcpIp->addRecvData(
		"62D2711D7F0292CE545458C50F44390C"
		"6393BDB47FA3971C8B93EEE8B98F01CB"
		"87ADEFF96DCBE503C743AD76A0D35356"
		"E9D177404FF559D823CEB8115E094FA9"
		"A96C97044F4F5515E772BBB8BC4FD3A6"
		"712807861B0A6C07D6294085CAE35C1D"
		"9501423F028EF01A3E9132130DD8D898"
		"F5645B6A74FB3211680FB8AED0937833"
		"5964660634E72EB46D24A5E14F5562A5"
		"51E987F13DA2EA6FEE68F04B718FEDF8"
		"852771B866592362F2C5EA4D2D207C9B"
		"28EA4475F9BC177A08F018E4F6AB5D58"
		"EA2EA911BED6E9F528F551CAB3515D26"
		"074C6DD188560BF686933EC450B2D414"
		"2FEBA9D4875E90803395312FFBA8CAC9"
		"D5C1CF82C39D368AA46DE8D137678BA7"
		"C02E307EE34010BE7D57C11EA9D705C1"
		"30A368B3F6F804481939D6B219F5AE0F"
		"5BBD5635183C93102CC4958F4664AF16"
		"E32F1E6F1204995C27B4888A4AFA22B4"
		"08ADB52C6BFE243C1E2059B884923060"
		"3EFF045C2E7F3367512C3713958A818B"
		"72FC7FFE28B5A8F482986A106B70AA92"
		"26F4FDD1C42CF298E090977F45933204"
		"C6454666A4B6731BC536CF03C5BA0A69"
		"10E7C112A8A23D9DF143153839580D98"
		"BB07AE067C39E01176EE9036716948C7"
		"7444F98A184A5DB84875510FE79A984C"
		"3F4A2399C6315064D15227E0F754C19F"
		"5F2F80155F9C0993FDDD7A11C80A3C28"
		"7A02D11B49635247654334AD02030100"
		"01A353305130090603551D1304023000"
		);
	TEST_STRING_EQUAL(
		"0103444154" // MessageName=DAT
		"0B0100" // TcpIpDestination = 0
		"0D820200" // SimpleDataBlock=298
		"62D2711D7F0292CE545458C50F44390C"
		"6393BDB47FA3971C8B93EEE8B98F01CB"
		"87ADEFF96DCBE503C743AD76A0D35356"
		"E9D177404FF559D823CEB8115E094FA9"
		"A96C97044F4F5515E772BBB8BC4FD3A6"
		"712807861B0A6C07D6294085CAE35C1D"
		"9501423F028EF01A3E9132130DD8D898"
		"F5645B6A74FB3211680FB8AED0937833"
		"5964660634E72EB46D24A5E14F5562A5"
		"51E987F13DA2EA6FEE68F04B718FEDF8"
		"852771B866592362F2C5EA4D2D207C9B"
		"28EA4475F9BC177A08F018E4F6AB5D58"
		"EA2EA911BED6E9F528F551CAB3515D26"
		"074C6DD188560BF686933EC450B2D414"
		"2FEBA9D4875E90803395312FFBA8CAC9"
		"D5C1CF82C39D368AA46DE8D137678BA7"
		"C02E307EE34010BE7D57C11EA9D705C1"
		"30A368B3F6F804481939D6B219F5AE0F"
		"5BBD5635183C93102CC4958F4664AF16"
		"E32F1E6F1204995C27B4888A4AFA22B4"
		"08ADB52C6BFE243C1E2059B884923060"
		"3EFF045C2E7F3367512C3713958A818B"
		"72FC7FFE28B5A8F482986A106B70AA92"
		"26F4FDD1C42CF298E090977F45933204"
		"C6454666A4B6731BC536CF03C5BA0A69"
		"10E7C112A8A23D9DF143153839580D98"
		"BB07AE067C39E01176EE9036716948C7"
		"7444F98A184A5DB84875510FE79A984C"
		"3F4A2399C6315064D15227E0F754C19F"
		"5F2F80155F9C0993FDDD7A11C80A3C28"
		"7A02D11B49635247654334AD02030100"
		"01A353305130090603551D1304023000"
		, packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<recv=512>", result->getString());
	result->clear();

	// incoming data end
	tcpIp->addRecvData(
		"040303010302030302010202020300A9"
		"00A73081A4310B300906035504061302"
		"5255310F300D060355040813064D6F73"
		"636F77310F300D060355040713064D6F"
		"73636F77311B3019060355040A131242"
		"50432050726F63657373696E67204C4C"
		"43311A3018060355040B131150726F63"
		"657373696E672043656E747265311B30"
		"19060355040313125445535420504F53"
		"20434120534841323536311D301B0609"
		"2A864886F70D010901160E696E666F40"
		"62706362742E636F6D0E000000"
		);
	TEST_STRING_EQUAL(
		"0103444154" // MessageName=DAT
		"0B0100" // TcpIpDestination = 0
		"0D81BD" // SimpleDataBlock=298
		"040303010302030302010202020300A9"
		"00A73081A4310B300906035504061302"
		"5255310F300D060355040813064D6F73"
		"636F77310F300D060355040713064D6F"
		"73636F77311B3019060355040A131242"
		"50432050726F63657373696E67204C4C"
		"43311A3018060355040B131150726F63"
		"657373696E672043656E747265311B30"
		"19060355040313125445535420504F53"
		"20434120534841323536311D301B0609"
		"2A864886F70D010901160E696E666F40"
		"62706362742E636F6D0E000000"
		, packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<recv=512>", result->getString());
	result->clear();

	// DSC recv
	packetLayer->recvPacket("01034453430B0100");
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// DSC send
	tcpIp->remoteClose();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("01034453430B0100", packetLayer->getSendData());
	packetLayer->clearSendData();
	return true;
}

bool VendotekCommandLayerTest::testConnectionSlowSender() {
	commandLayer->reset();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// ----------------------------------------
	// Check conneciton to 178.62.190.140:51803
	// ----------------------------------------
	// CONN recv
	packetLayer->recvPacket("0103434F4E0B0A00C3972F8415AB000000");
	TEST_STRING_EQUAL("<connect:195.151.47.132,5547,0>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// CONN send
	tcpIp->connectComplete();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("0103434F4E0B0A00C3972F8415AB010200", packetLayer->getSendData());
	packetLayer->clearSendData();

	// DAT send
	packetLayer->recvPacket(
			"0103444154" // MessageName=DAT
			"0B0100" // TcpIpDestination = 0
			"0C03323938" // OutgoingByteCounter = 298
			"0E82012A160301012501000121030300" // SimpleDataBlock=298
			"0000574DA9332C56DCB15B5871A02483"
			"8F7D4D02D554BEF49F8C8072CFEF2B00"
			"009EC02CC030009FC0ADC09FC024C028"
			"006BC00AC0140039C0AFC0A3C02BC02F"
			"009EC0ACC09EC023C0270067C009C013"
			"0033C0AEC0A200ABC0A7C03800B3C036"
			"0091C0AB00AAC0A6C03700B2C0350090"
			"C0AA009DC09D003D0035C032C02AC00F"
			"C02EC026C005C0A1009CC09C003C002F"
			"C031C029C00EC02DC025C004C0A000AD"
			"00B7009500AC00B6009400A9C0A500AF"
			"008DC0A900A8C0A400AE008CC0A800FF"
			"0100005A0000000E000C000009313237"
			"2E302E302E31000D0016001406030601"
			"05030501040304010303030102030201"
			"000A001800160019001C0018001B0017"
			"0016001A0015001400130012000B0002"
			"0100001600000017000000230000"
			);
	TEST_STRING_EQUAL(
			"<send="
			"1603010125010001210303000000574D"
			"A9332C56DCB15B5871A024838F7D4D02"
			"D554BEF49F8C8072CFEF2B00009EC02C"
			"C030009FC0ADC09FC024C028006BC00A"
			"C0140039C0AFC0A3C02BC02F009EC0AC"
			"C09EC023C0270067C009C0130033C0AE"
			"C0A200ABC0A7C03800B3C0360091C0AB"
			"00AAC0A6C03700B2C0350090C0AA009D"
			"C09D003D0035C032C02AC00FC02EC026"
			"C005C0A1009CC09C003C002FC031C029"
			"C00EC02DC025C004C0A000AD00B70095"
			"00AC00B6009400A9C0A500AF008DC0A9"
			"00A8C0A400AE008CC0A800FF0100005A"
			"0000000E000C0000093132372E302E30"
			"2E31000D001600140603060105030501"
			"040304010303030102030201000A0018"
			"00160019001C0018001B00170016001A"
			"0015001400130012000B000201000016"
			"00000017000000230000"
			",len=298>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// data sended
	tcpIp->sendComplete();
	TEST_STRING_EQUAL("01034441540B01000C03323938", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

/*
>>>>001429: 2020-04-13 11:50:03,1701873 +0,0919668
 1F 02 0E 96 FB 01 03 44 41 54 0B 01 00 0D 82 02   ...ñ˚..DAT....Ç.
 00 16 03 03 00 41 02 00 00 3D 03 03 5E 94 27 BC   .....A...=..^î'º
 F2 8E 7E 8A C2 7F 85 85 5D 2C 3E 1A C9 36 57 46   Úé~ä¬ÖÖ],>.…6WF
 45 5D E7 3C 7F 31 F1 DE BE E8 FB D2 00 C0 2F 00   E]Á<1ÒﬁæË˚“.¿/.
 00 15 00 00 00 00 FF 01 00 01 00 00 0B 00 04 03   ......ˇ.........
 00 01 02 00 23 00 00 16 03 03 0D 44 0B 00 0D 40   ....#......D...@
 00 0D 3D 00 06 05 30 82 06 01 30 82 03 E9 A0 03   ..=...0Ç..0Ç.È .
 02 01 02 02 02 10 00 30 0D 06 09 2A 86 48 86 F7   .......0...*ÜHÜ˜
 0D 01 01 0B 05 00 30 81 A4 31 0B 30 09 06 03 55   ......0Å§1.0...U
 04 06 13 02 52 55 31 0F 30 0D 06 03 55 04 08 13   ....RU1.0...U...
 06 4D 6F 73 63 6F 77 31 0F 30 0D 06 03 55 04 07   .Moscow1.0...U..
 13 06 4D 6F 73 63 6F 77 31 1B 30 19 06 03 55 04   ..Moscow1.0...U.
 0A 13 12 42 50 43 20 50 72 6F 63 65 73 73 69 6E   ...BPC Processin
 67 20 4C 4C 43 31 1A 30 18 06 03 55 04 0B 13 11   g LLC1.0...U....
 50 72 6F 63 65 73 73 69 6E 67 20 43 65 6E 74 72   Processing Centr
 65 31 1B 30 19 06 03 55 04 03 13 12 54 45 53 54   e1.0...U....TEST
 20 50 4F 53 20 43 41 20 53 48 41 32 35 36 31 1D    POS CA SHA2561.
 30 1B 06 09 2A 86 48 86 F7 0D 01 09 01 16 0E 69   0...*ÜHÜ˜......i
 6E 66 6F 40 62 70 63 62 74 2E 63 6F 6D 30 1E 17   nfo@bpcbt.com0..
 0D 31 39 31 31 30 37 31 33 31 38 35 35 5A 17 0D   .191107131855Z..
 32 39 31 31 30 34 31 33 31 38 35 35 5A 30 81 8C   291104131855Z0Åå
 31 0B 30 09 06 03 55 04 06 13 02 52 55 31 0F 30   1.0...U....RU1.0
 0D 06 03 55 04 08 0C 06 4D 6F 73 63 6F 77 31 0F   ...U....Moscow1.
 30 0D 06 03 55 04 07 0C 06 4D 6F 73 63 6F 77 31   0...U....Moscow1
 1B 30 19 06 03 55 04 0A 0C 12 4C 4C 43 20 42 50   .0...U....LLC BP
 43 20 50 72 6F 63 65 73 73 69 6E 67 31 1A 30 18   C Processing1.0.
 06 03 55 04 0B 0C 11 50 72 6F 63 65 73 73 69 6E   ..U....Processin
 67 20 43 65 6E 74 65 72 31 22 30 20 06 03 55 04   g Center1"0 ..U.
 03 0C 19 70 31 2D 74 65 73 74 2E 62 70 63 70 72   ...p1-test.bpcpr
 6F 63 65 73 73 69 6E 67 2E 63 6F 6D 30 82 02 22   ocessing.com0Ç."
 30 0D 06 09 2A 86 48 86 F7 0D 01 01 01 05 00 03   0...*ÜHÜ˜.......
 82 02 0F 00 30 82 02 0A 02 82 02 01 00 EE C4 0F   Ç...0Ç...Ç...Óƒ.
 83 70 90 52 B2 4A BE 6A BA 8B 34 8B 31 35 AB 04   ÉpêR≤Jæj∫ã4ã15´.
 4C A4 2E

 1F 02 0E 96 FB 01 03 44 41 54 0B 01 00   L§....ñ˚..DAT...
 0D 82 02 00 62 D2 71 1D 7F 02 92 CE 54 54 58 C5   .Ç..b“q..íŒTTX≈
 0F 44 39 0C 63 93 BD B4 7F A3 97 1C 8B 93 EE E8   .D9.cìΩ¥£ó.ãìÓË
 B9 8F 01 CB 87 AD EF F9 6D CB E5 03 C7 43 AD 76   πè.Àá≠Ô˘mÀÂ.«C≠v
 A0 D3 53 56 E9 D1 77 40 4F F5 59 D8 23 CE B8 11    ”SVÈ—w@OıYÿ#Œ∏.
 5E 09 4F A9 A9 6C 97 04 4F 4F 55 15 E7 72 BB B8   ^.O©©ló.OOU.Árª∏
 BC 4F D3 A6 71 28 07 86 1B 0A 6C 07 D6 29 40 85   ºO”¶q(.Ü..l.÷)@Ö
 CA E3 5C 1D 95 01 42 3F 02 8E F0 1A 3E 91 32 13    „\.ï.B?.é.>ë2.
 0D D8 D8 98 F5 64 5B 6A 74 FB 32 11 68 0F B8 AE   .ÿÿ?ıd[jt˚2.h.∏Æ
 D0 93 78 33 59 64 66 06 34 E7 2E B4 6D 24 A5 E1   –ìx3Ydf.4Á.¥m$•·
 4F 55 62 A5 51 E9 87 F1 3D A2 EA 6F EE 68 F0 4B   OUb•QÈáÒ=¢ÍoÓhK
 71 8F ED F8 85 27 71 B8 66 59 23 62 F2 C5 EA 4D   qèÌ¯Ö'q∏fY#bÚ≈ÍM
 2D 20 7C 9B 28 EA 44 75 F9 BC 17 7A 08 F0 18 E4   - |õ(ÍDu˘º.z..‰
 F6 AB 5D 58 EA 2E A9 11 BE D6 E9 F5 28 F5 51 CA   ˆ´]XÍ.©.æ÷Èı(ıQ 
 B3 51 5D 26 07 4C 6D D1 88 56 0B F6 86 93 3E C4   ≥Q]&.Lm—àV.ˆÜì>ƒ
 50 B2 D4 14 2F EB A9 D4 87 5E 90 80 33 95 31 2F   P≤‘./Î©‘á^êÄ3ï1/
 FB A8 CA C9 D5 C1 CF 82 C3 9D 36 8A A4 6D E8 D1   ˚® …’¡œÇ√ù6ä§mË—
 37 67 8B A7 C0 2E 30 7E E3 40 10 BE 7D 57 C1 1E   7gãß¿.0~„@.æ}W¡.
 A9 D7 05 C1 30 A3 68 B3 F6 F8 04 48 19 39 D6 B2   ©◊.¡0£h≥ˆ¯.H.9÷≤
 19 F5 AE 0F 5B BD 56 35 18 3C 93 10 2C C4 95 8F   .ıÆ.[ΩV5.<ì.,ƒïè
 46 64 AF 16 E3 2F 1E 6F 12 04 99 5C 27 B4 88 8A   FdØ.„/.o..ô\'¥àä
 4A FA 22 B4 08 AD B5 2C 6B FE 24 3C 1E 20 59 B8   J˙"¥.≠µ,k˛$<. Y∏
 84 92 30 60 3E FF 04 5C 2E 7F 33 67 51 2C 37 13   Ñí0`>ˇ.\.3gQ,7.
 95 8A 81 8B 72 FC 7F FE 28 B5 A8 F4 82 98 6A 10   ïäÅãr¸˛(µ®ÙÇ?j.
 6B 70 AA 92 26 F4 FD D1 C4 2C F2 98 E0 90 97 7F   kp™í&Ù˝—ƒ,Ú?‡êó
 45 93 32 04 C6 45 46 66 A4 B6 73 1B C5 36 CF 03   Eì2.∆EFf§∂s.≈6œ.
 C5 BA 0A 69 10 E7 C1 12 A8 A2 3D 9D F1 43 15 38   ≈∫.i.Á¡.®¢=ùÒC.8
 39 58 0D 98 BB 07 AE 06 7C 39 E0 11 76 EE 90 36   9X.?ª.Æ.|9‡.vÓê6
 71 69 48 C7 74 44 F9 8A 18 4A 5D B8 48 75 51 0F   qiH«tD˘ä.J]∏HuQ.
 E7 9A 98 4C 3F 4A 23 99 C6 31 50 64 D1 52 27 E0   Áö?L?J#ô∆1Pd—R'‡
 F7 54 C1 9F 5F 2F 80 15 5F 9C 09 93 FD DD 7A 11   ˜T¡ü_/Ä._ú.ì˝›z.
 C8 0A 3C 28 7A 02 D1 1B 49 63 52 47 65 43 34 AD   ».<(z.—.IcRGeC4≠
 02 03 01 00 01 A3 53 30 51 30 09 06 03 55 1D 13   .....£S0Q0...U..
 04 02 30 00 3D 2E

 1F 02 0E 96 FB 01 03 44 41 54   ..0.=....ñ˚..DAT
 0B 01 00 0D 82 02 00 30 0B 06 03 55 1D 0F 04 04   ....Ç..0...U....
 03 02 05 E0 30 37 06 03 55 1D 25 01 01 FF 04 2D   ...‡07..U.%..ˇ.-
 30 2B 06 08 2B 06 01 05 05 07 03 01 06 08 2B 06   0+..+.........+.
 01 05 05 07 03 02 06 09 60 86 48 01 86 F8 42 04   ........`ÜH.Ü¯B.
 01 06 0A 2B 06 01 04 01 82 37 0A 03 03 30 0D 06   ...+....Ç7...0..
 09 2A 86 48 86 F7 0D 01 01 0B 05 00 03 82 02 01   .*ÜHÜ˜.......Ç..
 00 B1 AF B0 46 99 3C D7 1C DA C2 75 3A F4 74 AB   .±Ø∞Fô<◊.⁄¬u:Ùt´
 7E 40 02 4D A0 39 F1 73 26 54 25 D3 F2 CD 4E 58   ~@.M 9Òs&T%”ÚÕNX
 12 83 A1 50 42 AA 69 A9 0F 29 70 E4 F5 83 92 C6   .É°PB™i©.)p‰ıÉí∆
 4C 14 BF 93 BF 02 9C 09 DF 4B D4 B6 41 31 26 82   L.øìø.ú.ﬂK‘∂A1&Ç
 F9 7F 92 CD 52 55 A9 F0 72 23 B9 0C 8E 5E FD 21   ˘íÕRU©r#π.é^˝!
 9E 37 E7 B7 D7 DE 04 A8 E6 05 9B 14 42 50 9A 1A   û7Á∑◊ﬁ.®Ê.õ.BPö.
 AD EA 29 35 0E 58 72 76 E6 55 20 14 6D A2 3A 97   ≠Í)5.XrvÊU .m¢:ó
 BF 6C 1C 72 B6 1F C9 F4 5F 72 21 DB 16 24 FE 2C   øl.r∂.…Ù_r!€.$˛,
 9D BE 54 7D FE F3 A7 57 2B 9E 5D D3 4F 9F D5 64   ùæT}˛ÛßW+û]”Oü’d
 D7 B9 47 9A 7C F2 C0 41 4D 17 F3 83 D1 17 9D DC   ◊πGö|Ú¿AM.ÛÉ—.ù‹
 B9 B1 F8 60 0C 0B B2 EA A8 77 AF 25 D8 D9 9E 47   π±¯`..≤Í®wØ%ÿŸûG
 3C 03 44 C6 8F 95 82 0D D5 2F 01 91 07 E1 19 10   <.D∆èïÇ.’/.ë.·..
 64 61 FB FE FF FD 25 78 A8 E2 72 F6 4F 02 62 7B   da˚˛ˇ˝%x®‚rˆO.b{
 92 2E 52 A0 2B E4 A8 F0 C2 0E 7B FE EF 62 51 8A   í.R +‰®¬.{˛ÔbQä
 E4 66 09 10 A3 4F CC BF 30 BB 50 84 11 6F 56 49   ‰f..£OÃø0ªPÑ.oVI
 80 8A 81 82 19 EC 29 C3 2C 2D 02 30 11 9C BE 51   ÄäÅÇ.Ï)√,-.0.úæQ
 C9 CB 7E 52 92 F6 13 A0 C9 10 98 BC 37 10 D2 DC   …À~Ríˆ. ….?º7.“‹
 C3 DD 12 6F EC 60 43 F9 C5 8A 98 B6 8D D5 76 B2   √›.oÏ`C˘≈ä?∂ç’v≤
 E3 21 3A FE 70 E9 94 61 01 6F 92 11 E2 FE 40 CD   „!:˛pÈîa.oí.‚˛@Õ
 36 E5 09 A7 36 1B 66 AC 23 00 28 29 8B 0F 03 50   6Â.ß6.f¨#.()ã..P
 25 6B 0F 26 33 7F 6A 3C 06 81 BC 28 CA 57 30 4C   %k.&3j<.Åº( W0L
 54 C6 E6 0C 56 2E 17 98 73 A5 A0 42 77 A2 57 30   T∆Ê.V..?s• Bw¢W0
 64 FB 38 17 05 84 4C 9D FE EE EC 83 67 8F 0B 6B   d˚8..ÑLù˛ÓÏÉgè.k
 56 03 8A 05 6D E7 02 57 81 8D 39 35 10 33 B8 D1   V.ä.mÁ.WÅç95.3∏—
 E5 24 1D 36 CB 19 80 9B 22 E8 15 F3 EB A6 3B 70   Â$.6À.Äõ"Ë.ÛÎ¶;p
 96 EB 35 23 24 0C 44 1A 7E 21 30 92 44 D0 FE 4C   ñÎ5#$.D.~!0íD–˛L
 92 EE 35 0B 49 95 46 18 C9

 1F 02 0E 96 FB 01 03   íÓ5.IïF.…...ñ˚..
 44 41 54 0B 01 00 0D 82 02 00 1D 1F E0 84 F6 C0   DAT....Ç....‡Ñˆ¿
 A5 E8 14 93 28 9F 1A CA 6D 71 99 22 89 8A 8A 1E   •Ë.ì(ü. mqô"âää.
 22 43 FA 66 03 18 60 F7 B9 18 91 A5 E3 64 67 57   "C˙f..`˜π.ë•„dgW
 A4 40 66 E3 AB 4E 86 1D 13 8E 09 86 64 59 BA 06   §@f„´NÜ..é.ÜdY∫.
 AF 8F 53 FF B1 7B E8 17 46 CA BD EB 39 3F 24 28   ØèSˇ±{Ë.F ΩÎ9?$(
 EC C4 E9 A2 3F 4E 60 64 77 E1 1F 7C F9 62 D6 29   ÏƒÈ¢?N`dw·.|˘b÷)
 3E 5C 90 02 00 07 32 30 82 07 2E 30 82 05 16 A0   >\ê...20Ç..0Ç..
 03 02 01 02 02 09 00 A0 D3 00 39 16 38 DC D4 30   ....... ”.9.8‹‘0
 0D 06 09 2A 86 48 86 F7 0D 01 01 05 05 00 30 81   ...*ÜHÜ˜......0Å
 A4 31 0B 30 09 06 03 55 04 06 13 02 52 55 31 0F   §1.0...U....RU1.
 30 0D 06 03 55 04 08 13 06 4D 6F 73 63 6F 77 31   0...U....Moscow1
 0F 30 0D 06 03 55 04 07 13 06 4D 6F 73 63 6F 77   .0...U....Moscow
 31 1B 30 19 06 03 55 04 0A 13 12 42 50 43 20 50   1.0...U....BPC P
 72 6F 63 65 73 73 69 6E 67 20 4C 4C 43 31 1A 30   rocessing LLC1.0
 18 06 03 55 04 0B 13 11 50 72 6F 63 65 73 73 69   ...U....Processi
 6E 67 20 43 65 6E 74 72 65 31 1B 30 19 06 03 55   ng Centre1.0...U
 04 03 13 12 54 45 53 54 20 50 4F 53 20 43 41 20   ....TEST POS CA
 53 48 41 32 35 36 31 1D 30 1B 06 09 2A 86 48 86   SHA2561.0...*ÜHÜ
 F7 0D 01 09 01 16 0E 69 6E 66 6F 40 62 70 63 62   ˜......info@bpcb
 74 2E 63 6F 6D 30 1E 17 0D 31 39 31 31 30 37 31   t.com0...1911071
 32 31 39 35 33 5A 17 0D 33 39 31 31 30 32 31 32   21953Z..39110212
 31 39 35 33 5A 30 81 A4 31 0B 30 09 06 03 55 04   1953Z0Å§1.0...U.
 06 13 02 52 55 31 0F 30 0D 06 03 55 04 08 13 06   ...RU1.0...U....
 4D 6F 73 63 6F 77 31 0F 30 0D 06 03 55 04 07 13   Moscow1.0...U...
 06 4D 6F 73 63 6F 77 31 1B 30 19 06 03 55 04 0A   .Moscow1.0...U..
 13 12 42 50 43 20 50 72 6F 63 65 73 73 69 6E 67   ..BPC Processing
 20 4C 4C 43 31 1A 30 18 06 03 55 04 0B 13 11 50    LLC1.0...U....P
 72 6F 63 65 73 73 69 6E 67 20 43 65 6E 74 72 65   rocessing Centre
 31 1B 30 19 06 03 55 04 03 13 12 54 45 53 54 20   1.0...U....TEST
 50 4F 53 20 43 41 20 53 48 41 32 35 36 31 1D 30   POS CA SHA2561.0
 1B 06 09 2A 86 48 86 F7 0D 01 09 01 16 0E 69 6E   ...*ÜHÜ˜......in
 66 6F 40 62 70 63 62 74 2E 63 6F 6D 30 82 02 22   fo@bpcbt.com0Ç."
 30 0D 06 09 2A 86 48 86 F7 0D E1 9C

 1F 02 0E 96   0...*ÜHÜ˜.·ú...ñ
 FB 01 03 44 41 54 0B 01 00 0D 82 02 00 01 01 01   ˚..DAT....Ç.....
 05 00 03 82 02 0F 00 30 82 02 0A 02 82 02 01 00   ...Ç...0Ç...Ç...
 D5 D9 D2 B5 38 E4 0C EB BB B3 2E 26 7D E7 64 00   ’Ÿ“µ8‰.Îª≥.&}Ád.
 5B 17 60 5C 1A BC 42 0C D4 AD D6 E5 B7 A1 54 24   [.`\.ºB.‘≠÷Â∑°T$
 A9 E5 B5 CE AB 57 2F 58 18 00 A2 50 F9 46 B6 AF   ©ÂµŒ´W/X..¢P˘F∂Ø
 83 C2 51 FA BE 0E 7C 66 DB D3 1F BA 24 00 09 89   É¬Q˙æ.|f€”.∫$..â
 18 9F 49 8B E5 A3 81 A6 E0 02 A8 0A 44 DC 26 CA   .üIãÂ£Å¶‡.®.D‹& 
 5E F7 0A 4A 87 BC 47 A7 9F A3 57 4F F6 D9 1C CD   ^˜.JáºGßü£WOˆŸ.Õ
 08 EE 5B EC 6B A5 AB C7 AE E3 59 7B EF 1B 9D 75   .Ó[Ïk•´«Æ„Y{Ô.ùu
 1E A1 40 67 45 AF D1 C3 48 EF 0A E0 52 FC 4A 37   .°@gEØ—√HÔ.‡R¸J7
 BF DA 8C 68 8B F8 16 E3 AC 44 50 B5 D8 6D 3A 50   ø⁄åhã¯.„¨DPµÿm:P
 22 40 BA D5 C8 A2 B9 73 36 86 8D D8 78 A4 13 07   "@∫’»¢πs6Üçÿx§..
 93 22 8C 9A AA E2 3B 86 53 7B 50 8C 87 F4 81 8E   ì"åö™‚;ÜS{PåáÙÅé
 45 14 3D 59 63 D7 A4 00 15 46 A6 27 24 45 DC 2F   E.=Yc◊§..F¶'$E‹/
 75 3B 43 8F 9D 1B 13 31 CB 14 1D BB 83 E6 5D 42   u;Cèù..1À..ªÉÊ]B
 F7 E2 A0 93 13 D8 B4 99 07 8D E9 35 69 C1 C8 2B   ˜‚ ì.ÿ¥ô.çÈ5i¡»+
 79 6E 5D 50 CB 9B E0 5F B4 FE AE 29 13 EE 0E 66   yn]PÀõ‡_¥˛Æ).Ó.f
 9B 59 6A 9D E7 A9 6B 18 16 DA 55 C2 C4 10 99 0D   õYjùÁ©k..⁄U¬ƒ.ô.
 DA 76 41 17 1E 63 21 D6 64 21 C5 34 40 28 1B E1   ⁄vA..c!÷d!≈4@(.·
 1A 02 FC 84 17 3D F6 EE 9E 46 C2 5F A7 56 7A 6E   ..¸Ñ.=ˆÓûF¬_ßVzn
 1C 53 CF D6 DB BE 15 CE 32 A7 0B 67 3C 55 52 40   .Sœ÷€æ.Œ2ß.g<UR@
 D6 C0 33 73 0B 4F 68 70 A1 2A 1F 1B 7B BC D9 45   ÷¿3s.Ohp°*..{ºŸE
 32 C6 A3 63 CA 8F 97 C5 12 0A D7 0A EF 6F D5 91   2∆£c èó≈..◊.Ôo’ë
 AE A8 76 27 C8 6D 7F 60 FE 73 B7 65 2A C1 59 10   Æ®v'»m`˛s∑e*¡Y.
 5B AB A1 29 37 B5 08 6F 10 78 B2 8B 4F 10 34 DB   [´°)7µ.o.x≤ãO.4€
 75 15 53 60 60 12 FE 9A 99 50 C1 66 45 CB 33 25   u.S``.˛öôP¡fEÀ3%
 D7 96 22 E2 99 E9 17 96 7F 43 9F 68 48 EC 6E 9D   ◊ñ"‚ôÈ.ñCühHÏnù
 32 93 D4 CB 63 62 08 4D 27 D2 5D 57 65 35 5E 60   2ì‘Àcb.M'“]We5^`
 BF 30 2A C0 A6 AA D6 61 E7 27 D6 B7 64 70 C3 83   ø0*¿¶™÷aÁ'÷∑dp√É
 5A D4 A3 2D 89 C7 25 E0 AE 9D 4B E5 69 51 03 DA   Z‘£-â«%‡ÆùKÂiQ.⁄
 45 55 A7 A7 6B BE 75 6C 78 DC F8 02 68 CF 70 AC   EUßßkæulx‹¯.hœp¨
 11 F9 F9 28 56 D2 A4 5D FB E8 05 1D F9 E8 36 5E   .˘˘(V“§]˚Ë..˘Ë6^
 03 C1 C4 B1 B3 55 08 02 51 6A 64 0C F1 49 83

 1F   .¡ƒ±≥U..Qjd.ÒIÉ.
 02 0E 96 FB 01 03 44 41 54 0B 01 00 0D 82 02 00   ..ñ˚..DAT....Ç..
 5E EC 5E 5B 25 56 3F 7E CE C4 9D 21 14 20 D0 60   ^Ï^[%V?~Œƒù!. –`
 C7 99 71 02 03 01 00 01 A3 82 01 5F 30 82 01 5B   «ôq.....£Ç._0Ç.[
 30 1D 06 03 55 1D 0E 04 16 04 14 C3 16 AF F1 24   0...U......√.ØÒ$
 52 1C 7D 61 96 15 DA 8E 04 3E 2D 65 3F 64 8A 30   R.}añ.⁄é.>-e?dä0
 81 D9 06 03 55 1D 23 04 81 D1 30 81 CE 80 14 C3   ÅŸ..U.#.Å—0ÅŒÄ.√
 16 AF F1 24 52 1C 7D 61 96 15 DA 8E 04 3E 2D 65   .ØÒ$R.}añ.⁄é.>-e
 3F 64 8A A1 81 AA A4 81 A7 30 81 A4 31 0B 30 09   ?dä°Å™§Åß0Å§1.0.
 06 03 55 04 06 13 02 52 55 31 0F 30 0D 06 03 55   ..U....RU1.0...U
 04 08 13 06 4D 6F 73 63 6F 77 31 0F 30 0D 06 03   ....Moscow1.0...
 55 04 07 13 06 4D 6F 73 63 6F 77 31 1B 30 19 06   U....Moscow1.0..
 03 55 04 0A 13 12 42 50 43 20 50 72 6F 63 65 73   .U....BPC Proces
 73 69 6E 67 20 4C 4C 43 31 1A 30 18 06 03 55 04   sing LLC1.0...U.
 0B 13 11 50 72 6F 63 65 73 73 69 6E 67 20 43 65   ...Processing Ce
 6E 74 72 65 31 1B 30 19 06 03 55 04 03 13 12 54   ntre1.0...U....T
 45 53 54 20 50 4F 53 20 43 41 20 53 48 41 32 35   EST POS CA SHA25
 36 31 1D 30 1B 06 09 2A 86 48 86 F7 0D 01 09 01   61.0...*ÜHÜ˜....
 16 0E 69 6E 66 6F 40 62 70 63 62 74 2E 63 6F 6D   ..info@bpcbt.com
 82 09 00 A0 D3 00 39 16 38 DC D4 30 12 06 03 55   Ç.. ”.9.8‹‘0...U
 1D 13 01 01 FF 04 08 30 06 01 01 FF 02 01 01 30   ....ˇ..0...ˇ...0
 11 06 09 60 86 48 01 86 F8 42 01 01 04 04 03 02   ...`ÜH.Ü¯B......
 02 04 30 37 06 03 55 1D 25 01 01 FF 04 2D 30 2B   ..07..U.%..ˇ.-0+
 06 08 2B 06 01 05 05 07 03 01 06 08 2B 06 01 05   ..+.........+...
 05 07 03 02 06 09 60 86 48 01 86 F8 42 04 01 06   ......`ÜH.Ü¯B...
 0A 2B 06 01 04 01 82 37 0A 03 03 30 0D 06 09 2A   .+....Ç7...0...*
 86 48 86 F7 0D 01 01 05 05 00 03 82 02 01 00 BB   ÜHÜ˜.......Ç...ª
 36 59 C0 22 20 80 D8 27 27 3F 81 E8 A0 53 20 E1   6Y¿" Äÿ''?ÅË S ·
 AB E3 68 D8 8F 3C AC 2B B6 43 54 72 49 21 7E D2   ´„hÿè<¨+∂CTrI!~“
 D2 D6 72 B1 35 63 04 FE 1E 5A 91 D7 B6 EE DA 94   “÷r±5c.˛.Zë◊∂Ó⁄î
 06 ED A1 84 53 6A D5 DA B3 E8 E6 C5 93 CC B4 35   .Ì°ÑSj’⁄≥ËÊ≈ìÃ¥5
 EB 0A 9C 94 0F A7 28 9C 2A F5 50 3D ED 77 B6 C8   Î.úî.ß(ú*ıP=Ìw∂»
 A0 B6 B3 9F 2D 29 14 91 42 78 A7 F7 3B B6 8C BE    ∂≥ü-).ëBxß˜;∂åæ
 C0 A7 50 EE 6E 21 81 F6 01 A3 BA B7 6E 5A DB A4   ¿ßPÓn!Åˆ.£∫∑nZ€§
 A9 92

 1F 02 0E 96 FB 01 03 44 41 54 0B 01 00 0D   ©í...ñ˚..DAT....
 82 02 00 6D FA 7F 30 4B B9 15 BA 66 41 AA 28 FC   Ç..m˙0Kπ.∫fA™(¸
 56 D1 A6 E2 9F 77 58 60 E7 4A 45 65 B0 FB F7 9D   V—¶‚üwX`ÁJEe∞˚˜ù
 70 05 44 D2 0B 0C 7D 13 C5 69 47 14 75 77 75 F6   p.D“..}.≈iG.uwuˆ
 AF 7A 92 E1 2C 11 E2 E2 B3 50 89 24 6F A5 D6 3A   Øzí·,.‚‚≥Pâ$o•÷:
 C9 6D 00 A5 B7 BB DB B2 CB 74 9B 12 D9 D4 70 3A   …m.•∑ª€≤Àtõ.Ÿ‘p:
 F1 14 68 25 79 DA 8A F6 6B C5 82 E2 0C DA EC C0   Ò.h%y⁄äˆk≈Ç‚.⁄Ï¿
 4A FE CF EC FF 38 83 89 32 78 87 E8 31 E4 30 83   J˛œÏˇ8Éâ2xáË1‰0É
 81 87 38 1B 0D 02 7E 00 D8 42 92 B5 0E A5 D1 3E   Åá8...~.ÿBíµ.•—>
 46 60 56 50 A0 22 A3 97 AB 87 74 19 09 A7 4C DD   F`VP "£ó´át..ßL›
 E4 E9 32 16 CA 4F 62 AB 09 C2 95 21 A5 49 5A 40   ‰È2. Ob´.¬ï!•IZ@
 6A 8D 2D E3 BD 46 70 F1 A9 6E 8C 1E 56 CC 3B 26   jç-„ΩFpÒ©nå.VÃ;&
 1F E9 29 EC CB A5 B6 80 A0 25 73 6E 84 CE 76 12   .È)ÏÀ•∂Ä %snÑŒv.
 17 20 B2 72 CD F4 3F EF 51 83 73 DA A8 8C 94 BD   . ≤rÕÙ?ÔQÉs⁄®åîΩ
 A2 D9 FC 58 DC A5 8A 73 2B 79 3C 96 F1 BD B0 2B   ¢Ÿ¸X‹•äs+y<ñÒΩ∞+
 63 D8 EE 92 AA 86 5F 1E 42 53 A9 3C 72 DB 1A CF   cÿÓí™Ü_.BS©<r€.œ
 57 B2 A5 91 D0 61 B3 3A 6D 40 BA 85 81 EB BA EC   W≤•ë–a≥:m@∫ÖÅÎ∫Ï
 B3 A5 FE 54 B1 BF BC F0 BB 3C 30 1A DD 23 A7 A6   ≥•˛T±øºª<0.›#ß¶
 FC AF 78 AD C0 6F C5 33 A7 78 88 1A 11 F4 89 CD   ¸Øx≠¿o≈3ßxà..ÙâÕ
 6F 1A 43 E4 58 62 F4 CF 5E DB 07 63 0E AE D8 77   o.C‰XbÙœ^€.c.Æÿw
 58 A2 83 2A 57 FA C3 00 74 70 75 84 C0 E6 60 13   X¢É*W˙√.tpuÑ¿Ê`.
 65 08 71 B8 72 A8 7D BF B6 C6 79 BF 0C A6 EC 1D   e.q∏r®}ø∂∆yø.¶Ï.
 EF B8 52 52 17 77 3D 96 04 62 A0 F3 A3 75 FD 6C   Ô∏RR.w=ñ.b Û£u˝l
 79 FE B0 53 74 08 53 30 E3 38 BB ED 23 37 B2 44   y˛∞St.S0„8ªÌ#7≤D
 65 21 05 5D E4 18 BE 60 D5 64 FA E2 40 80 76 BB   e!.]‰.æ`’d˙‚@Ävª
 89 78 58 6D D3 13 A7 3C 57 BD 36 1C 73 DF 79 46   âxXm”.ß<WΩ6.sﬂyF
 2B 12 16 03 03 02 4D 0C 00 02 49 03 00 17 41 04   +.....M...I...A.
 C6 4E 0A 97 6A 95 45 64 C3 13 3C CF F1 2B 93 E4   ∆N.ójïEd√.<œÒ+ì‰
 86 D0 E0 2D 3F 6F EB A5 67 D3 40 16 64 C7 E7 BF   Ü–‡-?oÎ•g”@.d«Áø
 B1 AA 60 41 45 6B E2 D8 D7 39 E6 0E 39 50 DE 4E   ±™`AEk‚ÿ◊9Ê.9PﬁN
 9F 0F AF 6D A9 54 A1 29 A5 9F 14 9E CF 21 76 1E   ü.Øm©T°)•ü.ûœ!v.
 06 01 02 00 9B 04 47 23 3D E9 68 A8 46 EF 20 93   ....õ.G#=Èh®FÔ ì
 4C E0 10 1A 97 B3 15 2F D5 4A 8F 4C 27 92 CA 65   L‡..ó≥./’JèL'í e
 91 70 E5 EB 62

 1F 02 0E 96 FB 01 03 44 41 54 0B   ëpÂÎb...ñ˚..DAT.
 01 00 0D 82 02 00 8D D1 D8 AB 84 CD 56 E1 F5 6C   ...Ç..ç—ÿ´ÑÕV·ıl
 E0 86 7C 10 F8 F1 CF F5 85 B5 30 B7 EE 49 2C A0   ‡Ü|.¯ÒœıÖµ0∑ÓI,
 C5 F1 E3 65 76 72 16 25 5E 06 62 5B 14 F1 2D 0A   ≈Ò„evr.%^.b[.Ò-.
 45 95 08 C8 4B F6 E0 80 1B 1F 4F 3A E8 65 AF CA   Eï.»Kˆ‡Ä..O:ËeØ 
 60 CD 4E F1 EB ED 24 86 88 CA F5 E9 28 53 3D 08   `ÕNÒÎÌ$Üà ıÈ(S=.
 8E 66 92 6E 14 A3 C6 DB E0 49 9A FD 69 A2 27 7C   éfín.£∆€‡Iö˝i¢'|
 06 B6 45 5D CC 55 95 CD 13 48 F3 52 62 F9 D8 7C   .∂E]ÃUïÕ.HÛRb˘ÿ|
 AA F3 0A E6 E8 5E 0C 3E CD 73 64 87 0D DF C9 F9   ™Û.ÊË^.>Õsdá.ﬂ…˘
 4C 7A CD D3 20 EA C0 2C A9 54 F7 A3 AE 06 F4 8D   LzÕ” Í¿,©T˜£Æ.Ùç
 9D DF D3 9F 16 87 48 97 34 23 65 BE D8 8C D1 B5   ùﬂ”ü.áHó4#eæÿå—µ
 5C 3D FA 41 57 C3 3F B5 4C 23 62 32 E3 94 C9 84   \=˙AW√?µL#b2„î…Ñ
 E1 EA 31 53 B9 7A BF EB 74 61 1C B7 49 30 A8 29   ·Í1SπzøÎta.∑I0®)
 CE 80 14 32 ED B6 42 C3 E0 57 4E DF D7 2B D2 A5   ŒÄ.2Ì∂B√‡WNﬂ◊+“•
 51 F0 82 D7 D5 11 FD 14 0D 23 A1 92 0F 20 FB 2A   QÇ◊’.˝..#°í. ˚*
 29 66 D5 46 21 DF E1 F8 6B 9E 2E 6F 6E DE 67 19   )f’F!ﬂ·¯kû.onﬁg.
 2D DC 78 E5 6A 4B 45 81 59 99 A1 90 EE 7B 9B D5   -‹xÂjKEÅYô°êÓ{õ’
 76 11 18 E7 17 F8 2C 93 A3 60 E2 CA B6 2A 12 44   v..Á.¯,ì£`‚ ∂*.D
 2D DD CB 2D 18 A4 49 64 9E D3 F4 AE 0D 6E 0A 39   -›À-.§Idû”ÙÆ.n.9
 80 4A 03 AE BB 96 D1 38 BE F5 5A 59 C2 4A FA E9   ÄJ.Æªñ—8æıZY¬J˙È
 5E 4B 5A 1E 25 5F 0E FB DB CC A9 6D BA 6C CD 0D   ^KZ.%_.˚€Ã©m∫lÕ.
 DD 4C 5B 94 37 FC EA F1 F4 E5 48 3C E3 26 7B 9D   ›L[î7¸ÍÒÙÂH<„&{ù
 5A 99 90 E5 19 8A 8E 3A FA 03 9A 84 BF 31 FF 0B   ZôêÂ.äé:˙.öÑø1ˇ.
 83 F5 C1 A1 C2 17 DB 08 35 10 09 B3 00 8D 71 A0   Éı¡°¬.€.5..≥.çq
 AE 96 71 73 CA E9 18 8F 76 CE A7 2E 81 AB CF EA   Æñqs È.èvŒß.Å´œÍ
 8A FC 38 B7 2D 25 7B 38 C3 EC 74 4D B1 BE 3D CD   ä¸8∑-%{8√ÏtM±æ=Õ
 CE 9F 1C EF 81 AE 27 3E B7 53 06 D8 D4 60 D1 79   Œü.ÔÅÆ'>∑S.ÿ‘`—y
 9E 78 36 5D 8A D0 18 BB BC 5F 17 BA 74 08 82 0A   ûx6]ä–.ªº_.∫t.Ç.
 B9 59 0B 4E 0C 1A 26 30 A5 97 BF 95 1E 93 BD 37   πY.N..&0•óøï.ìΩ7
 0D 24 E7 CE 9F D8 13 F0 BF FB 79 7C 83 4C FF 5B   .$ÁŒüÿ.ø˚y|ÉLˇ[
 8D AE 56 20 B5 36 7D A3 E3 93 EE 8D ED 62 02 7F   çÆV µ6}£„ìÓçÌb.
 88 FD E6 F3 82 92 F8 16 03 03 00 D7 0D 00 00 CF   à˝ÊÛÇí¯....◊...œ
 03 01 02 40 00 1E 06 01 06 02 06 03 05 01 05 02   ...@............
 05 03 04 01 04 02 3C 4C

 1F 00 CA 96 FB 01 03 44   ......<L.. ñ˚..D
 41 54 0B 01 00 0D 81 BD 04 03 03 01 03 02 03 03   AT....ÅΩ........
 02 01 02 02 02 03 00 A9 00 A7 30 81 A4 31 0B 30   .......©.ß0Å§1.0
 09 06 03 55 04 06 13 02 52 55 31 0F 30 0D 06 03   ...U....RU1.0...
 55 04 08 13 06 4D 6F 73 63 6F 77 31 0F 30 0D 06   U....Moscow1.0..
 03 55 04 07 13 06 4D 6F 73 63 6F 77 31 1B 30 19   .U....Moscow1.0.
 06 03 55 04 0A 13 12 42 50 43 20 50 72 6F 63 65   ..U....BPC Proce
 73 73 69 6E 67 20 4C 4C 43 31 1A 30 18 06 03 55   ssing LLC1.0...U
 04 0B 13 11 50 72 6F 63 65 73 73 69 6E 67 20 43   ....Processing C
 65 6E 74 72 65 31 1B 30 19 06 03 55 04 03 13 12   entre1.0...U....
 54 45 53 54 20 50 4F 53 20 43 41 20 53 48 41 32   TEST POS CA SHA2
 35 36 31 1D 30 1B 06 09 2A 86 48 86 F7 0D 01 09   561.0...*ÜHÜ˜...
 01 16 0E 69 6E 66 6F 40 62 70 63 62 74 2E 63 6F   ...info@bpcbt.co
 6D 0E 00 00 00 9B 8D                              m....õç
 */
	// incoming data1
	tcpIp->addRecvData("");
	tcpIp->addRecvData(
		"16030300410200003D03035E9427BCF2"
		"8E7E8AC27F85855D2C3E1AC936574645"
		"5DE73C7F31F1DEBEE8FBD200C02F0000"
		"1500000000FF01000100000B00040300"
		"0102002300001603030D440B000D4000"
		"0D3D00060530820601308203E9A00302"
		"010202021000300D06092A864886F70D"
		"01010B05003081A4310B300906035504"
		"0613025255310F300D06035504081306"
		"4D6F73636F77310F300D060355040713"
		"064D6F73636F77311B3019060355040A"
		"13124250432050726F63657373696E67"
		"204C4C43311A3018060355040B131150"
		"726F63657373696E672043656E747265"
		"311B3019060355040313125445535420"
		"504F5320434120534841323536311D30"
		"1B06092A864886F70D010901160E696E"
		"666F4062706362742E636F6D301E170D"
		"3139313130373133313835355A170D32"
		"39313130343133313835355A30818C31"
		"0B3009060355040613025255310F300D"
		"06035504080C064D6F73636F77310F30"
		"0D06035504070C064D6F73636F77311B"
		"3019060355040A0C124C4C4320425043"
		"2050726F63657373696E67311A301806"
		"0355040B0C1150726F63657373696E67"
		"2043656E746572312230200603550403"
		"0C1970312D746573742E62706370726F"
		"63657373696E672E636F6D3082022230"
		"0D06092A864886F70D01010105000382"
		"020F003082020A0282020100EEC40F83"
		"709052B24ABE6ABA8B348B3135AB044C"
		);
	TEST_STRING_EQUAL(
		"0103444154"// MessageName=DAT
		"0B0100" // TcpIpDestination = 0
		"0D820200" // SimpleDataBlock=298
		"16030300410200003D03035E9427BCF2"
		"8E7E8AC27F85855D2C3E1AC936574645"
		"5DE73C7F31F1DEBEE8FBD200C02F0000"
		"1500000000FF01000100000B00040300"
		"0102002300001603030D440B000D4000"
		"0D3D00060530820601308203E9A00302"
		"010202021000300D06092A864886F70D"
		"01010B05003081A4310B300906035504"
		"0613025255310F300D06035504081306"
		"4D6F73636F77310F300D060355040713"
		"064D6F73636F77311B3019060355040A"
		"13124250432050726F63657373696E67"
		"204C4C43311A3018060355040B131150"
		"726F63657373696E672043656E747265"
		"311B3019060355040313125445535420"
		"504F5320434120534841323536311D30"
		"1B06092A864886F70D010901160E696E"
		"666F4062706362742E636F6D301E170D"
		"3139313130373133313835355A170D32"
		"39313130343133313835355A30818C31"
		"0B3009060355040613025255310F300D"
		"06035504080C064D6F73636F77310F30"
		"0D06035504070C064D6F73636F77311B"
		"3019060355040A0C124C4C4320425043"
		"2050726F63657373696E67311A301806"
		"0355040B0C1150726F63657373696E67"
		"2043656E746572312230200603550403"
		"0C1970312D746573742E62706370726F"
		"63657373696E672E636F6D3082022230"
		"0D06092A864886F70D01010105000382"
		"020F003082020A0282020100EEC40F83"
		"709052B24ABE6ABA8B348B3135AB044C"
		, packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<recv=512><recv=512>", result->getString());
	result->clear();

	// incoming data2
	tcpIp->addRecvData(
		"62D2711D7F0292CE545458C50F44390C"
		"6393BDB47FA3971C8B93EEE8B98F01CB"
		"87ADEFF96DCBE503C743AD76A0D35356"
		"E9D177404FF559D823CEB8115E094FA9"
		"A96C97044F4F5515E772BBB8BC4FD3A6"
		"712807861B0A6C07D6294085CAE35C1D"
		"9501423F028EF01A3E9132130DD8D898"
		"F5645B6A74FB3211680FB8AED0937833"
		"5964660634E72EB46D24A5E14F5562A5"
		"51E987F13DA2EA6FEE68F04B718FEDF8"
		"852771B866592362F2C5EA4D2D207C9B"
		"28EA4475F9BC177A08F018E4F6AB5D58"
		"EA2EA911BED6E9F528F551CAB3515D26"
		"074C6DD188560BF686933EC450B2D414"
		"2FEBA9D4875E90803395312FFBA8CAC9"
		"D5C1CF82C39D368AA46DE8D137678BA7"
		"C02E307EE34010BE7D57C11EA9D705C1"
		"30A368B3F6F804481939D6B219F5AE0F"
		"5BBD5635183C93102CC4958F4664AF16"
		"E32F1E6F1204995C27B4888A4AFA22B4"
		"08ADB52C6BFE243C1E2059B884923060"
		"3EFF045C2E7F3367512C3713958A818B"
		"72FC7FFE28B5A8F482986A106B70AA92"
		"26F4FDD1C42CF298E090977F45933204"
		"C6454666A4B6731BC536CF03C5BA0A69"
		"10E7C112A8A23D9DF143153839580D98"
		"BB07AE067C39E01176EE9036716948C7"
		"7444F98A184A5DB84875510FE79A984C"
		"3F4A2399C6315064D15227E0F754C19F"
		"5F2F80155F9C0993FDDD7A11C80A3C28"
		"7A02D11B49635247654334AD02030100"
		"01A353305130090603551D1304023000"
		);
	TEST_STRING_EQUAL(
		"0103444154" // MessageName=DAT
		"0B0100" // TcpIpDestination = 0
		"0D820200" // SimpleDataBlock=298
		"62D2711D7F0292CE545458C50F44390C"
		"6393BDB47FA3971C8B93EEE8B98F01CB"
		"87ADEFF96DCBE503C743AD76A0D35356"
		"E9D177404FF559D823CEB8115E094FA9"
		"A96C97044F4F5515E772BBB8BC4FD3A6"
		"712807861B0A6C07D6294085CAE35C1D"
		"9501423F028EF01A3E9132130DD8D898"
		"F5645B6A74FB3211680FB8AED0937833"
		"5964660634E72EB46D24A5E14F5562A5"
		"51E987F13DA2EA6FEE68F04B718FEDF8"
		"852771B866592362F2C5EA4D2D207C9B"
		"28EA4475F9BC177A08F018E4F6AB5D58"
		"EA2EA911BED6E9F528F551CAB3515D26"
		"074C6DD188560BF686933EC450B2D414"
		"2FEBA9D4875E90803395312FFBA8CAC9"
		"D5C1CF82C39D368AA46DE8D137678BA7"
		"C02E307EE34010BE7D57C11EA9D705C1"
		"30A368B3F6F804481939D6B219F5AE0F"
		"5BBD5635183C93102CC4958F4664AF16"
		"E32F1E6F1204995C27B4888A4AFA22B4"
		"08ADB52C6BFE243C1E2059B884923060"
		"3EFF045C2E7F3367512C3713958A818B"
		"72FC7FFE28B5A8F482986A106B70AA92"
		"26F4FDD1C42CF298E090977F45933204"
		"C6454666A4B6731BC536CF03C5BA0A69"
		"10E7C112A8A23D9DF143153839580D98"
		"BB07AE067C39E01176EE9036716948C7"
		"7444F98A184A5DB84875510FE79A984C"
		"3F4A2399C6315064D15227E0F754C19F"
		"5F2F80155F9C0993FDDD7A11C80A3C28"
		"7A02D11B49635247654334AD02030100"
		"01A353305130090603551D1304023000"
		, packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<recv=512>", result->getString());
	result->clear();

	// incoming data end
	tcpIp->addRecvData(
		"040303010302030302010202020300A9"
		"00A73081A4310B300906035504061302"
		"5255310F300D060355040813064D6F73"
		"636F77310F300D060355040713064D6F"
		"73636F77311B3019060355040A131242"
		"50432050726F63657373696E67204C4C"
		"43311A3018060355040B131150726F63"
		"657373696E672043656E747265311B30"
		"19060355040313125445535420504F53"
		"20434120534841323536311D301B0609"
		"2A864886F70D010901160E696E666F40"
		"62706362742E636F6D0E000000"
		);
	TEST_STRING_EQUAL(
		"0103444154" // MessageName=DAT
		"0B0100" // TcpIpDestination = 0
		"0D81BD" // SimpleDataBlock=298
		"040303010302030302010202020300A9"
		"00A73081A4310B300906035504061302"
		"5255310F300D060355040813064D6F73"
		"636F77310F300D060355040713064D6F"
		"73636F77311B3019060355040A131242"
		"50432050726F63657373696E67204C4C"
		"43311A3018060355040B131150726F63"
		"657373696E672043656E747265311B30"
		"19060355040313125445535420504F53"
		"20434120534841323536311D301B0609"
		"2A864886F70D010901160E696E666F40"
		"62706362742E636F6D0E000000"
		, packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<recv=512>", result->getString());
	result->clear();

/*
<<<<001455: 2020-04-13 11:50:03,9199317 +0,0000053
 1F 04 14 97 FB 01 03 44 41 54 0B 01 00 0C 04 31   ...ó˚..DAT.....1
 33 32 32 0D 82 04 00 16 03 03 04 D8 0B 00 04 D4   322.Ç......ÿ...‘
 00 04 D1 00 04 CE 30 82 04 CA 30 82 02 B2 A0 03   ..—..Œ0Ç. 0Ç.≤ .
 02 01 02 02 02 10 04 30 0D 06 09 2A 86 48 86 F7   .......0...*ÜHÜ˜
 0D 01 01 0B 05 00 30 81 A4 31 0B 30 09 06 03 55   ......0Å§1.0...U
 04 06 13 02 52 55 31 0F 30 0D 06 03 55 04 08 13   ....RU1.0...U...
 06 4D 6F 73 63 6F 77 31 0F 30 0D 06 03 55 04 07   .Moscow1.0...U..
 13 06 4D 6F 73 63 6F 77 31 1B 30 19 06 03 55 04   ..Moscow1.0...U.
 0A 13 12 42 50 43 20 50 72 6F 63 65 73 73 69 6E   ...BPC Processin
 67 20 4C 4C 43 31 1A 30 18 06 03 55 04 0B 13 11   g LLC1.0...U....
 50 72 6F 63 65 73 73 69 6E 67 20 43 65 6E 74 72   Processing Centr
 65 31 1B 30 19 06 03 55 04 03 13 12 54 45 53 54   e1.0...U....TEST
 20 50 4F 53 20 43 41 20 53 48 41 32 35 36 31 1D    POS CA SHA2561.
 30 1B 06 09 2A 86 48 86 F7 0D 01 09 01 16 0E 69   0...*ÜHÜ˜......i
 6E 66 6F 40 62 70 63 62 74 2E 63 6F 6D 30 1E 17   nfo@bpcbt.com0..
 0D 31 39 31 31 31 34 31 36 32 30 34 34 5A 17 0D   .191114162044Z..
 32 39 31 31 31 31 31 36 32 30 34 34 5A 30 56 31   291111162044Z0V1
 0B 30 09 06 03 55 04 06 13 02 41 55 31 13 30 11   .0...U....AU1.0.
 06 03 55 04 08 0C 0A 53 6F 6D 65 2D 53 74 61 74   ..U....Some-Stat
 65 31 11 30 0F 06 03 55 04 0A 0C 08 50 4F 53 49   e1.0...U....POSI
 54 52 4F 4E 31 0C 30 0A 06 03 55 04 0B 0C 03 50   TRON1.0...U....P
 4F 53 31 11 30 0F 06 03 55 04 03 0C 08 62 70 63   OS1.0...U....bpc
 30 30 30 32 38 30 82 01 22 30 0D 06 09 2A 86 48   000280Ç."0...*ÜH
 86 F7 0D 01 01 01 05 00 03 82 01 0F 00 30 82 01   Ü˜.......Ç...0Ç.
 0A 02 82 01 01 00 C0 73 24 E5 70 98 10 B4 47 F7   ..Ç...¿s$Âp?.¥G˜
 C4 2B 16 E6 49 E2 53 25 54 39 EA 4F 6F 60 7E 6E   ƒ+.ÊI‚S%T9ÍOo`~n
 4C 57 8B 45 D4 2D D2 22 0D 88 AF E7 BF 16 EA D0   LWãE‘-“".àØÁø.Í–
 95 54 60 00 97 9C 14 CC 0A B1 B3 53 3C 06 A1 C8   ïT`.óú.Ã.±≥S<.°»
 32 07 8C F9 3E A6 A6 57 DE F1 9A 40 88 9A B9 41   2.å˘>¶¶WﬁÒö@àöπA
 EE A5 1A 6A 09 27 40 60 03 BD 45 B1 04 83 1C 98   Ó•.j.'@`.ΩE±.É.?
 C1 34 DA 7E 86 5F 43 A8 1C 90 80 04 D5 92 5F 40   ¡4⁄~Ü_C®.êÄ.’í_@
 6D CF 33 50 47 51 C2 27 36 80 B3 92 C2 48 EE 2E   mœ3PGQ¬'6Ä≥í¬HÓ.
 32 A4 FF 32 E6 94 EE B3 64 0A BD F6 AC E0 65 CF   2§ˇ2ÊîÓ≥d.Ωˆ¨‡eœ
 5F D4 FC ED F0 38 78 A6 64 91 4B 60 49 DD BE B6   _‘¸Ì8x¶dëK`I›æ∂
 76 8C 79 3C 87 FA DB D5 EF F3 0F 47 5A B4 4A DA   våy<á˙€’ÔÛ.GZ¥J⁄
 7C 8E 6F 6C 05 92 7F 2A F9 CA 25 EB 60 97 3D D3   |éol.í*˘ %Î`ó=”
 28 38 6F 00 D2 8D DD F6 23 B8 29 5F 42 5A 5C 97   (8o.“ç›ˆ#∏)_BZ\ó
 20 A7 AF 26 07 A9 E6 80 AC 77 DE B8 AA 3D DF 69    ßØ&.©ÊÄ¨wﬁ∏™=ﬂi
 D1 FA 7D 23 B1 58 DC 44 73 5D EC E0 31 63 8E 34   —˙}#±X‹Ds]Ï‡1cé4
 12 9D EB 2C 4B E7 08 6F 70 2B 37 3A 89 6B AC AD   .ùÎ,KÁ.op+7:âk¨≠
 C7 02 6E 12 C8 59 02 03 01 00 01 A3 53 30 51 30   «.n.»Y.....£S0Q0
 09 06 03 55 1D 13 04 02 30 00 30 0B 06 03 55 1D   ...U....0.0...U.
 0F 04 04 03 02 05 E0 30 37 06 03 55 1D 25 01 01   ......‡07..U.%..
 FF 04 2D 30 2B 06 08 2B 06 01 05 05 07 03 01 06   ˇ.-0+..+........
 08 2B 06 01 05 05 07 03 02 06 09 60 86 48 01 86   .+.........`ÜH.Ü
 F8 42 04 01 06 0A 2B 06 01 04 01 82 37 0A 03 03   ¯B....+....Ç7...
 30 0D 06 09 2A 86 48 86 F7 0D 01 01 0B 05 00 03   0...*ÜHÜ˜.......
 82 02 01 00 3B A8 11 AA 75 21 6A 47 7B 70 F5 97   Ç...;®.™u!jG{pıó
 FD 6D B5 83 8C 2A 12 FD 12 13 79 01 CA ED 15 34   ˝mµÉå*.˝..y. Ì.4
 0F 3C 21 CF 93 06 94 CC AB 24 B9 27 0A 76 4A AD   .<!œì.îÃ´$π'.vJ≠
 EF 1D 7D E6 EF D3 70 1C D9 C7 F9 2E DF BF FE 88   Ô.}ÊÔ”p.Ÿ«˘.ﬂø˛à
 4E 9C 0F BE 79 D8 60 7F 14 17 B2 0A CA 8B 51 D5   Nú.æyÿ`..≤. ãQ’
 9E F2 0C 1B F9 F8 68 66 C3 5C C1 60 FC AA 20 8E   ûÚ..˘¯hf√\¡`¸™ é
 67 B0 E7 B7 B9 15 54 C9 4C 96 AA 0C AE C7 4B 6D   g∞Á∑π.T…Lñ™.Æ«Km
 33 9D 1A 30 61 67 07 EC 51 07 FE E4 C0 4B F4 EA   3ù.0ag.ÏQ.˛‰¿KÙÍ
 69 F8 A2 85 A0 B4 5C A7 05 28 F7 5A A3 F7 EF 25   i¯¢Ö ¥\ß.(˜Z£˜Ô%
 C9 06 44 A7 B6 AE B3 30 6C EC A8 72 E5 46 54 99   ….Dß∂Æ≥0lÏ®rÂFTô
 F1 8F 6A CD D5 B6 12 EB A6 F0 7A D0 3D 08 46 55   ÒèjÕ’∂.Î¶z–=.FU
 75 0F BD 88 AD 53 3F F5 63 6F 8A 10 89 C0 93 FE   u.Ωà≠S?ıcoä.â¿ì˛
 25 0E B6 16 72 70 87 25 A6 5A BE F9 F5 68 7C 5B   %.∂.rpá%¶Zæ˘ıh|[
 98 00 22 94 7E 72 69 FD CD C8 8B A1 7A D0 C0 FD   ?."î~ri˝Õ»ã°z–¿˝
 F2 EA 26 CE 71 1C 20 B3 6F AB 1F 6C 63 D4 F8 2A   ÚÍ&Œq. ≥o´.lc‘¯*
 B4 51 A0 37 F3 05 09 50 71 5E 92 AB 4B C7 AD E2   ¥Q 7Û..Pq^í´K«≠‚
 97 CD 6F 98 3E 2A 15 51 30 35 EC 6D 3C 92 35 8D   óÕo?>*.Q05Ïm<í5ç
 29 A9 F5 C6 9C 30 63 6F C5 3F E1 71 27 9D 16 FE   )©ı∆ú0co≈?·q'ù.˛
 44 49 A1 5C 97 2B F1 34 D5

 1F 00 F0 97 FB 01 03   DI°\ó+Ò4’..ó˚..
 44 41 54 0B 01 00 0C 04 31 35 34 33 0D 81 DD B3   DAT.....1543.Å›≥
 B3 7F C3 8E 03 F1 57 5F CE 1A 8A 42 47 37 9A 4C   ≥√é.ÒW_Œ.äBG7öL
 8B 6D C5 85 A6 CE C3 6F 90 4A AF FE A0 96 E6 6D   ãm≈Ö¶Œ√oêJØ˛ ñÊm
 9B 6D D0 9D 70 BB 72 F7 EB 67 AC 66 B9 00 83 6E   õm–ùpªr˜Îg¨fπ.Én
 57 DB 8E 14 06 3C 68 7E C7 F2 A8 31 03 B3 9E BB   W€é..<h~«Ú®1.≥ûª
 CA 8E 7B 9E CB 9B 72 BB 52 9F BA 21 A7 3A 1E AB    é{ûÀõrªRü∫!ß:.´
 45 49 7C BD 78 C5 1F 88 1E 85 C3 CC 30 D3 64 08   EI|Ωx≈.à.Ö√Ã0”d.
 97 07 46 D0 72 FC FD F4 6B 15 41 22 67 4E 74 28   ó.F–r¸˝Ùk.A"gNt(
 12 50 DC 3B 74 DE 18 24 C1 AC AF D9 4F 27 2A CF   .P‹;tﬁ.$¡¨ØŸO'*œ
 7A FE 58 5D D8 F5 C7 B0 5B 2F C0 68 C7 E5 95 02   z˛X]ÿı«∞[/¿h«Âï.
 10 50 7E A9 3D 65 AF 1A 01 6D 66 4D F7 16 88 0E   .P~©=eØ..mfM˜.à.
 BD E8 64 44 BE 9B 5D 91 CF 18 27 A5 62 DD CD F3   ΩËdDæõ]ëœ.'•b›ÕÛ
 F9 3F 48 B9 E4 17 A1 D1 6C ED 3A F7 70 72 55 00   ˘?Hπ‰.°—lÌ:˜prU.
 F8 A6 A8 21 DA 2C 73 41 39 61 5C 78 2F 92 AD 4F   ¯¶®!⁄,sA9a\x/í≠O
 12 91 1B 0E D5 9C 86 25 0B F9 F3 E2 F1 51

 1F 00   .ë..’úÜ%.˘Û‚ÒQ..
 5D 97 FB 01 03 44 41 54 0B 01 00 0C 04 31 36 31   ]ó˚..DAT.....161
 38 0D 4B 16 03 03 00 46 10 00 00 42 41 04 34 4E   8.K....F...BA.4N
 6C 01 DA C3 43 14 B2 63 72 5C 20 E8 D1 33 DF DA   l.⁄√C.≤cr\ Ë—3ﬂ⁄
 EF D5 A4 A8 9F 1C 44 E2 3E 5E 2F B4 8D C9 61 76   Ô’§®ü.D‚>^/¥ç…av
 0B 65 05 D1 5E 24 0C 69 F4 69 6F 0F B4 57 14 C7   .e.—^$.iÙio.¥W.«
 6D 9E 7A DF 00 53 A1 27 F7 57 DC B8 D6 56 F5 AF   mûzﬂ.S°'˜W‹∏÷VıØ

 1F 01 21 97 FB 01 03 44 41 54 0B 01 00 0C 04 31   ..!ó˚..DAT.....1
 38 38 37 0D 82 01 0D 16 03 03 01 08 0F 00 01 04   887.Ç...........
 04 01 01 00 5F C1 09 5F 47 6A 17 01 49 9D 9A 73   ...._¡._Gj..Iùös
 30 48 92 B0 EC 8D A9 72 C3 6C 69 40 3E ED 3D 55   0Hí∞Ïç©r√li@>Ì=U
 79 EF 19 57 83 BF ED B3 08 4F C4 23 B6 E9 00 3E   yÔ.WÉøÌ≥.Oƒ#∂È.>
 5B A2 BC 29 2E D1 30 5C B8 6C 04 32 93 E6 C6 85   [¢º).—0\∏l.2ìÊ∆Ö
 27 D3 0C 9B 3C F8 B2 A9 2F AC 7E 50 A5 53 B2 88   '”.õ<¯≤©/¨~P•S≤à
 75 A4 1E CC A8 00 A8 B0 65 87 9C AA 08 A0 7F 86   u§.Ã®.®∞eáú™. Ü
 D8 B6 9E 57 52 80 F9 65 D2 14 C0 5E 9A 16 39 16   ÿ∂ûWRÄ˘e“.¿^ö.9.
 66 1A D4 B7 37 54 B4 65 EB A8 0B 5E BF 9F BC 03   f.‘∑7T¥eÎ®.^øüº.
 F7 7F 4E 39 4D B9 48 7B 75 14 D5 25 57 E9 7A 21   ˜N9MπH{u.’%WÈz!
 FA CD AE 7E 21 4E 8E 70 29 78 89 5F 5E 0B D9 5E   ˙ÕÆ~!Nép)xâ_^.Ÿ^
 70 12 7B 51 39 F5 55 B7 64 4F CA F1 F6 82 AB D2   p.{Q9ıU∑dO ÒˆÇ´“
 0C 41 95 E3 3E EF 5C 5E 7F 38 26 4B 70 E3 53 E8   .Aï„>Ô\^8&Kp„SË
 76 71 56 7C 35 DD 7A 7D C5 68 E0 DE 5C C8 92 D6   vqV|5›z}≈h‡ﬁ\»í÷
 18 61 8A EB 60 15 34 75 C3 A5 98 93 C8 A5 C8 8A   .aäÎ`.4u√•?ì»•»ä
 5B 01 AC AC 01 BD F5 B2 72 F1 80 78 F2 0A 43 A3   [.¨¨.Ωı≤rÒÄxÚ.C£
 49 61 32 17 AB 1D DA 2F 82 96 A2 1A 93 3E 8F 24   Ia2.´.⁄/Çñ¢.ì>è$
 89 DE 9B B4 2F 6B

 1F 00 45 97 FB 01 03 44 41 54   âﬁõ¥/k..Eó˚..DAT
 0B 01 00 0C 04 31 39 33 38 0D 33 14 03 03 00 01   .....1938.3.....
 01 16 03 03 00 28 00 00 00 00 00 00 00 00 24 CA   .....(........$ 
 65 7A 7C F5 5B 72 F6 F5 81 50 96 A1 6E FB 0E 4B   ez|ı[rˆıÅPñ°n˚.K
 83 A1 AA D7 9E 01 39 7C D8 97 8F AC 75 7D 04 A8   É°™◊û.9|ÿóè¨u}.®
 */

/*
 01 03 44 41 54 0B 01 00 0C 04 31
 33 32 32 0D 82 04 00 16 03 03 04 D8 0B 00 04 D4
 00 04 D1 00 04 CE 30 82 04 CA 30 82 02 B2 A0 03
 02 01 02 02 02 10 04 30 0D 06 09 2A 86 48 86 F7
 0D 01 01 0B 05 00 30 81 A4 31 0B 30 09 06 03 55
 04 06 13 02 52 55 31 0F 30 0D 06 03 55 04 08 13
 06 4D 6F 73 63 6F 77 31 0F 30 0D 06 03 55 04 07
 13 06 4D 6F 73 63 6F 77 31 1B 30 19 06 03 55 04
 0A 13 12 42 50 43 20 50 72 6F 63 65 73 73 69 6E
 67 20 4C 4C 43 31 1A 30 18 06 03 55 04 0B 13 11
 50 72 6F 63 65 73 73 69 6E 67 20 43 65 6E 74 72
 65 31 1B 30 19 06 03 55 04 03 13 12 54 45 53 54
 20 50 4F 53 20 43 41 20 53 48 41 32 35 36 31 1D
 30 1B 06 09 2A 86 48 86 F7 0D 01 09 01 16 0E 69
 6E 66 6F 40 62 70 63 62 74 2E 63 6F 6D 30 1E 17
 0D 31 39 31 31 31 34 31 36 32 30 34 34 5A 17 0D
 32 39 31 31 31 31 31 36 32 30 34 34 5A 30 56 31
 0B 30 09 06 03 55 04 06 13 02 41 55 31 13 30 11
 06 03 55 04 08 0C 0A 53 6F 6D 65 2D 53 74 61 74
 65 31 11 30 0F 06 03 55 04 0A 0C 08 50 4F 53 49
 54 52 4F 4E 31 0C 30 0A 06 03 55 04 0B 0C 03 50
 4F 53 31 11 30 0F 06 03 55 04 03 0C 08 62 70 63
 30 30 30 32 38 30 82 01 22 30 0D 06 09 2A 86 48
 86 F7 0D 01 01 01 05 00 03 82 01 0F 00 30 82 01
 0A 02 82 01 01 00 C0 73 24 E5 70 98 10 B4 47 F7
 C4 2B 16 E6 49 E2 53 25 54 39 EA 4F 6F 60 7E 6E
 4C 57 8B 45 D4 2D D2 22 0D 88 AF E7 BF 16 EA D0
 95 54 60 00 97 9C 14 CC 0A B1 B3 53 3C 06 A1 C8
 32 07 8C F9 3E A6 A6 57 DE F1 9A 40 88 9A B9 41
 EE A5 1A 6A 09 27 40 60 03 BD 45 B1 04 83 1C 98
 C1 34 DA 7E 86 5F 43 A8 1C 90 80 04 D5 92 5F 40
 6D CF 33 50 47 51 C2 27 36 80 B3 92 C2 48 EE 2E
 32 A4 FF 32 E6 94 EE B3 64 0A BD F6 AC E0 65 CF
 5F D4 FC ED F0 38 78 A6 64 91 4B 60 49 DD BE B6
 76 8C 79 3C 87 FA DB D5 EF F3 0F 47 5A B4 4A DA
 7C 8E 6F 6C 05 92 7F 2A F9 CA 25 EB 60 97 3D D3
 28 38 6F 00 D2 8D DD F6 23 B8 29 5F 42 5A 5C 97
 20 A7 AF 26 07 A9 E6 80 AC 77 DE B8 AA 3D DF 69
 D1 FA 7D 23 B1 58 DC 44 73 5D EC E0 31 63 8E 34
 12 9D EB 2C 4B E7 08 6F 70 2B 37 3A 89 6B AC AD
 C7 02 6E 12 C8 59 02 03 01 00 01 A3 53 30 51 30
 09 06 03 55 1D 13 04 02 30 00 30 0B 06 03 55 1D
 0F 04 04 03 02 05 E0 30 37 06 03 55 1D 25 01 01
 FF 04 2D 30 2B 06 08 2B 06 01 05 05 07 03 01 06
 08 2B 06 01 05 05 07 03 02 06 09 60 86 48 01 86
 F8 42 04 01 06 0A 2B 06 01 04 01 82 37 0A 03 03
 30 0D 06 09 2A 86 48 86 F7 0D 01 01 0B 05 00 03
 82 02 01 00 3B A8 11 AA 75 21 6A 47 7B 70 F5 97
 FD 6D B5 83 8C 2A 12 FD 12 13 79 01 CA ED 15 34
 0F 3C 21 CF 93 06 94 CC AB 24 B9 27 0A 76 4A AD
 EF 1D 7D E6 EF D3 70 1C D9 C7 F9 2E DF BF FE 88
 4E 9C 0F BE 79 D8 60 7F 14 17 B2 0A CA 8B 51 D5
 9E F2 0C 1B F9 F8 68 66 C3 5C C1 60 FC AA 20 8E
 67 B0 E7 B7 B9 15 54 C9 4C 96 AA 0C AE C7 4B 6D
 33 9D 1A 30 61 67 07 EC 51 07 FE E4 C0 4B F4 EA
 69 F8 A2 85 A0 B4 5C A7 05 28 F7 5A A3 F7 EF 25
 C9 06 44 A7 B6 AE B3 30 6C EC A8 72 E5 46 54 99
 F1 8F 6A CD D5 B6 12 EB A6 F0 7A D0 3D 08 46 55
 75 0F BD 88 AD 53 3F F5 63 6F 8A 10 89 C0 93 FE
 25 0E B6 16 72 70 87 25 A6 5A BE F9 F5 68 7C 5B
 98 00 22 94 7E 72 69 FD CD C8 8B A1 7A D0 C0 FD
 F2 EA 26 CE 71 1C 20 B3 6F AB 1F 6C 63 D4 F8 2A
 B4 51 A0 37 F3 05 09 50 71 5E 92 AB 4B C7 AD E2
 97 CD 6F 98 3E 2A 15 51 30 35 EC 6D 3C 92 35 8D
 29 A9 F5 C6 9C 30 63 6F C5 3F E1 71 27 9D 16 FE
 44 49 A1 5C 97 2B F1
 */
	// DAT send
	packetLayer->recvPacket(
			"0103444154" // MessageName=DAT
			"0B0100" // TcpIpDestination = 0
			"0C03323938" // OutgoingByteCounter = 298
			"0D82012A160301012501000121030300" // SimpleDataBlock=298

			"0103444154" // MessageName=DAT
			"0B0100" // TcpIpDestination = 0
			"0C0431333232" // OutgoingByteCounter = 1322
			"0E820400"//SimpleDataBlock=1024
			"16030304D80B0004D40004D10004CE30"
			"8204CA308202B2A00302010202021004"
			"300D06092A864886F70D01010B050030"
			"81A4310B300906035504061302525531"
			"0F300D060355040813064D6F73636F77"
			"310F300D060355040713064D6F73636F"
			"77311B3019060355040A131242504320"
			"50726F63657373696E67204C4C43311A"
			"3018060355040B131150726F63657373"
			"696E672043656E747265311B30190603"
			"55040313125445535420504F53204341"
			"20534841323536311D301B06092A8648"
			"86F70D010901160E696E666F40627063"
			"62742E636F6D301E170D313931313134"
			"3136323034345A170D32393131313131"
			"36323034345A3056310B300906035504"
			"06130241553113301106035504080C0A"
			"536F6D652D53746174653111300F0603"
			"55040A0C08504F534954524F4E310C30"
			"0A060355040B0C03504F533111300F06"
			"035504030C0862706330303032383082"
			"0122300D06092A864886F70D01010105"
			"000382010F003082010A0282010100C0"
			"7324E5709810B447F7C42B16E649E253"
			"255439EA4F6F607E6E4C578B45D42DD2"
			"220D88AFE7BF16EAD095546000979C14"
			"CC0AB1B3533C06A1C832078CF93EA6A6"
			"57DEF19A40889AB941EEA51A6A092740"
			"6003BD45B104831C98C134DA7E865F43"
			"A81C908004D5925F406DCF33504751C2"
			"273680B392C248EE2E32A4FF32E694EE"
			"B3640ABDF6ACE065CF5FD4FCEDF03878"
			"A664914B6049DDBEB6768C793C87FADB"
			"D5EFF30F475AB44ADA7C8E6F6C05927F"
			"2AF9CA25EB60973DD328386F00D28DDD"
			"F623B8295F425A5C9720A7AF2607A9E6"
			"80AC77DEB8AA3DDF69D1FA7D23B158DC"
			"44735DECE031638E34129DEB2C4BE708"
			"6F702B373A896BACADC7026E12C85902"
			"03010001A353305130090603551D1304"
			"023000300B0603551D0F0404030205E0"
			"30370603551D250101FF042D302B0608"
			"2B0601050507030106082B0601050507"
			"030206096086480186F8420401060A2B"
			"0601040182370A0303300D06092A8648"
			"86F70D01010B050003820201003BA811"
			"AA75216A477B70F597FD6DB5838C2A12"
			"FD12137901CAED15340F3C21CF930694"
			"CCAB24B9270A764AADEF1D7DE6EFD370"
			"1CD9C7F92EDFBFFE884E9C0FBE79D860"
			"7F1417B20ACA8B51D59EF20C1BF9F868"
			"66C35CC160FCAA208E67B0E7B7B91554"
			"C94C96AA0CAEC74B6D339D1A30616707"
			"EC5107FEE4C04BF4EA69F8A285A0B45C"
			"A70528F75AA3F7EF25C90644A7B6AEB3"
			"306CECA872E5465499F18F6ACDD5B612"
			"EBA6F07AD03D084655750FBD88AD533F"
			"F5636F8A1089C093FE250EB616727087"
			"25A65ABEF9F5687C5B980022947E7269"
			"FDCDC88BA17AD0C0FDF2EA26CE711C20"
			"B36FAB1F6C63D4F82AB451A037F30509"
			"50715E92AB4BC7ADE297CD6F983E2A15"
			"513035EC6D3C92358D29A9F5C69C3063"
			"6FC53FE171279D16FE4449A15C972BF1"
			);
	TEST_STRING_EQUAL(
			"<send="
			"1603010125010001210303000000574D"
			"A9332C56DCB15B5871A024838F7D4D02"
			"D554BEF49F8C8072CFEF2B00009EC02C"
			"C030009FC0ADC09FC024C028006BC00A"
			"C0140039C0AFC0A3C02BC02F009EC0AC"
			"C09EC023C0270067C009C0130033C0AE"
			"C0A200ABC0A7C03800B3C0360091C0AB"
			"00AAC0A6C03700B2C0350090C0AA009D"
			"C09D003D0035C032C02AC00FC02EC026"
			"C005C0A1009CC09C003C002FC031C029"
			"C00EC02DC025C004C0A000AD00B70095"
			"00AC00B6009400A9C0A500AF008DC0A9"
			"00A8C0A400AE008CC0A800FF0100005A"
			"0000000E000C0000093132372E302E30"
			"2E31000D001600140603060105030501"
			"040304010303030102030201000A0018"
			"00160019001C0018001B00170016001A"
			"0015001400130012000B000201000016"
			"00000017000000230000"
			",len=298>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// data sended
	tcpIp->sendComplete();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("", result->getString());

	// DSC recv
	packetLayer->recvPacket("01034453430B0100");
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// DSC send
	tcpIp->remoteClose();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("01034453430B0100", packetLayer->getSendData());
	packetLayer->clearSendData();
	return true;
}

bool VendotekCommandLayerTest::testConfirmableDataBlock() {
	commandLayer->reset();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// ----------------------------------------
	// Check conneciton to 178.62.190.140:51803
	// ----------------------------------------
	// CONN recv
	packetLayer->recvPacket("0103434F4E0B0A00C3972F8415AB000000");
	TEST_STRING_EQUAL("<connect:195.151.47.132,5547,0>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// CONN send
	tcpIp->connectComplete();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("0103434F4E0B0A00C3972F8415AB0109D0", packetLayer->getSendData());
	packetLayer->clearSendData();

	// DAT send
	packetLayer->recvPacket(
			"0103444154" // MessageName=DAT
			"0B0100" // TcpIpDestination = 0
			"0C03323938" // OutgoingByteCounter = 298
			"0E82012A" // SimpleDataBlock=298
			"16030101250100012103035EAACABDDF"
			"B59104E67519FD0C6644F9FCB1C2495A"
			"71B3AB7C31B47455033AEF00009EC02C"
			"C030009FC0ADC09FC024C028006BC00A"
			"C0140039C0AFC0A3C02BC02F009EC0AC"
			"C09EC023C0270067C009C0130033C0AE"
			"C0A200ABC0A7C03800B3C0360091C0AB"
			"00AAC0A6C03700B2C0350090C0AA009D"
			"C09D003D0035C032C02AC00FC02EC026"
			"C005C0A1009CC09C003C002FC031C029"
			"C00EC02DC025C004C0A000AD00B70095"
			"00AC00B6009400A9C0A500AF008DC0A9"
			"00A8C0A400AE008CC0A800FF0100005A"
			"0000000E000C0000093132372E302E30"
			"2E31000D001600140603060105030501"
			"040304010303030102030201000A0018"
			"00160019001C0018001B00170016001A"
			"0015001400130012000B000201000016"
			"00000017000000230000"
			);
	TEST_STRING_EQUAL(
			"<send="
			"16030101250100012103035EAACABDDF"
			"B59104E67519FD0C6644F9FCB1C2495A"
			"71B3AB7C31B47455033AEF00009EC02C"
			"C030009FC0ADC09FC024C028006BC00A"
			"C0140039C0AFC0A3C02BC02F009EC0AC"
			"C09EC023C0270067C009C0130033C0AE"
			"C0A200ABC0A7C03800B3C0360091C0AB"
			"00AAC0A6C03700B2C0350090C0AA009D"
			"C09D003D0035C032C02AC00FC02EC026"
			"C005C0A1009CC09C003C002FC031C029"
			"C00EC02DC025C004C0A000AD00B70095"
			"00AC00B6009400A9C0A500AF008DC0A9"
			"00A8C0A400AE008CC0A800FF0100005A"
			"0000000E000C0000093132372E302E30"
			"2E31000D001600140603060105030501"
			"040304010303030102030201000A0018"
			"00160019001C0018001B00170016001A"
			"0015001400130012000B000201000016"
			"00000017000000230000"
			",len=298>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// data sended
	tcpIp->sendComplete();
	TEST_STRING_EQUAL(
			"0103444154" // MessageName=DAT
			"0B0100" // TcpIpDestination = 0
			"0C03323938", // OutgoingByteCounter = 298
			packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// incoming data1
	tcpIp->addRecvData("");
	tcpIp->addRecvData(
			"16030300410200003D03035EAACABEB2"
			"9917A49FF4094969B3C23DD02946FAA6"
			"0666DEC467F0C4FA1D6ACF00C02F0000"
			"1500000000FF01000100000B00040300"
			"0102002300001603030D440B000D4000"
			"0D3D00060530820601308203E9A00302"
			"010202021000300D06092A864886F70D"
			"01010B05003081A4310B300906035504"
			"0613025255310F300D06035504081306"
			"4D6F73636F77310F300D060355040713"
			"064D6F73636F77311B3019060355040A"
			"13124250432050726F63657373696E67"
			"204C4C43311A3018060355040B131150"
			"726F63657373696E672043656E747265"
			"311B3019060355040313125445535420"
			"504F5320434120534841323536311D30"
			"1B06092A864886F70D010901160E696E"
			"666F4062706362742E636F6D301E170D"
			"3139313130373133313835355A170D32"
			"39313130343133313835355A30818C31"
			"0B3009060355040613025255310F300D"
			"06035504080C064D6F73636F77310F30"
			"0D06035504070C064D6F73636F77311B"
			"3019060355040A0C124C4C4320425043"
			"2050726F63657373696E67311A301806"
			"0355040B0C1150726F63657373696E67"
			"2043656E746572312230200603550403"
			"0C1970312D746573742E62706370726F"
			"63657373696E672E636F6D3082022230"
			"0D06092A864886F70D01010105000382"
			"020F003082020A0282020100EEC40F83"
			"709052B24ABE6ABA8B348B3135AB044C");
	TEST_STRING_EQUAL(
			"0103444154" // MessageName=DAT
			"0B0100" // TcpIpDestination = 0
			"0D820200" // SimpleDataBlock=298
			"16030300410200003D03035EAACABEB2"
			"9917A49FF4094969B3C23DD02946FAA6"
			"0666DEC467F0C4FA1D6ACF00C02F0000"
			"1500000000FF01000100000B00040300"
			"0102002300001603030D440B000D4000"
			"0D3D00060530820601308203E9A00302"
			"010202021000300D06092A864886F70D"
			"01010B05003081A4310B300906035504"
			"0613025255310F300D06035504081306"
			"4D6F73636F77310F300D060355040713"
			"064D6F73636F77311B3019060355040A"
			"13124250432050726F63657373696E67"
			"204C4C43311A3018060355040B131150"
			"726F63657373696E672043656E747265"
			"311B3019060355040313125445535420"
			"504F5320434120534841323536311D30"
			"1B06092A864886F70D010901160E696E"
			"666F4062706362742E636F6D301E170D"
			"3139313130373133313835355A170D32"
			"39313130343133313835355A30818C31"
			"0B3009060355040613025255310F300D"
			"06035504080C064D6F73636F77310F30"
			"0D06035504070C064D6F73636F77311B"
			"3019060355040A0C124C4C4320425043"
			"2050726F63657373696E67311A301806"
			"0355040B0C1150726F63657373696E67"
			"2043656E746572312230200603550403"
			"0C1970312D746573742E62706370726F"
			"63657373696E672E636F6D3082022230"
			"0D06092A864886F70D01010105000382"
			"020F003082020A0282020100EEC40F83"
			"709052B24ABE6ABA8B348B3135AB044C"
			, packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<recv=2448><recv=2448>", result->getString());
	result->clear();

	// incoming data2
	tcpIp->addRecvData(
		"62D2711D7F0292CE545458C50F44390C"
		"6393BDB47FA3971C8B93EEE8B98F01CB"
		"87ADEFF96DCBE503C743AD76A0D35356"
		"E9D177404FF559D823CEB8115E094FA9"
		"A96C97044F4F5515E772BBB8BC4FD3A6"
		"712807861B0A6C07D6294085CAE35C1D"
		"9501423F028EF01A3E9132130DD8D898"
		"F5645B6A74FB3211680FB8AED0937833"
		"5964660634E72EB46D24A5E14F5562A5"
		"51E987F13DA2EA6FEE68F04B718FEDF8"
		"852771B866592362F2C5EA4D2D207C9B"
		"28EA4475F9BC177A08F018E4F6AB5D58"
		"EA2EA911BED6E9F528F551CAB3515D26"
		"074C6DD188560BF686933EC450B2D414"
		"2FEBA9D4875E90803395312FFBA8CAC9"
		"D5C1CF82C39D368AA46DE8D137678BA7"
		"C02E307EE34010BE7D57C11EA9D705C1"
		"30A368B3F6F804481939D6B219F5AE0F"
		"5BBD5635183C93102CC4958F4664AF16"
		"E32F1E6F1204995C27B4888A4AFA22B4"
		"08ADB52C6BFE243C1E2059B884923060"
		"3EFF045C2E7F3367512C3713958A818B"
		"72FC7FFE28B5A8F482986A106B70AA92"
		"26F4FDD1C42CF298E090977F45933204"
		"C6454666A4B6731BC536CF03C5BA0A69"
		"10E7C112A8A23D9DF143153839580D98"
		"BB07AE067C39E01176EE9036716948C7"
		"7444F98A184A5DB84875510FE79A984C"
		"3F4A2399C6315064D15227E0F754C19F"
		"5F2F80155F9C0993FDDD7A11C80A3C28"
		"7A02D11B49635247654334AD02030100"
		"01A353305130090603551D1304023000"
		);
	TEST_STRING_EQUAL(
		"0103444154" // MessageName=DAT
		"0B0100" // TcpIpDestination = 0
		"0D820200" // SimpleDataBlock=298
		"62D2711D7F0292CE545458C50F44390C"
		"6393BDB47FA3971C8B93EEE8B98F01CB"
		"87ADEFF96DCBE503C743AD76A0D35356"
		"E9D177404FF559D823CEB8115E094FA9"
		"A96C97044F4F5515E772BBB8BC4FD3A6"
		"712807861B0A6C07D6294085CAE35C1D"
		"9501423F028EF01A3E9132130DD8D898"
		"F5645B6A74FB3211680FB8AED0937833"
		"5964660634E72EB46D24A5E14F5562A5"
		"51E987F13DA2EA6FEE68F04B718FEDF8"
		"852771B866592362F2C5EA4D2D207C9B"
		"28EA4475F9BC177A08F018E4F6AB5D58"
		"EA2EA911BED6E9F528F551CAB3515D26"
		"074C6DD188560BF686933EC450B2D414"
		"2FEBA9D4875E90803395312FFBA8CAC9"
		"D5C1CF82C39D368AA46DE8D137678BA7"
		"C02E307EE34010BE7D57C11EA9D705C1"
		"30A368B3F6F804481939D6B219F5AE0F"
		"5BBD5635183C93102CC4958F4664AF16"
		"E32F1E6F1204995C27B4888A4AFA22B4"
		"08ADB52C6BFE243C1E2059B884923060"
		"3EFF045C2E7F3367512C3713958A818B"
		"72FC7FFE28B5A8F482986A106B70AA92"
		"26F4FDD1C42CF298E090977F45933204"
		"C6454666A4B6731BC536CF03C5BA0A69"
		"10E7C112A8A23D9DF143153839580D98"
		"BB07AE067C39E01176EE9036716948C7"
		"7444F98A184A5DB84875510FE79A984C"
		"3F4A2399C6315064D15227E0F754C19F"
		"5F2F80155F9C0993FDDD7A11C80A3C28"
		"7A02D11B49635247654334AD02030100"
		"01A353305130090603551D1304023000"
		, packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<recv=512>", result->getString());
	result->clear();

	// incoming data end
	tcpIp->addRecvData(
		"040303010302030302010202020300A9"
		"00A73081A4310B300906035504061302"
		"5255310F300D060355040813064D6F73"
		"636F77310F300D060355040713064D6F"
		"73636F77311B3019060355040A131242"
		"50432050726F63657373696E67204C4C"
		"43311A3018060355040B131150726F63"
		"657373696E672043656E747265311B30"
		"19060355040313125445535420504F53"
		"20434120534841323536311D301B0609"
		"2A864886F70D010901160E696E666F40"
		"62706362742E636F6D0E000000"
		);
	TEST_STRING_EQUAL(
		"0103444154" // MessageName=DAT
		"0B0100" // TcpIpDestination = 0
		"0D81BD" // SimpleDataBlock=298
		"040303010302030302010202020300A9"
		"00A73081A4310B300906035504061302"
		"5255310F300D060355040813064D6F73"
		"636F77310F300D060355040713064D6F"
		"73636F77311B3019060355040A131242"
		"50432050726F63657373696E67204C4C"
		"43311A3018060355040B131150726F63"
		"657373696E672043656E747265311B30"
		"19060355040313125445535420504F53"
		"20434120534841323536311D301B0609"
		"2A864886F70D010901160E696E666F40"
		"62706362742E636F6D0E000000"
		, packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<recv=512>", result->getString());
	result->clear();

	// DAT send
	packetLayer->recvPacket(
			"0103444154" // MessageName=DAT
			"0B0100" // TcpIpDestination = 0
			"0C03323938" // OutgoingByteCounter = 298
			"0D82012A160301012501000121030300" // SimpleDataBlock=298

			"0103444154" // MessageName=DAT
			"0B0100" // TcpIpDestination = 0
			"0C0431333232" // OutgoingByteCounter = 1322
			"0D820400"//SimpleDataBlock=1024
			"16030304D80B0004D40004D10004CE30"
			"8204CA308202B2A00302010202021004"
			"300D06092A864886F70D01010B050030"
			"81A4310B300906035504061302525531"
			"0F300D060355040813064D6F73636F77"
			"310F300D060355040713064D6F73636F"
			"77311B3019060355040A131242504320"
			"50726F63657373696E67204C4C43311A"
			"3018060355040B131150726F63657373"
			"696E672043656E747265311B30190603"
			"55040313125445535420504F53204341"
			"20534841323536311D301B06092A8648"
			"86F70D010901160E696E666F40627063"
			"62742E636F6D301E170D313931313134"
			"3136323034345A170D32393131313131"
			"36323034345A3056310B300906035504"
			"06130241553113301106035504080C0A"
			"536F6D652D53746174653111300F0603"
			"55040A0C08504F534954524F4E310C30"
			"0A060355040B0C03504F533111300F06"
			"035504030C0862706330303032383082"
			"0122300D06092A864886F70D01010105"
			"000382010F003082010A0282010100C0"
			"7324E5709810B447F7C42B16E649E253"
			"255439EA4F6F607E6E4C578B45D42DD2"
			"220D88AFE7BF16EAD095546000979C14"
			"CC0AB1B3533C06A1C832078CF93EA6A6"
			"57DEF19A40889AB941EEA51A6A092740"
			"6003BD45B104831C98C134DA7E865F43"
			"A81C908004D5925F406DCF33504751C2"
			"273680B392C248EE2E32A4FF32E694EE"
			"B3640ABDF6ACE065CF5FD4FCEDF03878"
			"A664914B6049DDBEB6768C793C87FADB"
			"D5EFF30F475AB44ADA7C8E6F6C05927F"
			"2AF9CA25EB60973DD328386F00D28DDD"
			"F623B8295F425A5C9720A7AF2607A9E6"
			"80AC77DEB8AA3DDF69D1FA7D23B158DC"
			"44735DECE031638E34129DEB2C4BE708"
			"6F702B373A896BACADC7026E12C85902"
			"03010001A353305130090603551D1304"
			"023000300B0603551D0F0404030205E0"
			"30370603551D250101FF042D302B0608"
			"2B0601050507030106082B0601050507"
			"030206096086480186F8420401060A2B"
			"0601040182370A0303300D06092A8648"
			"86F70D01010B050003820201003BA811"
			"AA75216A477B70F597FD6DB5838C2A12"
			"FD12137901CAED15340F3C21CF930694"
			"CCAB24B9270A764AADEF1D7DE6EFD370"
			"1CD9C7F92EDFBFFE884E9C0FBE79D860"
			"7F1417B20ACA8B51D59EF20C1BF9F868"
			"66C35CC160FCAA208E67B0E7B7B91554"
			"C94C96AA0CAEC74B6D339D1A30616707"
			"EC5107FEE4C04BF4EA69F8A285A0B45C"
			"A70528F75AA3F7EF25C90644A7B6AEB3"
			"306CECA872E5465499F18F6ACDD5B612"
			"EBA6F07AD03D084655750FBD88AD533F"
			"F5636F8A1089C093FE250EB616727087"
			"25A65ABEF9F5687C5B980022947E7269"
			"FDCDC88BA17AD0C0FDF2EA26CE711C20"
			"B36FAB1F6C63D4F82AB451A037F30509"
			"50715E92AB4BC7ADE297CD6F983E2A15"
			"513035EC6D3C92358D29A9F5C69C3063"
			"6FC53FE171279D16FE4449A15C972BF1"
			);
	TEST_STRING_EQUAL(
			"<send="
			"1603010125010001210303000000574D"
			"A9332C56DCB15B5871A024838F7D4D02"
			"D554BEF49F8C8072CFEF2B00009EC02C"
			"C030009FC0ADC09FC024C028006BC00A"
			"C0140039C0AFC0A3C02BC02F009EC0AC"
			"C09EC023C0270067C009C0130033C0AE"
			"C0A200ABC0A7C03800B3C0360091C0AB"
			"00AAC0A6C03700B2C0350090C0AA009D"
			"C09D003D0035C032C02AC00FC02EC026"
			"C005C0A1009CC09C003C002FC031C029"
			"C00EC02DC025C004C0A000AD00B70095"
			"00AC00B6009400A9C0A500AF008DC0A9"
			"00A8C0A400AE008CC0A800FF0100005A"
			"0000000E000C0000093132372E302E30"
			"2E31000D001600140603060105030501"
			"040304010303030102030201000A0018"
			"00160019001C0018001B00170016001A"
			"0015001400130012000B000201000016"
			"00000017000000230000"
			",len=298>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// data sended
	tcpIp->sendComplete();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("", result->getString());

	// DSC recv
	packetLayer->recvPacket("01034453430B0100");
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// DSC send
	tcpIp->remoteClose();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("01034453430B0100", packetLayer->getSendData());
	packetLayer->clearSendData();
	return true;
}
