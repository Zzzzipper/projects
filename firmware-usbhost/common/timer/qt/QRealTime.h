#ifndef TIMER_QREALTIME_H__
#define TIMER_QREALTIME_H__

#include "timer/include/RealTime.h"

class QRealTime : public RealTimeInterface {
public:
	virtual bool setDateTime(DateTime *date);
	virtual void getDateTime(DateTime *date);
	virtual uint32_t getUnixTimestamp();
};

#endif
