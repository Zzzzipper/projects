#include "config.h"
#ifdef DEVICE_USB2
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

#include "Usb2PaxD200.h"
#include "Usb2Ingenico.h"

#include "defines.h"
#include "common.h"

#include <stdint.h>
#include <stdio.h>
#include "my_hal.h"
#include "usbh_def.h"
#include "usb2h_cdc.h"
//#include "usbh_vendor_PAX_D200.h"
//#include "usbh_vendor_Ingenico.h"
//#include "usbh_hid.h"


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
class Usb2Engine {
public:
	static Usb2Engine *get();
	Usb2Engine();
	~Usb2Engine();

	enum DeviceType
	{
		Event_USB_Device_PAX_D200 = 1,
		Event_USB_Device_Ingenico = 2,
	};

	enum EventType
	{
		Event_USB_Active			= 0x1A00 | 0x01,
		Event_USB_Disconnected		= 0x1A00 | 0x02,
		Event_USB_Data_Received		= 0x1A00 | 0x03,
		Event_USB_Data_Transmitted	= 0x1A00 | 0x04,
	};

	void init(TimerEngine *timerEngine);
	void deinit();
	void execute();
	bool isInitialized();
	AbstractUart *getPaxD200Uart() { return &paxd200; }
	AbstractUart *getIngenicoUart() { return &ingenico; }

	void procActive();
	void procTransmitCallback(USBH_HandleTypeDef *phost);
	void procReceiveCallback(USBH_HandleTypeDef *phost);

private:
	bool initialized;
	int count;
	Usb2PaxD200 paxd200;
	Usb2Ingenico ingenico;
};
#endif
