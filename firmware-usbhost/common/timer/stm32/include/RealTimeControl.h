#ifndef __RealTimeControl_H
#define __RealTimeControl_H

#include "timer/include/RealTime.h"

#include "stm32f4xx_rtc.h"
/*
 * Потребление VCC_BAT для RTC:
 *
 * V_BAT: 3.0 V
 * Без инициализации, без питания: 0.02 uA
 * С питанием, без инициализации: 0.10 uA
 * С питанием, после инициализации: 0.02 uA
 * Без питания, после инициализации: 1.0 uA
 *
 * Расчет времени работы, при емкости батарейки 35 млА/ч
 * 35000/1 = 35000 ч = 35000/24/365 = 4 года.
 *
 * */

class RealTimeControl {
public:
	enum Mode {
		ExternalClock, // Часы работают от батарейки.
		InternalClock // Часы работают только от основного питания.
	};

	static RealTimeControl *get();
	static RealTimeControl *get(enum Mode mode);
	RealTimeControl(enum Mode mode);

	bool setTime(RTC_TimeTypeDef *time);
	bool setDate(RTC_DateTypeDef *date);
	void getTime(RTC_TimeTypeDef *time);
	void getDate(RTC_DateTypeDef *date);
	uint32_t getTotalSeconds();
	uint32_t getSubSecond();
	// Возвращает кол-во секунд с 1970 года. Источник: https://stm32f4-discovery.net/2014/07/library-19-use-internal-rtc-on-stm32f4xx-devices/
	uint32_t getUnixTimestamp();

private:
	void init(enum Mode mode);
	void setDefaultDateTime();
};

class RealTimeStm32 : public RealTimeInterface {
public:
	void init();
	virtual bool setDateTime(DateTime *datetime);
	virtual void getDateTime(DateTime *datetime);
	virtual uint32_t getUnixTimestamp();
};

#endif
