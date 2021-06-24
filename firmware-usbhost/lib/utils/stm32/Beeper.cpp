
#include "Beeper.h"

#include "common/timer/stm32/include/SystemTimer.h"
#include "common.h"

#include "stm32f4xx_conf.h"

static Beeper *INSTANCE = NULL;
#define DEFAULT_TIMER_PRESCALER_DIVIDER 1000000

Beeper *Beeper::get()
{
	if (INSTANCE == NULL) INSTANCE = new Beeper();
	return INSTANCE;
}

Beeper::Beeper()
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	GPIO_StructInit(&gpio);
	gpio.GPIO_Mode = GPIO_Mode_AF;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_Pin = GPIO_Pin_0;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	// Переконфигурируем выход 0 для работы с таймером. Теперь сигнал формируемый таймером будет идти прямо на вывод контроллера.
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_TIM2);
}

void Beeper::timerInit(uint32_t period)
{
	TIM_TimeBaseInitTypeDef base_timer;
	TIM_TimeBaseStructInit(&base_timer);

	base_timer.TIM_Prescaler = SystemCoreClock/DEFAULT_TIMER_PRESCALER_DIVIDER - 1; // делитель частоты, получаем 1 Мгц
	base_timer.TIM_Period = period;  					// Период
	TIM_TimeBaseInit(TIM2, &base_timer);

	TIM_OCInitTypeDef oc_init;
	TIM_OCStructInit(&oc_init);
	oc_init.TIM_OCMode = TIM_OCMode_Toggle;   			// переключаем состояние ноги при каждом переполнении таймера
	oc_init.TIM_OutputState = TIM_OutputState_Enable;

	TIM_OC1Init(TIM2,&oc_init);   						// заносим данные в первый канал
	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM2, ENABLE);
}

void Beeper::start()
{
	gpio.GPIO_Mode = GPIO_Mode_AF;
	GPIO_Init(GPIOA, &gpio);

	TIM_Cmd(TIM2, ENABLE);   // Запускаем счёт
}

void Beeper::stop()
{
	TIM_Cmd(TIM2, DISABLE);   // Останавливаем счёт

	// Отключаем ногу от таймера, переводим в 0.
	// Нужно для того, чтобы не сжечь наш динамик после генерации частоты
//	GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_RTC_50Hz);

	gpio.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(GPIOA, &gpio);

	GPIO_ResetBits(GPIOA, GPIO_Pin_0);
}

void Beeper::beep(uint32_t hz, uint32_t timeMs)
{
	uint32_t period = (DEFAULT_TIMER_PRESCALER_DIVIDER/(hz * 2)) - 1;

	timerInit(period);
	start();
	SystemTimer::get()->delay_ms(timeMs);
	stop();
}

void Beeper::initAndStart(uint32_t hz) {
	uint32_t period = (DEFAULT_TIMER_PRESCALER_DIVIDER/(hz * 2)) - 1;
	timerInit(period);
	start();
}
