#pragma once

#include "stm32f4xx_conf.h"

#include "common/utils/include/List.h"

//typedef void (*GPIO_Callback)(uint16_t GPIO_Pin);

class GPIO_Listener
{
private:
protected:
	uint32_t extiMask;

public:
	GPIO_Listener(uint32_t extiMask) :
			extiMask(extiMask)
	{
	}

	virtual ~GPIO_Listener()
	{
	}

	uint32_t getExtiMask()
	{
		return extiMask;
	}

	void clearGpio_IT()
	{
		EXTI_ClearFlag(extiMask);
	}

	virtual void gpioHandler(uint32_t it_mask) = 0;
};


class InterruptManager;


// --------------------------------------------------------------------------------

class InterruptManager
{
private:
	List<GPIO_Listener> *gpioListeners;

public:
	static InterruptManager * get();

	InterruptManager();
	bool addListener(GPIO_Listener *listener);
	bool removeListener(GPIO_Listener *listener);

	void gpio_irq(uint32_t it_mask);
};
