#include "timer/include/Timer.h"
#include "platform/include/platform.h"

Timer::Timer(TimerObserver *listener) : listener(listener), value(0), count(0) {
}

Timer::~Timer() {
	delete listener;
}

void Timer::start(uint32_t value) {
	ATOMIC {
		this->value = value;
		this->count = 0;
	}
}

void Timer::stop() {
	ATOMIC {
		this->value = 0;
		this->count = 0;
	}
}

void Timer::tick(uint32_t step) {
	if(this->value > this->count) {
		this->count += step;
	}
}

void Timer::proc() {
	if(this->value == 0) {
		return;
	}
	if(this->value <= this->count) {
		this->stop();
		this->listener->proc();
	}
}
