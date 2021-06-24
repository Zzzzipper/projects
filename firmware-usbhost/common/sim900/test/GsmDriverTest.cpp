#include "sim900/include/GsmDriver.h"
#include "sim900/test/TestGsmHardware.h"
#include "config/include/ConfigModem.h"
#include "timer/include/TimerEngine.h"
#include "uart/include/TestUart.h"
#include "event/include/TestEventEngine.h"
#include "mdb/MdbProtocol.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

namespace Gsm {

class TestDriverEventEngine : public TestEventEngine {
public:
	TestDriverEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case Driver::Event_NetworkUp: procEvent(envelope, Driver::Event_NetworkUp, "NetworkUp"); break;
		case Driver::Event_NetworkDown: procEvent(envelope, Driver::Event_NetworkDown, "NetworkDown"); break;
		case Driver::Event_SignalQuality: procEvent(envelope, Driver::Event_SignalQuality, "SignalQuality"); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}
};

class TestCommandParser : public CommandParserInterface {
public:
	virtual void setObserver(Command::Observer *observer) {
		this->observer = observer;
	}

	virtual void reset() {
		send << "<reset>";
	}

	virtual bool execute(Command *command) {
		send << "<command=";
		switch(command->getType()) {
		case Command::Type_Result: send << "R,"; break;
		case Command::Type_GsnData: send << "GSN,"; break;
		case Command::Type_CgmrData: send << "CGMR,"; break;
		case Command::Type_CcidData: send << "CCID,"; break;
		case Command::Type_CopsData: send << "COPS,"; break;
		case Command::Type_CregData: send << "CREG,"; break;
		case Command::Type_CgattData: send << "CGATT,"; break;
		case Command::Type_CifsrData: send << "CIFSR,"; break;
		case Command::Type_CipStatus: send << "CIPSTATUS,"; break;
		case Command::Type_SendData: send << "SEND,"; break;
		case Command::Type_RecvData: send << "RECV,"; break;
		case Command::Type_CipClose: send << "CIPCLOSE,"; break;
		default: send << "#ERROR";
		}
		send << command->getText() << ",";
		send.addStr((char*)command->getData(), command->getDataLen());
		send << "," << command->getTimeout() << ">";
		return true;
	}

	void clearSend() { send.clear(); }
	const char *getSend() { return send.getString(); }
	void recvResponse(Command::Result result, const char *data) { observer->procResponse(result, data); }
	void recvEvent(const char *data) { observer->procEvent(data); }

private:
	Command::Observer *observer;
	StringBuilder send;
};

class DriverTest : public TestSet {
public:
	DriverTest();
	bool init();
	void cleanup();
	bool testInit();
	bool gotoStateCommandReady();
	bool restartReg();
	bool upReg();
	bool upGprs();
	bool testStateCommandReadyReset();
	bool testStateCommandExecutionReset();
	bool testStateCommandReadyNext();
	bool testResetCfunTimeout();

private:
	StringBuilder *result;
	ConfigBoot *config;
	Gsm::TestHardware *hardware;
	TestCommandParser *parser;
	TimerEngine *timerEngine;
	TestDriverEventEngine *eventEngine;
	StatStorage *stat;
	Driver *driver;
};

TEST_SET_REGISTER(Gsm::DriverTest);

DriverTest::DriverTest() {
	TEST_CASE_REGISTER(DriverTest, testInit);
	TEST_CASE_REGISTER(DriverTest, testStateCommandReadyReset);
	TEST_CASE_REGISTER(DriverTest, testStateCommandExecutionReset);
	TEST_CASE_REGISTER(DriverTest, testStateCommandReadyNext);
	TEST_CASE_REGISTER(DriverTest, testResetCfunTimeout);
}

bool DriverTest::init() {
	result = new StringBuilder;
	config = new ConfigBoot;
	hardware = new Gsm::TestHardware(result);
	parser = new TestCommandParser;
	timerEngine = new TimerEngine;
	eventEngine = new TestDriverEventEngine(result);
	stat = new StatStorage;
	driver = new Driver(config, hardware, parser, timerEngine, eventEngine, stat);
	return true;
}

void DriverTest::cleanup() {
	delete driver;
	delete stat;
	delete eventEngine;
	delete timerEngine;
	delete parser;
	delete hardware;
	delete config;
	delete result;
}

bool DriverTest::testInit() {
	TEST_NUMBER_EQUAL(true, driver->init(NULL));
	TEST_STRING_EQUAL("<Gsm::Hardware::init><Gsm::Hardware::pressPowerButton>", result->getString());
	result->clear();

	// Power button press timeout
	timerEngine->tick(POWER_BUTTON_PRESS);
	timerEngine->execute();
	TEST_STRING_EQUAL("<Gsm::Hardware::releasePowerButton>", result->getString());
	result->clear();

	// Power up check
	timerEngine->tick(POWER_SHUTDOWN_CHECK);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());

	// Power up check
	hardware->setStatusUp(true);
	timerEngine->tick(POWER_SHUTDOWN_CHECK);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());

	// AT
	TEST_STRING_EQUAL("<reset><command=R,AT,,500>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// ATZ
	TEST_STRING_EQUAL("<command=R,ATZ,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// ATE
	TEST_STRING_EQUAL("<command=R,ATE0,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+COPS
	TEST_STRING_EQUAL("<command=R,AT+COPS=3,1,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CGMR
	TEST_STRING_EQUAL("<command=CGMR,AT+CGMR,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "1418B08SIM800C32_BT_EAT");

	// AT+GSN
	TEST_STRING_EQUAL("<command=GSN,AT+GSN,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "123456789012345");

	// AT+CPIN?
	TEST_STRING_EQUAL("<command=R,AT+CPIN?,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "READY");

	// AT+CCID
	TEST_STRING_EQUAL("<command=CCID,AT+CCID,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "898600810906F8048812");

	// AT+CREG=1
	TEST_STRING_EQUAL("<command=R,AT+CREG=1,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CREG?
	TEST_STRING_EQUAL("<command=CREG,AT+CREG?,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "+CREG: 1,1");

	// AT+CIPSHUT
	TEST_STRING_EQUAL("<command=R,AT+CIPSHUT,,10000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CGATT?
	TEST_STRING_EQUAL("<command=CGATT,AT+CGATT?,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "1");

	// AT+CIPRXGET=1
	TEST_STRING_EQUAL("<command=R,AT+CIPRXGET=1,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CIPRXGET=1
	TEST_STRING_EQUAL("<command=R,AT+CIPMUX=1,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CSTT=internet,gdata,gdata
	TEST_STRING_EQUAL("<command=R,AT+CSTT=internet,gdata,gdata,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CIICR
	TEST_STRING_EQUAL("<command=R,AT+CIICR,,40000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CIFSR
	TEST_STRING_EQUAL("<command=CIFSR,AT+CIFSR,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+COPS?
	TEST_STRING_EQUAL("<command=COPS,AT+COPS?,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "MegaFon");
	TEST_STRING_EQUAL("<event=1,NetworkUp>", result->getString());

	TEST_NUMBER_EQUAL(27, stat->get(Mdb::DeviceContext::Info_Gsm_State)->get());
	TEST_NUMBER_EQUAL(1, stat->get(Mdb::DeviceContext::Info_Gsm_HardResetCount)->get());
	TEST_NUMBER_EQUAL(0, stat->get(Mdb::DeviceContext::Info_Gsm_SoftResetCount)->get());
	TEST_NUMBER_EQUAL(0, stat->get(Mdb::DeviceContext::Info_Gsm_CommandMax)->get());

	TEST_STRING_EQUAL("123456789012345", config->getImei());
	TEST_STRING_EQUAL("898600810906F8048812", config->getIccid());
	TEST_STRING_EQUAL("1418B08SIM800C32_BT_EAT", config->getGsmFirmwareVersion());
	return true;
}

bool DriverTest::gotoStateCommandReady() {
	TEST_NUMBER_EQUAL(true, driver->init(NULL));
	TEST_STRING_EQUAL("<Gsm::Hardware::init><Gsm::Hardware::pressPowerButton>", result->getString());
	result->clear();

	// Power button press timeout
	timerEngine->tick(POWER_BUTTON_PRESS);
	timerEngine->execute();
	TEST_STRING_EQUAL("<Gsm::Hardware::releasePowerButton>", result->getString());
	result->clear();

	// Power up check
	hardware->setStatusUp(true);
	timerEngine->tick(POWER_SHUTDOWN_CHECK);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());

	// AT
	TEST_STRING_EQUAL("<reset><command=R,AT,,500>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// ATZ
	TEST_STRING_EQUAL("<command=R,ATZ,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// ATE
	TEST_STRING_EQUAL("<command=R,ATE0,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+COPS
	TEST_STRING_EQUAL("<command=R,AT+COPS=3,1,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CGMR
	TEST_STRING_EQUAL("<command=CGMR,AT+CGMR,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "Revision:1418B08SIM800C32_BT_EAT");

	// AT+GSN
	TEST_STRING_EQUAL("<command=GSN,AT+GSN,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "123456789012345");

	// AT+CPIN?
	TEST_STRING_EQUAL("<command=R,AT+CPIN?,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "READY");

	// AT+CCID
	TEST_STRING_EQUAL("<command=CCID,AT+CCID,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "898600810906F8048812");

	// AT+CREG=1
	TEST_STRING_EQUAL("<command=R,AT+CREG=1,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CREG?
	TEST_STRING_EQUAL("<command=CREG,AT+CREG?,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "+CREG: 1,1");

	// AT+CIPSHUT
	TEST_STRING_EQUAL("<command=R,AT+CIPSHUT,,10000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CGATT?
	TEST_STRING_EQUAL("<command=CGATT,AT+CGATT?,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "1");

	// AT+CIPRXGET=1
	TEST_STRING_EQUAL("<command=R,AT+CIPRXGET=1,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CIPMUX=1
	TEST_STRING_EQUAL("<command=R,AT+CIPMUX=1,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CSTT=internet,gdata,gdata
	TEST_STRING_EQUAL("<command=R,AT+CSTT=internet,gdata,gdata,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CIICR
	TEST_STRING_EQUAL("<command=R,AT+CIICR,,40000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CIFSR
	TEST_STRING_EQUAL("<command=CIFSR,AT+CIFSR,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+COPS?
	TEST_STRING_EQUAL("<command=COPS,AT+COPS?,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "MegaFon");
	TEST_STRING_EQUAL("<event=1,NetworkUp>", result->getString());
	result->clear();
	return true;
}

bool DriverTest::restartReg() {
	// AT+CFUN=0
	TEST_STRING_EQUAL("<command=R,AT+CFUN=0,,10000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CFUN=1
	TEST_STRING_EQUAL("<command=R,AT+CFUN=1,,10000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");
	return true;
}

bool DriverTest::upReg() {
	// AT
	TEST_STRING_EQUAL("<reset><command=R,AT,,500>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// ATZ
	TEST_STRING_EQUAL("<command=R,ATZ,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// ATE
	TEST_STRING_EQUAL("<command=R,ATE0,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+COPS
	TEST_STRING_EQUAL("<command=R,AT+COPS=3,1,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CGMR
	TEST_STRING_EQUAL("<command=CGMR,AT+CGMR,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "Revision:1418B08SIM800C32_BT_EAT");

	// AT+GSN
	TEST_STRING_EQUAL("<command=GSN,AT+GSN,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "123456789012345");

	// AT+CPIN?
	TEST_STRING_EQUAL("<command=R,AT+CPIN?,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "READY");

	// AT+CCID
	TEST_STRING_EQUAL("<command=CCID,AT+CCID,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "898600810906F8048812");

	// AT+CREG=1
	TEST_STRING_EQUAL("<command=R,AT+CREG=1,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CREG?
	TEST_STRING_EQUAL("<command=CREG,AT+CREG?,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "+CREG: 1,1");
	return true;
}

bool DriverTest::upGprs() {
	// AT+CIPSHUT
	TEST_STRING_EQUAL("<command=R,AT+CIPSHUT,,10000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CGATT?
	TEST_STRING_EQUAL("<command=CGATT,AT+CGATT?,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "1");

	// AT+CIPRXGET=1
	TEST_STRING_EQUAL("<command=R,AT+CIPRXGET=1,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CIPRXGET=1
	TEST_STRING_EQUAL("<command=R,AT+CIPMUX=1,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CSTT=internet,gdata,gdata
	TEST_STRING_EQUAL("<command=R,AT+CSTT=internet,gdata,gdata,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CIICR
	TEST_STRING_EQUAL("<command=R,AT+CIICR,,40000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+CIFSR
	TEST_STRING_EQUAL("<command=CIFSR,AT+CIFSR,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");

	// AT+COPS?
	TEST_STRING_EQUAL("<command=COPS,AT+COPS?,,5000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "MegaFon");
	TEST_STRING_EQUAL("<event=1,NetworkUp>", result->getString());
	return true;
}

bool DriverTest::testStateCommandReadyReset() {
	TEST_NUMBER_EQUAL(true, gotoStateCommandReady());

	driver->reset();
	TEST_STRING_EQUAL("<event=1,NetworkDown>", result->getString());
	result->clear();

	// restart REG
	TEST_NUMBER_EQUAL(true, restartReg());

	// up REG
	TEST_NUMBER_EQUAL(true, upReg());

	// up GPRS
	TEST_NUMBER_EQUAL(true, upGprs());
	return true;
}

bool DriverTest::testStateCommandExecutionReset() {
	TEST_NUMBER_EQUAL(true, gotoStateCommandReady());

	// add first command
	Command command1(NULL);
	command1.set(Command::Type_Result, "AT+CIPSSL=1");
	TEST_NUMBER_EQUAL(true, driver->execute(&command1));

	// reset
	driver->reset();
	TEST_STRING_EQUAL("", result->getString());

	// first command result
	parser->clearSend();
	parser->recvResponse(Command::Result_ERROR, "");
	TEST_STRING_EQUAL("<event=1,NetworkDown>", result->getString());
	result->clear();

	// restart REG
	TEST_NUMBER_EQUAL(true, restartReg());

	// up REG
	TEST_NUMBER_EQUAL(true, upReg());

	// up GPRS
	TEST_NUMBER_EQUAL(true, upGprs());
	return true;
}

bool DriverTest::testStateCommandReadyNext() {
	TEST_NUMBER_EQUAL(true, gotoStateCommandReady());

	// add first command
	Command command1(NULL);
	command1.set(Command::Type_Result, "AT+CIPSSL=0");
	TEST_NUMBER_EQUAL(true, driver->execute(&command1));

	// add second command
	Command command2(NULL);
	command2.set(Command::Type_Result, "AT+CIPSSL=1");
	TEST_NUMBER_EQUAL(true, driver->execute(&command2));

	// first command result
	parser->clearSend();
	parser->recvResponse(Command::Result_OK, "");
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// reset
	driver->reset();
	TEST_STRING_EQUAL("<event=1,NetworkDown>", result->getString());
	result->clear();

	// restart REG
	TEST_NUMBER_EQUAL(true, restartReg());

	// up REG
	TEST_NUMBER_EQUAL(true, upReg());

	// up GPRS
	TEST_NUMBER_EQUAL(true, upGprs());
	return true;
}

bool DriverTest::testResetCfunTimeout() {
	TEST_NUMBER_EQUAL(true, gotoStateCommandReady());

	driver->reset();
	TEST_STRING_EQUAL("<event=1,NetworkDown>", result->getString());
	result->clear();

	// AT+CFUN=0
	TEST_STRING_EQUAL("<command=R,AT+CFUN=0,,10000>", parser->getSend());
	parser->clearSend();
	parser->recvResponse(Command::Result_TIMEOUT, NULL);
	TEST_STRING_EQUAL("<Gsm::Hardware::pressPowerButton>", result->getString());
	result->clear();

	// Power down check
	timerEngine->tick(POWER_SHUTDOWN_CHECK);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Power down check
	timerEngine->tick(POWER_SHUTDOWN_CHECK);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Power down check
	hardware->setStatusUp(false);
	timerEngine->tick(POWER_SHUTDOWN_CHECK);
	timerEngine->execute();
	TEST_STRING_EQUAL("<Gsm::Hardware::releasePowerButton>", result->getString());
	result->clear();

	// Power off delay
	timerEngine->tick(POWER_SHUTDOWN_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("<Gsm::Hardware::pressPowerButton>", result->getString());
	result->clear();

	// Power button delay
	timerEngine->tick(POWER_BUTTON_PRESS);
	timerEngine->execute();
	TEST_STRING_EQUAL("<Gsm::Hardware::releasePowerButton>", result->getString());
	result->clear();

	// Power up check
	hardware->setStatusUp(true);
	timerEngine->tick(POWER_SHUTDOWN_CHECK);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());

	// up REG
	TEST_NUMBER_EQUAL(true, upReg());

	// up GPRS
	TEST_NUMBER_EQUAL(true, upGprs());
	return true;
}

}
