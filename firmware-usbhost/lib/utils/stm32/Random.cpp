#include "config.h"
#include "defines.h"
#include "cmsis_boot/stm32f4xx_conf.h"

#include "Random.h"

Random &Random::get()
{
	static Random INSTANCE;
	return INSTANCE;
}

Random::Random()
{
	// Включаем RNG
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
	RNG_Cmd(ENABLE);
}

Random::~Random()
{
	RNG_Cmd(DISABLE);
}

uint32_t Random::getValue()
{
	while(RNG_GetFlagStatus(RNG_FLAG_DRDY) == RESET);
	return RNG_GetRandomNumber();
}
