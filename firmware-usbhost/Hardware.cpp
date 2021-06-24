#include <stddef.h>
#include <stdio.h>

#include "stm32f4xx_conf.h"
#include "Hardware.h"
#include "defines.h"


static Hardware *HW_INSTANCE = NULL;



Hardware *Hardware::get()
{
	if (HW_INSTANCE == NULL) HW_INSTANCE = new Hardware();
	return HW_INSTANCE;
}

Hardware::Hardware()
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_Mode = GPIO_Mode_IN;
	gpio.GPIO_PuPd = GPIO_PuPd_UP;
	gpio.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_Init(GPIOC, &gpio);

	uint8_t state0 = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7);
	uint8_t state1 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4);
	uint8_t state2 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5);
	int rev = 0;
	if (state0 == Bit_RESET) rev |= 0b001;
	if (state1 == Bit_RESET) rev |= 0b010;
	if (state2 == Bit_RESET) rev |= 0b100;

	if (rev == 0) version = HW_2_0_0;
	else if (rev == 1) version = HW_3_0_0;
	else version = HW_UNKNOWN;
}

uint32_t Hardware::getVersion()
{
	return version;
}
