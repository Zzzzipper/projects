#ifndef __SYSTEM_TIMER_H
#define __SYSTEM_TIMER_H

#include "defines.h"

class SystemTimer {
private:
	void init();
	__inline uint8_t compare(int32_t tp);
	__inline uint32_t getValue();

public:
	static SystemTimer *get();
	SystemTimer();

	void delay_us(uint32_t us);
	void delay_ms(uint32_t ms);
	
	uint32_t getMs();
	uint32_t getMsMax();
	uint32_t getUs();
	uint32_t getUsMax();
	uint32_t getCurrentAndLastTimeDiff(uint32_t lastTimeMs);
};

#endif
