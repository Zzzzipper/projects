#include "timer/include/TimerEngine.h"

TimerEngine::TimerEngine() {
	this->timers = new List<Timer>(20);
	this->tickTimers = new List<Timer>(10);
}

TimerEngine::~TimerEngine() {
	delete this->tickTimers;
	delete this->timers;
}

void TimerEngine::deleteTimer(Timer *timer) {
	timers->remove(timer);
#ifdef INTERRUPT_TIMERS
	tickTimers->remove(timer);
#endif
	delete timer;
}

void TimerEngine::execute() {
	for(uint16_t i = 0; i < timers->getSize(); i++) {
		timers->get(i)->proc();
	}
}

void TimerEngine::tick(uint32_t tickSize) {
	for(uint16_t i = 0; i < timers->getSize(); i++) {
		timers->get(i)->tick(tickSize);
	}
#ifdef INTERRUPT_TIMERS
	for(uint16_t i = 0; i < tickTimers->getSize(); i++) {
		tickTimers->get(i)->tick(tickSize);
	}
	for(uint16_t i = 0; i < tickTimers->getSize(); i++) {
		tickTimers->get(i)->proc();
	}
#endif
}
