#include "common/timer/stm32/include/RealTimeControl.h"
#include "common/logger/include/Logger.h"

#include "common.h"
#include "defines.h"
#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_pwr.h"

static RealTimeControl *INSTANCE = NULL;
static const uint32_t BACKUP_DATA_EXTERNAL_CLOCK = 0x32F2;
static const uint32_t BACKUP_DATA_INTERNAL_CLOCK = 0x32F3;

/* Internal RTC defines */
#define RTC_LEAP_YEAR(year)             ((((year) % 4 == 0) && ((year) % 100 != 0)) || ((year) % 400 == 0))
#define RTC_DAYS_IN_YEAR(x)             RTC_LEAP_YEAR(x) ? 366 : 365
#define RTC_OFFSET_YEAR                 1970
#define RTC_SECONDS_PER_DAY             86400
#define RTC_SECONDS_PER_HOUR            3600
#define RTC_SECONDS_PER_MINUTE          60
#define RTC_BCD2BIN(x)                  ((((x) >> 4) & 0x0F) * 10 + ((x) & 0x0F))
#define RTC_CHAR2NUM(x)                 ((x) - '0')
#define RTC_CHARISNUM(x)                ((x) >= '0' && (x) <= '9')

/* Days in a month */
static uint8_t TM_RTC_Months[2][12] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},	/* Not leap year */
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}	/* Leap year */
};



RealTimeControl *RealTimeControl::get() {
	if (INSTANCE == NULL) INSTANCE = new RealTimeControl(RealTimeControl::ExternalClock);
	return INSTANCE;
}

RealTimeControl *RealTimeControl::get(enum RealTimeControl::Mode mode) {
	if (INSTANCE == NULL) INSTANCE = new RealTimeControl(mode);
	return INSTANCE;
}

RealTimeControl::RealTimeControl(enum Mode mode) {
	INSTANCE = this;

	LOG_DEBUG(LOG_RTC, "RTC created");
	init(mode);
}

void RealTimeControl::init(enum Mode mode) {
	RTC_InitTypeDef rtc;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	PWR_BackupAccessCmd(ENABLE);

	uint32_t backupData = RTC_ReadBackupRegister(RTC_BKP_DR0);
	bool needConfig =	((mode == ExternalClock && backupData != BACKUP_DATA_EXTERNAL_CLOCK)) ||
						((mode == InternalClock && backupData != BACKUP_DATA_INTERNAL_CLOCK));

	if (needConfig)
	{
		if (mode == ExternalClock) {
			LOG_DEBUG(LOG_RTC, "External clock");

			RCC_LSEConfig(RCC_LSE_ON);
			while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {};

			RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE); // внешний кварц на 32768 Гц
			backupData = BACKUP_DATA_EXTERNAL_CLOCK;
		}
		else {
			LOG_DEBUG(LOG_RTC, "Internal clock");

			RCC_LSICmd(ENABLE);
			while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) {};

			RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI); // внутренние 32768 Гц
			backupData = BACKUP_DATA_INTERNAL_CLOCK;
		}

		RCC_RTCCLKCmd(ENABLE);
		RTC_WaitForSynchro();

		RTC_StructInit(&rtc);
		RTC_Init(&rtc);

		setDefaultDateTime();
		RTC_WriteBackupRegister(RTC_BKP_DR0, backupData);
		RTC_WaitForSynchro();

		LOG_DEBUG(LOG_RTC, "RTC config created");

	} else {
		LOG_DEBUG(LOG_RTC, "RTC already writed " << (backupData == BACKUP_DATA_EXTERNAL_CLOCK ? "External" : "Internal") << " clock");

		RTC_WaitForSynchro();
		RTC_ClearFlag(RTC_FLAG_ALRAF);
	}
}

void RealTimeControl::setDefaultDateTime() {
	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;

	RTC_TimeStructInit(&time);
	time.RTC_Hours = 12;
	time.RTC_Minutes = 0;
	time.RTC_Seconds = 0;

	RTC_DateStructInit(&date);
	date.RTC_Date = 1;
	date.RTC_Month = 1;
	date.RTC_Year = 17;

	setTime(&time);
	setDate(&date);
}

bool RealTimeControl::setTime(RTC_TimeTypeDef *time) {
	return (RTC_SetTime(RTC_Format_BIN, time) == SUCCESS);
}

bool RealTimeControl::setDate(RTC_DateTypeDef *date) {
	return (RTC_SetDate(RTC_Format_BIN, date) == SUCCESS);
}

void RealTimeControl::getTime(RTC_TimeTypeDef *time) {
	RTC_GetTime(RTC_Format_BIN, time);
}

void RealTimeControl::getDate(RTC_DateTypeDef *date) {
	RTC_GetDate(RTC_Format_BIN, date);
}

uint32_t RealTimeControl::getTotalSeconds() {
	return RTC->TR;
}

uint32_t RealTimeControl::getUnixTimestamp() {
	RTC_TimeTypeDef _time;
	RTC_DateTypeDef _date;
	getTime(&_time);
	getDate(&_date);

	uint32_t days = 0, seconds = 0;
	uint16_t i;
	uint16_t year = (uint16_t) (_date.RTC_Year + 2000);
	/* Year is below offset year */
	if (year < RTC_OFFSET_YEAR) {
		return 0;
	}
	/* Days in back years */
	for (i = RTC_OFFSET_YEAR; i < year; i++) {
		days += RTC_DAYS_IN_YEAR(i);
	}
	/* Days in current year */
	for (i = 1; i < _date.RTC_Month; i++) {
		days += TM_RTC_Months[RTC_LEAP_YEAR(year)][i - 1];
	}
	/* Day starts with 1 */
	days += _date.RTC_Date - 1;
	seconds = days * RTC_SECONDS_PER_DAY;
	seconds += _time.RTC_Hours * RTC_SECONDS_PER_HOUR;
	seconds += _time.RTC_Minutes * RTC_SECONDS_PER_MINUTE;
	seconds += _time.RTC_Seconds;

	/* seconds = days * 86400; */
	return seconds;
}

uint32_t RealTimeControl::getSubSecond() {
	return RTC_GetSubSecond();
}

void RealTimeStm32::init() {
	RealTimeControl::get();
}

bool RealTimeStm32::setDateTime(DateTime *datetime) {
	RTC_TimeTypeDef time;
	RTC_TimeStructInit(&time);
	time.RTC_Hours = datetime->hour;
	time.RTC_Minutes = datetime->minute;
	time.RTC_Seconds = datetime->second;
	if(RealTimeControl::get()->setTime(&time) == false) {
		LOG_ERROR(LOG_RTC, "Time setting failed");
		return false;
	}

	RTC_DateTypeDef date;
	RTC_DateStructInit(&date);
	date.RTC_Year = datetime->year;
	date.RTC_Month = datetime->month;
	date.RTC_Date = datetime->day;
	if(RealTimeControl::get()->setDate(&date) == false) {
		LOG_ERROR(LOG_RTC, "Date setting failed");
		return false;
	}

	LOG_DEBUG(LOG_RTC, "Date and time setting succeed");
	return true;
}

void RealTimeStm32::getDateTime(DateTime *datetime) {
	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;
	RealTimeControl::get()->getTime(&time);
	RealTimeControl::get()->getDate(&date);
//	LOG_DEBUG(LOG_RTC, "Read date/time: " << date.RTC_Year << "-" << date.RTC_Month << "-" << date.RTC_Date << " " << time.RTC_Hours << ":" << time.RTC_Minutes << ":" << time.RTC_Seconds);

	datetime->hour = time.RTC_Hours;
	datetime->minute = time.RTC_Minutes;
	datetime->second = time.RTC_Seconds;
	datetime->year = date.RTC_Year;
	datetime->month = date.RTC_Month;
	datetime->day = date.RTC_Date;
}

uint32_t RealTimeStm32::getUnixTimestamp() {
	return RealTimeControl::get()->getUnixTimestamp();
}
