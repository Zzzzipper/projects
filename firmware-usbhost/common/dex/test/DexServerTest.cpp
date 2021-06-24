#include "dex/include/DexServer.h"
#include "dex/DexCrc.h"
#include "dex/test/TestDataGenerator.h"
#include "dex/test/TestDataParser.h"
#include "uart/include/TestUart.h"
#include "timer/include/TimerEngine.h"
#include "test/include/Test.h"

namespace Dex {

class ServerTest : public TestSet {
public:
	ServerTest();
	bool init();
	void cleanup();
	bool testSlaveFirstHandShake();
	bool testSlaveSecondHandShake();
	bool testSlaveChangeToMasterByDle0();
	bool testSendData();
	bool testSendDataNak();
	bool testMasterFirstHandShake();
	bool testMasterSecondHandShake();
	bool testRecvData();
	bool testRecvDataDoubleEOT();
	bool testRecvDataWrongCrc();

private:
	TestUart *uart;
	TimerEngine *timerEngine;
	TestDataGenerator *testDataGenerator;
	TestDataParser *testDataParser;
	Dex::Crc *crc;
	Dex::Server *server;
};

TEST_SET_REGISTER(Dex::ServerTest);

ServerTest::ServerTest() {
	TEST_CASE_REGISTER(ServerTest, testSlaveFirstHandShake);
	TEST_CASE_REGISTER(ServerTest, testSlaveSecondHandShake);
	TEST_CASE_REGISTER(ServerTest, testSlaveChangeToMasterByDle0);
	TEST_CASE_REGISTER(ServerTest, testSendData);
	TEST_CASE_REGISTER(ServerTest, testSendDataNak);
	TEST_CASE_REGISTER(ServerTest, testMasterFirstHandShake);
	TEST_CASE_REGISTER(ServerTest, testMasterSecondHandShake);
	TEST_CASE_REGISTER(ServerTest, testRecvData);
	TEST_CASE_REGISTER(ServerTest, testRecvDataDoubleEOT);
	TEST_CASE_REGISTER(ServerTest, testRecvDataWrongCrc);
}

bool ServerTest::init() {
	this->uart = new TestUart(256);
	this->timerEngine = new TimerEngine();
	this->testDataGenerator = new TestDataGenerator();
	this->testDataParser = new TestDataParser();
	this->crc = new Dex::Crc();
	this->server = new Dex::Server();
	this->server->init(this->uart, this->timerEngine, testDataGenerator, NULL);
	return true;
}

void ServerTest::cleanup() {
	delete this->server;
	delete this->crc;
	delete this->testDataParser;
	delete this->testDataGenerator;
	delete this->timerEngine;
	delete this->uart;
}

bool ServerTest::testSlaveFirstHandShake() {
	Dex::Server::State state = this->server->getState();
	TEST_NUMBER_EQUAL(state, Dex::Server::State_Wait);

	//recv ENQ
	uart->addRecvData(DexControl_ENQ);

	//send DLE,0
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1030", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Slave_FHS_1, server->getState());
	uart->clearSendBuffer();

	//recv DLE,SOH,setup,DLE,ETX,CRC0,CRC1
	DexHandShakeRequest req;
	req.set(DexOperation_Recv);
	crc->start();
	crc->add(reinterpret_cast<const char*>(&req), sizeof(req));
	crc->addUint8(DexControl_ETX);
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_SOH);
	uart->addRecvData(&req, sizeof(req));
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_ETX);
	uart->addRecvData(crc->getLowByte());
	uart->addRecvData(crc->getHighByte());

	//send DLE,1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1031", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Slave_FHS_2, server->getState());
	uart->clearSendBuffer();

	//recv EOT
	uart->addRecvData(DexControl_EOT);
	TEST_NUMBER_EQUAL(Dex::Server::State_Slave_SHS_1, server->getState());
	return true;
}

bool ServerTest::testSlaveSecondHandShake() {
	TEST_NUMBER_EQUAL(true, testSlaveFirstHandShake());

	//send ENQ
	timerEngine->tick(DEX_TIMEOUT_INTERSESSION);
	timerEngine->execute();
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Slave_SHS_2, server->getState());
	uart->clearSendBuffer();

	//recv DLE,0
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData('0');

	//send DLE,SOH,setup,DLE,ETX,CRC0,CRC1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("100130304550484F5230303030315230314C30311003FE80", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Slave_SHS_3, server->getState());
	uart->clearSendBuffer();

	//recv DLE,1
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData('1');

	//send EOT
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("04", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_SendData_1, server->getState());
	uart->clearSendBuffer();
	return true;
}

bool ServerTest::testSlaveChangeToMasterByDle0() {
	Dex::Server::State state = this->server->getState();
	TEST_NUMBER_EQUAL(state, Dex::Server::State_Wait);

	//recv ENQ
	uart->addRecvData(DexControl_ENQ);

	//send DLE,0
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1030", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Slave_FHS_1, server->getState());
	uart->clearSendBuffer();

	//recv unwaited DLE0 and start MasterFirstHandshake
	uart->addRecvData("1030");

	//send DLE,SOH,setup,DLE,ETX,CRC0,CRC1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("10014550484F523030303031535230314C30311003F990", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Master_FHS_2, server->getState());
	uart->clearSendBuffer();

	//recv DLE,1
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData('1');

	//send EOT
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("04", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Master_SHS_1, server->getState());
	uart->clearSendBuffer();

	//recv ENQ
	uart->addRecvData(DexControl_ENQ);

	//send DLE,0
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1030", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Master_SHS_2, server->getState());
	uart->clearSendBuffer();

	//recv DLE,SOH,setup,DLE,ETX,CRC0,CRC1
	DexHandShakeResponse req;
	req.set(DexResponse_OK);
	crc->start();
	crc->add(reinterpret_cast<const char*>(&req), sizeof(req));
	crc->addUint8(DexControl_ETX);
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_SOH);
	uart->addRecvData(&req, sizeof(req));
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_ETX);
	uart->addRecvData(crc->getLowByte());
	uart->addRecvData(crc->getHighByte());

	//send DLE,1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1031", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Master_SHS_3, server->getState());
	uart->clearSendBuffer();

	//recv EOT
	uart->addRecvData(DexControl_EOT);
	TEST_NUMBER_EQUAL(Dex::Server::State_SendData_1, server->getState());
	return true;
}

bool ServerTest::testSendData() {
	testSlaveSecondHandShake();

	//send ENQ
	timerEngine->tick(DEX_TIMEOUT_INTERSESSION);
	timerEngine->execute();
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_SendData_2, server->getState());
	uart->clearSendBuffer();

	//recv DLE,0
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData('0');

	//send DLE,STX,data1,DLE,ETB,CRC0,CRC1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1002313131313110177348", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_SendData_3, server->getState());
	uart->clearSendBuffer();

	//recv DLE,1
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData('1');

	//send DLE,STX,data1,DLE,ETB,CRC0,CRC1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("10023232323232333333333310172390", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_SendData_3, server->getState());
	uart->clearSendBuffer();

	//recv DLE,0
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData('0');

	//send DLE,STX,data1,DLE,ETX,CRC0,CRC1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("10023434343434353535353536363636361003AF20", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_SendData_4, server->getState());
	uart->clearSendBuffer();

	//recv DLE,1
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData('1');

	//send EOT
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("04", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Wait, server->getState());
	uart->clearSendBuffer();
	return true;
}

bool ServerTest::testSendDataNak() {
	testSlaveSecondHandShake();

	//send ENQ
	timerEngine->tick(DEX_TIMEOUT_INTERSESSION);
	timerEngine->execute();
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_SendData_2, server->getState());
	uart->clearSendBuffer();

	//recv DLE,0
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData('0');

	//send DLE,STX,data1,DLE,ETB,CRC0,CRC1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1002313131313110177348", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_SendData_3, server->getState());
	uart->clearSendBuffer();

	//recv DLE,1
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData('1');

	//send DLE,STX,data1,DLE,ETB,CRC0,CRC1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("10023232323232333333333310172390", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_SendData_3, server->getState());
	uart->clearSendBuffer();

	//recv NAK
	uart->addRecvData(DexControl_NAK);

	//send DLE,STX,data1,DLE,ETB,CRC0,CRC1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("10023232323232333333333310172390", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_SendData_3, server->getState());
	uart->clearSendBuffer();

	//recv DLE,0
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData('0');

	//send DLE,STX,data1,DLE,ETX,CRC0,CRC1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("10023434343434353535353536363636361003AF20", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_SendData_4, server->getState());
	uart->clearSendBuffer();

	//recv DLE,1
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData('1');

	//send EOT
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("04", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Wait, server->getState());
	uart->clearSendBuffer();
	return true;
}

bool ServerTest::testMasterFirstHandShake() {
	Dex::Server::State state = this->server->getState();
	TEST_NUMBER_EQUAL(state, Dex::Server::State_Wait);

	//send ENQ
	server->recvData(testDataParser, NULL);
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Master_FHS_1, server->getState());
	uart->clearSendBuffer();

	//recv DLE,0
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData('0');

	//send DLE,SOH,setup,DLE,ETX,CRC0,CRC1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("10014550484F523030303031525230314C30311003385C", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Master_FHS_2, server->getState());
	uart->clearSendBuffer();

	//recv DLE,1
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData('1');

	//send EOT
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("04", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Master_SHS_1, server->getState());
	uart->clearSendBuffer();
	return true;
}

bool ServerTest::testMasterSecondHandShake() {
	TEST_NUMBER_EQUAL(true, testMasterFirstHandShake());

	//recv ENQ
	uart->addRecvData(DexControl_ENQ);

	//send DLE,0
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1030", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Master_SHS_2, server->getState());
	uart->clearSendBuffer();

	//recv DLE,SOH,setup,DLE,ETX,CRC0,CRC1
	DexHandShakeResponse req;
	req.set(DexResponse_OK);
	crc->start();
	crc->add(reinterpret_cast<const char*>(&req), sizeof(req));
	crc->addUint8(DexControl_ETX);
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_SOH);
	uart->addRecvData(&req, sizeof(req));
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_ETX);
	uart->addRecvData(crc->getLowByte());
	uart->addRecvData(crc->getHighByte());

	//send DLE,1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1031", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_Master_SHS_3, server->getState());
	uart->clearSendBuffer();

	//recv EOT
	uart->addRecvData(DexControl_EOT);
	TEST_NUMBER_EQUAL(Dex::Server::State_RecvData_1, server->getState());
	return true;
}

bool ServerTest::testRecvData() {
	TEST_NUMBER_EQUAL(true, testMasterSecondHandShake());

	//recv ENQ
	uart->addRecvData(DexControl_ENQ);

	//send DLE,0
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1030", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<start=0>", testDataParser->getData());
	TEST_NUMBER_EQUAL(Dex::Server::State_RecvData_2, server->getState());
	uart->clearSendBuffer();
	testDataParser->clearData();

	//recv DLE,STX,data1,DLE,ETB,CRC0,CRC1
	uint8_t data1[] = { 0x21, 0x21, 0x21, 0x21, 0x21 };
	crc->start();
	crc->add(reinterpret_cast<const char*>(data1), sizeof(data1));
	crc->addUint8(DexControl_ETB);
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_STX);
	uart->addRecvData(data1, sizeof(data1));
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_ETB);
	uart->addRecvData(crc->getLowByte());
	uart->addRecvData(crc->getHighByte());

	//send DLE,1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1031", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("2121212121", testDataParser->getData());
	TEST_NUMBER_EQUAL(Dex::Server::State_RecvData_2, server->getState());
	uart->clearSendBuffer();
	testDataParser->clearData();

	//recv DLE,STX,data1,DLE,ETB,CRC0,CRC1
	uint8_t data2[] = { 0x34, 0x34, 0x34, 0x34, 0x34 };
	crc->start();
	crc->add(reinterpret_cast<const char*>(data2), sizeof(data2));
	crc->addUint8(DexControl_ETB);
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_STX);
	uart->addRecvData(data2, sizeof(data2));
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_ETB);
	uart->addRecvData(crc->getLowByte());
	uart->addRecvData(crc->getHighByte());

	//send DLE,0
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1030", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("3434343434", testDataParser->getData());
	TEST_NUMBER_EQUAL(Dex::Server::State_RecvData_2, server->getState());
	uart->clearSendBuffer();
	testDataParser->clearData();

	//recv DLE,STX,data1,DLE,ETB,CRC0,CRC1
	uint8_t data3[] = { 0x56, 0x56, 0x56, 0x56, 0x56 };
	crc->start();
	crc->add(reinterpret_cast<const char*>(data3), sizeof(data3));
	crc->addUint8(DexControl_ETX);
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_STX);
	uart->addRecvData(data3, sizeof(data3));
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_ETX);
	uart->addRecvData(crc->getLowByte());
	uart->addRecvData(crc->getHighByte());

	//send DLE,1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1031", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("5656565656", testDataParser->getData());
	TEST_NUMBER_EQUAL(Dex::Server::State_RecvData_3, server->getState());
	uart->clearSendBuffer();
	testDataParser->clearData();

	//recv EOT
	uart->addRecvData(DexControl_EOT);
	TEST_STRING_EQUAL("<complete>", testDataParser->getData());
	TEST_NUMBER_EQUAL(Dex::Server::State_Wait, server->getState());
	return true;
}

bool ServerTest::testRecvDataDoubleEOT() {
	TEST_NUMBER_EQUAL(true, testMasterSecondHandShake());

	//recv ENQ
	uart->addRecvData(DexControl_ENQ);

	//send DLE,0
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1030", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<start=0>", testDataParser->getData());
	TEST_NUMBER_EQUAL(Dex::Server::State_RecvData_2, server->getState());
	uart->clearSendBuffer();
	testDataParser->clearData();

	//recv ENQ
	uart->addRecvData(DexControl_ENQ);

	//ignore
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::Server::State_RecvData_2, server->getState());
	uart->clearSendBuffer();

	//recv DLE,STX,data1,DLE,ETB,CRC0,CRC1
	uint8_t data1[] = { 0x21, 0x21, 0x21, 0x21, 0x21 };
	crc->start();
	crc->add(reinterpret_cast<const char*>(data1), sizeof(data1));
	crc->addUint8(DexControl_ETB);
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_STX);
	uart->addRecvData(data1, sizeof(data1));
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_ETB);
	uart->addRecvData(crc->getLowByte());
	uart->addRecvData(crc->getHighByte());

	//send DLE,1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1031", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("2121212121", testDataParser->getData());
	TEST_NUMBER_EQUAL(Dex::Server::State_RecvData_2, server->getState());
	uart->clearSendBuffer();
	testDataParser->clearData();
	return true;
}

bool ServerTest::testRecvDataWrongCrc() {
	TEST_NUMBER_EQUAL(true, testMasterSecondHandShake());

	//recv ENQ
	uart->addRecvData(DexControl_ENQ);

	//send DLE,0
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1030", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<start=0>", testDataParser->getData());
	TEST_NUMBER_EQUAL(Dex::Server::State_RecvData_2, server->getState());
	uart->clearSendBuffer();
	testDataParser->clearData();

	//recv DLE,STX,data1,DLE,ETB,CRC0,CRC1
	uint8_t data1[] = { 0x21, 0x21, 0x21, 0x21, 0x21 };
	crc->start();
	crc->add(reinterpret_cast<const char*>(data1), sizeof(data1));
	crc->addUint8(DexControl_ETB);
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_STX);
	uart->addRecvData(data1, sizeof(data1));
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_ETB);
	uart->addRecvData(crc->getLowByte());
	uart->addRecvData(crc->getHighByte());

	//send DLE,1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1031", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("2121212121", testDataParser->getData());
	TEST_NUMBER_EQUAL(Dex::Server::State_RecvData_2, server->getState());
	uart->clearSendBuffer();
	testDataParser->clearData();

	//recv DLE,STX,data1,DLE,ETB,CRC0,CRC1
	uint8_t data2[] = { 0x34, 0x34, 0x34, 0x34, 0x34 };
	crc->start();
	crc->add(reinterpret_cast<const char*>(data2), sizeof(data2));
	crc->addUint8(DexControl_ETB);
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_STX);
	uart->addRecvData(data2, sizeof(data2));
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_ETB);
	uart->addRecvData(crc->getLowByte());
	uart->addRecvData(crc->getHighByte());

	//send DLE,0
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1030", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("3434343434", testDataParser->getData());
	TEST_NUMBER_EQUAL(Dex::Server::State_RecvData_2, server->getState());
	uart->clearSendBuffer();
	testDataParser->clearData();

	//recv DLE,STX,data1,DLE,ETB,CRC0,CRC1
	uint8_t data3[] = { 0x56, 0x56, 0x56, 0x56, 0x56 };
	crc->start();
	crc->add(reinterpret_cast<const char*>(data3), sizeof(data3));
	crc->addUint8(DexControl_ETX);
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_STX);
	uart->addRecvData(data3, sizeof(data3));
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_ETX);
	uart->addRecvData(0x11);
	uart->addRecvData(0x22);

	//send DLE,1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("15", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", testDataParser->getData());
	TEST_NUMBER_EQUAL(Dex::Server::State_RecvData_2, server->getState());
	uart->clearSendBuffer();
	testDataParser->clearData();

	//recv DLE,STX,data1,DLE,ETB,CRC0,CRC1
	crc->start();
	crc->add(reinterpret_cast<const char*>(data3), sizeof(data3));
	crc->addUint8(DexControl_ETX);
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_STX);
	uart->addRecvData(data3, sizeof(data3));
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_ETX);
	uart->addRecvData(crc->getLowByte());
	uart->addRecvData(crc->getHighByte());

	//send DLE,1
	timerEngine->tick(DEX_TIMEOUT_SENDING);
	timerEngine->execute();
	TEST_HEXDATA_EQUAL("1031", uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("5656565656", testDataParser->getData());
	TEST_NUMBER_EQUAL(Dex::Server::State_RecvData_3, server->getState());
	uart->clearSendBuffer();
	testDataParser->clearData();

	//recv EOT
	uart->addRecvData(DexControl_EOT);
	TEST_STRING_EQUAL("<complete>", testDataParser->getData());
	TEST_NUMBER_EQUAL(Dex::Server::State_Wait, server->getState());
	return true;
}

}
