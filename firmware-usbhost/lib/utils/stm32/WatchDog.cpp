#include "WatchDog.h"

#include "stm32f4xx_conf.h"

void WD_Init(uint32_t ms) {
	/* Enable write access to IWDG_PR and IWDG_RLR registers */
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

	/* IWDG counter clock: 40KHz(LSI) / 32 = 1.25 KHz */
	IWDG_SetPrescaler(IWDG_Prescaler_128);

	// 0.8*x=ms
	uint32_t val = ms * 10 / 32;
	IWDG_SetReload(val);

	/* Reload IWDG counter */
	IWDG_ReloadCounter();

	/* Enable IWDG (the LSI oscillator will be enabled by hardware) */
	IWDG_Enable();
}

