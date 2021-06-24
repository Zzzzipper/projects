#include "InterruptManager.h"

#include "common/logger/include/LogTargetUart.h"
#include "common/logger/include/Logger.h"

#include "common.h"
#include "config.h"
#include "defines.h"

static InterruptManager *INSTANCE = NULL;

InterruptManager * InterruptManager::get()
{
	if (INSTANCE == NULL)
		INSTANCE = new InterruptManager();
	return INSTANCE;
}

InterruptManager::InterruptManager()
{
	INSTANCE = this;
	LOG_INFO(LOG_INTERRUPT_MANAGER, "Инициализация");

	gpioListeners = new List<GPIO_Listener>();
}

bool InterruptManager::addListener(GPIO_Listener *listener)
{
	LOG_DEBUG(LOG_INTERRUPT_MANAGER, "Добавляем слушателя " << "GPIO, маска: " << listener->getExtiMask());

	listener->clearGpio_IT();
	gpioListeners->add(listener);
	return true;
}

bool InterruptManager::removeListener(GPIO_Listener *listener)
{
	LOG_DEBUG(LOG_INTERRUPT_MANAGER, "Удаляем слушателя " << "GPIO, маска: " << listener->getExtiMask());

	gpioListeners->remove(listener);
	return true;
}

void InterruptManager::gpio_irq(uint32_t it_mask)
{
	for (int i = 0; i < gpioListeners->getSize(); i++)
	{
		GPIO_Listener *l = gpioListeners->get(i);
		if (l->getExtiMask() & it_mask)
			l->gpioHandler(it_mask);
	}
}

// -------------------------------------------------------------------

extern "C" void EXTI0_IRQHandler(void)
{
	LOG_DEBUG(LOG_INTERRUPT_MANAGER, "EXTI0_IRQHandler");

	uint32_t mask = 0b1;

	uint32_t it_mask = EXTI_GetFlagStatus(mask);

	EXTI_ClearFlag(mask);
	InterruptManager::get()->gpio_irq(it_mask);
}

extern "C" void EXTI1_IRQHandler(void)
{
	LOG_DEBUG(LOG_INTERRUPT_MANAGER, "EXTI1_IRQHandler");

	uint32_t mask = 0b10;
	uint32_t it_mask = EXTI_GetFlagStatus(mask);

	EXTI_ClearFlag(mask);
	InterruptManager::get()->gpio_irq(it_mask);
}

extern "C" void EXTI2_IRQHandler(void)
{
	LOG_DEBUG(LOG_INTERRUPT_MANAGER, "EXTI2_IRQHandler");

	uint32_t mask = 0b100;
	uint32_t it_mask = EXTI_GetFlagStatus(mask);

	EXTI_ClearFlag(mask);
	InterruptManager::get()->gpio_irq(it_mask);
}

extern "C" void EXTI3_IRQHandler(void)
{
	LOG_DEBUG(LOG_INTERRUPT_MANAGER, "EXTI3_IRQHandler");

	uint32_t mask = 0b1000;
	uint32_t it_mask = EXTI_GetFlagStatus(mask);

	EXTI_ClearFlag(mask);
	InterruptManager::get()->gpio_irq(it_mask);
}

extern "C" void EXTI4_IRQHandler(void)
{
	LOG_DEBUG(LOG_INTERRUPT_MANAGER, "EXTI4_IRQHandler");

	uint32_t mask = 0b10000;
	uint32_t it_mask = EXTI_GetFlagStatus(mask);

	EXTI_ClearFlag(mask);
	InterruptManager::get()->gpio_irq(it_mask);
}

extern "C" void EXTI9_5_IRQHandler(void)
{
	LOG_DEBUG(LOG_INTERRUPT_MANAGER, "EXTI9_5_IRQHandler");

	uint32_t mask = 0b1111100000;
	uint32_t it_mask = EXTI_GetFlagStatus(mask);

	EXTI_ClearFlag(mask);
	InterruptManager::get()->gpio_irq(it_mask);
}

extern "C" void EXTI15_10_IRQHandler(void)
{
	LOG_DEBUG(LOG_INTERRUPT_MANAGER, "EXTI15_10_IRQHandler");

	uint32_t mask = 0b1111110000000000;
	uint32_t it_mask = EXTI_GetFlagStatus(mask);

	EXTI_ClearFlag(mask);
	InterruptManager::get()->gpio_irq(it_mask);
}
