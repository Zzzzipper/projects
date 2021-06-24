#include "timer/include/TimerEngine.h"
#include "utils/include/StringBuilder.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class TimerEngineTest : public TestSet {
public:
	TimerEngineTest();
	bool init();
	void cleanup();
	bool test();
	bool testStartTimerByAnotherTimer();

private:
	StringBuilder result;
	Timer *timer;

	void procTimer1();
	void procTimer2();
	void procTimer3();
	void procTimer4();
};

TEST_SET_REGISTER(TimerEngineTest);

TimerEngineTest::TimerEngineTest() {
	TEST_CASE_REGISTER(TimerEngineTest, test);
	TEST_CASE_REGISTER(TimerEngineTest, testStartTimerByAnotherTimer);
}

bool TimerEngineTest::init() {
	return true;
}

void TimerEngineTest::cleanup() {
	result.clear();
}

void TimerEngineTest::procTimer1() {
	result << "<timer1>";
}

void TimerEngineTest::procTimer2() {
	result << "<timer2>";
}

void TimerEngineTest::procTimer3() {
	result << "<timer3>";
}

void TimerEngineTest::procTimer4() {
	result << "<timer4>";
	timer->start(1);
}

bool TimerEngineTest::test() {
	TimerEngine engine;
	Timer *timer1 = engine.addTimer<TimerEngineTest, &TimerEngineTest::procTimer1>(this);
	Timer *timer2 = engine.addTimer<TimerEngineTest, &TimerEngineTest::procTimer2>(this);
	Timer *timer3 = engine.addTimer<TimerEngineTest, &TimerEngineTest::procTimer3>(this, TimerEngine::ProcInTick);

	timer1->start(500);
	timer2->start(1000);
	timer3->start(1000);
	TEST_STRING_EQUAL("", result.getString());

	// tick 100
	engine.tick(100);
	TEST_STRING_EQUAL("", result.getString());

	engine.execute();
	TEST_STRING_EQUAL("", result.getString());

	// tick 500
	engine.tick(500);
	TEST_STRING_EQUAL("", result.getString());

	engine.execute();
	TEST_STRING_EQUAL("<timer1>", result.getString());
	result.clear();

	// tick 500
	engine.tick(500);
	TEST_STRING_EQUAL("<timer3>", result.getString());
	result.clear();

	engine.execute();
	TEST_STRING_EQUAL("<timer2>", result.getString());
	result.clear();
	return true;
}

/*
 * Проверка ситуации, что взведенный таймер из обработчика другого таймер, не сработает в этот обход.
 */
bool TimerEngineTest::testStartTimerByAnotherTimer() {
	TimerEngine engine;
	Timer *timer1 = engine.addTimer<TimerEngineTest, &TimerEngineTest::procTimer4>(this, TimerEngine::ProcInTick);
	timer = engine.addTimer<TimerEngineTest, &TimerEngineTest::procTimer1>(this, TimerEngine::ProcInTick);

	timer1->start(500);
	TEST_STRING_EQUAL("", result.getString());

	// tick 100
	engine.tick(100);
	TEST_STRING_EQUAL("", result.getString());

	engine.execute();
	TEST_STRING_EQUAL("", result.getString());

	// tick 500
	engine.tick(500);
	TEST_STRING_EQUAL("<timer4>", result.getString());
	result.clear();

	engine.execute();
	TEST_STRING_EQUAL("", result.getString());

	// tick 500
	engine.tick(500);
	TEST_STRING_EQUAL("<timer1>", result.getString());
	result.clear();

	engine.execute();
	TEST_STRING_EQUAL("", result.getString());
	return true;
}
