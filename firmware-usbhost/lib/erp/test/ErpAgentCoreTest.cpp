#include "lib/erp/ErpAgentCore.h"
#include "lib/modem/ConfigMaster.h"

#include "common/memory/include/RamMemory.h"
#include "common/config/include/ConfigModem.h"
#include "common/http/include/TestHttp.h"
#include "common/http/include/HttpClient.h"
#include "common/event/include/TestEventEngine.h"
#include "common/timer/include/TestRealTime.h"
#include "common/utils/include/TestLed.h"
#include "common/sim900/test/TestGsmSignalQuality.h"
#include "common/logger/include/Logger.h"
#include "common/test/include/Test.h"

class TestErpAgentCoreEcp: public EcpInterface {
public:
	virtual bool loadAudit(EventObserver *) override { return true; }
};

class TestErpAgentCoreEventEngine : public TestEventEngine {
public:
	TestErpAgentCoreEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}
};

class TestErpProtocol : public ErpProtocolInterface {
public:
	TestErpProtocol(StringBuilder *result) : result(result) {}
	virtual void setObserver(EventObserver *observer) { (void)observer; }
	virtual bool sendAuditRequest(const char *, StringBuilder *) {
		*result << "<audit>";
		return true;
	}
	virtual bool sendConfigRequest(const char *, StringBuilder *) {
		*result << "<config>";
		return true;
	}
	virtual bool sendSyncRequest(const char *login, uint32_t decimalPoint, uint16_t signalQuality, ConfigEventList *events, StringBuilder *reqData) {
		(void)login;
		(void)decimalPoint;
		(void)signalQuality;
		(void)events;
		(void)reqData;
		*result << "<sync>";
		return true;
	}
	virtual bool sendPingRequest(const char *login, uint32_t decimalPoint, StringBuilder *reqData) {
		*result << "<ping>";
		return true;
	}

private:
	StringBuilder *result;
};

class TestGramophone : public GramophoneInterface {
public:
	TestGramophone(StringBuilder *result) : result(result) {}
	virtual void play(Melody *, EventObserver * = NULL) { *result << "<melody>"; }

private:
	StringBuilder *result;
};

class ErpAgentCoreTest : public TestSet {
public:
	ErpAgentCoreTest();
	bool init();
	void cleanup();
	bool testSyncAndPing();
	bool testSync();
	bool testSyncHeartBitError();
	bool testAuditButton();
	bool testAuditButtonWhenSync();

private:
	StringBuilder *result;
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	ConfigMaster *master;
	ConfigModem *config;
	TimerEngine *timerEngine;
	TestErpAgentCoreEventEngine *eventEngine;
	TestGramophone *gramophone;
	Gsm::SignalQualityInterface *signalQuality;
	TestErpAgentCoreEcp *ecp;
	TestLed *leds;
	TestErpProtocol *erp;
	ErpAgentCore *core;

	void initConfig();
	bool deliverEvent(EventInterface *event);
};

TEST_SET_REGISTER(ErpAgentCoreTest);

ErpAgentCoreTest::ErpAgentCoreTest() {
	TEST_CASE_REGISTER(ErpAgentCoreTest, testSyncAndPing);
	TEST_CASE_REGISTER(ErpAgentCoreTest, testSync);
	TEST_CASE_REGISTER(ErpAgentCoreTest, testSyncHeartBitError);
	TEST_CASE_REGISTER(ErpAgentCoreTest, testAuditButton);
	TEST_CASE_REGISTER(ErpAgentCoreTest, testAuditButtonWhenSync);
}

bool ErpAgentCoreTest::init() {
	result = new StringBuilder(2048, 2048);
	realtime = new TestRealTime;
	DateTime datetime(15, 1, 1);
	realtime->setDateTime(&datetime);
	initConfig();
	timerEngine = new TimerEngine;
	eventEngine = new TestErpAgentCoreEventEngine(result);
	gramophone = new TestGramophone(result);
	signalQuality = new Gsm::TestSignalQuality(1, result);
	ecp = new TestErpAgentCoreEcp;
	leds = new TestLed(result);
	erp = new TestErpProtocol(result);
	core = new ErpAgentCore(master, timerEngine, eventEngine, NULL, signalQuality, erp, ecp, gramophone, realtime, NULL, NULL, leds);
	return true;
}

void ErpAgentCoreTest::initConfig() {
	memory = new RamMemory(64000);
	stat = new StatStorage;
	config = new ConfigModem(memory, realtime, stat);
	config->init();
	master = new ConfigMaster(config);

	ConfigBoot *configBoot = config->getBoot();
	configBoot->setHardwareVersion(0x01000002);
	configBoot->setFirmwareVersion(0x01020003);
	configBoot->setServerDomain("erp.ephor.online");
	configBoot->setServerPort(443);
	configBoot->setServerPassword("password");
}

void ErpAgentCoreTest::cleanup() {
	delete core;
	delete erp;
	delete leds;
	delete gramophone;
	delete eventEngine;
	delete timerEngine;
	delete master;
	delete config;
	delete stat;
	delete memory;
	delete realtime;
	delete result;
}

bool ErpAgentCoreTest::deliverEvent(EventInterface *event) {
	EventEnvelope envelope(EVENT_DATA_SIZE);
	if(event->pack(&envelope) == false) { return false; }
	core->proc(&envelope);
	return true;
}

bool ErpAgentCoreTest::testSyncAndPing() {
	core->reset();
	TEST_STRING_EQUAL("<subscribe=3072><ledServer=0>", result->getString());
	result->clear();

	// Wait 10 minutes
	timerEngine->tick(10*60*1000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<GSM::SQ::get>", result->getString());
	result->clear();

	// Signal Quality response
	EventUint16Interface event1(signalQuality->getDeviceId(), Gsm::Driver::Event_SignalQuality, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("<sync><ledServer=1>", result->getString());
	result->clear();

	// Recv Sync Response
	DateTime datetime1(15, 1, 1);
	ErpProtocol::EventSync event2(Http::Client::Event_RequestComplete, datetime1, 0, 0);
	core->proc(&event2);
	TEST_STRING_EQUAL("<ledServer=2>", result->getString());
	result->clear();

	DateTime datetime2;
	realtime->getDateTime(&datetime2);
	TEST_NUMBER_EQUAL(2015, datetime2.getYear());
	TEST_NUMBER_EQUAL(ConfigBoot::FirmwareState_None, config->getBoot()->getFirmwareState());

	// Wait 55 minutes
	for(uint16_t i = 0; i < 11; i++) {
		timerEngine->tick(55*60*1000);
		timerEngine->execute();
		TEST_STRING_EQUAL("<ping><ledServer=1>", result->getString());
		result->clear();

		// Recv Ping Response
		DateTime datetime1(15, 1, 1);
		ErpProtocol::EventSync event2(Http::Client::Event_RequestComplete, datetime1, 0, 0);
		core->proc(&event2);
		TEST_STRING_EQUAL("<ledServer=2>", result->getString());
		result->clear();
	}
	result->clear();

	// Wait 5 minutes
	timerEngine->tick(5*60*1000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<GSM::SQ::get>", result->getString());
	result->clear();
	return true;
}

bool ErpAgentCoreTest::testSync() {
	core->reset();
	TEST_STRING_EQUAL("<subscribe=3072><ledServer=0>", result->getString());
	result->clear();

	// Wait 10 minutes
	timerEngine->tick(10*60*1000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<GSM::SQ::get>", result->getString());
	result->clear();

	// Signal Quality response
	EventUint16Interface event1(signalQuality->getDeviceId(), Gsm::Driver::Event_SignalQuality, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("<sync><ledServer=1>", result->getString());
	result->clear();

	// Recv Sync Response
	DateTime datetime1(15, 1, 1);
	ErpProtocol::EventSync event2(Http::Client::Event_RequestComplete, datetime1, 0, 0);
	core->proc(&event2);

	DateTime datetime2;
	realtime->getDateTime(&datetime2);
	TEST_NUMBER_EQUAL(2015, datetime2.getYear());
	TEST_NUMBER_EQUAL(ConfigBoot::FirmwareState_None, config->getBoot()->getFirmwareState());

	// Wait 55 minutes
	for(uint16_t i = 0; i < 11; i++) {
		timerEngine->tick(55*60*1000);
		timerEngine->execute();
		TEST_STRING_EQUAL("<ledServer=2>", result->getString());
	}
	result->clear();

	// Wait 5 minutes
	timerEngine->tick(5*60*1000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<GSM::SQ::get>", result->getString());
	result->clear();

	// Signal Quality response
	EventUint16Interface event3(signalQuality->getDeviceId(), Gsm::Driver::Event_SignalQuality, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event3));
	TEST_STRING_EQUAL("<sync><ledServer=1>", result->getString());
	result->clear();

	// Recv Sync Response
	ErpProtocol::EventSync event4(Http::Client::Event_RequestComplete, datetime1, 0, 0);
	core->proc(&event4);

	// Wait 20 minutes
	for(uint16_t i = 0; i < 4; i++) {
		timerEngine->tick(55*60*1000);
		timerEngine->execute();
		TEST_STRING_EQUAL("<ledServer=2>", result->getString());
	}
	result->clear();

	// Wait 5 minutes
	config->getEvents()->add(ConfigEvent::Type_FiscalNotInited);
	timerEngine->tick(5*60*1000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<GSM::SQ::get>", result->getString());
	result->clear();

	// Signal Quality response
	EventUint16Interface event5(signalQuality->getDeviceId(), Gsm::Driver::Event_SignalQuality, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event5));
	TEST_STRING_EQUAL("<sync><ledServer=1>", result->getString());
	result->clear();

	// Recv Sync Response
	config->getEvents()->clear();
	ErpProtocol::EventSync event6(Http::Client::Event_RequestComplete, datetime1, 0, 0);
	core->proc(&event6);

	// Wait 55 minutes
	for(uint16_t i = 0; i < 11; i++) {
		timerEngine->tick(55*60*1000);
		timerEngine->execute();
		TEST_STRING_EQUAL("<ledServer=2>", result->getString());
	}
	result->clear();

	// Wait 5 minutes
	timerEngine->tick(5*60*1000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<GSM::SQ::get>", result->getString());
	result->clear();

	// Signal Quality response
	EventUint16Interface event7(signalQuality->getDeviceId(), Gsm::Driver::Event_SignalQuality, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event7));
	TEST_STRING_EQUAL("<sync><ledServer=1>", result->getString());
	result->clear();

	// Recv Sync Response
	DateTime datetime3(18, 1, 1);
	ErpProtocol::EventSync event8(Http::Client::Event_RequestComplete, datetime3, 0x01, 0);
	core->proc(&event8);

	DateTime datetime4;
	realtime->getDateTime(&datetime4);
	TEST_NUMBER_EQUAL(2018, datetime4.getYear());
	TEST_NUMBER_EQUAL(ConfigBoot::FirmwareState_UpdateRequired, config->getBoot()->getFirmwareState());
	return true;
}

bool ErpAgentCoreTest::testSyncHeartBitError() {
	core->reset();
	TEST_STRING_EQUAL("<subscribe=3072><ledServer=0>", result->getString());
	result->clear();

	// Wait 10 minutes
	timerEngine->tick(10*60*1000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<GSM::SQ::get>", result->getString());
	result->clear();

	// Signal Quality response
	EventUint16Interface event1(signalQuality->getDeviceId(), Gsm::Driver::Event_SignalQuality, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("<sync><ledServer=1>", result->getString());
	result->clear();

	// Recv Sync Response
	Event event2(Http::Client::Event_RequestError);
	core->proc(&event2);
	TEST_STRING_EQUAL("<ledServer=3>", result->getString());
	result->clear();

	// Wait 5 minutes
	timerEngine->tick(5*60*1000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<GSM::SQ::get>", result->getString());
	result->clear();

	// Signal Quality response
	EventUint16Interface event3(signalQuality->getDeviceId(), Gsm::Driver::Event_SignalQuality, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event3));
	TEST_STRING_EQUAL("<sync><ledServer=1>", result->getString());
	result->clear();

	// Recv Sync Response
	DateTime datetime2(15, 1, 1);
	ErpProtocol::EventSync event6(Http::Client::Event_RequestComplete, datetime2, 0, 0);
	core->proc(&event6);
	TEST_STRING_EQUAL("<ledServer=2>", result->getString());
	result->clear();
	return true;
}

bool ErpAgentCoreTest::testAuditButton() {
	core->reset();
	TEST_STRING_EQUAL("<subscribe=3072><ledServer=0>", result->getString());
	result->clear();

	// Wait 10 minutes
	timerEngine->tick(10*60*1000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<GSM::SQ::get>", result->getString());
	result->clear();

	// Signal Quality response
	EventUint16Interface event1(signalQuality->getDeviceId(), Gsm::Driver::Event_SignalQuality, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("<sync><ledServer=1>", result->getString());
	result->clear();

	// Recv Sync Response
	DateTime datetime1(15, 1, 1);
	ErpProtocol::EventSync event2(Http::Client::Event_RequestComplete, datetime1, 0, 0);
	core->proc(&event2);
	TEST_STRING_EQUAL("<ledServer=2>", result->getString());
	result->clear();

	// Push audit button
	core->sendAudit();
	TEST_STRING_EQUAL("<ledServer=1>", result->getString());

	// Generate audit
	timerEngine->tick(1);
	timerEngine->execute();
	timerEngine->tick(1);
	timerEngine->execute();
	timerEngine->tick(1);
	timerEngine->execute();
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("<ledServer=1><audit><ledServer=1>", result->getString());
	result->clear();

	// Recv Audit Response
	Event event3(Http::Client::Event_RequestComplete, (uint32_t)1);
	core->proc(&event3);
	TEST_STRING_EQUAL("<config><ledServer=1>", result->getString());
	result->clear();
#if 0 //todo: спрятать генерацию аудита и обновление конфига в ConfigMaster
	// Recv Config Response
	Event event4(Http::Client::Event_RequestComplete);
	core->proc(&event4);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("<melody>", gramophone->getResult());
	gramophone->clearResult();

	// End melody
	Event event5(GramophoneInterface::Event_Complete);
	core->proc(&event5);
	TEST_STRING_EQUAL("", result->getString());

	// Wait 10 minutes
	timers->tick(10*60*1000);
	timers->execute();
	TEST_STRING_EQUAL("<sync>", result->getString());
	result->clear();
#endif
	return true;
}

bool ErpAgentCoreTest::testAuditButtonWhenSync() {
	core->reset();
	TEST_STRING_EQUAL("<subscribe=3072><ledServer=0>", result->getString());
	result->clear();

	// Wait 10 minutes
	timerEngine->tick(10*60*1000);
	timerEngine->execute();
	TEST_STRING_EQUAL("<GSM::SQ::get>", result->getString());
	result->clear();

	// Signal Quality response
	EventUint16Interface event1(signalQuality->getDeviceId(), Gsm::Driver::Event_SignalQuality, 70);
	TEST_NUMBER_EQUAL(true, deliverEvent(&event1));
	TEST_STRING_EQUAL("<sync><ledServer=1>", result->getString());
	result->clear();

	// Push audit button
	core->sendAudit();
	TEST_STRING_EQUAL("", result->getString());

	// Recv Sync Response
	DateTime datetime1(15, 1, 1);
	ErpProtocol::EventSync event2(Http::Client::Event_RequestComplete, datetime1, 0, 0);
	core->proc(&event2);
	TEST_STRING_EQUAL("<ledServer=2><ledServer=1>", result->getString());
	result->clear();

	// Generate audit
	timerEngine->tick(1);
	timerEngine->execute();
	timerEngine->tick(1);
	timerEngine->execute();
	timerEngine->tick(1);
	timerEngine->execute();
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("<audit><ledServer=1>", result->getString());
	result->clear();

	// Recv Audit Response
	Event event3(Http::Client::Event_RequestComplete, (uint32_t)1);
	core->proc(&event3);
	TEST_STRING_EQUAL("<config><ledServer=1>", result->getString());
	result->clear();
	return true;
}
