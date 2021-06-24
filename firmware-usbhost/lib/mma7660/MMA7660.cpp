#include "MMA7660.h"

#include "common.h"
#include "common/uart/include/UartStream.h"
#include "common/platform/include/platform.h"
#include "common/i2c/I2C.h"

#include <stdio.h>
#include <string.h>

#include "stm32f4xx_conf.h"
#include "defines.h"

/*
 * Модуль Акселерометра MMA7660
 * Тип - Синглетон
 * Источник: http://www.dessy.ru/include/images/ware/pdf/s/sen21853p.pdf
 */

static MMA7660 *INSTANCE = NULL;

#define MMA7660_ADDR 0x98
#define MMA7660_X 0x00
#define MMA7660_Y 0x01
#define MMA7660_Z 0x02
#define MMA7660_TILT 0x03
#define MMA7660_SRST 0x04
#define MMA7660_SPCNT 0x05
#define MMA7660_INTSU 0x06
#define MMA7660_MODE 0x07
#define MMA7660_SR 0x08
#define MMA7660_PDET 0x09
#define MMA7660_PD 0x0A
#define MMA_I2C I2C_3


MMA7660 * MMA7660::get()
{
	if (INSTANCE == NULL) INSTANCE = new MMA7660();
	return INSTANCE;
}

MMA7660::MMA7660()
{
	I2C::get(MMA_I2C)->setObserver(this);
    initialized = init();
}

bool MMA7660::init()
{
	uint8_t data[1];

	data[0] = 0; // Выключаем
	if (!I2C::get(MMA_I2C)->syncWriteData(MMA7660_ADDR, MMA7660_MODE, 1, data, 1)) return false;

	data[0] = 4; // 8 Samples/Second
	if (!I2C::get(MMA_I2C)->syncWriteData(MMA7660_ADDR, MMA7660_SR, 1, data, 1)) return false;

	data[0] = 1; // Включаем
	if(!I2C::get(MMA_I2C)->syncWriteData(MMA7660_ADDR, MMA7660_MODE, 1, data, 1)) return false;

	return true;
}

struct MMA7660::Values MMA7660::asyncReadValues()
{
	if (!initialized) return Values();
	signed char syncData[3];

	I2C::get(MMA_I2C)->syncReadData(MMA7660_ADDR, MMA7660_X, 1, (uint8_t *) syncData, 3);
	while(!I2C::get(MMA_I2C)->isAsyncCompleted());
	return getValues(syncData);
}

struct MMA7660::Values MMA7660::getValues(signed char data[])
{
	Values values;
	if (!initialized) {
		return values;
	}

	values.x = ((signed char)(data[0] << 2)) / 4;
	values.y = ((signed char)(data[1] << 2)) / 4;
	values.z = ((signed char)(data[2] << 2)) / 4;
	return values;
}

bool MMA7660::isInitialized()
{
	return initialized;
}


void MMA7660::syncPrint(Uart *uart)
{
	StringBuilder str = "MMA7660 sync print";
	signed char syncData[3];
	I2C::get(MMA_I2C)->syncReadData(MMA7660_ADDR, MMA7660_X, 1, (uint8_t *) syncData, 3);
	while(!I2C::get(MMA_I2C)->isAsyncCompleted());
	Values values = getValues(syncData);
	if (initialized)
		str << ", x: " << values.x << ", y: " << values.y << ", z: " << values.z << "\r\n";
	else
		str << ", не отвечает !\r\n";

	UartStream stream(uart);
	stream.send(str);
}

void MMA7660::asyncPrint(Uart *uart)
{
	this->asyncPrintUart = uart;
	I2C::get(MMA_I2C)->asyncReadData(MMA7660_ADDR, MMA7660_X, 1, (uint8_t *) asyncData, 3, GlobalId_I2C + 1);
}

void MMA7660::proc(Event *ev)
{
	if (ev->getType() == GlobalId_I2C + 1) {
		StringBuilder str = "MMA7660 async print";
		Values values = getValues(asyncData);
		if (initialized)
			str << ", x: " << values.x << ", y: " << values.y << ", z: " << values.z << "\r\n";
		else
			str << ", не отвечает !\r\n";

		UartStream stream(asyncPrintUart);
		stream.send(str);
	}
}
