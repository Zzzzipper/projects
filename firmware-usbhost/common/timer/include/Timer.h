#ifndef COMMON_TIMER_TIMER_H__
#define COMMON_TIMER_TIMER_H__

#include <stdint.h>

class TimerEngine;

class TimerObserver {
public:
	virtual ~TimerObserver() {}
	virtual void proc() = 0;
};

class Timer {
public:
	~Timer();
	void start(uint32_t value);
	void stop();

protected:
	TimerObserver *listener;
	volatile uint32_t value;
	volatile uint32_t count;

	Timer(TimerObserver *listener);
	Timer(const Timer &c);
	Timer& operator=(const Timer &c);
	void tick(uint32_t step);
	void proc();

	friend class TimerEngine;
};

#endif
