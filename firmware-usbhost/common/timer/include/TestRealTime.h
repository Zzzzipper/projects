#ifndef TIMER_TESTREALTIME_H__
#define TIMER_TESTREALTIME_H__

#include "RealTime.h"

class TestRealTime : public RealTimeInterface {
public:
	TestRealTime() {
		datetime.year = 0;
		datetime.month = 1;
		datetime.day = 2;
		datetime.hour = 10;
		datetime.minute = 20;
		datetime.second = 30;
		unixTimeStamp = 123456789;
	}

	void setUnixTimeStamp(uint32_t unixTimeStamp) { this->unixTimeStamp = unixTimeStamp; }
	void setDateTime(const char *str) { stringToDateTime(str, &datetime); }

	virtual bool setDateTime(DateTime *datetime) { this->datetime.set(datetime); return true; }
	virtual void getDateTime(DateTime *datetime) { datetime->set(&this->datetime); }
	virtual uint32_t getUnixTimestamp() { return unixTimeStamp; }

private:
	DateTime datetime;
	uint32_t unixTimeStamp;
};

#endif
