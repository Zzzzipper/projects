
#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "config.h"
#include "common/logger/include/Logger.h"
#include "stm32f4xx_conf.h"

#include "RFID.h"

#include "common/platform/include/platform.h"
#include "common/utils/include/StringBuilder.h"

/*
 * Модуль RFID
 * Тип - Синглетон
 *
 * Пример работы с библиотекой: https://github.com/adafruit/Adafruit_NFCShield_I2C/tree/master/examples
 *
 */

static RFID *INSTANCE = NULL;

#define Irq_PORT	TEST_PORT
#define Irq_PIN		TEST_PIN


RFID *RFID::get()
{
	if (INSTANCE == NULL) {
		LOG_ERROR(LOG_RFID, "Must be initialized first step!");
	}
	return INSTANCE;
}


RFID::RFID(I2C *i2c): MFRC532(i2c)
{
	if (INSTANCE) LOG_ERROR(LOG_RFID, "Already initialized!");
	INSTANCE = this;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	// RFID_IRQ
	GPIO_InitTypeDef gpio;

	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_Mode = GPIO_Mode_IN;
	gpio.GPIO_Pin = Irq_PIN;
	gpio.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(Irq_PORT, &gpio);
}

void RFID::setResetPin(bool hiLevel)
{
}

#ifndef RFID_WITHOUT_IRQ
uint8_t RFID::wirereadstatus(void)
{
	uint8_t result = GPIO_ReadInputDataBit(Irq_PORT, Irq_PIN);

	if (result == Bit_SET)
		return PN532_I2C_BUSY;
	else
		return PN532_I2C_READY;
}
#else
uint8_t RFID::wirereadstatus(void)
{
		return PN532_I2C_READY;
}
#endif

void RFID::testCardRead(int timeout)
{
	uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
	uint8_t uidLength;

	bool success = RFID::get()->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, timeout);

	String str;
	if (success) {
		str = "Found an ISO14443A card\r\n";
		str << "  UID Length: " << uidLength << " bytes\r\n";
		str << "  UID Value: ";
		for(int i = 0; i < uidLength; i++) {
			str << uid[i];
		}
	} else {
		str = "Not found card";
	}

	LOG_INFO(LOG_RFID, str.getString());
}



