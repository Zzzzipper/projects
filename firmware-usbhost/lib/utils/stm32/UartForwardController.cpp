
#include "UartForwardController.h"

#include "../defines.h"

#include "stm32f4xx_conf.h"

UartForwardController::UartForwardController(Uart *source, Uart *target, GPIO_TypeDef* rxPort, uint32_t rxPin):
	GPIO_Listener(rxGpio.GPIO_Pin),
	source(source), target(target), rxPort(rxPort)
{
	rxGpio.GPIO_Pin = rxPin;

	InterruptManager::get()->addListener(this);
}

UartForwardController::~UartForwardController()
{
	InterruptManager::get()->removeListener(this);
}

void UartForwardController::start()
{
	// Освобождаем ногу TargetUart.Tx от периферии Uart
	target->disableTransmit();

	// Настраиваем TargetUart.Tx на выход.
	GPIO_StructInit(&txGpio);

	txGpio.GPIO_Pin = (1 << target->getTxPinSource());
	txGpio.GPIO_Mode = GPIO_Mode_OUT;
	txGpio.GPIO_OType = target->getTxOutputType();
	txGpio.GPIO_PuPd = GPIO_PuPd_UP;
	txGpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(target->getTxPort(), &txGpio);


	// Включаем прерывание дублирующей ноги для SourceUart.RX на оба фронта
	rxGpio.GPIO_Mode = GPIO_Mode_IN;
	rxGpio.GPIO_PuPd = GPIO_PuPd_UP;
	rxGpio.GPIO_Speed = GPIO_Speed_2MHz;

	GPIO_Init(rxPort, &rxGpio);

//	nvic.NVIC_IRQChannelCmd = ENABLE;
//	nvic.NVIC_IRQChannel = EXTI9_5_IRQn;
//	nvic.NVIC_IRQChannelPreemptionPriority = IRQ_PRIORITY_EXTI;
//	nvic.NVIC_IRQChannelSubPriority = IRQ_SUB_PRIORITY_EXTI;
//	NVIC_Init(&nvic);
//
//	nvic.NVIC_IRQChannel = v;
//	NVIC_Init(&nvic);

}

void UartForwardController::stop()
{
	// Возвращаем все, как было.
//	NVIC_DisableIRQ(EXTI9_5_IRQn);
//	NVIC_DisableIRQ(EXTI15_10_IRQn);
}

void UartForwardController::gpioHandler(uint32_t it_mask)
{
	// В прерываниии изменяем значение ноги Tx.

	bool stateIn = GPIO_ReadInputDataBit(rxPort, rxGpio.GPIO_Pin);
	GPIO_WriteBit(target->getTxPort(), txGpio.GPIO_Pin, !stateIn ? Bit_SET : Bit_RESET);
}
