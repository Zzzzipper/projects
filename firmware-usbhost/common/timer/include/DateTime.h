#ifndef TIMER_DATETIME_H__
#define TIMER_DATETIME_H__

#define LOG_DATE(_d) (_d).getYear() << "." << (_d).month << "." << (_d).day
#define LOG_TIME(_d) (_d).hour << ":" << (_d).minute << ":" << (_d).second
#define LOG_DATETIME(_d) LOG_DATE(_d) << " " << LOG_TIME(_d)

#include "common/utils/include/StringBuilder.h"

#include <stdint.h>

enum WeekDay {
	WeekDay_Sunday = 0,
	WeekDay_Monday,
	WeekDay_Tuesday,
	WeekDay_Wednesday,
	WeekDay_Thursday,
	WeekDay_Friday,
	WeekDay_Saturday,
};

#pragma pack(push,1)
struct DateTime {
	uint8_t year;   // from 1 to 99
	uint8_t month;  // from 1 to 12
	uint8_t day;    // from 1 to 31
	uint8_t hour;   // from 0 to 23
	uint8_t minute; // from 0 to 59
	uint8_t second; // from 0 to 59

	DateTime();
	DateTime(const DateTime &dt);
	DateTime(uint8_t year, uint8_t month, uint8_t day, uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0);
	bool set(int32_t totalSeconds);
	void set(DateTime *datetime);
	bool addSeconds(int32_t seconds);
	bool removeSeconds(int32_t seconds);
	int32_t secondsTo(DateTime *datetime) const;
	int32_t getMonthDays() const;
	int32_t getYearDays() const;
	int32_t getTotalDays() const;
	int32_t getTotalSeconds() const;
	uint16_t getYear() const;
	WeekDay getWeekDay() const;
	int32_t getTimeSeconds() const;
};
#pragma pack(pop)

extern bool stringToDateTime(const char *str, DateTime *datetime);
extern bool stringToDateTime(const char *str, uint16_t strLen, DateTime *datetime);
extern void datetime2string(const DateTime *d, StringBuilder *s);
extern uint16_t fiscal2datetime(const char *str, uint16_t strLen, DateTime *datetime);
extern void datetime2fiscal(DateTime *d, StringBuilder *s);

#endif
