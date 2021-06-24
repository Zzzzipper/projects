#include <stdio.h>
#include <string.h>
#include "stm32f4xx_conf.h"
#include "common/logger/include/Logger.h"

#include "SPI.h"

#if 0
static SPI *pSPI[] 				= {NULL, NULL, NULL};
static GPIO_TypeDef* NSS_PORT[]	= {0, GPIOC, GPIOA};
static uint16_t NSS_PIN[]		= {0, GPIO_Pin_1, GPIO_Pin_15};
#else
typedef GPIO_TypeDef* P_GPIO;
static SPI *pSPI[]     = {NULL, NULL, NULL};
const P_GPIO NSS_PORT[]   = {0, GPIOC, GPIOA};
const uint16_t NSS_PIN[]  = {0, GPIO_Pin_1, GPIO_Pin_15};
#endif

SPI *SPI::get(enum enSPI index)
{
	if (pSPI[index] == NULL) pSPI[index] = new SPI(index);
	return pSPI[index];
}

SPI::SPI(enum enSPI index)
{
	LOG_DEBUG(LOG_SPI, "Создание модуля SPI" << (int)index + 1);
	this->index = index;
	spi = NULL;
	receiveBuffer = new Fifo<uint8_t>(16);
	config();
	setChipEnable(true);
}

SPI::~SPI()
{
	// FIXME: SPI, не реализован деструктор.
	LOG_ERROR(LOG_SPI, "Удаление модуля SPI" << (int)index + 1 << "полностью не реализовано");

	SPI_Cmd(spi, DISABLE);
	delete receiveBuffer;
	pSPI[index] = NULL;
}

void SPI::config() {

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_StructInit(&GPIO_InitStructure);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;


	switch ((int) index) {
	case SPI_2:
		if (spi != SPI2) {
			spi = SPI2;

			NVIC_DisableIRQ(SPI2_IRQn);

			RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
			RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
			RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

			// SPI
			GPIO_PinAFConfig(GPIOC, GPIO_PinSource2, GPIO_AF_SPI2);		// MISO
			GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_SPI2);		// MOSI
			GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);	// SCK

			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
			GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
			GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
			GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
			GPIO_Init(GPIOC, &GPIO_InitStructure);

			GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
			GPIO_Init(GPIOB, &GPIO_InitStructure);

			// TODO: NVIC, SPI2
			NVIC_InitStructure.NVIC_IRQChannel = SPI2_IRQn;
			NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = IRQ_PRIORITY_SPI2;
			NVIC_InitStructure.NVIC_IRQChannelSubPriority = IRQ_SUB_PRIORITY_SPI2;
			NVIC_Init(&NVIC_InitStructure);

			// RESET
//			RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
//			GPIO_InitTypeDef gpio;
//			GPIO_StructInit(&gpio);
//			gpio.GPIO_OType = GPIO_OType_PP;
//			gpio.GPIO_Mode = GPIO_Mode_OUT;
//			gpio.GPIO_Pin = GPIO_Pin_0;
//			GPIO_Init(GPIOC, &gpio);
		}
		break;

	case SPI_3:
		if (spi != SPI3) {
			spi = SPI3;

			NVIC_DisableIRQ(SPI3_IRQn);

			RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
			RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

			GPIO_TypeDef* spiGpio = GPIOB;

			// SPI
			GPIO_PinAFConfig(spiGpio, GPIO_PinSource3, GPIO_AF_SPI3);
			GPIO_PinAFConfig(spiGpio, GPIO_PinSource4, GPIO_AF_SPI3);
			GPIO_PinAFConfig(spiGpio, GPIO_PinSource5, GPIO_AF_SPI3);

			// B3 = SCK
			// B4 = MOSI
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
			GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
			GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
			GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
			GPIO_Init(spiGpio, &GPIO_InitStructure);

			// B5 = MISO
			GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
			GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
			GPIO_Init(spiGpio, &GPIO_InitStructure);

			// TODO: NVIC, SPI3
			NVIC_InitStructure.NVIC_IRQChannel = SPI3_IRQn;
			NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = IRQ_PRIORITY_SPI3;
			NVIC_InitStructure.NVIC_IRQChannelSubPriority = IRQ_SUB_PRIORITY_SPI3;
			NVIC_Init(&NVIC_InitStructure);
		}
		break;

	default:
		LOG_ERROR(LOG_SPI,
				"Обработчик для SPI" << (int)index + 1 << " не реализован");
		return;
	}


	// Настройка пина NSS
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin = NSS_PIN[index];
	GPIO_Init(NSS_PORT[index], &GPIO_InitStructure);

	SPI_InitTypeDef spiTypeDef;
	SPI_StructInit(&spiTypeDef);
	spiTypeDef.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	spiTypeDef.SPI_Mode = SPI_Mode_Master;
	spiTypeDef.SPI_DataSize = SPI_DataSize_8b;
	spiTypeDef.SPI_CPOL = SPI_CPOL_Low;
	spiTypeDef.SPI_CPHA = SPI_CPHA_1Edge;
	spiTypeDef.SPI_NSS = SPI_NSS_Soft;

	spiTypeDef.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; // 10 Мгц
	spiTypeDef.SPI_FirstBit = SPI_FirstBit_MSB;
	spiTypeDef.SPI_CRCPolynomial = 7;
	SPI_Init(spi, &spiTypeDef);

	SPI_NSSInternalSoftwareConfig(spi, SPI_NSSInternalSoft_Set);
	SPI_I2S_ITConfig(spi, SPI_I2S_IT_RXNE, ENABLE);
	SPI_Cmd(spi, ENABLE);
}

void SPI::sendData(uint8_t data)
{
	while (SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_TXE) == RESET);	// Ждем освобождения буфера SPI_DR
	SPI_I2S_SendData(spi, data);
}

uint8_t SPI::receiveData(uint8_t data)
{
	while(SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_BSY) == SET);		// Ждем полного завершения передачи
	sendData(data);
	while (receiveBuffer->getSize() == 0);
	return receiveBuffer->pop();
}

void SPI::setChipEnable(bool enable)
{
	switch ((int) index) {
	case SPI_2:
	case SPI_3:
		if (!enable)	{
			while(SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_BSY) == SET);
			GPIO_SetBits(NSS_PORT[index], NSS_PIN[index]);
		}
		else GPIO_ResetBits(NSS_PORT[index], NSS_PIN[index]);
	break;

	default:
		LOG_ERROR(LOG_SPI, "Обработчик для SPI" << (int)index + 1 << " не реализован");
	break;
	}
}

void SPI::receive_isr(uint8_t data)
{
	receiveBuffer->push(data);
}

// ==============================================

void SPI_IRQ(SPI_TypeDef *spi, enum enSPI spiIndex)
{
	pSPI[spiIndex]->receive_isr(SPI_I2S_ReceiveData(spi));
}

extern "C" void SPI2_IRQHandler()
{
	SPI_IRQ(SPI2, SPI_2);
}

extern "C" void SPI3_IRQHandler()
{
	SPI_IRQ(SPI3, SPI_3);
}
