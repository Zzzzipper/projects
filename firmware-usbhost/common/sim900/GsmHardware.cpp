#include "common/sim900/GsmHardware.h"
#include "defines.h"

namespace Gsm {

void Hardware::init() {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

	// Инициализируем ногу PwrKey
	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_Pin = SIM900_PWR_KEY;
#if (HW_VERSION < HW_3_0_2)
	gpio.GPIO_OType = GPIO_OType_PP;
#elif (HW_VERSION >= HW_3_0_2)
	gpio.GPIO_OType = GPIO_OType_OD;
#else
	#error "HW_VERSION must be defined in project settings"
#endif

	GPIO_Init(SIM900_KEY_AND_LED_PORT, &gpio);
	releasePowerButton();

	// Инициализируем ногу Status
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_Mode = GPIO_Mode_IN;
	gpio.GPIO_Pin = SIM900_STATUS;
	GPIO_Init(SIM900_KEY_AND_LED_PORT, &gpio);
}

void Hardware::pressPowerButton() {
#if (HW_VERSION < HW_3_0_2)
	GPIO_SetBits(SIM900_KEY_AND_LED_PORT, SIM900_PWR_KEY);
#elif (HW_VERSION >= HW_3_0_2)
	GPIO_ResetBits(SIM900_KEY_AND_LED_PORT, SIM900_PWR_KEY);
#else
	#error "HW_VERSION must be defined in project settings"
#endif
}

void Hardware::releasePowerButton() {
#if (HW_VERSION < HW_3_0_2)
	GPIO_ResetBits(SIM900_KEY_AND_LED_PORT, SIM900_PWR_KEY);
#elif (HW_VERSION >= HW_3_0_2)
	GPIO_SetBits(SIM900_KEY_AND_LED_PORT, SIM900_PWR_KEY);
#else
	#error "HW_VERSION must be defined in project settings"
#endif
}

// TODO: Работает, с версии HW: 01.01.13
bool Hardware::isStatusUp() {
	return GPIO_ReadInputDataBit(SIM900_KEY_AND_LED_PORT, SIM900_STATUS) == Bit_RESET;
}

}
