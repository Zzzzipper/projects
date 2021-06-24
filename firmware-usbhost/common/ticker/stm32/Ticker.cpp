#include "ticker/include/Ticker.h"
#include "common/platform/include/platform.h"
#include "common.h"
#include "logger/include/Logger.h"
#include "stm32f4xx_conf.h"
#include "defines.h"

#define TICK_SIZE 1
#define APB_SPEED		84		// Скорость шины таймера, в Mhz

static Ticker *instance = NULL;

Ticker *Ticker::get() {
	if(instance == NULL) {
		instance = new Ticker();
	}
	return instance;
}

Ticker::Ticker() : consumer(NULL) {
	// Настраиваем таймер 4 на прерывание раз в 1 млс
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	TIM_TimeBaseInitTypeDef timerStruct;

	timerStruct.TIM_Period = (TICK_SIZE*1000)-1;
	timerStruct.TIM_Prescaler = APB_SPEED-1;
	timerStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	timerStruct.TIM_CounterMode = TIM_CounterMode_Up;
	timerStruct.TIM_RepetitionCounter = 0x0000;

	TIM_TimeBaseInit(TIM4, &timerStruct);
	TIM_Cmd(TIM4, ENABLE);
	
	TIM_ITConfig(TIM4, TIM_DIER_UIE, ENABLE);

	// TODO: NVIC, TIM4
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = IRQ_PRIORITY_TIM4;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = IRQ_SUB_PRIORITY_TIM4;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

Ticker::~Ticker() {
	// Отключаем таймер
	TIM_Cmd(TIM4, DISABLE);

	// Запрещаем прерывание
	TIM_ITConfig(TIM4, TIM_DIER_UIE, DISABLE);
	NVIC_DisableIRQ(TIM4_IRQn);
}

void Ticker::tick() {
	if(consumer == NULL) {
		LOG_ERROR(LOG_TIMER, "Consumer are not register");
		return;
	}
	this->consumer->tick(TICK_SIZE);
}

void Ticker::registerConsumer(TickerListener *consumer) {
	if(consumer == NULL) {
		LOG_WARN(LOG_TIMER, "Consumer already register");
	}
	this->consumer = consumer;
}

extern "C" void TIM4_IRQHandler(void) {
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	if(instance == NULL) return;
	instance->tick();
}

