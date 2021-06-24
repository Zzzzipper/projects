#pragma once

#include "common.h"

#include "common/uart/stm32/include/uart.h"
#include "lib/utils/stm32/InterruptManager.h"

class UartForwardController: GPIO_Listener
{
private:
	Uart *source;
	Uart *target;

	GPIO_TypeDef* rxPort;
	GPIO_InitTypeDef rxGpio;
	GPIO_InitTypeDef txGpio;

	NVIC_InitTypeDef nvic;

protected:

public:
	UartForwardController(Uart *source, Uart *target, GPIO_TypeDef* rxPort, uint32_t rxPin);
	~UartForwardController();

	void start();
	void stop();

	virtual void gpioHandler(uint32_t it_mask) override;

};
