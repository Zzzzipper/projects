#include "stm32f4xx_conf.h"

#include "HardwareUartForwardController.h"

#include "defines.h"

void HardwareUartForwardController::init()
{
#if (HW_VERSION >= HW_3_3_0)
	GPIO_InitTypeDef gpio;

	GPIO_StructInit(&gpio);

	gpio.GPIO_Pin = UART_FORWARDING_PIN;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_OType = GPIO_OType_OD;
	gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(UART_FORWARDING_PORT, &gpio);

	stop();
#endif
}

void HardwareUartForwardController::start()
{
#if (HW_VERSION >= HW_3_3_0)
	GPIO_ResetBits(UART_FORWARDING_PORT, UART_FORWARDING_PIN);
#endif
}

void HardwareUartForwardController::stop()
{
#if (HW_VERSION >= HW_3_3_0)
	GPIO_SetBits(UART_FORWARDING_PORT, UART_FORWARDING_PIN);
#endif
}
