#include "lib/modem/EventRegistrar.h"
#include "lib/sale_manager/include/SaleManager.h"

#include "common/memory/include/RamMemory.h"
#include "common/timer/include/TestRealTime.h"
#include "common/event/include/TestEventEngine.h"
#include "common/test/include/Test.h"
#include "common/logger/include/Logger.h"

class TestEventRegistrarEventEngine : public TestEventEngine {
public:
	TestEventRegistrarEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventInterface *event) {
		switch(event->getType()) {
//		case MdbMasterBillValidator::Event_Ready: *result << "<event=Ready>"; break;
//		case MdbMasterBillValidator::Event_Error: procEventUint16(event, MdbMasterBillValidator::Event_Error, "Error"); break;
//		case MdbMasterBillValidator::Event_Deposite: procEventUint32(event, MdbMasterBillValidator::Event_Deposite, "Deposite"); break;
		default: TestEventEngine::transmit(event);
		}
		return true;
	}
};

class EventRegistrarTest : public TestSet {
public:
	EventRegistrarTest();
	bool init();
	void cleanup();
	bool testTooLongInit();
	bool testDisabledAfterEnabled();

private:
	StringBuilder *result;
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	ConfigModem *config;
	TimerEngine *timers;
	TestEventRegistrarEventEngine *eventEngine;
	EventRegistrar *registrar;
};

TEST_SET_REGISTER(EventRegistrarTest);

EventRegistrarTest::EventRegistrarTest() {
	TEST_CASE_REGISTER(EventRegistrarTest, testTooLongInit);
	TEST_CASE_REGISTER(EventRegistrarTest, testDisabledAfterEnabled);
}

bool EventRegistrarTest::init() {
	result = new StringBuilder(1024, 1024);
	memory = new RamMemory(32000),
	memory->setAddress(0);
	realtime = new TestRealTime;
	stat = new StatStorage;
	config = new ConfigModem(memory, realtime, stat);
	config->init();
	timers = new TimerEngine;
	eventEngine = new TestEventRegistrarEventEngine(result);
	registrar = new EventRegistrar(config, timers, eventEngine, realtime);
	return true;
}

void EventRegistrarTest::cleanup() {
	delete registrar;
	delete eventEngine;
	delete timers;
	delete config;
	delete stat;
	delete realtime;
	delete memory;
	delete result;
}

#if 1
bool EventRegistrarTest::testTooLongInit() {
	EventDeviceId deviceId((uint16_t)0);

	realtime->setDateTime("2018-01-01 10:30:50");
	registrar->reset();
	TEST_NUMBER_EQUAL(0, config->getEvents()->getLen());

	// Too long not enable
	timers->tick(AUTOMAT_DISABLE_TIMEOUT);
	timers->execute();

	TEST_NUMBER_EQUAL(1, config->getEvents()->getLen());
	uint16_t index = config->getEvents()->getLast();
	ConfigEvent event;
	config->getEvents()->get(index, &event);
	TEST_NUMBER_EQUAL(ConfigEvent::Type_SaleDisabled, event.getCode());
	StringBuilder datetime;
	datetime << LOG_DATETIME(*(event.getDate()));
	TEST_STRING_EQUAL("2018.1.1 10:30:50", datetime.getString());

	// Enable
	realtime->setDateTime("2018-01-01 12:00:50");
	EventEnvelope envelope1(50);
	EventUint8Interface event1(deviceId, SaleManager::Event_AutomatState, true);
	TEST_NUMBER_EQUAL(true, event1.pack(&envelope1));
	registrar->proc(&envelope1);

	TEST_NUMBER_EQUAL(2, config->getEvents()->getLen());
	index = config->getEvents()->getLast();
	config->getEvents()->get(index, &event);
	TEST_NUMBER_EQUAL(ConfigEvent::Type_SaleEnabled, event.getCode());
	datetime.clear();
	datetime << LOG_DATETIME(*(event.getDate()));
	TEST_STRING_EQUAL("2018.1.1 12:0:50", datetime.getString());
	return true;
}

bool EventRegistrarTest::testDisabledAfterEnabled() {
	EventDeviceId deviceId((uint16_t)0);

	registrar->reset();
	TEST_NUMBER_EQUAL(0, config->getEvents()->getLen());

	// Enable
	EventEnvelope envelope1(50);
	EventUint8Interface event1(deviceId, SaleManager::Event_AutomatState, true);
	TEST_NUMBER_EQUAL(true, event1.pack(&envelope1));
	registrar->proc(&envelope1);
	TEST_NUMBER_EQUAL(0, config->getEvents()->getLen());

	// Disable
	realtime->setDateTime("2018-01-02 10:30:50");
	EventEnvelope envelope2(50);
	EventUint8Interface event2(deviceId, SaleManager::Event_AutomatState, (uint8_t)false);
	TEST_NUMBER_EQUAL(true, event2.pack(&envelope2));
	registrar->proc(&envelope2);
	TEST_NUMBER_EQUAL(0, config->getEvents()->getLen());

	// Timeout
	timers->tick(AUTOMAT_DISABLE_TIMEOUT);
	timers->execute();

	TEST_NUMBER_EQUAL(1, config->getEvents()->getLen());
	uint16_t index = config->getEvents()->getLast();
	ConfigEvent event;
	config->getEvents()->get(index, &event);
	TEST_NUMBER_EQUAL(ConfigEvent::Type_SaleDisabled, event.getCode());
	StringBuilder datetime;
	datetime << LOG_DATETIME(*(event.getDate()));
	TEST_STRING_EQUAL("2018.1.2 10:30:50", datetime.getString());

	// Enable
	realtime->setDateTime("2018-01-02 12:00:50");
	EventEnvelope envelope3(50);
	EventUint8Interface event3(deviceId, SaleManager::Event_AutomatState, (uint8_t)true);
	TEST_NUMBER_EQUAL(true, event3.pack(&envelope3));
	registrar->proc(&envelope3);

	TEST_NUMBER_EQUAL(2, config->getEvents()->getLen());
	index = config->getEvents()->getLast();
	config->getEvents()->get(index, &event);
	TEST_NUMBER_EQUAL(ConfigEvent::Type_SaleEnabled, event.getCode());
	datetime.clear();
	datetime << LOG_DATETIME(*(event.getDate()));
	TEST_STRING_EQUAL("2018.1.2 12:0:50", datetime.getString());
	return true;
}
#else
bool EventRegistrarTest::testTooLongInit() {
	realtime->setDateTime("2018-01-01 10:30:50");
	registrar->reset();
	TEST_NUMBER_EQUAL(0, config->getLen());

	// Too long not enable
	timers->tick(AUTOMAT_DISABLE_TIMEOUT);
	timers->execute();

	TEST_NUMBER_EQUAL(1, config->getLen());
	uint16_t index = config->getLast();
	ConfigEvent event;
	config->get(index, &event);
	TEST_NUMBER_EQUAL(ConfigEvent::Type_SaleDisabled, event.getCode());
	StringBuilder datetime;
	datetime << LOG_DATETIME(*(event.getDate()));
	TEST_STRING_EQUAL("2018.1.1 10:30:50", datetime.getString());

	// Enable
	realtime->setDateTime("2018-01-01 12:00:50");
	Event event1(SaleManager::Event_AutomatState, (uint8_t)true);
	registrar->proc(&event1);

	TEST_NUMBER_EQUAL(2, config->getLen());
	index = config->getLast();
	config->get(index, &event);
	TEST_NUMBER_EQUAL(ConfigEvent::Type_SaleEnabled, event.getCode());
	datetime.clear();
	datetime << LOG_DATETIME(*(event.getDate()));
	TEST_STRING_EQUAL("2018.1.1 12:0:50", datetime.getString());
	return true;
}

bool EventRegistrarTest::testDisabledAfterEnabled() {
	registrar->reset();
	TEST_NUMBER_EQUAL(0, config->getLen());

	// Enable
	Event event1(SaleManager::Event_AutomatState, (uint8_t)true);
	registrar->proc(&event1);
	TEST_NUMBER_EQUAL(0, config->getLen());

	// Disable
	realtime->setDateTime("2018-01-02 10:30:50");
	Event event2(SaleManager::Event_AutomatState, (uint8_t)false);
	registrar->proc(&event2);
	TEST_NUMBER_EQUAL(0, config->getLen());

	// Timeout
	timers->tick(AUTOMAT_DISABLE_TIMEOUT);
	timers->execute();

	TEST_NUMBER_EQUAL(1, config->getLen());
	uint16_t index = config->getLast();
	ConfigEvent event;
	config->get(index, &event);
	TEST_NUMBER_EQUAL(ConfigEvent::Type_SaleDisabled, event.getCode());
	StringBuilder datetime;
	datetime << LOG_DATETIME(*(event.getDate()));
	TEST_STRING_EQUAL("2018.1.2 10:30:50", datetime.getString());

	// Enable
	realtime->setDateTime("2018-01-02 12:00:50");
	Event event3(SaleManager::Event_AutomatState, (uint8_t)true);
	registrar->proc(&event3);

	TEST_NUMBER_EQUAL(2, config->getLen());
	index = config->getLast();
	config->get(index, &event);
	TEST_NUMBER_EQUAL(ConfigEvent::Type_SaleEnabled, event.getCode());
	datetime.clear();
	datetime << LOG_DATETIME(*(event.getDate()));
	TEST_STRING_EQUAL("2018.1.2 12:0:50", datetime.getString());
	return true;
}
#endif
