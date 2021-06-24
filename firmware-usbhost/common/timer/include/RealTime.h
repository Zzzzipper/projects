#ifndef TIMER_REALTIME_H__
#define TIMER_REALTIME_H__

#include "DateTime.h"

class RealTimeInterface {
public:
	virtual ~RealTimeInterface() {}
	virtual bool setDateTime(DateTime *date) = 0;
	virtual void getDateTime(DateTime *date) = 0;
	virtual uint32_t getUnixTimestamp() = 0;
};

#endif
