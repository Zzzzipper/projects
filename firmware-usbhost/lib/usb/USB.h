#include "config.h"
#ifdef DEVICE_USB
#pragma once
/*
 * Тип: Синглетон
 * Описание: Модуль, для работы с USB
 *
 *
 * Инструкция:
 *
 * Костяк модуля генерируется утилитой STM32CubeMX. Из сгенерированного проекта берутся с небольшой модификацией следующие файлы:
 * Middlewares\ST\STM32_USB_Host_Library\Class\* -> extern\STM32_USB_Host_Library\Class\
 * Middlewares\ST\STM32_USB_Host_Library\Core\*  -> extern\STM32_USB_Host_Library\Core\
 * Drivers\STM32F4xx_HAL_Driver\ -> extern\STM32_USB_Host_Library\HAL\ - содержит только необходимые файлы из HAL библиотеки
 * Drivers\CMSIS\Device\ST\STM32F4xx\Include\stm32f407xx.h -> extern\STM32_USB_Host_Library\HAL\my_stm32f407xx.h - Оставлено только самое необходимое
 * Src\usb_host.c -> + lib\usb\USB.cpp
 * Inc\usb_host.h -> + lib\usb\USB.h
 * Src\usbh_conf.c -> lib\usb\usbh_conf.c
 * Inc\usbh_conf.h -> lib\usb\usbh_conf.h
 *
 */

#include <stdint.h>
#include <stdio.h>

#include "defines.h"
#include "common.h"

#include "usbh_def.h"
#include "usbh_vendor_PAX_D200.h"
#include "usbh_vendor_ibox.h"
#include "usbh_cdc.h"
#include "AndroidAccessory.h"
//#include "usbh_vendor_Ingenico.h"
//#include "usbh_hid.h"

#include "UsbController.h"

#include "common/timer/include/TimerEngine.h"
#include "common/utils/include/Fifo.h"
#include "common/utils/include/Event.h"



extern "C" void HAL_Delay(uint32_t delay);
extern "C" void HAL_Delay_us(uint32_t delay);
extern "C" void OTG_FS_IRQHandler(void);
extern "C" uint32_t USB_GetCurrentAndLastTimeDiff(uint32_t lastTimeMs);
extern "C" uint32_t USB_GetCurrentMs();

/*
 * 	0x03 0x04 0x01 0x00 6 ->
 * 	0xf3 0x84 0x02 0x00 ...
 *
 * 	04
 * 	02 17 00 00 05 00 33 30 30 30 30 04 03 00 36 34 33 56 06 00 32 30 31 30 30 30 68 82
	04
 *
 */


class UsbController;

class USB
{
public:
	static USB *get();
	USB();
	~USB();

	enum DeviceType
	{
		Event_USB_Device_PAX_D200,
	};

	enum EventType
	{
		Event_USB_Active			= 0x1A00 | 0x01,
		Event_USB_Disconnected		= 0x1A00 | 0x02,
		Event_USB_Data_Received		= 0x1A00 | 0x03,
		Event_USB_Data_Transmitted	= 0x1A00 | 0x04,
	};

	void setTransmitBuffer(uint8_t *buf, uint32_t bufSize);
	void setReceiveBuffer(uint8_t *buf, uint32_t bufSize);

	void init(TimerEngine *timerEngine);
	void start();

	void deinit();
	void stop();

	void reload();

	void execute();
	bool isInitialized();
#if 0
	inline Fifo<uint8_t>* getTransmitBuffer();
	inline Fifo<uint8_t>* getReceiveBuffer();
	inline void setEventObserver(EventObserver *eventObserver);
	inline EventObserver *getEventObserver();
#else
	Fifo<uint8_t>* getTransmitBuffer();
	Fifo<uint8_t>* getReceiveBuffer();
	void setEventObserver(EventObserver *eventObserver);
	EventObserver *getEventObserver();
	UsbController *getController();

#endif
	void startTimer();
	void procTimer();
#ifdef DEBUG
	void test();
#endif

private:
	TimerEngine *timerEngine;
	Timer *timer;
	Fifo<uint8_t> *transmitBuffer;
	Fifo<uint8_t> *receiveBuffer;
	EventObserver *eventObserver;
	bool initialized;
	int count;
	UsbController *controller;

	void initTimer(int period);

#ifdef USB_ACCESSORY_MODE
public:
	// Управление количеством перезагрузок
	const int MAX_RESET_COUNT = 3;
	void setResetCounterBuf(uint8_t* buf);
	bool incReset();

private:
	uint8_t *_buf = nullptr;

#endif

};
#endif
