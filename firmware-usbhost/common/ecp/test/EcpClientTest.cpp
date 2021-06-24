#include "ecp/EcpClientCommandLayer.h"
#include "ecp/EcpProtocol.h"
#include "dex/test/TestDataParser.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/TestEventObserver.h"
#include "utils/include/Hex.h"
#include "test/include/Test.h"

class TestEcpClientPacketLayer : public Ecp::ClientPacketLayerInterface {
public:
	TestEcpClientPacketLayer() : recvBuf(256) {}
	virtual ~TestEcpClientPacketLayer() {}
	void clearSendData() { sendBuf.clear(); }
	const char *getSendData() { return sendBuf.getString(); }
	void recvData(const char *hex) {
		uint16_t len = hexToData(hex, strlen(hex), recvBuf.getData(), recvBuf.getSize());
		recvBuf.setLen(len);
		observer->procRecvData(recvBuf.getData(), recvBuf.getLen());
	}
	void recvError(Ecp::Error error) {
		observer->procRecvError(error);
	}

	virtual void setObserver(Observer *observer) { this->observer = observer; }
	virtual bool connect() {
		sendBuf << "<connect>";
		return true;
	}
	virtual void disconnect() { sendBuf << "<disconnect>"; }
	virtual bool sendData(const Buffer *data) {
		for(uint16_t i = 0; i < data->getLen(); i++) {
			sendBuf.addHex((*data)[i]);
		}
		return true;
	}

private:
	Observer *observer;
	StringBuilder sendBuf;
	Buffer recvBuf;
};

class TestEcpClientObserver : public TestEventObserver {
public:
	TestEcpClientObserver(StringBuilder *result) : TestEventObserver(result) {}
	virtual void proc(Event *event) {
		switch(event->getType()) {
		case Ecp::Client::Event_ConnectOK: *result << "<event=ConnectOK>"; break;
		case Ecp::Client::Event_ConnectError: *result << "<event=ConnectError," << event->getUint16() << ">"; break;
		case Ecp::Client::Event_Disconnect: *result << "<event=Disconnect>"; break;
		case Ecp::Client::Event_UploadOK: *result << "<event=UploadOK>"; break;
		case Ecp::Client::Event_UploadError: *result << "<event=UploadError," << event->getUint16() << ">"; break;
		case Ecp::Client::Event_DownloadOK: *result << "<event=DownloadOK>"; break;
		case Ecp::Client::Event_DownloadError: *result << "<event=DownloadError," << event->getUint16() << ">"; break;
		case Ecp::Client::Event_ResponseOK: *result << "<event=ResponseOK>"; break;
		case Ecp::Client::Event_ResponseError: *result << "<event=ResponseError>"; break;
		default: *result << "<event=" << event->getType() << ">";
		}
	}
};

class EcpClientTest : public TestSet {
public:
	EcpClientTest();
	bool init();
	void cleanup();
	bool testDisconnect();
	bool testRemoteDisconnect();
	bool testUpload();
	bool testDownload();
	bool testRecvData();

private:
	StringBuilder *result;
	TimerEngine *timerEngine;
	TestEcpClientObserver *observer;
	TestEcpClientPacketLayer *packetLayer;
	Ecp::ClientCommandLayer *client;

	bool gotoStateReady();
	bool recvAnswer(uint8_t command, uint8_t resultCode);
	bool recvAnswer(const char *hexData);
};

TEST_SET_REGISTER(EcpClientTest);

EcpClientTest::EcpClientTest() {
	TEST_CASE_REGISTER(EcpClientTest, testDisconnect);
	TEST_CASE_REGISTER(EcpClientTest, testRemoteDisconnect);
	TEST_CASE_REGISTER(EcpClientTest, testUpload);
	TEST_CASE_REGISTER(EcpClientTest, testDownload);
	TEST_CASE_REGISTER(EcpClientTest, testRecvData);
}

bool EcpClientTest::init() {
	this->result = new StringBuilder(1024, 1024);
	this->timerEngine = new TimerEngine();
	this->observer = new TestEcpClientObserver(result);
	this->packetLayer = new TestEcpClientPacketLayer;
	this->client = new Ecp::ClientCommandLayer(this->timerEngine, this->packetLayer);
	this->client->setObserver(observer);
	return true;
}

void EcpClientTest::cleanup() {
	delete this->client;
	delete this->packetLayer;
	delete this->observer;
	delete this->timerEngine;
	delete this->result;
}

class EcpTestDataGenerator : public Dex::DataGenerator {
public:
	virtual void reset() { index = 0; }
	virtual void next() { index++; if(index > 1) { index = 1; } }
	virtual bool isLast() { return index >= 1; }
	virtual const void *getData() { return data[index]; }
	virtual uint16_t getLen() { return 10; }

private:
	int index;
	static const char data[2][10];
};

const char EcpTestDataGenerator::data[2][10] = {
	{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A },
	{ 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A },
};

bool EcpClientTest::gotoStateReady() {
	TEST_NUMBER_EQUAL(true, client->connect());

	// Connect
	TEST_STRING_EQUAL("<connect>", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// Connected
	client->procConnect();

	// Setup
	TEST_STRING_EQUAL("01", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// Recv ACK
	packetLayer->recvData("0100");
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=ConnectOK>", result->getString());
	result->clear();
	return true;
}

bool EcpClientTest::testDisconnect() {
	gotoStateReady();

	client->disconnect();
	TEST_STRING_EQUAL("<disconnect>", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	client->procDisconnect();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=Disconnect>", result->getString());
	return true;
}

bool EcpClientTest::testRemoteDisconnect() {
	gotoStateReady();

	client->procDisconnect();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=Disconnect>", result->getString());
	return true;
}

bool EcpClientTest::testUpload() {
	EcpTestDataGenerator generator;
	gotoStateReady();

	// Start upload
	TEST_NUMBER_EQUAL(true, client->uploadData(Ecp::Destination_FirmwareGsm, &generator));
	TEST_STRING_EQUAL("020100000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv ACK
	packetLayer->recvData("0200");

	// Send Data
	TEST_STRING_EQUAL("030102030405060708090A", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv ACK
	packetLayer->recvData("0300");

	// Send Data
	TEST_STRING_EQUAL("031112131415161718191A", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv BUSY
	packetLayer->recvData("0301");

	// Resend Data
	timerEngine->tick(ECP_BUSY_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("031112131415161718191A", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv ACK
	packetLayer->recvData("0300");

	// Send End
	TEST_STRING_EQUAL("04", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv BUSY
	packetLayer->recvData("0401");
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("", result->getString());

	// Resend End
	timerEngine->tick(ECP_BUSY_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("04", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv BUSY
	packetLayer->recvData("0401");
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("", result->getString());

	// Resend End
	timerEngine->tick(ECP_BUSY_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("04", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv ACK
	packetLayer->recvData("0400");
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=UploadOK>", result->getString());
	return true;
}

bool EcpClientTest::testDownload() {
	TestDataParser parser;
	gotoStateReady();

	// Start Download
	TEST_NUMBER_EQUAL(true, client->downloadData(Ecp::Source_Audit, &parser));
	TEST_STRING_EQUAL("0601", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv ACK
	packetLayer->recvData("060000000000");
	TEST_STRING_EQUAL("<start=0>", parser.getData());
	parser.clearData();

	// Send Request
	TEST_STRING_EQUAL("07", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv Data
	packetLayer->recvData("07000102030405060708090A");
	TEST_STRING_EQUAL("0102030405060708090A", parser.getData());
	parser.clearData();

	// Send Request
	TEST_STRING_EQUAL("07", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv Last Data
	packetLayer->recvData("070B1112131415161718191A");
	TEST_STRING_EQUAL("1112131415161718191A<complete>", parser.getData());
	parser.clearData();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
}

bool EcpClientTest::testRecvData() {
	gotoStateReady();

	// Send TableInfoRequest
	TEST_NUMBER_EQUAL(true, client->getTableInfo(Ecp::Table_Event));
	TEST_STRING_EQUAL("090100", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv response
	packetLayer->recvData("090005000000");
	TEST_STRING_EQUAL("<event=ResponseOK>", result->getString());
	result->clear();

	// Send TableEntryRequest
	TEST_NUMBER_EQUAL(true, client->getTableEntry(Ecp::Table_Event, 0));
	TEST_STRING_EQUAL("0A010000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv response
	packetLayer->recvData("0A0005000000");
	TEST_STRING_EQUAL("<event=ResponseOK>", result->getString());
	result->clear();

	// Send TableEntryRequest
	TEST_NUMBER_EQUAL(true, client->getTableEntry(Ecp::Table_Event, 255));
	TEST_STRING_EQUAL("0A0100FF000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv response
	packetLayer->recvData("0A0A");
	TEST_STRING_EQUAL("<event=ResponseError>", result->getString());
	result->clear();

	// Send DateTimeRequest
	TEST_NUMBER_EQUAL(true, client->getDateTime());
	TEST_STRING_EQUAL("0B", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv response
	packetLayer->recvData("0B00010203040506");
	TEST_STRING_EQUAL("<event=ResponseOK>", result->getString());
	return true;
}
