#if 1
#include "ecp/EcpServerCommandLayer.h"
#include "ecp/EcpProtocol.h"
#include "timer/include/TimerEngine.h"
#include "timer/include/RealTime.h"
#include "utils/include/Hex.h"
#include "utils/include/TestEventObserver.h"
#include "uart/include/TestUart.h"
#include "dex/test/TestDataParser.h"
#include "dex/test/TestDataGenerator.h"
#include "test/include/Test.h"

class TestTableProcessor : public Ecp::TableProcessor {
public:
	TestTableProcessor(uint16_t tableId, uint32_t tableSize) : tableId(tableId), tableSize(tableSize) {}
	void setTableExist(uint16_t tableId) { this->tableId = tableId; }
	void setTableSize(uint32_t tableSize) { this->tableSize = tableSize; }

	virtual bool isTableExist(uint16_t tableId) {
		return (this->tableId == tableId);
	}

	virtual uint32_t getTableSize(uint16_t tableId) {
		if(this->tableId != tableId) {
			LOG_ERROR(LOG_ECP, "Wrong table id " << this->tableId << "<>" << tableId);
			return 0;
		}
		return tableSize;
	}

	virtual uint16_t getTableEntry(uint16_t tableId, uint32_t entryIndex, uint8_t *buf, uint16_t bufSize) {
		if(this->tableId != tableId) {
			LOG_ERROR(LOG_ECP, "Wrong table id " << this->tableId << "<>" << tableId);
			return 0;
		}
		uint8_t data[] = { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7 };
		if(sizeof(data) > bufSize) {
			LOG_ERROR(LOG_ECP, "Buffer too small " << (int) sizeof(data) << "<>" << bufSize);
			return 0;
		}
		for(uint16_t i = 0; i < sizeof(data); i++) {
			buf[i] = data[i];
		}
		return sizeof(data);
	}

	virtual uint16_t getDateTime(uint8_t *buf, uint16_t bufSize) {
		DateTime *datetime = (DateTime*)buf;
		if(sizeof(*datetime) > bufSize) {
			return 0;
		}
		datetime->year = 1;
		datetime->month = 2;
		datetime->day = 3;
		datetime->hour = 10;
		datetime->minute = 20;
		datetime->second = 30;
		return sizeof(*datetime);
	}

private:
	uint16_t tableId;
	uint32_t tableSize;
};

class TestEcpServerPacketLayer : public Ecp::ServerPacketLayerInterface {
public:
	TestEcpServerPacketLayer() : recvBuf(256) {}
	virtual ~TestEcpServerPacketLayer() {}
	void clearSendData() { sendBuf.clear(); }
	const char *getSendData() { return sendBuf.getString(); }
	void recvData(const char *hex) {
		uint16_t len = hexToData(hex, strlen(hex), recvBuf.getData(), recvBuf.getSize());
		recvBuf.setLen(len);
		observer->procRecvData(recvBuf.getData(), recvBuf.getLen());
	}
	void recvError(Ecp::Error error) {
		observer->procError(error);
	}

	virtual void setObserver(Observer *observer) { this->observer = observer; }
	virtual void reset() { sendBuf << "<reset>"; }
	virtual void shutdown() { sendBuf << "<shutdown>"; }
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

class TestEcpServerObserver : public TestEventObserver {
public:
	TestEcpServerObserver(StringBuilder *result) : TestEventObserver(result) {}
	virtual void proc(Event *event) {
		switch(event->getType()) {
		case Ecp::Server::Event_Connect: *result << "<event=Connect>"; break;
		case Ecp::Server::Event_Disconnect: *result << "<event=Disconnect>"; break;
		default: *result << "<event=" << event->getType() << ">";
		}
	}
};

class EcpServerTest : public TestSet {
public:
	EcpServerTest();
	bool init();
	void cleanup();
	bool testDisconnectRemote();
	bool testDisconnect();
	bool testUploadAsync();
	bool testUploadSync();
	bool testUploadCompleteAsync();
	bool testDownload();
	bool testTable();

private:
	StringBuilder *result;
	TimerEngine *timerEngine;
	TestEcpServerObserver *observer;
	TestDataParser *parser;
	TestDataGenerator *generator;
	TestEcpServerPacketLayer *packetLayer;
	Ecp::ServerCommandLayer *server;

	bool gotoStateReady();
	bool recvCommand(const char *data);
};

TEST_SET_REGISTER(EcpServerTest);

EcpServerTest::EcpServerTest() {
	TEST_CASE_REGISTER(EcpServerTest, testDisconnectRemote);
	TEST_CASE_REGISTER(EcpServerTest, testDisconnect);
	TEST_CASE_REGISTER(EcpServerTest, testUploadAsync);
	TEST_CASE_REGISTER(EcpServerTest, testUploadSync);
	TEST_CASE_REGISTER(EcpServerTest, testUploadCompleteAsync);
	TEST_CASE_REGISTER(EcpServerTest, testDownload);
	TEST_CASE_REGISTER(EcpServerTest, testTable);
}

bool EcpServerTest::init() {
	this->result = new StringBuilder(1024, 1024);
	this->timerEngine = new TimerEngine();
	this->observer = new TestEcpServerObserver(result);
	this->parser = new TestDataParser;
	this->generator = new TestDataGenerator;
	this->packetLayer = new TestEcpServerPacketLayer;
	this->server = new Ecp::ServerCommandLayer(this->timerEngine, this->packetLayer);
	this->server->setObserver(observer);
	this->server->setGsmParser(parser);
	this->server->setConfigGenerator(generator);
	return true;
}

void EcpServerTest::cleanup() {
	delete this->server;
	delete this->packetLayer;
	delete this->generator;
	delete this->parser;
	delete this->observer;
	delete this->timerEngine;
	delete this->result;
}

bool EcpServerTest::testDisconnectRemote() {
	// Reset
	server->reset();
	TEST_STRING_EQUAL("<reset>", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Remote connect
	server->procConnect();
	TEST_STRING_EQUAL("<event=Connect>", result->getString());
	result->clear();

	// Recv Setup
	packetLayer->recvData("01");

	// Send ACK
	TEST_STRING_EQUAL("0100", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Remote disconnect
	server->procDisconnect();
	TEST_STRING_EQUAL("<event=Disconnect>", result->getString());
	result->clear();
	return true;
}

bool EcpServerTest::testDisconnect() {
	// Reset
	server->reset();
	TEST_STRING_EQUAL("<reset>", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Remote connect
	server->procConnect();
	TEST_STRING_EQUAL("<event=Connect>", result->getString());
	result->clear();

	// Recv Setup
	packetLayer->recvData("01");

	// Send ACK
	TEST_STRING_EQUAL("0100", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Local disconnect
	server->disconnect();
	TEST_STRING_EQUAL("<disconnect>", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Disconnect confirm
	server->procDisconnect();
	TEST_STRING_EQUAL("<event=Disconnect>", result->getString());
	result->clear();
	return true;
}

bool EcpServerTest::gotoStateReady() {
	server->reset();
	TEST_STRING_EQUAL("<reset>", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Connected
	server->procConnect();
	TEST_STRING_EQUAL("<event=Connect>", result->getString());
	result->clear();

	// Recv Setup
	packetLayer->recvData("01");

	// Send ACK
	TEST_STRING_EQUAL("0100", packetLayer->getSendData());
	packetLayer->clearSendData();
	return true;
}

bool EcpServerTest::testUploadAsync() {
	TEST_NUMBER_EQUAL(true, gotoStateReady());

	// Recv UploadStart
	parser->setStartResult(Dex::DataParser::Result_Ok);
	packetLayer->recvData("020100000000");
	TEST_STRING_EQUAL("<start=0>", parser->getData());
	parser->clearData();

	// Send ACK
	TEST_STRING_EQUAL("0200", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv UploadData
	parser->setProcDataResult(Dex::DataParser::Result_Ok);
	packetLayer->recvData("030102030405060708090A");
	TEST_STRING_EQUAL("0102030405060708090A", parser->getData());
	parser->clearData();

	// Send ACK
	TEST_STRING_EQUAL("0300", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv UploadData
	parser->setProcDataResult(Dex::DataParser::Result_Async);
	packetLayer->recvData("031112131415161718191A");
	TEST_STRING_EQUAL("1112131415161718191A", parser->getData());
	parser->clearData();

	// Wait async result
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// Recv Confirm
	Event event1(Dex::DataParser::Event_AsyncOk);
	server->proc(&event1);
	TEST_STRING_EQUAL("0300", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv UploadData
	parser->setProcDataResult(Dex::DataParser::Result_Busy);
	packetLayer->recvData("033132333435363738393A");
	TEST_STRING_EQUAL("3132333435363738393A", parser->getData());
	parser->clearData();

	// Send ACK
	TEST_STRING_EQUAL("0301", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv UploadEnd
	parser->setCompleteResult(Dex::DataParser::Result_Async);
	packetLayer->recvData("04");
	TEST_STRING_EQUAL("<complete>", parser->getData());
	parser->clearData();

	// Wait async result
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// Recv Confirm
	Event event2(Dex::DataParser::Event_AsyncOk);
	server->proc(&event2);
	TEST_STRING_EQUAL("0400", packetLayer->getSendData());
	packetLayer->clearSendData();
	return true;
}

bool EcpServerTest::testUploadSync() {
	TEST_NUMBER_EQUAL(true, gotoStateReady());

	// Recv UploadStart
	parser->setStartResult(Dex::DataParser::Result_Ok);
	packetLayer->recvData("020100000000");
	TEST_STRING_EQUAL("<start=0>", parser->getData());
	parser->clearData();

	// Send ACK
	TEST_STRING_EQUAL("0200", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv UploadData
	parser->setProcDataResult(Dex::DataParser::Result_Ok);
	packetLayer->recvData("030102030405060708090A");
	TEST_STRING_EQUAL("0102030405060708090A", parser->getData());
	parser->clearData();

	// Send ACK
	TEST_STRING_EQUAL("0300", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv UploadData
	parser->setProcDataResult(Dex::DataParser::Result_Async);
	packetLayer->recvData("031112131415161718191A");
	TEST_STRING_EQUAL("1112131415161718191A", parser->getData());
	parser->clearData();

	// Wait async result
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// Send Confirm
	Event event1(Dex::DataParser::Event_AsyncOk);
	server->proc(&event1);
	TEST_STRING_EQUAL("0300", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv UploadData
	parser->setProcDataResult(Dex::DataParser::Result_Busy);
	packetLayer->recvData("033132333435363738393A");
	TEST_STRING_EQUAL("3132333435363738393A", parser->getData());
	parser->clearData();

	// Send ACK
	TEST_STRING_EQUAL("0301", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv UploadEnd
	parser->setCompleteResult(Dex::DataParser::Result_Ok);
	packetLayer->recvData("04");
	TEST_STRING_EQUAL("<complete>", parser->getData());
	parser->clearData();

	// Send Confirm
	TEST_STRING_EQUAL("0400", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Disconnect
	server->procDisconnect();
	TEST_STRING_EQUAL("<event=Disconnect>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", parser->getData());
	parser->clearData();
	return true;
}

bool EcpServerTest::testUploadCompleteAsync() {
	TEST_NUMBER_EQUAL(true, gotoStateReady());

	// Recv UploadStart
	parser->setStartResult(Dex::DataParser::Result_Ok);
	packetLayer->recvData("020100000000");
	TEST_STRING_EQUAL("<start=0>", parser->getData());
	parser->clearData();

	// Send ACK
	TEST_STRING_EQUAL("0200", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv UploadData
	parser->setProcDataResult(Dex::DataParser::Result_Ok);
	packetLayer->recvData("030102030405060708090A");
	TEST_STRING_EQUAL("0102030405060708090A", parser->getData());
	parser->clearData();

	// Send ACK
	TEST_STRING_EQUAL("0300", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv UploadData
	parser->setProcDataResult(Dex::DataParser::Result_Async);
	packetLayer->recvData("031112131415161718191A");
	TEST_STRING_EQUAL("1112131415161718191A", parser->getData());
	parser->clearData();

	// Wait async result
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// Send Confirm
	Event event1(Dex::DataParser::Event_AsyncOk);
	server->proc(&event1);
	TEST_STRING_EQUAL("0300", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv UploadData
	parser->setProcDataResult(Dex::DataParser::Result_Busy);
	packetLayer->recvData("033132333435363738393A");
	TEST_STRING_EQUAL("3132333435363738393A", parser->getData());
	parser->clearData();

	// Send ACK
	TEST_STRING_EQUAL("0301", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv UploadEnd
	parser->setCompleteResult(Dex::DataParser::Result_Busy);
	packetLayer->recvData("04");
	TEST_STRING_EQUAL("<complete>", parser->getData());
	parser->clearData();

	// Send Confirm
	TEST_STRING_EQUAL("0401", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv UploadEnd
	parser->setCompleteResult(Dex::DataParser::Result_Busy);
	packetLayer->recvData("04");
	TEST_STRING_EQUAL("<complete>", parser->getData());
	parser->clearData();

	// Send Confirm
	TEST_STRING_EQUAL("0401", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv UploadEnd
	parser->setCompleteResult(Dex::DataParser::Result_Ok);
	packetLayer->recvData("04");
	TEST_STRING_EQUAL("<complete>", parser->getData());
	parser->clearData();

	// Send Confirm
	TEST_STRING_EQUAL("0400", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Disconnect
	server->procDisconnect();
	TEST_STRING_EQUAL("<event=Disconnect>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", parser->getData());
	parser->clearData();
	return true;
}

bool EcpServerTest::testDownload() {
	TEST_NUMBER_EQUAL(true, gotoStateReady());

	// Recv DownloadStart
	packetLayer->recvData("0601");

	// Send Response
	TEST_STRING_EQUAL("0607", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv DownloadStart
	packetLayer->recvData("0602");

	// Send Response
	TEST_STRING_EQUAL("060000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv DownloadData
	packetLayer->recvData("07");

	// Send Data
	TEST_STRING_EQUAL("07003131313131", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv DownloadData
	packetLayer->recvData("07");

	// Send Data
	TEST_STRING_EQUAL("070032323232323333333333", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv DownloadData
	packetLayer->recvData("07");

	// Send Last Data
	TEST_STRING_EQUAL("070B343434343435353535353636363636", packetLayer->getSendData());
	packetLayer->clearSendData();
	return true;
}

bool EcpServerTest::testTable() {
	TestTableProcessor processor(Ecp::Table_Event, 10);
	TEST_NUMBER_EQUAL(true, gotoStateReady());

	// Recv TableInfoRequest
	packetLayer->recvData("090100");
	parser->clearData();

	// Send ACK(Error_TableNotFound)
	TEST_STRING_EQUAL("0909", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv TableInfoRequest
	server->setTableProcessor(&processor);
	packetLayer->recvData("090100");
	parser->clearData();

	// Send TableInfoResponse
	TEST_STRING_EQUAL("09000A000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv TableEntryRequest
	packetLayer->recvData("0A010000000000");
	parser->clearData();

	// Send TableEntryRequest
	TEST_STRING_EQUAL("0A00A0A1A2A3A4A5A6A7", packetLayer->getSendData());
	packetLayer->clearSendData();

	// Recv DateTimeRequest
	packetLayer->recvData("0B");
	parser->clearData();

	// Send DateTimeRequest
	TEST_STRING_EQUAL("0B000102030A141E", packetLayer->getSendData());
	packetLayer->clearSendData();
	return true;
}
#else
#include "ecp/include/EcpServer.h"
#include "ecp/EcpProtocol.h"
#include "timer/include/Engine.h"
#include "timer/include/RealTime.h"
#include "utils/include/Hex.h"
#include "utils/include/TestEvent.h"
#include "uart/include/TestUart.h"
#include "dex/test/TestDataParser.h"
#include "dex/test/TestDataGenerator.h"
#include "test/include/Test.h"

class TestTableProcessor : public Ecp::Server::TableProcessor {
public:
	TestTableProcessor(uint16_t tableId, uint32_t tableSize) : tableId(tableId), tableSize(tableSize) {}
	void setTableExist(uint16_t tableId) { this->tableId = tableId; }
	void setTableSize(uint32_t tableSize) { this->tableSize = tableSize; }

	virtual bool isTableExist(uint16_t tableId) {
		return (this->tableId == tableId);
	}

	virtual uint32_t getTableSize(uint16_t tableId) {
		if(this->tableId != tableId) {
			LOG_ERROR(LOG_ECP, "Wrong table id " << this->tableId << "<>" << tableId);
			return 0;
		}
		return tableSize;
	}

	virtual uint16_t getTableEntry(uint16_t tableId, uint32_t entryIndex, uint8_t *buf, uint16_t bufSize) {
		if(this->tableId != tableId) {
			LOG_ERROR(LOG_ECP, "Wrong table id " << this->tableId << "<>" << tableId);
			return 0;
		}
		uint8_t data[] = { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7 };
		if(sizeof(data) > bufSize) {
			LOG_ERROR(LOG_ECP, "Buffer too small " << sizeof(data) << "<>" << bufSize);
			return 0;
		}
		for(uint16_t i = 0; i < sizeof(data); i++) {
			buf[i] = data[i];
		}
		return sizeof(data);
	}

	virtual uint16_t getDateTime(uint8_t *buf, uint16_t bufSize) {
		DateTime *datetime = (DateTime*)buf;
		if(sizeof(*datetime) > bufSize) {
			return 0;
		}
		datetime->year = 1;
		datetime->month = 2;
		datetime->day = 3;
		datetime->hour = 10;
		datetime->minute = 20;
		datetime->second = 30;
		return sizeof(*datetime);
	}

private:
	uint16_t tableId;
	uint32_t tableSize;
};

class EcpServerTest : public TestSet {
public:
	EcpServerTest();
	void init();
	void cleanup();
	bool testKeepAlive();
	bool testDisconnectKeepAlive();
	bool testCutPacket();
	bool testWrongCrc();
	bool testUploadAsync();
	bool testUploadSync();
	bool testDownload();
	bool testTable();

private:
	TestUart *uart;
	TimerEngine *timerEngine;
	TestEventObserver *observer;
	TestDataParser *parser;
	TestDataGenerator *generator;
	Ecp::Crc *crc;
	Ecp::Server *server;

	bool gotoStateReady();
	bool recvCommand(const char *data);
};

TEST_SET_REGISTER(EcpServerTest);

EcpServerTest::EcpServerTest() {
	TEST_CASE_REGISTER(EcpServerTest, testKeepAlive);
	TEST_CASE_REGISTER(EcpServerTest, testDisconnectKeepAlive);
	TEST_CASE_REGISTER(EcpServerTest, testCutPacket);
	TEST_CASE_REGISTER(EcpServerTest, testWrongCrc);
	TEST_CASE_REGISTER(EcpServerTest, testUploadAsync);
	TEST_CASE_REGISTER(EcpServerTest, testUploadSync);
	TEST_CASE_REGISTER(EcpServerTest, testDownload);
	TEST_CASE_REGISTER(EcpServerTest, testTable);
}

void EcpServerTest::init() {
	this->uart = new TestUart(256);
	this->timerEngine = new TimerEngine();
	this->observer = new TestEventObserver;
	this->crc = new Ecp::Crc;
	this->parser = new TestDataParser;
	this->generator = new TestDataGenerator;
	this->server = new Ecp::Server(this->uart, this->timerEngine);
	this->server->setObserver(observer);
	this->server->setGsmParser(parser);
	this->server->setConfigGenerator(generator);
}

void EcpServerTest::cleanup() {
	delete this->server;
	delete this->generator;
	delete this->parser;
	delete this->crc;
	delete this->observer;
	delete this->timerEngine;
	delete this->uart;
}

bool EcpServerTest::recvCommand(const char *hex) {
	uint8_t data[256];
	uint16_t dataLen = hexToData(hex, strlen(hex), data, sizeof(data));
	uart->addRecvData(Ecp::Control_STX);
	uart->addRecvData(dataLen);
	crc->start(dataLen);
	for(uint16_t i = 0; i < dataLen; i++) {
		uart->addRecvData(data[i]);
		crc->add(data[i]);
	}
	uart->addRecvData(crc->getCrc());
	timerEngine->tick(10);
	timerEngine->execute();
	return true;
}

bool EcpServerTest::testKeepAlive() {
	server->reset();

	// Recv ENQ
	uart->addRecvData(0x05);

	// Send ACK
	TEST_HEXDATA_EQUAL("06", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Ecp::Server::Event_Connect, observer->getType());
	uart->clearSendBuffer();
	result->clear();

	// Recv Setup
	TEST_NUMBER_EQUAL(true, recvCommand("01"));

	// Send ACK
	TEST_HEXDATA_EQUAL("0602010003", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(0, observer->getType());
	uart->clearSendBuffer();

	// Recv ENQ
	timerEngine->tick(ECP_KEEP_ALIVE_PERIOD);
	timerEngine->execute();
	uart->addRecvData(0x05);

	// Send ACK
	TEST_HEXDATA_EQUAL("06", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(0, observer->getType());
	uart->clearSendBuffer();

	// Recv ENQ
	timerEngine->tick(ECP_KEEP_ALIVE_PERIOD);
	timerEngine->execute();
	uart->addRecvData(0x05);

	// Send ACK
	TEST_HEXDATA_EQUAL("06", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(0, observer->getType());
	uart->clearSendBuffer();

	// No keep-alive
	timerEngine->tick(ECP_KEEP_ALIVE_TIMEOUT*2);
	timerEngine->execute();
	TEST_NUMBER_EQUAL(Ecp::Server::Event_Disconnect, observer->getType());
	result->clear();

	return true;
}

bool EcpServerTest::testDisconnectKeepAlive() {
	server->reset();

	// Recv ENQ
	uart->addRecvData(0x05);

	// Send ACK
	TEST_HEXDATA_EQUAL("06", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Ecp::Server::Event_Connect, observer->getType());
	uart->clearSendBuffer();
	result->clear();

	// Recv Setup
	TEST_NUMBER_EQUAL(true, recvCommand("01"));

	// Send ACK
	TEST_HEXDATA_EQUAL("0602010003", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(0, observer->getType());
	uart->clearSendBuffer();

	// Recv ENQ
	timerEngine->tick(ECP_KEEP_ALIVE_PERIOD);
	timerEngine->execute();
	server->disconnect();
	uart->addRecvData(0x05);

	// Send ACK
	TEST_HEXDATA_EQUAL("04", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(0, observer->getType());
	uart->clearSendBuffer();

	// Recv EOT
	uart->addRecvData(0x04);
	TEST_NUMBER_EQUAL(Ecp::Server::Event_Disconnect, observer->getType());
	result->clear();
	return true;
}

bool EcpServerTest::testCutPacket() {
	server->reset();

	// Recv ENQ
	uart->addRecvData(0x05);

	// Send ACK
	TEST_HEXDATA_EQUAL("06", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv Setup
	uart->addRecvData(Ecp::Control_STX);
	uart->addRecvData(2);
	uart->addRecvData(31);

	// Send nothing
	timerEngine->tick(ECP_PACKET_TIMEOUT);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv Setup
	TEST_NUMBER_EQUAL(true, recvCommand("01"));

	// Send ACK
	TEST_HEXDATA_EQUAL("0602010003", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	return true;
}

bool EcpServerTest::testWrongCrc() {
	server->reset();

	// Recv ENQ
	uart->addRecvData(0x05);

	// Send ACK
	TEST_HEXDATA_EQUAL("06", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_NUMBER_EQUAL(Ecp::Server::Event_Connect, observer->getType());
	result->clear();

	// Recv Setup
	uart->addRecvData(Ecp::Control_STX);
	uart->addRecvData(2);
	uart->addRecvData(31);
	uart->addRecvData(32);
	uart->addRecvData(50);

	// Send NAK
	TEST_HEXDATA_EQUAL("15", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv Setup
	TEST_NUMBER_EQUAL(true, recvCommand("01"));

	// Send ACK
	TEST_HEXDATA_EQUAL("0602010003", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	return true;
}

bool EcpServerTest::gotoStateReady() {
	server->reset();

	// Recv ENQ
	uart->addRecvData(0x05);

	// Send ACK
	TEST_HEXDATA_EQUAL("06", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_NUMBER_EQUAL(Ecp::Server::Event_Connect, observer->getType());
	result->clear();

	// Recv Setup
	TEST_NUMBER_EQUAL(true, recvCommand("01"));

	// Send ACK
	TEST_HEXDATA_EQUAL("0602010003", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	return true;
}

bool EcpServerTest::testUploadAsync() {
	gotoStateReady();

	// Recv UploadStart
	parser->setStartResult(Dex::DataParser::Result_Ok);
	TEST_NUMBER_EQUAL(true, recvCommand("020100000000"));
	TEST_STRING_EQUAL("<start=0>", parser->getData());
	parser->clearData();

	// Send ACK
	TEST_HEXDATA_EQUAL("0602020000", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv UploadData
	parser->setProcDataResult(Dex::DataParser::Result_Ok);
	TEST_NUMBER_EQUAL(true, recvCommand("030102030405060708090A"));
	TEST_STRING_EQUAL("0102030405060708090A", parser->getData());
	parser->clearData();

	// Send ACK
	TEST_HEXDATA_EQUAL("0602030001", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv UploadData
	parser->setProcDataResult(Dex::DataParser::Result_Async);
	TEST_NUMBER_EQUAL(true, recvCommand("031112131415161718191A"));
	TEST_STRING_EQUAL("1112131415161718191A", parser->getData());
	parser->clearData();

	// Wait async result
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());

	// Recv Confirm
	Event event1(Dex::DataParser::Event_AsyncOk);
	server->proc(&event1);
	TEST_HEXDATA_EQUAL("0602030001", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv UploadData
	parser->setProcDataResult(Dex::DataParser::Result_Busy);
	TEST_NUMBER_EQUAL(true, recvCommand("033132333435363738393A"));
	TEST_STRING_EQUAL("3132333435363738393A", parser->getData());
	parser->clearData();

	// Send ACK
	TEST_HEXDATA_EQUAL("0602030100", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv UploadEnd
	parser->setCompleteResult(Dex::DataParser::Result_Async);
	TEST_NUMBER_EQUAL(true, recvCommand("04"));
	TEST_STRING_EQUAL("<complete>", parser->getData());
	parser->clearData();

	// Wait async result
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());

	// Recv Confirm
	Event event2(Dex::DataParser::Event_AsyncOk);
	server->proc(&event2);
	TEST_HEXDATA_EQUAL("0602040006", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	return true;
}

bool EcpServerTest::testUploadSync() {
	gotoStateReady();

	// Recv UploadStart
	parser->setStartResult(Dex::DataParser::Result_Ok);
	TEST_NUMBER_EQUAL(true, recvCommand("020100000000"));
	TEST_STRING_EQUAL("<start=0>", parser->getData());
	parser->clearData();

	// Send ACK
	TEST_HEXDATA_EQUAL("0602020000", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv UploadData
	parser->setProcDataResult(Dex::DataParser::Result_Ok);
	TEST_NUMBER_EQUAL(true, recvCommand("030102030405060708090A"));
	TEST_STRING_EQUAL("0102030405060708090A", parser->getData());
	parser->clearData();

	// Send ACK
	TEST_HEXDATA_EQUAL("0602030001", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv UploadData
	parser->setProcDataResult(Dex::DataParser::Result_Async);
	TEST_NUMBER_EQUAL(true, recvCommand("031112131415161718191A"));
	TEST_STRING_EQUAL("1112131415161718191A", parser->getData());
	parser->clearData();

	// Wait async result
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());

	// Recv Confirm
	Event event1(Dex::DataParser::Event_AsyncOk);
	server->proc(&event1);
	TEST_HEXDATA_EQUAL("0602030001", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv UploadData
	parser->setProcDataResult(Dex::DataParser::Result_Busy);
	TEST_NUMBER_EQUAL(true, recvCommand("033132333435363738393A"));
	TEST_STRING_EQUAL("3132333435363738393A", parser->getData());
	parser->clearData();

	// Send ACK
	TEST_HEXDATA_EQUAL("0602030100", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv UploadEnd
	parser->setCompleteResult(Dex::DataParser::Result_Ok);
	TEST_NUMBER_EQUAL(true, recvCommand("04"));
	TEST_STRING_EQUAL("<complete>", parser->getData());
	parser->clearData();

	// Send Confirm
	TEST_HEXDATA_EQUAL("0602040006", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	timerEngine->tick(ECP_KEEP_ALIVE_PERIOD);
	timerEngine->execute();
	timerEngine->tick(ECP_KEEP_ALIVE_PERIOD);
	timerEngine->execute();
	TEST_NUMBER_EQUAL(Ecp::Server::Event_Disconnect, observer->getType());
	result->clear();
	TEST_STRING_EQUAL("", parser->getData());
	parser->clearData();
	return true;
}

bool EcpServerTest::testDownload() {
	gotoStateReady();

	// Recv DownloadStart
	TEST_NUMBER_EQUAL(true, recvCommand("0601"));

	// Send Response
	TEST_HEXDATA_EQUAL("060606000000000000", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv DownloadData
	TEST_NUMBER_EQUAL(true, recvCommand("07"));

	// Send Data
	TEST_HEXDATA_EQUAL("06070700313131313131", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv DownloadData
	TEST_NUMBER_EQUAL(true, recvCommand("07"));

	// Send Data
	TEST_HEXDATA_EQUAL("060C0700323232323233333333330A", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv DownloadData
	TEST_NUMBER_EQUAL(true, recvCommand("07"));

	// Send Last Data
	TEST_HEXDATA_EQUAL("0611070B3434343434353535353536363636362A", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	return true;
}

bool EcpServerTest::testTable() {
	TestTableProcessor processor(Ecp::Table_Event, 10);
	gotoStateReady();

	// Recv TableInfoRequest
	TEST_NUMBER_EQUAL(true, recvCommand("090100"));
	parser->clearData();

	// Send ACK(Error_TableNotFound)
	TEST_HEXDATA_EQUAL("0602090902", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv TableInfoRequest
	server->setTableProcessor(&processor);
	TEST_NUMBER_EQUAL(true, recvCommand("090100"));
	parser->clearData();

	// Send TableInfoResponse
	TEST_HEXDATA_EQUAL("060609000A00000005", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv TableEntryRequest
	TEST_NUMBER_EQUAL(true, recvCommand("0A010000000000"));
	parser->clearData();

	// Send TableEntryRequest
	TEST_HEXDATA_EQUAL("060A0A00A0A1A2A3A4A5A6A700", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv DateTimeRequest
	TEST_NUMBER_EQUAL(true, recvCommand("0B"));
	parser->clearData();

	// Send DateTimeRequest
	TEST_HEXDATA_EQUAL("06080B000102030A141E03", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	return true;
}
#endif
