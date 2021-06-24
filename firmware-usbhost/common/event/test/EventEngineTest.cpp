#include "event/include/EventEngine.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class EventEngineTest : public TestSet {
public:
	EventEngineTest();
	bool test();

private:
};

TEST_SET_REGISTER(EventEngineTest);

EventEngineTest::EventEngineTest() {
	TEST_CASE_REGISTER(EventEngineTest, test);
}

class TestEventSubscriber : public EventSubscriber {
public:
	TestEventSubscriber(uint16_t id, StringBuilder *result) : id(id), result(result) {}
	virtual void proc(EventEnvelope *envelope) {
		*result << "<event=" << id << "," << envelope->getType() << ">";
	}

private:
	uint16_t id;
	StringBuilder *result;
};

class TestEvent : public EventInterface {
public:
	TestEvent(uint16_t type) : EventInterface(type) {}
	virtual bool open(EventEnvelope *envelope) {
		type = envelope->getType();
		return true;
	}
	virtual bool pack(EventEnvelope *envelope) {
		envelope->setType(type);
		return true;
	}
};

// Подумать как лучше делать:
// - по подписке, то есть регистрировать связь наблюдателя и источника
// - по адресам, тогда источнику событий нужно сообщать кому он отправляет события
bool EventEngineTest::test() {
	StringBuilder result;

	uint16_t a1 = 0x0100;
	uint16_t a2 = 0x0200;
	uint16_t a3 = 0x0300;
	TestEventSubscriber sub1(1, &result);
	TestEventSubscriber sub2(2, &result);
	TestEventSubscriber sub3(3, &result);

	EventEngine engine(5, 50, 5);
	TEST_NUMBER_EQUAL(true, engine.subscribe(&sub1, a1));
	TEST_NUMBER_EQUAL(true, engine.subscribe(&sub2, a2));
	TEST_NUMBER_EQUAL(true, engine.subscribe(&sub3, a3));
	TEST_NUMBER_EQUAL(false, engine.subscribe(&sub1, a1));

	TestEvent event1(a1 | 11);
	TestEvent event2(a2 | 22);
	TestEvent event3(a3 | 33);
	TEST_NUMBER_EQUAL(true, engine.transmit(&event1));
	TEST_NUMBER_EQUAL(true, engine.transmit(&event2));
	TEST_NUMBER_EQUAL(true, engine.transmit(&event3));
	TEST_STRING_EQUAL("", result.getString());

	engine.execute();
	TEST_STRING_EQUAL("<event=1,267><event=2,534><event=3,801>", result.getString());
	return true;
}
