#include "include/TimeTable.h"

#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"

WeekTable::WeekTable() : value(0) {

}

WeekTable::WeekTable(uint8_t value) : value(value) {

}

void WeekTable::set(WeekDay weekDay) {
	value |= 1 << weekDay;
}

void WeekTable::unset(WeekDay weekDay) {
	value &= ~(1 << weekDay);
}

void WeekTable::toggle(WeekDay weekDay) {
	value ^= 1 << weekDay;
}

bool WeekTable::check(WeekDay weekDay) {
	return ((value >> weekDay) & 0x01);
}

void WeekTable::setValue(uint8_t value) {
	this->value = value;
}

uint8_t WeekTable::getValue() {
	return value;
}

Time::Time() :
	hour(0),
	minute(0),
	second(0)
{

}

Time::Time(uint8_t hour, uint8_t minute, uint8_t second) :
	hour(hour),
	minute(minute),
	second(second)
{

}

void Time::set(Time *time) {
	this->hour = time->hour;
	this->minute = time->minute;
	this->second = time->second;
}

bool Time::set(uint8_t hour, uint8_t minute, uint8_t second) {
	this->hour = hour;
	if(hour > 24) {
		return false;
	}
	this->minute = minute;
	if(minute > 59) {
		return false;
	}
	this->second = second;
	if(second > 59) {
		return false;
	}
	return true;
}

uint8_t Time::getHour() {
	return hour;
}

uint8_t	Time::getMinute() {
	return minute;
}

uint8_t Time::getSecond() {
	return second;
}

int32_t Time::getTotalSeconds() {
	int32_t minutes = hour * 60 + minute;
	return (minutes * 60 + second);
}

bool stringToTime(StringParser *p, Time *time) {
	uint8_t hour;
	if(p->getNumber(&hour) == false) {
		return false;
	}
	if(p->compareAndSkip(":") == false) {
		return false;
	}
	uint8_t minute;
	if(p->getNumber(&minute) == false) {
		return false;
	}
	if(p->compareAndSkip(":") == false) {
		return false;
	}
	uint8_t second;
	if(p->getNumber(&second) == false) {
		return false;
	}
	return time->set(hour, minute, second);
}

bool stringToTime(const char *str, Time *time) {
	StringParser p(str);
	return stringToTime(&p, time);
}

void TimeInterval::set(TimeInterval *ti) {
	this->from.set(&ti->from);
	this->interval = ti->interval;
}

void TimeInterval::set(Time *time, uint32_t interval) {
	this->from.set(time);
	this->interval = interval;
}

void TimeInterval::set(uint8_t hour, uint8_t minute, uint8_t second, uint32_t interval) {
	this->from.set(hour, minute, second);
	this->interval = interval;
}

bool TimeInterval::check(int32_t nowSeconds) {
	int32_t fromSeconds = from.getTotalSeconds();
	if(fromSeconds > nowSeconds) {
		return false;
	}
	int32_t toSeconds = fromSeconds + interval;
	if(nowSeconds > toSeconds) {
		return false;
	}
	return true;
}

Time *TimeInterval::getTime() {
	return &from;
}

uint32_t TimeInterval::getInterval() {
	return interval;
}

void TimeTable::clear() {
	this->week.setValue(0);
	this->interval.set(0, 0, 0, 0);
}

void TimeTable::set(TimeTable *tt) {
	this->week.setValue(tt->week.getValue());
	this->interval.set(&tt->interval);
}

void TimeTable::setWeekDay(WeekDay day, bool enable) {
	if(enable == true) {
		week.set(day);
	} else {
		week.unset(day);
	}
}

void TimeTable::setWeekTable(uint8_t week) {
	this->week.setValue(week);
}

void TimeTable::setInterval(Time *time, uint32_t interval) {
	this->interval.set(time, interval);
}

void TimeTable::setInterval(uint8_t hour, uint8_t minute, uint8_t second, uint32_t interval) {
	this->interval.set(hour, minute, second, interval);
}

bool TimeTable::check(int32_t seconds, WeekDay day) {
	if(week.check(day) == false) {
		return false;
	}
	if(interval.check(seconds) == false) {
		return false;
	}
	return true;
}

WeekTable *TimeTable::getWeek() {
	return &week;
}

TimeInterval *TimeTable::getInterval() {
	return &interval;
}
