#ifndef COMMON_TIMER_TIMEMETER_H
#define COMMON_TIMER_TIMEMETER_H

#include "logger/include/Logger.h"

#include <stdint.h>

class SystemTimer;

class MicroSecondMeter {
public:
	MicroSecondMeter();
	void start();
	void meter();
	uint32_t getDif() { return dif; }

private:
	SystemTimer *st;
	uint32_t from;
	uint32_t to;
	uint32_t dif;
	uint32_t maxValue;
};

class MicroCycleMeter {
public:
	void start(uint32_t cycleNumber);
	void cycle();

private:
	MicroSecondMeter meter;
	uint32_t cntNumber;
	uint32_t cnt;
	uint32_t time;
	uint32_t cycleMaxTime;
	uint32_t globalMaxTime;
};

#define MIM_START(__maxIntervalSize) MicroIntervalMeter::get()->start(__maxIntervalSize)
#define MIM_CHECK() if(MicroIntervalMeter::get()->check() == false) { \
LOG_ERROR(LOG_TM, "!!!TOO BIG INTERVAL (exp=" << MicroIntervalMeter::get()->getMaxSize() << ",act=" << MicroIntervalMeter::get()->getSize() << ")"); \
}

class MicroIntervalMeter {
public:
	static MicroIntervalMeter *get();
	void start(uint32_t maxIntervalSize);
	bool check();
	uint32_t getSize() { return meter.getDif(); }
	uint32_t getMaxSize() { return maxIntervalSize; }

private:
	MicroSecondMeter meter;
	uint32_t maxIntervalSize;

	MicroIntervalMeter();
};

#endif
