#include "test/include/Test.h"
#include "sim900/include/GsmTcpConnection.h"
#include "sim900/include/GsmDriver.h"
#include "sim900/command/GsmCommand.h"
#include "timer/include/TimerEngine.h"
#include "logger/include/Logger.h"

namespace Gsm {

class TestCommandProcessor : public Gsm::CommandProcessor {
public:
	TestCommandProcessor() : executeReturn(true), sendData(1024) {}

	virtual bool execute(Command *command) { return procExecute("<command=", command); }
	virtual bool executeOutOfTurn(Command *command) { return procExecute("<command-out-of-turn=", command); }
	virtual void reset() { sendData << "<reset>"; }
	virtual void dump(StringBuilder *data) { (void)data; }

	void setExecuteReturn(bool value) { executeReturn = value; }
	void setRecvData(const uint8_t *data, uint16_t dataLen) { strncpy((char*)command->getData(), (char*)data, dataLen); }
	void sendResponse(Command::Result result, const char *data) { command->deliverResponse(result, data); }
	const char *getSendData() { return sendData.getString(); }
	void clearSendData() { sendData.clear(); }

private:
	bool executeReturn;
	Command *command;
	StringBuilder sendData;

	virtual bool procExecute(const char *prefix, Command *command) {
		if(executeReturn == false) {
			return false;
		}
		sendData << prefix << commandTypeToString(command->getType()) << "," << command->getTimeout() << "," << command->getText();
		const uint8_t *data = command->getData();
		uint16_t dataLen = command->getDataLen();
		if(command->getType() == Command::Type_SendData) {
			sendData << ",data=";
			for(uint16_t i = 0; i < dataLen; i++) {
				sendData.addHex(data[i]);
			}
		} else if(command->getType() == Command::Type_RecvData) {
			sendData << ",bufLen=" << command->getDataLen();
		}
		sendData << ">";
		this->command = command;
		return true;
	}

	const char *commandTypeToString(Gsm::Command::Type type) {
		switch(type) {
		case Command::Type_Result: return "R";
		case Command::Type_GsnData: return "GSN";
		case Command::Type_CgmrData: return "CGMR";
		case Command::Type_CregData: return "CREG";
		case Command::Type_CgattData: return "CGATT";
		case Command::Type_CifsrData: return "CIFSR";
		case Command::Type_CipStatus: return "CIPSTATUS";
		case Command::Type_SendData: return "SEND";
		case Command::Type_RecvData: return "RECV";
		case Command::Type_CipClose: return "CIPCLOSE";
		default: return "ERROR";
		}
	}
};

class TestClientObserver : public EventObserver  {
public:
	void proc(Event *event) {
		switch(event->getType()) {
			case TcpConnection::Event_ConnectOk:	 result << "<TcpConnection::Event_ConnectOk>"; return;
			case TcpConnection::Event_ConnectError:	 result << "<TcpConnection::Event_ConnectError>"; return;
			case TcpConnection::Event_IncomingData:	 result << "<TcpConnection::Event_IncomingData>"; return;
			case TcpConnection::Event_RecvDataOk:	 result << "<TcpConnection::Event_RecvDataOk,recvLen=" << event->getUint16() << ">"; return;
			case TcpConnection::Event_RecvDataError: result << "<TcpConnection::Event_RecvDataError>"; return;
			case TcpConnection::Event_SendDataOk:	 result << "<TcpConnection::Event_SendDataOk>"; return;
			case TcpConnection::Event_SendDataError: result << "<TcpConnection::Event_SendDataError>"; return;
			case TcpConnection::Event_Close:		 result << "<TcpConnection::Event_Close>"; return;
			default: result << "<" << event->getType() << ">";
		}
	}
	const char *getResult() const { return result.getString(); }
	void clearResult() { result.clear(); }

private:
	StringBuilder result;
};

class TcpConnectionTest : public TestSet {
public:
	TcpConnectionTest();
	bool init();
	void cleanup();
	bool testConnect();
	bool testStateGprsCheckExecuteError();
	bool testStateSslExecuteError();
	bool testStateOpenExecuteError();
	bool testReconnectByMissConnectOk();
	bool testReconnectByWrongCipStatus();
	bool testReconnectTooManyTries();
	bool testSendCommandExecuteError();
	bool testSendCommandError();
	bool testCloseBeforeSendCommand();
	bool testRecvCommandExecuteError();
	bool testCloseBeforeRecvCommand();
	bool testIncommingDataBeforeRecvComplete();
	bool testCloseExecuteError();
	bool testIdleDisconnect();
	bool testSendAndIdleDisconnect();
	bool testSendRecvAndIdleDisconnect();

private:
	TimerEngine *timerEngine;
	TestCommandProcessor *commandProcessor;
	StatStorage *stat;
	TestClientObserver *eventObserver;
	TcpConnection *conn;
};

TEST_SET_REGISTER(Gsm::TcpConnectionTest);

TcpConnectionTest::TcpConnectionTest() {
	TEST_CASE_REGISTER(TcpConnectionTest, testConnect);
	TEST_CASE_REGISTER(TcpConnectionTest, testStateGprsCheckExecuteError);
	TEST_CASE_REGISTER(TcpConnectionTest, testStateSslExecuteError);
	TEST_CASE_REGISTER(TcpConnectionTest, testStateOpenExecuteError);
	TEST_CASE_REGISTER(TcpConnectionTest, testReconnectByMissConnectOk);
	TEST_CASE_REGISTER(TcpConnectionTest, testReconnectByWrongCipStatus);
	TEST_CASE_REGISTER(TcpConnectionTest, testReconnectTooManyTries);
	TEST_CASE_REGISTER(TcpConnectionTest, testSendCommandExecuteError);
	TEST_CASE_REGISTER(TcpConnectionTest, testSendCommandError);
	TEST_CASE_REGISTER(TcpConnectionTest, testCloseBeforeSendCommand);
	TEST_CASE_REGISTER(TcpConnectionTest, testRecvCommandExecuteError);
	TEST_CASE_REGISTER(TcpConnectionTest, testCloseBeforeRecvCommand);
	TEST_CASE_REGISTER(TcpConnectionTest, testIncommingDataBeforeRecvComplete);
	TEST_CASE_REGISTER(TcpConnectionTest, testCloseExecuteError);
	TEST_CASE_REGISTER(TcpConnectionTest, testIdleDisconnect);
	TEST_CASE_REGISTER(TcpConnectionTest, testSendAndIdleDisconnect);
	TEST_CASE_REGISTER(TcpConnectionTest, testSendRecvAndIdleDisconnect);
}

bool TcpConnectionTest::init() {
	timerEngine = new TimerEngine;
	commandProcessor = new TestCommandProcessor;
	stat = new StatStorage;
	eventObserver = new TestClientObserver;
	conn = new TcpConnection(timerEngine, commandProcessor, 0, stat);
	conn->setObserver(eventObserver);
	return true;
}

void TcpConnectionTest::cleanup() {
	delete conn;	
	delete eventObserver;
	delete stat;
	delete commandProcessor;
	delete timerEngine;
}

bool TcpConnectionTest::testConnect() {
	// cipstatus
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	TEST_STRING_EQUAL("<command=CIPSTATUS,5000,AT+CIPSTATUS>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, "IP STATUS");

#ifdef ERP_SSL_OFF
	// cipstart
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#else
	// cipssl
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSSL=0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);

	// cipstart
	TEST_STRING_EQUAL("<command-out-of-turn=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#endif

	// wait cipstart event
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// cipstart event
	conn->procEvent("0, CONNECT OK");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_ConnectOk>", eventObserver->getResult());
	eventObserver->clearResult();

	// cipsend
	TEST_NUMBER_EQUAL(true, conn->send((uint8_t*)"0123456789ABCDEF", 16));
	TEST_STRING_EQUAL("<command=SEND,20000,AT+CIPSEND=0,16,data=30313233343536373839414243444546>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// cipsend result
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_SendDataOk>", eventObserver->getResult());

	// ciprxget event
	eventObserver->clearResult();
	conn->procEvent("+CIPRXGET: 1,0");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_IncomingData>", eventObserver->getResult());
	eventObserver->clearResult();

	// ciprxget
	char recvBuf[20];
	TEST_NUMBER_EQUAL(true, conn->recv((uint8_t*)recvBuf, sizeof(recvBuf)));
	TEST_STRING_EQUAL("<command=RECV,5000,AT+CIPRXGET=2,0,20,bufLen=20>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	const char recvData[] = "0123456789";
	commandProcessor->setRecvData((uint8_t*)recvData, strlen(recvData));
	commandProcessor->sendResponse(Command::Result_OK, "+CIPRXGET: 2,0,10,0");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_RecvDataOk,recvLen=10>", eventObserver->getResult());
	eventObserver->clearResult();
	TEST_SUBSTR_EQUAL(recvData, recvBuf, strlen(recvData));

	// close
	conn->close();
	TEST_STRING_EQUAL("<command=CIPCLOSE,5000,AT+CIPCLOSE=0,0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, "0, CLOSE OK");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_Close>", eventObserver->getResult());
	return true;
}

bool TcpConnectionTest::testStateGprsCheckExecuteError() {
	// cipstatus
	commandProcessor->setExecuteReturn(false);
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	TEST_STRING_EQUAL("<reset>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// network up
	commandProcessor->setExecuteReturn(true);
	Event event3(Gsm::Driver::Event_NetworkUp);
	conn->proc(&event3);

#ifdef ERP_SSL_OFF
	// cipstart
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#else
	// cipssl
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSSL=0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);

	// cipstart
	TEST_STRING_EQUAL("<command-out-of-turn=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#endif
	return true;
}

bool TcpConnectionTest::testStateSslExecuteError() {
	// cipstatus
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	TEST_STRING_EQUAL("<command=CIPSTATUS,5000,AT+CIPSTATUS>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->setExecuteReturn(false);
	commandProcessor->sendResponse(Command::Result_OK, "IP STATUS");

	// cipssl
	TEST_STRING_EQUAL("<reset>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// network up
	commandProcessor->setExecuteReturn(true);
	Event event3(Gsm::Driver::Event_NetworkUp);
	conn->proc(&event3);

#ifdef ERP_SSL_OFF
	// cipstart
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#else
	// cipssl
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSSL=0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);

	// cipstart
	TEST_STRING_EQUAL("<command-out-of-turn=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#endif
	return true;
}

bool TcpConnectionTest::testStateOpenExecuteError() {
#ifndef ERP_SSL_OFF
	// cipstatus
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	TEST_STRING_EQUAL("<command=CIPSTATUS,5000,AT+CIPSTATUS>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, "IP STATUS");

	// cipssl
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSSL=0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->setExecuteReturn(false);
	commandProcessor->sendResponse(Command::Result_OK, NULL);

	// cipstart
	TEST_STRING_EQUAL("<reset>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// network up
	commandProcessor->setExecuteReturn(true);
	Event event3(Gsm::Driver::Event_NetworkUp);
	conn->proc(&event3);

	// cipssl
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSSL=0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);

	// cipstart
	TEST_STRING_EQUAL("<command-out-of-turn=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#endif
	return true;
}

bool TcpConnectionTest::testReconnectByMissConnectOk() {
	// cipstatus
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	TEST_STRING_EQUAL("<command=CIPSTATUS,5000,AT+CIPSTATUS>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, "IP STATUS");

#ifdef ERP_SSL_OFF
	// cipstart
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#else
	// cipssl
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSSL=0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);

	// cipstart
	TEST_STRING_EQUAL("<command-out-of-turn=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#endif

	// miss event
	timerEngine->tick(AT_CIPSTART_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	TEST_STRING_EQUAL("<reset>", commandProcessor->getSendData());
	commandProcessor->clearSendData();

	// network up
	Event event3(Gsm::Driver::Event_NetworkUp);
	conn->proc(&event3);

#ifdef ERP_SSL_OFF
	// cipstart
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#else
	// cipssl
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSSL=0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);

	// cipstart
	TEST_STRING_EQUAL("<command-out-of-turn=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#endif
	return true;
}

bool TcpConnectionTest::testReconnectByWrongCipStatus() {
	// cipstatus
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	TEST_STRING_EQUAL("<command=CIPSTATUS,5000,AT+CIPSTATUS>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, "PDP DEACT");

	// reset
	TEST_STRING_EQUAL("<reset>", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// network up
	commandProcessor->clearSendData();
	Event event1(Gsm::Driver::Event_NetworkUp);
	conn->proc(&event1);

#ifdef ERP_SSL_OFF
	// cipstart
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#else
	// cipssl
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSSL=0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);

	// cipstart
	TEST_STRING_EQUAL("<command-out-of-turn=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#endif
	return true;
}

bool TcpConnectionTest::testReconnectTooManyTries() {
#ifndef ERP_SSL_OFF
	// cipstatus
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	TEST_STRING_EQUAL("<command=CIPSTATUS,5000,AT+CIPSTATUS>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, "IP STATUS");

	uint16_t tryMax = GSM_TCP_TRY_MAX_NUMBER;
	for(uint16_t tryNum = 0; tryNum < tryMax; tryNum++) {
		// cipssl
		TEST_STRING_EQUAL("<command=R,5000,AT+CIPSSL=0>", commandProcessor->getSendData());
		commandProcessor->clearSendData();
		TEST_STRING_EQUAL("", eventObserver->getResult());

		// cipstart execute failed
		commandProcessor->setExecuteReturn(false);
		commandProcessor->sendResponse(Command::Result_OK, NULL);
		TEST_STRING_EQUAL("<reset>", commandProcessor->getSendData());
		commandProcessor->clearSendData();
		TEST_STRING_EQUAL("", eventObserver->getResult());

		// network up
		commandProcessor->setExecuteReturn(true);
		Event event1(Gsm::Driver::Event_NetworkUp);
		conn->proc(&event1);
	}

	// cipssl
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSSL=0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// cipstart execute failed
	commandProcessor->setExecuteReturn(false);
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_ConnectError>", eventObserver->getResult());
	eventObserver->clearResult();
#endif
	return true;
}

bool TcpConnectionTest::testSendCommandExecuteError() {
	// connect()
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	commandProcessor->sendResponse(Command::Result_OK, "IP PROCESSING");
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	conn->procEvent("0, CONNECT OK");
	commandProcessor->clearSendData();

	// send
	commandProcessor->clearSendData();
	eventObserver->clearResult();
	commandProcessor->setExecuteReturn(false);
	TEST_NUMBER_EQUAL(true, conn->send((uint8_t*)"0123456789ABCDEF", 16));
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// fake timeout
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_Close>", eventObserver->getResult());
	return true;
}

bool TcpConnectionTest::testSendCommandError() {
	// connect()
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	commandProcessor->sendResponse(Command::Result_OK, "IP STATUS");
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	conn->procEvent("0, CONNECT OK");

	// send
	commandProcessor->clearSendData();
	eventObserver->clearResult();
	TEST_NUMBER_EQUAL(true, conn->send((uint8_t*)"0123456789ABCDEF", 16));
	TEST_STRING_EQUAL("<command=SEND,20000,AT+CIPSEND=0,16,data=30313233343536373839414243444546>", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// send error
	commandProcessor->clearSendData();
	commandProcessor->sendResponse(Command::Result_ERROR, NULL);
	TEST_STRING_EQUAL("<command=CIPCLOSE,5000,AT+CIPCLOSE=0,0>", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// close complete
	commandProcessor->clearSendData();
	commandProcessor->sendResponse(Command::Result_OK, "0, CLOSE OK");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_Close>", eventObserver->getResult());
	return true;
}

bool TcpConnectionTest::testCloseBeforeSendCommand() {
	// connect()
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	commandProcessor->sendResponse(Command::Result_OK, "IP STATUS");
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	conn->procEvent("0, CONNECT OK");
	commandProcessor->clearSendData();

	// send
	eventObserver->clearResult();
	TEST_NUMBER_EQUAL(true, conn->send((uint8_t*)"0123456789ABCDEF", 16));
	TEST_STRING_EQUAL("<command=SEND,20000,AT+CIPSEND=0,16,data=30313233343536373839414243444546>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// close before command
	conn->procEvent("0, CLOSED");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// send failed
	commandProcessor->sendResponse(Command::Result_ERROR, NULL);
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_Close>", eventObserver->getResult());
	eventObserver->clearResult();
	return true;
}

bool TcpConnectionTest::testRecvCommandExecuteError() {
	// connect()
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	commandProcessor->sendResponse(Command::Result_OK, "IP STATUS");
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	conn->procEvent("0, CONNECT OK");
	commandProcessor->clearSendData();

	// ciprxget event
	eventObserver->clearResult();
	commandProcessor->setExecuteReturn(false);
	conn->procEvent("+CIPRXGET: 1,0");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_IncomingData>", eventObserver->getResult());
	eventObserver->clearResult();

	// ciprxget execute failed
	char recvBuf[20];
	TEST_NUMBER_EQUAL(true, conn->recv((uint8_t*)recvBuf, sizeof(recvBuf)));
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// fake timeout
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_Close>", eventObserver->getResult());
	return true;
}

bool TcpConnectionTest::testCloseBeforeRecvCommand() {
	// connect()
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	commandProcessor->sendResponse(Command::Result_OK, "IP STATUS");
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	conn->procEvent("0, CONNECT OK");
	commandProcessor->clearSendData();

	// ciprxget event
	eventObserver->clearResult();
	conn->procEvent("+CIPRXGET: 1,0");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_IncomingData>", eventObserver->getResult());
	eventObserver->clearResult();

	// ciprxget
	char recvBuf[20];
	TEST_NUMBER_EQUAL(true, conn->recv((uint8_t*)recvBuf, sizeof(recvBuf)));
	TEST_STRING_EQUAL("<command=RECV,5000,AT+CIPRXGET=2,0,20,bufLen=20>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// close before command
	conn->procEvent("0, CLOSED");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// recv failed
	commandProcessor->sendResponse(Command::Result_ERROR, NULL);
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_Close>", eventObserver->getResult());
	return true;
}

bool TcpConnectionTest::testIncommingDataBeforeRecvComplete() {
	// connect()
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	commandProcessor->sendResponse(Command::Result_OK, "IP STATUS");
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	conn->procEvent("0, CONNECT OK");
	commandProcessor->clearSendData();

	// ciprxget event
	eventObserver->clearResult();
	conn->procEvent("+CIPRXGET: 1,0");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_IncomingData>", eventObserver->getResult());
	eventObserver->clearResult();

	// ciprxget
	char recvBuf[20];
	TEST_NUMBER_EQUAL(true, conn->hasRecvData());
	TEST_NUMBER_EQUAL(true, conn->recv((uint8_t*)recvBuf, sizeof(recvBuf)));
	TEST_STRING_EQUAL("<command=RECV,5000,AT+CIPRXGET=2,0,20,bufLen=20>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// unwaited ciprxget event
	eventObserver->clearResult();
	conn->procEvent("+CIPRXGET: 1,0");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());
	eventObserver->clearResult();

	// recv data
	const char recvData1[] = "0123456789";
	commandProcessor->setRecvData((uint8_t*)recvData1, strlen(recvData1));
	commandProcessor->sendResponse(Command::Result_OK, "+CIPRXGET: 2,0,10,0");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_RecvDataOk,recvLen=10>", eventObserver->getResult());
	eventObserver->clearResult();
	TEST_SUBSTR_EQUAL(recvData1, recvBuf, strlen(recvData1));

	// ciprxget
	TEST_NUMBER_EQUAL(true, conn->hasRecvData());
	TEST_NUMBER_EQUAL(true, conn->recv((uint8_t*)recvBuf, sizeof(recvBuf)));
	TEST_STRING_EQUAL("<command=RECV,5000,AT+CIPRXGET=2,0,20,bufLen=20>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// recv data
	const char recvData2[] = "ABCDEF0123";
	commandProcessor->setRecvData((uint8_t*)recvData2, strlen(recvData2));
	commandProcessor->sendResponse(Command::Result_OK, "+CIPRXGET: 2,0,10,0");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_RecvDataOk,recvLen=10>", eventObserver->getResult());
	eventObserver->clearResult();
	TEST_SUBSTR_EQUAL(recvData2, recvBuf, strlen(recvData2));
	return true;
}

bool TcpConnectionTest::testCloseExecuteError() {
	// connect()
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	commandProcessor->sendResponse(Command::Result_OK, "IP STATUS");
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	conn->procEvent("0, CONNECT OK");
	commandProcessor->clearSendData();
	eventObserver->clearResult();

	// close
	commandProcessor->setExecuteReturn(false);
	conn->close();
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_Close>", eventObserver->getResult());
	return true;
}

bool TcpConnectionTest::testIdleDisconnect() {
	// cipstatus
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	TEST_STRING_EQUAL("<command=CIPSTATUS,5000,AT+CIPSTATUS>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, "IP STATUS");

#ifdef ERP_SSL_OFF
	// cipstart
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#else
	// cipssl
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSSL=0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);

	// cipstart
	TEST_STRING_EQUAL("<command-out-of-turn=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#endif

	// wait cipstart event
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// cipstart event
	conn->procEvent("0, CONNECT OK");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_ConnectOk>", eventObserver->getResult());
	eventObserver->clearResult();

	// idle timeout
	timerEngine->tick(GSM_TCP_IDLE_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("<command=CIPCLOSE,5000,AT+CIPCLOSE=0,0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// close
	commandProcessor->sendResponse(Command::Result_OK, "0, CLOSE OK");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_Close>", eventObserver->getResult());
	return true;
}

bool TcpConnectionTest::testSendAndIdleDisconnect() {
	// cipstatus
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	TEST_STRING_EQUAL("<command=CIPSTATUS,5000,AT+CIPSTATUS>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, "IP STATUS");

#ifdef ERP_SSL_OFF
	// cipstart
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#else
	// cipssl
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSSL=0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);

	// cipstart
	TEST_STRING_EQUAL("<command-out-of-turn=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#endif

	// wait cipstart event
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// cipstart event
	conn->procEvent("0, CONNECT OK");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_ConnectOk>", eventObserver->getResult());
	eventObserver->clearResult();

	// idle timeout
	timerEngine->tick(GSM_TCP_IDLE_TIMEOUT/2);
	timerEngine->execute();
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// cipsend
	TEST_NUMBER_EQUAL(true, conn->send((uint8_t*)"0123456789ABCDEF", 16));
	TEST_STRING_EQUAL("<command=SEND,20000,AT+CIPSEND=0,16,data=30313233343536373839414243444546>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// cipsend result
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_SendDataOk>", eventObserver->getResult());
	eventObserver->clearResult();

	// idle timeout
	timerEngine->tick(GSM_TCP_IDLE_TIMEOUT/2);
	timerEngine->execute();
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// idle timeout
	timerEngine->tick(GSM_TCP_IDLE_TIMEOUT/2);
	timerEngine->execute();
	TEST_STRING_EQUAL("<command=CIPCLOSE,5000,AT+CIPCLOSE=0,0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// close
	commandProcessor->sendResponse(Command::Result_OK, "0, CLOSE OK");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_Close>", eventObserver->getResult());
	return true;
}

bool TcpConnectionTest::testSendRecvAndIdleDisconnect() {
	// cipstatus
	TEST_NUMBER_EQUAL(true, conn->connect("test.sambery.net", 1234, TcpIp::Mode_TcpIp));
	TEST_STRING_EQUAL("<command=CIPSTATUS,5000,AT+CIPSTATUS>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, "IP STATUS");

#ifdef ERP_SSL_OFF
	// cipstart
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#else
	// cipssl
	TEST_STRING_EQUAL("<command=R,5000,AT+CIPSSL=0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);

	// cipstart
	TEST_STRING_EQUAL("<command-out-of-turn=R,5000,AT+CIPSTART=0,\"TCP\",\"test.sambery.net\",\"1234\">", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	commandProcessor->sendResponse(Command::Result_OK, NULL);
#endif

	// cipstart event
	conn->procEvent("0, CONNECT OK");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_ConnectOk>", eventObserver->getResult());
	eventObserver->clearResult();

	// idle timeout
	timerEngine->tick(GSM_TCP_IDLE_TIMEOUT/2);
	timerEngine->execute();
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// cipsend
	TEST_NUMBER_EQUAL(true, conn->send((uint8_t*)"0123456789ABCDEF", 16));
	TEST_STRING_EQUAL("<command=SEND,20000,AT+CIPSEND=0,16,data=30313233343536373839414243444546>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// cipsend result
	commandProcessor->sendResponse(Command::Result_OK, NULL);
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_SendDataOk>", eventObserver->getResult());
	eventObserver->clearResult();

	// idle timeout
	timerEngine->tick(GSM_TCP_IDLE_TIMEOUT/2);
	timerEngine->execute();
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// ciprxget event
	eventObserver->clearResult();
	conn->procEvent("+CIPRXGET: 1,0");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_IncomingData>", eventObserver->getResult());
	eventObserver->clearResult();

	// ciprxget
	char recvBuf[20];
	TEST_NUMBER_EQUAL(true, conn->recv((uint8_t*)recvBuf, sizeof(recvBuf)));
	TEST_STRING_EQUAL("<command=RECV,5000,AT+CIPRXGET=2,0,20,bufLen=20>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());
	const char recvData[] = "0123456789";
	commandProcessor->setRecvData((uint8_t*)recvData, strlen(recvData));
	commandProcessor->sendResponse(Command::Result_OK, "+CIPRXGET: 2,0,10,0");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_RecvDataOk,recvLen=10>", eventObserver->getResult());
	eventObserver->clearResult();
	TEST_SUBSTR_EQUAL(recvData, recvBuf, strlen(recvData));

	// idle timeout
	timerEngine->tick(GSM_TCP_IDLE_TIMEOUT/2);
	timerEngine->execute();
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// idle timeout
	timerEngine->tick(GSM_TCP_IDLE_TIMEOUT/2);
	timerEngine->execute();
	TEST_STRING_EQUAL("<command=CIPCLOSE,5000,AT+CIPCLOSE=0,0>", commandProcessor->getSendData());
	commandProcessor->clearSendData();
	TEST_STRING_EQUAL("", eventObserver->getResult());

	// close
	commandProcessor->sendResponse(Command::Result_OK, "0, CLOSE OK");
	TEST_STRING_EQUAL("", commandProcessor->getSendData());
	TEST_STRING_EQUAL("<TcpConnection::Event_Close>", eventObserver->getResult());
	return true;
}

}
