#ifndef TIMER_TIMETABLE_H__
#define TIMER_TIMETABLE_H__

#include "DateTime.h"

#include <stdint.h>

class WeekTable {
public:
	WeekTable();
	WeekTable(uint8_t value);
	void set(WeekDay weekDay);
	void unset(WeekDay weekDay);
	void toggle(WeekDay weekDay);
	bool check(WeekDay weekDay);
	void setValue(uint8_t value);
	uint8_t getValue();
private:
	uint8_t value;
};

class Time {
public:
	Time();
	Time(uint8_t hour, uint8_t minute, uint8_t second);
	void set(Time *time);
	bool set(uint8_t hour, uint8_t minute, uint8_t second);
	uint8_t getHour();
	uint8_t	getMinute();
	uint8_t getSecond();
	int32_t getTotalSeconds();

private:
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

extern bool stringToTime(const char *str, Time *time);

class TimeInterval {
public:
	void set(TimeInterval *ti);
	void set(Time *time, uint32_t interval);
	void set(uint8_t hour, uint8_t minute, uint8_t second, uint32_t interval);
	bool check(int32_t seconds);
	Time *getTime();
	uint32_t getInterval();

private:
	Time from;
	uint32_t interval;
};

class TimeTable {
public:
	void clear();
	void set(TimeTable *tt);
	void setWeekDay(WeekDay day, bool enable);
	void setWeekTable(uint8_t week);
	void setInterval(Time *time, uint32_t interval);
	void setInterval(uint8_t hour, uint8_t minute, uint8_t second, uint32_t interval);
	bool check(int32_t seconds, WeekDay day);
	WeekTable *getWeek();
	TimeInterval *getInterval();

private:
	WeekTable week;
	TimeInterval interval;
};

#endif
