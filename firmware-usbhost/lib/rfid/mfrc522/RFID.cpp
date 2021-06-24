
#include <stdio.h>
#include <string.h>

#include "defines.h"

#include "stm32f4xx_conf.h"
#include "Rfid.h"

#include "common.h"
#include "common/platform/include/platform.h"
#include "common/logger/include/Logger.h"

/*
 * Модуль RFID
 * Тип - Синглетон
 * Источник: http://arduino-kit.ru/userfiles/image/RFIDRC522.pdf
 */

static RFID *INSTANCE = NULL;
volatile bool spiDataReceived;
volatile uint8_t spiReceivedData;

RFID * RFID::get()
{
	if (INSTANCE == NULL) INSTANCE = new RFID(SPI2);
	return INSTANCE;
}


RFID::RFID(SPI_TypeDef* spi): MFRC522(), spi(spi)
{
	LOG_ERROR(LOG_MODEM, "1");
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	GPIO_TypeDef* spiGpio = GPIOB;

	// SPI
	GPIO_PinAFConfig(spiGpio, GPIO_PinSource13, GPIO_AF_SPI2);
	GPIO_PinAFConfig(spiGpio, GPIO_PinSource14, GPIO_AF_SPI2);
	GPIO_PinAFConfig(spiGpio, GPIO_PinSource15, GPIO_AF_SPI2);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_13 | GPIO_Pin_15;
	GPIO_Init(spiGpio, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Pin =   GPIO_Pin_14;
	GPIO_Init(spiGpio, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_Init(spiGpio, &GPIO_InitStructure);

	LOG_ERROR(LOG_MODEM, "2");
	SPI_InitTypeDef spiTypeDef;
	SPI_StructInit(&spiTypeDef);
	spiTypeDef.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	spiTypeDef.SPI_Mode = SPI_Mode_Master;
	spiTypeDef.SPI_DataSize = SPI_DataSize_8b;
	spiTypeDef.SPI_CPOL = SPI_CPOL_Low;
	spiTypeDef.SPI_CPHA = SPI_CPHA_1Edge;
	spiTypeDef.SPI_NSS = SPI_NSS_Soft;

	spiTypeDef.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
	spiTypeDef.SPI_FirstBit = SPI_FirstBit_MSB;
	spiTypeDef.SPI_CRCPolynomial = 7;
	SPI_Init(spi, &spiTypeDef);

	SPI_Cmd(spi, ENABLE);
	SPI_NSSInternalSoftwareConfig(spi, SPI_NSSInternalSoft_Set);

	NVIC_EnableIRQ(SPI2_IRQn);
	SPI_I2S_ITConfig(spi, SPI_I2S_IT_RXNE, ENABLE);

	// RFID_RESET
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOC, &gpio);

	// RFID_IRQ
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_Mode = GPIO_Mode_IN;
	gpio.GPIO_Pin = GPIO_Pin_1;
	gpio.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &gpio);


	setResetPin(true);
	setChipEnable(false);
	setResetPin(false);

	PCD_Reset();
	PCD_Init();
	LOG_ERROR(LOG_MODEM, "3");
}

void RFID::sendData(uint8_t data)
{
	while (SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_TXE) == RESET); // Ждем освобождения буфера SPI_DR
	SPI_I2S_SendData(spi, data);
}

static int dataIndex = 0;
uint8_t RFID::receiveData(uint8_t data)
{
	while(SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_BSY) == SET); // Ждем полного завершения передачи
	spiDataReceived = false;
	sendData(data);
	while (spiDataReceived == false);
	return spiReceivedData;
}
/*
extern "C" void SPI2_IRQHandler()
{
	spiReceivedData = SPI_I2S_ReceiveData(SPI2);
	spiDataReceived = true;
}
*/
void RFID::PCD_WriteRegister(uint8_t reg, uint8_t value)
{
	setChipEnable(true);
	sendData(reg & 0x7E);
	sendData(value);
	setChipEnable(false);
}

/**
 * Writes a number of uint8_ts to the specified register in the MFRC522 chip.
 * The interface is described in the datasheet section 8.1.2.
 */
void RFID::PCD_WriteRegister(uint8_t reg, uint8_t count, uint8_t *values)
{
	setChipEnable(true);
	sendData(reg & 0x7E);
	for (uint8_t index = 0; index < count; index++)
		sendData(values[index]);

	setChipEnable(false);
}

/**
 * Reads a uint8_t from the specified register in the MFRC522 chip.
 * The interface is described in the datasheet section 8.1.2.
 */
uint8_t RFID::PCD_ReadRegister(uint8_t reg)
{
	setChipEnable(true);
	sendData(0x80 | (reg & 0x7E));
	uint8_t data = receiveData(0);
	setChipEnable(false);
	return data;
}

/**
 * Reads a number of uint8_ts from the specified register in the MFRC522 chip.
 * The interface is described in the datasheet section 8.1.2.
 */
void RFID::PCD_ReadRegister(uint8_t reg, uint8_t count, uint8_t *values, uint8_t rxAlign)
{
	if (count == 0) {
		return;
	}
	setChipEnable(true);

	LOG("Reading " << count << "bytes from register");
	uint8_t address = 0x80 | (reg & 0x7E);
	uint8_t index = 0;
	count--;
	sendData(address);
	while (index < count)
	{
		if (index == 0 && rxAlign)
		{		// Only update bit positions rxAlign..7 in values[0]
			// Create bit mask for bit positions rxAlign..7
			uint8_t mask = 0;
			for (uint8_t i = rxAlign; i <= 7; i++) {
				mask |= (1 << i);
			}
			// Read value and tell that we want to read the same address again.
			uint8_t value = receiveData(address);

			// Apply mask to both current value of values[0] and the new data in value.
			values[0] = (values[index] & ~mask) | (value & mask);
		}
		else { // Normal case
			values[index] = receiveData(address);
		}
		index++;
	}
	values[index] = receiveData(0);

	setChipEnable(false);
}

void RFID::setResetPin(bool hiLevel)
{
	if (hiLevel)	GPIO_SetBits(GPIOC, GPIO_Pin_0);
	else			GPIO_ResetBits(GPIOC, GPIO_Pin_0);
}

void RFID::setChipEnable(bool enable)
{
	if (!enable)	{
		while(SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_BSY) == SET); // пїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅ
		GPIO_SetBits(GPIOB, GPIO_Pin_12);
	}
	else	GPIO_ResetBits(GPIOB, GPIO_Pin_12);
}


