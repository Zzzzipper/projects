#include "timer/include/DateTime.h"
#include "utils/include/Number.h"
#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"

DateTime::DateTime() :
	year(1),
	month(1),
	day(1),
	hour(0),
	minute(0),
	second(0)
{}

DateTime::DateTime(const DateTime &dt) :
	year(dt.year),
	month(dt.month),
	day(dt.day),
	hour(dt.hour),
	minute(dt.minute),
	second(dt.second)
{}

DateTime::DateTime(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) :
	year(year),
	month(month),
	day(day),
	hour(hour),
	minute(minute),
	second(second)
{}

bool DateTime::set(int32_t totalSeconds) {
	if(totalSeconds < 0) {
		return false;
	}
	second = totalSeconds % 60;
	int32_t minutes = totalSeconds / 60;
	minute = minutes % 60;
	int32_t hours = minutes / 60;
	hour = hours % 24;
	int32_t days = hours / 24;

	year = days / 365 + 1;
	int32_t yearDays = getYearDays();
	while(yearDays > days) {
		year = year - 1;
		yearDays = getYearDays();
	}
	days = days - yearDays;

	month = 12;
	int32_t monthDays = getMonthDays();
	while(monthDays > days) {
		month = month - 1;
		monthDays = getMonthDays();
	}

	day = days - getMonthDays() + 1;
	return true;
}

void DateTime::set(DateTime *datetime) {
	year = datetime->year;
	month = datetime->month;
	day = datetime->day;
	hour = datetime->hour;
	minute = datetime->minute;
	second = datetime->second;
}

int32_t DateTime::secondsTo(DateTime *datetime) const {
	int32_t d1 = getTotalSeconds();
	int32_t d2 = datetime->getTotalSeconds();
	return (d2 - d1);
}

bool DateTime::addSeconds(int32_t seconds) {
	return set(getTotalSeconds() + seconds);
}

bool DateTime::removeSeconds(int32_t seconds) {
	int32_t totalSeconds = getTotalSeconds();
	if(totalSeconds < seconds) {
		totalSeconds = 0;
	} else {
		totalSeconds = totalSeconds - seconds;
	}
	return set(totalSeconds);
}

int32_t DateTime::getMonthDays() const {
	int32_t daysInFullMonths[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
	uint8_t fullMonths = month - 1;
	int32_t days = daysInFullMonths[fullMonths];
	if(fullMonths >= 2 && (year % 4) == 0) {
		days += 1;
	}
	return days;
}

int32_t DateTime::getTimeSeconds() const {
	int32_t minutes = hour * 60 + minute;
	return (minutes * 60 + second);
}

/**
 * Расчет поправки для Грегорианского календаря:
 * correction = (year - 1)/4 - (year - 1)/100 + (year - 1)/400
 * Поправка рассчитыватся только для полностью завершенных лет, поэтому (year - 1).
 * В нашем случае год начинается с 2001, поэтому поправки 100 и 400 лет неактуальны.
 *
 * ul dn = 365*y + y/4 - y/100 + y/400 + (m*306 + 5)/10 + (d - 1);
 * ( y + y/4 - y/100 + y/400 + t[m-1] + d) % 7
 */
int32_t DateTime::getYearDays() const {
	uint8_t fullYears = year - 1;
	return (fullYears*365 + fullYears /4);
}

int32_t DateTime::getTotalDays() const {
	return (getYearDays() + getMonthDays() + day - 1);
}

int32_t DateTime::getTotalSeconds() const {
	int32_t days = getTotalDays();
	int32_t hours = days * 24 + hour;
	int32_t minutes = hours * 60 + minute;
	return (minutes * 60 + second);
}

uint16_t DateTime::getYear() const {
	return (2000 + year);
}

WeekDay DateTime::getWeekDay() const {
	return (WeekDay)(( getTotalDays() + 1) % 7);
}

bool stringToDateTime(StringParser *p, DateTime *datetime) {
	uint16_t n;

	if(p->getNumber(&n) == false || n < 2000) {
		return false;
	}
	datetime->year = n - 2000;
	if(p->compareAndSkip("-") == false) {
		return false;
	}
	if(p->getNumber(&n) == false) {
		return false;
	}
	datetime->month = n;
	if(p->compareAndSkip("-") == false) {
		return false;
	}
	if(p->getNumber(&n) == false) {
		return false;
	}
	datetime->day = n;
#if 0
	if(p->skipEqual(" T") == false) {
		return false;
	}
#else
	p->skipEqual(" T");
#endif
	if(p->getNumber(&n) == false) {
		return false;
	}
	datetime->hour = n;
	if(p->compareAndSkip(":") == false) {
		return false;
	}
	if(p->getNumber(&n) == false) {
		return false;
	}
	datetime->minute = n;
	if(p->compareAndSkip(":") == false) {
		return false;
	}
	if(p->getNumber(&n) == false) {
		return false;
	}
	datetime->second = n;
	return true;
}

bool stringToDateTime(const char *str, DateTime *datetime) {
	StringParser p(str);
	return stringToDateTime(&p, datetime);
}

bool stringToDateTime(const char *str, uint16_t strLen, DateTime *datetime) {
	StringParser p(str, strLen);
	return stringToDateTime(&p, datetime);
}

void datetime2string(const DateTime *d, StringBuilder *s) {
	*s << d->getYear() << "-";
	if(d->month < 10) { *s << "0"; } *s << d->month << "-";
	if(d->day < 10) { *s << "0"; } *s << d->day << " ";
	if(d->hour < 10) { *s << "0"; } *s << d->hour << ":";
	if(d->minute < 10) { *s << "0"; } *s << d->minute << ":";
	if(d->second < 10) { *s << "0"; } *s << d->second;
}

uint16_t fiscal2datetime(const char *str, uint16_t strLen, DateTime *datetime) {
	StringParser p(str, strLen);
	uint16_t year = 0;
	if(p.getNumber(4, &year) == false) { return 0; }
	if(year < 2000) { return 0; }
	datetime->year = year - 2000;
	if(p.getNumber(2, &(datetime->month)) == false) { return 0; }
	if(p.getNumber(2, &(datetime->day)) == false) { return 0; }
	if(p.compareAndSkip("T") == false) { return 0; }
	if(p.getNumber(2, &(datetime->hour)) == false) { return 0; }
	if(p.getNumber(2, &(datetime->minute)) == false) { return 0; }
	datetime->second = 0;
	return p.parsedLen();
}

void datetime2fiscal(DateTime *d, StringBuilder *s) {
	*s << d->getYear();
	if(d->month < 10) { *s << "0"; } *s << d->month;
	if(d->day < 10) { *s << "0"; } *s << d->day;
	*s << "T";
	if(d->hour < 10) { *s << "0"; } *s << d->hour;
	if(d->minute < 10) { *s << "0"; } *s << d->minute;
}
