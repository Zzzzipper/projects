#include "test/include/Test.h"
#include "sim900/command/GsmCommandParser.h"
#include "timer/include/TimerEngine.h"
#include "uart/include/TestUart.h"
#include "utils/include/TestEventObserver.h"
#include "logger/include/Logger.h"

namespace Gsm {

class TestCommandObserver : public Command::Observer {
public:
	virtual ~TestCommandObserver() {}
	void clear() { str.clear(); }
	const char *getResult() { return str.getString(); }
	virtual void procResponse(Command::Result result, const char *data) {
		str << "<response=" << result << "," << (data == NULL ? "NULL" : data)  << ">";
	}
	virtual void procEvent(const char *data) {
		str << "<event=" << data << ">";
	}

private:
	StringBuilder str;
};

class CommandParserTest : public TestSet {
public:
	CommandParserTest();
	bool init();
	void cleanup();
	bool testCommandResult();
	bool testCommandResultError();
	bool testCommandGsn();
	bool testCommandCgmr();
	bool testCommandCcid();
	bool testCommandCsq();
	bool testCommandCops();
	bool testCommandCreg();
	bool testCommandCipShut();
	bool testCommandCipShutError();
	bool testCommandCgatt();
	bool testCommandCifsr();
	bool testCommandCifsrError();
	bool testCommandCipPing();
	bool testCommandCipStatus();
	bool testCommandCipStatusError();
	bool testCommandCipSendData();
	bool testCommandCipSendError();
	bool testCommandCipSendDataUnwaitedEvent();
	bool testCommandCipSendDataUnwaitedEvent2();
	bool testCommandCipRxGet();
	bool testCommandCipRxGetError();
	bool testCommandCipRxGetUnwaitedEvent();
	bool testCommandCipRxGetUnwaitedEvent2();
	bool testCommandCipRxGetUnwaitedEvent3();
	bool testCommandCipClose();
	bool testCommandCipCloseError();

private:
	TestUart *uart;
	TimerEngine *timerEngine;
	TestCommandObserver *observer;
	CommandParser *parser;
};

TEST_SET_REGISTER(Gsm::CommandParserTest);

CommandParserTest::CommandParserTest() {
	TEST_CASE_REGISTER(CommandParserTest, testCommandResult);
	TEST_CASE_REGISTER(CommandParserTest, testCommandResultError);
	TEST_CASE_REGISTER(CommandParserTest, testCommandGsn);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCgmr);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCcid);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCsq);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCops);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCreg);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipShut);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipShutError);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCgatt);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCifsr);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCifsrError);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipPing);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipStatus);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipStatusError);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipSendData);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipSendError);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipSendDataUnwaitedEvent);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipSendDataUnwaitedEvent2);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipRxGet);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipRxGetError);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipRxGetUnwaitedEvent);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipRxGetUnwaitedEvent2);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipRxGetUnwaitedEvent3);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipClose);
	TEST_CASE_REGISTER(CommandParserTest, testCommandCipCloseError);
}

bool CommandParserTest::init() {
	uart = new TestUart(1024);
	timerEngine = new TimerEngine;
	observer = new TestCommandObserver;
	parser = new CommandParser(uart, timerEngine);
	parser->setObserver(observer);
	parser->reset();
	return true;
}

void CommandParserTest::cleanup() {
	delete parser;
	delete observer;
	delete timerEngine;
	delete uart;
}

bool CommandParserTest::testCommandResult() {
	Command command(NULL);

	command.set(Command::Type_Result);
	command.setText() << "AT";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("OK\r\n");
	TEST_STRING_EQUAL("<response=0,>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandResultError() {
	Command command(NULL);

	command.set(Command::Type_Result);
	command.setText() << "AT+CIPSTART=0,tcp,erp.ephor.online,443";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPSTART=0,tcp,erp.ephor.online,443\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("ERROR\r\n");
	TEST_STRING_EQUAL("<response=2,>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandGsn() {
	Command command(NULL);

	command.set(Command::Type_GsnData);
	command.setText() << "AT+GSN";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+GSN\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("869696049189017\r\n");
	uart->addRecvString("OK\r\n");
	TEST_STRING_EQUAL("<response=0,869696049189017>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCgmr() {
	Command command(NULL);
	command.set(Command::Type_CgmrData);
	command.setText() << "AT+CGMR";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CGMR\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("Revision:1418B08SIM800C24_BT\r\n");
	uart->addRecvString("OK\r\n");
	TEST_STRING_EQUAL("<response=0,1418B08SIM800C24_BT>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCcid() {
	Command command(NULL);
	command.set(Command::Type_CcidData);
	command.setText() << "AT+CCID";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CCID\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("897010210702964638ff\r\n");
	uart->addRecvString("OK\r\n");
	TEST_STRING_EQUAL("<response=0,897010210702964638ff>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCsq() {
	Command command(NULL);
	command.set(Command::Type_CsqData);
	command.setText() << "AT+CSQ";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CSQ\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("+CSQ: 10\r\n");
	uart->addRecvString("OK\r\n");
	TEST_STRING_EQUAL("<response=0,10>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCops() {
	Command command(NULL);
	command.set(Command::Type_CopsData);
	command.setText() << "AT+COPS?";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+COPS?\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("+COPS: 0,1,\"BeeLine\"\r\n");
	uart->addRecvString("OK\r\n");
	TEST_STRING_EQUAL("<response=0,BeeLine>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCreg() {
	Command command(NULL);
	command.set(Command::Type_CregData);
	command.setText() << "AT+CREG?";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CREG?\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("+CREG: 1,1\r\n");
	uart->addRecvString("OK\r\n");
	TEST_STRING_EQUAL("<response=0,+CREG: 1,1>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCipShut() {
	Command command(NULL);

	command.set(Command::Type_Result);
	command.setText() << "CIPSHUT";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("CIPSHUT\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("SHUT OK\r\n");
	TEST_STRING_EQUAL("<response=0,>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCipShutError() {
	Command command(NULL);

	command.set(Command::Type_Result);
	command.setText() << "AT+CIPSHUT";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPSHUT\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("ERROR\r\n");
	TEST_STRING_EQUAL("<response=2,>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCgatt() {
	Command command(NULL);

	command.set(Command::Type_CgattData);
	command.setText() << "AT+CGATT?";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CGATT?\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("+CGATT: 1\r\n");
	uart->addRecvString("OK\r\n");
	TEST_STRING_EQUAL("<response=0,1>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCifsr() {
	Command command(NULL);
	command.set(Command::Type_CifsrData);
	command.setText() << "AT+CIFSR";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIFSR\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("100.80.203.55\r\n");
	TEST_STRING_EQUAL("<response=0,100.80.203.55>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCifsrError() {
	Command command(NULL);
	command.set(Command::Type_CifsrData);
	command.setText() << "AT+CIFSR";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIFSR\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("ERROR\r\n");
	TEST_STRING_EQUAL("<response=2,>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCipPing() {
	// success
	Command command(NULL);
	command.set(Command::Type_CipPing);
	command.setText() << "AT+CIPPING=\"devs.ephor.online\",1";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPPING=\"devs.ephor.online\",1\r\n", (const char*)uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("+CIPPING: 1,\"84.42.23.196\",5,47\r\n");
	uart->addRecvString("OK\r\n");
	TEST_STRING_EQUAL("<response=0,5>", observer->getResult());
	observer->clear();

	// too long
	Command command2(NULL);
	command2.set(Command::Type_CipPing);
	command2.setText() << "AT+CIPPING=\"devs.ephor.online\",1";
	parser->execute(&command2);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPPING=\"devs.ephor.online\",1\r\n", (const char*)uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// command response
	uart->addRecvString("+CIPPING: 1,\"84.42.23.196\",600,255\r\n");
	uart->addRecvString("OK\r\n");
	TEST_STRING_EQUAL("<response=2,600>", observer->getResult());
	observer->clear();

	// error
	Command command3(NULL);
	command3.set(Command::Type_CipPing);
	command3.setText() << "AT+CIPPING=\"devs.ephor.online\",1";
	parser->execute(&command3);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPPING=\"devs.ephor.online\",1\r\n", (const char*)uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// command response
	uart->addRecvString("ERROR\r\n");
	TEST_STRING_EQUAL("<response=2,>", observer->getResult());
	observer->clear();
	return true;
}

bool CommandParserTest::testCommandCipStatus() {
	Command command(NULL);
	command.set(Command::Type_CipStatus);
	command.setText() << "AT+CIPSTATUS";
	parser->execute(&command);
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPSTATUS\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	uart->addRecvString("OK\r\n");
	uart->addRecvString("\r\n");
	uart->addRecvString("STATE: IP STATUS\r\n");
	uart->addRecvString("\r\n");
	uart->addRecvString("C: 0,,\"\",\"\",\"\",\"INITIAL\"\r\n");
	uart->addRecvString("C: 1,,\"\",\"\",\"\",\"INITIAL\"\r\n");
	uart->addRecvString("C: 2,,\"\",\"\",\"\",\"INITIAL\"\r\n");
	uart->addRecvString("C: 3,,\"\",\"\",\"\",\"INITIAL\"\r\n");
	uart->addRecvString("C: 4,,\"\",\"\",\"\",\"INITIAL\"\r\n");
	uart->addRecvString("C: 5,,\"\",\"\",\"\",\"INITIAL\"\r\n");
	TEST_STRING_EQUAL("<response=0,IP STATUS>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCipStatusError() {
	Command command(NULL);
	command.set(Command::Type_CipStatus);
	command.setText() << "AT+CIPSTATUS";
	parser->execute(&command);
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPSTATUS\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("ERROR\r\n");
	TEST_STRING_EQUAL("<response=2,ERROR>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCipSendData() {
	uint8_t data[] = "0, SEND OK\r\n1, SEND OK\r\n";
	Command command(NULL);
	command.set(Command::Type_SendData);
	command.setText() << "AT+CIPSEND=1," << strlen((char*)data);
	command.setData(data, strlen((char*)data));
	parser->execute(&command);
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPSEND=1,24\r\n", (const char*)uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());

	// firestarter
	uart->addRecvString("> ");
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());

	// send after delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("0, SEND OK\r\n1, SEND OK\r\n", (const char*)uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("1, SEND OK\r\n");
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<response=0,1, SEND OK>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCipSendError() {
	uint8_t data[] = "0, SEND OK\r\n1, SEND OK\r\n";
	Command command(NULL);
	command.set(Command::Type_SendData);
	command.setText() << "AT+CIPSEND=1," << strlen((char*)data);
	command.setData(data, strlen((char*)data));
	parser->execute(&command);
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPSEND=1,24\r\n", (const char*)uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("ERROR\r\n");
	TEST_STRING_EQUAL("<event=ERROR>", observer->getResult());
	observer->clear();

	// error
	timerEngine->tick(AT_COMMAND_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("<response=6,NULL>", observer->getResult());
	observer->clear();
	return true;
}

bool CommandParserTest::testCommandCipSendDataUnwaitedEvent() {
	char data[] = "0, SEND OK\r\n1, SEND OK\r\n";
	Command command(NULL);
	command.set(Command::Type_SendData);
	command.setText() << "AT+CIPSEND=1," << strlen(data);
	command.setData((uint8_t*)data, strlen(data));
	parser->execute(&command);
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPSEND=1,24\r\n", (const char*)uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());

	// firestarter
	uart->addRecvString("> ");
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL(data, (const char*)uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());
	observer->clear();

	// unwaited event
	uart->addRecvString("\r\n+CIPRXGET: 1,0\r\n");
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<event=+CIPRXGET: 1,0>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("1, SEND OK\r\n");
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<response=0,1, SEND OK>", observer->getResult());
	observer->clear();
	return true;
}

bool CommandParserTest::testCommandCipSendDataUnwaitedEvent2() {
	char data[] = "0, SEND OK\r\n1, SEND OK\r\n";
	Command command(NULL);
	command.set(Command::Type_SendData);
	command.setText() << "AT+CIPSEND=1," << strlen(data);
	command.setData((uint8_t*)data, strlen(data));
	parser->execute(&command);
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPSEND=1,24\r\n", (const char*)uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());

	// firestarter
	uart->addRecvString("> ");

	// unwaited event
	uart->addRecvString("\r\n+CIPRXGET: 1,0\r\n");
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<event=+CIPRXGET: 1,0>", observer->getResult());
	observer->clear();

	// unwaited event
	timerEngine->tick(AT_CIPSEND_SEND_DELAY);
	timerEngine->execute();
	uart->addRecvString("\r\n+CIPRXGET: 1,1\r\n");
	TEST_SUBSTR_EQUAL(data, (const char*)uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<event=+CIPRXGET: 1,1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("1, SEND OK\r\n");
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<response=0,1, SEND OK>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCipRxGet() {
	// +CIPRXGET
	uart->addRecvString("+CIPRXGET: 2,0,351,0\r\n");
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<event=+CIPRXGET: 2,0,351,0>", observer->getResult());
	observer->clear();

	// execute(AT+CIPRXGET)
	char data[] = "0, RECV OK\r\n1, RECV OK\r\n";
	uint8_t buf[32];
	Command command(NULL);
	command.set(Command::Type_RecvData);
	command.setText() << "AT+CIPRXGET=2,1," << sizeof(buf);
	command.setData(buf, sizeof(buf));
	parser->execute(&command);
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPRXGET=2,1,32\r\n", (const char*)uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());

	// recv info
	uart->addRecvString("+CIPRXGET: 2,1,24,0\r\n");
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// recv data
	uart->addRecvString(data);
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// confirm
	uart->addRecvString("OK\r\n");
	TEST_STRING_EQUAL("<response=0,+CIPRXGET: 2,1,24,0>", observer->getResult());
	TEST_SUBSTR_EQUAL(data, (char*)buf, (sizeof(data) - 1));
	observer->clear();
	return true;
}

bool CommandParserTest::testCommandCipRxGetError() {
	// +CIPRXGET
	uart->addRecvString("+CIPRXGET: 2,0,351,0\r\n");
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<event=+CIPRXGET: 2,0,351,0>", observer->getResult());
	observer->clear();

	// execute(AT+CIPRXGET)
	uint8_t buf[32];
	Command command(NULL);
	command.set(Command::Type_RecvData);
	command.setText() << "AT+CIPRXGET=2,1," << sizeof(buf);
	command.setData(buf, sizeof(buf));
	parser->execute(&command);
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// delay
	timerEngine->tick(AT_COMMAND_DELAY);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPRXGET=2,1,32\r\n", (const char*)uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// recv error
	uart->addRecvString("ERROR\r\n");
	TEST_STRING_EQUAL("<response=2,>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCipRxGetUnwaitedEvent() {
	// +CIPRXGET
	uart->addRecvString("+CIPRXGET: 1,0\r\n");
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<event=+CIPRXGET: 1,0>", observer->getResult());
	observer->clear();

	// +CIPRXGET
	uart->addRecvString("+CIPRXGET: 1,0\r\n");
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<event=+CIPRXGET: 1,0>", observer->getResult());
	observer->clear();

	// execute(AT+CIPRXGET)
	uint8_t buf[32];
	Command command(NULL);
	command.set(Command::Type_RecvData);
	command.setText() << "AT+CIPRXGET=2,1," << sizeof(buf);
	command.setData(buf, sizeof(buf));
	parser->execute(&command);
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// unwaited event
	uart->addRecvString("+CIPRXGET: 1,0\r\n");
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<event=+CIPRXGET: 1,0>", observer->getResult());
	observer->clear();

	// delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPRXGET=2,1,32\r\n", (const char*)uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());

	// recv info
	uart->addRecvString("+CIPRXGET: 2,1,24,0\r\n");
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// recv data
	char data[] = "0, RECV OK\r\n1, RECV OK\r\n";
	uart->addRecvString(data);
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// confirm
	uart->addRecvString("OK\r\n");
	TEST_STRING_EQUAL("<response=0,+CIPRXGET: 2,1,24,0>", observer->getResult());
	TEST_SUBSTR_EQUAL(data, (char*)buf, (sizeof(data) - 1));
	observer->clear();
	return true;
}

bool CommandParserTest::testCommandCipRxGetUnwaitedEvent2() {
	// +CIPRXGET
	uart->addRecvString("+CIPRXGET: 1,0\r\n");
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<event=+CIPRXGET: 1,0>", observer->getResult());
	observer->clear();

	// execute(AT+CIPRXGET)
	uint8_t buf[32];
	Command command(NULL);
	command.set(Command::Type_RecvData);
	command.setText() << "AT+CIPRXGET=2,1," << sizeof(buf);
	command.setData(buf, sizeof(buf));
	parser->execute(&command);
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// unwaited event
	uart->addRecvString("+CIPRXGET:");
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPRXGET=2,1,32\r\n", (const char*)uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());

	// unwaited event
	uart->addRecvString(" 1,0\r\n");
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<event=+CIPRXGET: 1,0>", observer->getResult());
	observer->clear();

	// recv info
	uart->addRecvString("+CIPRXGET: 2,1,24,0\r\n");
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// recv data
	char data[] = "0, RECV OK\r\n1, RECV OK\r\n";
	uart->addRecvString(data);
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// confirm
	uart->addRecvString("OK\r\n");
	TEST_STRING_EQUAL("<response=0,+CIPRXGET: 2,1,24,0>", observer->getResult());
	TEST_SUBSTR_EQUAL(data, (char*)buf, (sizeof(data) - 1));
	observer->clear();
	return true;
}

bool CommandParserTest::testCommandCipRxGetUnwaitedEvent3() {
	// +CIPRXGET
	uart->addRecvString("+CIPRXGET: 1,0\r\n");
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<event=+CIPRXGET: 1,0>", observer->getResult());
	observer->clear();

	// +CIPRXGET
	uart->addRecvString("+CIPRXGET: 1,0\r\n");
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("<event=+CIPRXGET: 1,0>", observer->getResult());
	observer->clear();

	// execute(AT+CIPRXGET)
	uint8_t buf[32];
	Command command(NULL);
	command.set(Command::Type_RecvData);
	command.setText() << "AT+CIPRXGET=2,1," << sizeof(buf);
	command.setData(buf, sizeof(buf));
	parser->execute(&command);
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPRXGET=2,1,32\r\n", (const char*)uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", observer->getResult());

	// recv info
	uart->addRecvString("+CIPRXGET: 2,1,24,0\r\n");
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// recv data
	char data[] = "0, RECV OK\r\n1, RECV OK\r\n";
	uart->addRecvString(data);
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("", observer->getResult());

	// unwaited event
	uart->addRecvString("+CIPRXGET: 1,0\r\n");
	TEST_SUBSTR_EQUAL("", (const char*)uart->getSendData(), uart->getSendLen());
	TEST_STRING_EQUAL("<event=+CIPRXGET: 1,0>", observer->getResult());
	observer->clear();

	// confirm
	uart->addRecvString("OK\r\n");
	TEST_STRING_EQUAL("<response=0,+CIPRXGET: 2,1,24,0>", observer->getResult());
	TEST_SUBSTR_EQUAL(data, (char*)buf, (sizeof(data) - 1));
	observer->clear();
	return true;
}

bool CommandParserTest::testCommandCipClose() {
	Command command(NULL);
	command.set(Command::Type_CipClose);
	command.setText() << "AT+CIPCLOSE=0,1";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPCLOSE=0,1\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("0, CLOSE OK\r\n");
	TEST_STRING_EQUAL("<response=0,0, CLOSE OK>", observer->getResult());
	return true;
}

bool CommandParserTest::testCommandCipCloseError() {
	Command command(NULL);
	command.set(Command::Type_CipClose);
	command.setText() << "AT+CIPCLOSE=0,1";
	parser->execute(&command);

	// command delay
	timerEngine->tick(100);
	timerEngine->execute();
	TEST_SUBSTR_EQUAL("AT+CIPCLOSE=0,1\r\n", (const char*)uart->getSendData(), uart->getSendLen());

	// unwaited event
	uart->addRecvString("+CREG: 1\r\n");
	TEST_STRING_EQUAL("<event=+CREG: 1>", observer->getResult());
	observer->clear();

	// command response
	uart->addRecvString("ERROR\r\n");
	TEST_STRING_EQUAL("<response=2,>", observer->getResult());
	return true;
}

}
