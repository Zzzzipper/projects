#include "fiscal_register/atol/AtolTaskLayer.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Hex.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class TestAtolPacketLayer : public Atol::PacketLayerInterface {
public:
	TestAtolPacketLayer(StringBuilder *result) : result(result), observer(NULL) {}
	virtual void setObserver(Atol::PacketLayerObserver *observer) { this->observer = observer; }
	virtual bool connect(const char *domainname, uint16_t port, TcpIp::Mode mode) {
		*result << "<PL::connect=" << domainname << "," << port << "," << mode << ">";
		return true;
	}
	virtual bool sendPacket(const uint8_t *data, const uint16_t dataLen)  {
		*result << "<PL::sendPacket=" << dataLen << ",data=";
		for(uint16_t i = 0; i < dataLen; i++) {
			result->addHex(data[i]);
		}
		*result << ">";
		return true;
	}
	virtual bool disconnect()  {
		*result << "<PL::disconnect>";
		return true;
	}
	bool addRecvData(uint8_t packetId, const char *hex) {
		Buffer buf(ATOL_PACKET_MAX_SIZE);
		if(hexToData(hex, &buf) == 0) {
			LOG_ERROR(LOG_TEST, "ERROR");
			return false;
		}
		observer->procRecvData(packetId, buf.getData(), buf.getLen());
		return true;
	}

private:
	StringBuilder *result;
	Atol::PacketLayerObserver *observer;
};

class TestAtolTaskLayerObserver : public Atol::TaskLayerObserver {
public:
	TestAtolTaskLayerObserver(StringBuilder *str) : str(str) {}
	virtual void procRecvData(const uint8_t *data, const uint16_t dataLen) {
		*str << "<event=RecvData,data=";
		for(uint16_t i = 0; i < dataLen; i++) {
			str->addHex(data[i]);
		}
		*str << ">";
	}
	virtual void procError(Atol::TaskLayerObserver::Error error) {
		switch(error) {
		case TaskLayerObserver::Error_OK: *str << "<event=OK>"; return;
		case TaskLayerObserver::Error_ConnectFailed: *str << "<event=ConnectFailed>"; return;
		case TaskLayerObserver::Error_RemoteClose: *str << "<event=RemoteClose>"; return;
//		case TaskLayerObserver::Error_SendFailed: *str << "<event=SendFailed>"; return;
//		case TaskLayerObserver::Error_RecvFailed: *str << "<event=RecvFailed>"; return;
//		case TaskLayerObserver::Error_RecvTimeout: *str << "<event=RecvTimeout>"; return;
		default: *str << "<event=" << error << ">";
		}
	}

private:
	StringBuilder *str;
};

class AtolTaskLayerTest : public TestSet {
public:
	AtolTaskLayerTest();
	bool init();
	void cleanup();
	bool testTaskSync();
	bool testTaskAsync();
	bool testDisconnect();

private:
	StringBuilder *result;
	TimerEngine *timerEngine;
	TestAtolPacketLayer *packetLayer;
	TestAtolTaskLayerObserver *observer;
	Atol::TaskLayer *layer;
};

TEST_SET_REGISTER(AtolTaskLayerTest);

AtolTaskLayerTest::AtolTaskLayerTest() {
	TEST_CASE_REGISTER(AtolTaskLayerTest, testTaskSync);
	TEST_CASE_REGISTER(AtolTaskLayerTest, testTaskAsync);
	TEST_CASE_REGISTER(AtolTaskLayerTest, testDisconnect);
}

bool AtolTaskLayerTest::init() {
	this->result = new StringBuilder(2048, 2048);
	this->timerEngine = new TimerEngine();
	this->packetLayer = new TestAtolPacketLayer(result);
	this->observer = new TestAtolTaskLayerObserver(result);
	this->layer = new Atol::TaskLayer(this->timerEngine, this->packetLayer);
	this->layer->setObserver(observer);
	return true;
}

void AtolTaskLayerTest::cleanup() {
	delete this->layer;
	delete this->observer;
	delete this->packetLayer;
	delete this->timerEngine;
	delete this->result;
}

bool AtolTaskLayerTest::testTaskSync() {
	// connect
	layer->connect("test", 1234, TcpIp::Mode_TcpIp);
	TEST_STRING_EQUAL("<PL::connect=test,1234,0>", result->getString());
	result->clear();

	// connect complete
	layer->procError(Atol::PacketLayerObserver::Error_OK);
	TEST_STRING_EQUAL("", result->getString());

	// delay
	timerEngine->tick(500);
	timerEngine->execute();
	TEST_STRING_EQUAL("<PL::sendPacket=1,data=C4>", result->getString());
	result->clear();

	// abort complete
	packetLayer->addRecvData(0, "A3");
	TEST_STRING_EQUAL("<event=OK>", result->getString());
	result->clear();

	// add task
	Buffer data1(32);
	data1.addUint8(0x01);
	data1.addUint8(0x02);
	data1.addUint8(0x03);
	data1.addUint8(0x04);
	layer->sendRequest(data1.getData(), data1.getLen());
	TEST_STRING_EQUAL("<PL::sendPacket=7,data=C1030201020304>", result->getString());
	result->clear();

	// sync response
	packetLayer->addRecvData(0, "A3A1A2A3");
	TEST_STRING_EQUAL("<event=RecvData,data=A1A2A3>", result->getString());
	result->clear();

	// disconnect
	layer->disconnect();
	TEST_STRING_EQUAL("<PL::disconnect>", result->getString());
	result->clear();

	// connect complete
	layer->procError(Atol::PacketLayerObserver::Error_OK);
	TEST_STRING_EQUAL("<event=OK>", result->getString());
	result->clear();
	return true;
}

bool AtolTaskLayerTest::testTaskAsync() {
	// connect
	layer->connect("test", 1234, TcpIp::Mode_TcpIp);
	TEST_STRING_EQUAL("<PL::connect=test,1234,0>", result->getString());
	result->clear();

	// connect complete
	layer->procError(Atol::PacketLayerObserver::Error_OK);
	TEST_STRING_EQUAL("", result->getString());

	// delay
	timerEngine->tick(500);
	timerEngine->execute();
	TEST_STRING_EQUAL("<PL::sendPacket=1,data=C4>", result->getString());
	result->clear();

	// abort complete
	packetLayer->addRecvData(0, "A3");
	TEST_STRING_EQUAL("<event=OK>", result->getString());
	result->clear();

	// add task
	Buffer data1(32);
	data1.addUint8(0x01);
	data1.addUint8(0x02);
	data1.addUint8(0x03);
	data1.addUint8(0x04);
	layer->sendRequest(data1.getData(), data1.getLen());
	TEST_STRING_EQUAL("<PL::sendPacket=7,data=C1030201020304>", result->getString());
	result->clear();

	// sync response
	packetLayer->addRecvData(0, "A2");
	TEST_STRING_EQUAL("", result->getString());

	// async response
	packetLayer->addRecvData(0, "A602B1B2B3");
	TEST_STRING_EQUAL("<event=RecvData,data=B1B2B3>", result->getString());
	result->clear();

	// disconnect
	layer->disconnect();
	TEST_STRING_EQUAL("<PL::disconnect>", result->getString());
	result->clear();

	// connect complete
	layer->procError(Atol::PacketLayerObserver::Error_OK);
	TEST_STRING_EQUAL("<event=OK>", result->getString());
	result->clear();
	return true;
}

bool AtolTaskLayerTest::testDisconnect() {
	// connect
	layer->connect("test", 1234, TcpIp::Mode_TcpIp);
	TEST_STRING_EQUAL("<PL::connect=test,1234,0>", result->getString());
	result->clear();

	// connect complete
	layer->procError(Atol::PacketLayerObserver::Error_OK);
	TEST_STRING_EQUAL("", result->getString());

	// delay
	timerEngine->tick(500);
	timerEngine->execute();
	TEST_STRING_EQUAL("<PL::sendPacket=1,data=C4>", result->getString());
	result->clear();

	// abort complete
	packetLayer->addRecvData(0, "A3");
	TEST_STRING_EQUAL("<event=OK>", result->getString());
	result->clear();

	// add task
	Buffer data1(32);
	data1.addUint8(0x01);
	data1.addUint8(0x02);
	data1.addUint8(0x03);
	data1.addUint8(0x04);
	layer->sendRequest(data1.getData(), data1.getLen());
	TEST_STRING_EQUAL("<PL::sendPacket=7,data=C1030201020304>", result->getString());
	result->clear();

	// sync response
	packetLayer->addRecvData(0, "A2");
	TEST_STRING_EQUAL("", result->getString());

	// async response
	packetLayer->addRecvData(0, "A602B1B2B3");
	TEST_STRING_EQUAL("<event=RecvData,data=B1B2B3>", result->getString());
	result->clear();

	// disconnect
	layer->disconnect();
	TEST_STRING_EQUAL("<PL::disconnect>", result->getString());
	result->clear();

	// connect complete
	layer->procError(Atol::PacketLayerObserver::Error_OK);
	TEST_STRING_EQUAL("<event=OK>", result->getString());
	result->clear();
	return true;
}
