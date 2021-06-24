
#include "include/SystemTimer.h"

#include "stm32f4xx_conf.h"
#include "defines.h"

/*
 * Тип: Синглетон
 * Класс использует системный таймер STM32 для формирования задержки в млс и мкс.
 *
 */

void _delay_us(uint32_t us) // mcs
{
	SystemTimer::get()->delay_us(us);
}
void _delay_ms(uint32_t ms) // mls
{
	SystemTimer::get()->delay_ms(ms);
}

static SystemTimer *instance = new SystemTimer();

SystemTimer *SystemTimer::get()
{
	if (instance) return instance;
	return new SystemTimer();
}

SystemTimer::SystemTimer()
{
	instance = this;

	init();
}

void SystemTimer::init()
{
	if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk))
	{
		CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
		DWT->CYCCNT = 0;
		DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	}
}

uint32_t SystemTimer::getValue()
{
	return DWT->CYCCNT;
}

uint32_t SystemTimer::getUs()
{
	return getValue() / (SystemCoreClock/1000000);
}

uint32_t SystemTimer::getUsMax() {
	return 0xFFFFFFFF / (SystemCoreClock/1000000);
}

uint32_t SystemTimer::getMs()
{
	return getValue() / (SystemCoreClock/1000);
}

uint32_t SystemTimer::getMsMax()
{
	return 0xFFFFFFFF / (SystemCoreClock/1000);
}

uint8_t SystemTimer::compare(int32_t tp)
{
	return (((int32_t)getValue() - tp) < 0);
}

void SystemTimer::delay_us(uint32_t us) // us = микро
{
	int32_t tp = getValue() + us * (SystemCoreClock/1000000);
	while (compare(tp));
}

void SystemTimer::delay_ms(uint32_t ms) // mls = милли
{
	int32_t tp = getValue() + ms * (SystemCoreClock/1000);
	while (compare(tp));
}

uint32_t SystemTimer::getCurrentAndLastTimeDiff(uint32_t lastTimeMs)
{
	uint32_t currentMs = getMs();
	if (currentMs >= lastTimeMs) return currentMs - lastTimeMs;

	return currentMs + (getMsMax() - lastTimeMs);
}
