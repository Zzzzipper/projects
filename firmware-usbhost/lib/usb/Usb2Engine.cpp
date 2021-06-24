#include "config.h"
#ifdef DEVICE_USB2
#include "Usb2Engine.h"

#include "common/logger/include/Logger.h"
#include "common/timer/stm32/include/SystemTimer.h"
#include "common/platform/arm/include/platform.h"

#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "stm32f4xx_conf.h"
#include "config.h"

#define APB_SPEED				84		// Скорость шины таймера, в Mhz

extern USBH_HandleTypeDef 		hUsbHostFS;
extern HCD_HandleTypeDef 		hhcd_USB_OTG_FS;

USBH_HandleTypeDef hUsbHostFS;

static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);

static Usb2Engine *INSTANCE = NULL;

Usb2Engine *Usb2Engine::get() {
	if(INSTANCE == NULL) INSTANCE = new Usb2Engine();
	return INSTANCE;
}

Usb2Engine::Usb2Engine() :
	initialized(false),
	count(0)
{
	LOG_DEBUG(LOG_USB, "USB constructor");
}

void Usb2Engine::init(TimerEngine *timerEngine) {
	if(initialized) {
		LOG_DEBUG(LOG_USB, "reinit");
		USBH_ReEnumerate(&hUsbHostFS);
		return;
	}

	LOG_DEBUG(LOG_USB, "init");
	USBH_Init(&hUsbHostFS, USBH_UserProcess, HOST_FS);
	paxd200.init(timerEngine);
	ingenico.init(timerEngine);

	USBH_Start(&hUsbHostFS);
	initialized = true;
}

void Usb2Engine::deinit() {
	LOG_DEBUG(LOG_USB, "deinit");

	TIM_Cmd(TIM7, DISABLE);
	USBH_Stop(&hUsbHostFS);
	USBH_DeInit(&hUsbHostFS);

	initialized = false;
	ATOMIC {
		ingenico.deinit();
		paxd200.deinit();
	}
}

void Usb2Engine::execute() {
	if(!initialized) return;

	USBH_Process(&hUsbHostFS);
//	ingenico.execute();
	paxd200.execute();

	if(count++ % 1000000 == 0) {
		printf("+");
		fflush(stdout);
	}
}

void Usb2Engine::procActive() {
	LOG_DEBUG(LOG_USB, "procActive");
//	ingenico.procActive();
	paxd200.procActive();
}

void Usb2Engine::procTransmitCallback(USBH_HandleTypeDef *phost) {
	LOG_DEBUG(LOG_USB, "procTransmitCallback");
//	ingenico.procTransmitCallback(phost);
	paxd200.procTransmitCallback(phost);
}

void Usb2Engine::procReceiveCallback(USBH_HandleTypeDef *phost) {
	LOG_DEBUG(LOG_USB, "procReceiveCallback");
//	ingenico.procReceiveCallback(phost);
	paxd200.procReceiveCallback(phost);
}


static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id) {
#if 0
	EventObserver *eventObserver = INSTANCE->getEventObserver();
	switch (id) {
	case HOST_USER_SELECT_CONFIGURATION:
		LOG_DEBUG(LOG_USB, "USB Select configuration");
	break;

	case HOST_USER_DISCONNECTION:
		LOG_DEBUG(LOG_USB, "USB Disconnected");
		usb_state = APPLICATION_DISCONNECT;
		if(eventObserver) {
			Event event(Usb2Engine::Event_USB_Disconnected);
			eventObserver->proc(&event);
		}
	break;

	case HOST_USER_CLASS_ACTIVE:
		LOG_DEBUG(LOG_USB, "USB Active");
		usb_state = APPLICATION_READY;
		if(INSTANCE->getTransmitBuffer()) INSTANCE->getTransmitBuffer()->clear();
		if(INSTANCE->getReceiveBuffer()) INSTANCE->getReceiveBuffer()->clear();
		INSTANCE->startTimer();
	break;

	case HOST_USER_CONNECTION:
		LOG_DEBUG(LOG_USB, "USB Connected");
		usb_state = APPLICATION_START;
	break;

	default:
		LOG_WARN(LOG_USB, "USBH_UserProcess, id: " << id);
	break;
	}
#else
	switch (id) {
	case HOST_USER_SELECT_CONFIGURATION:
		LOG_DEBUG(LOG_USB, "USB Select configuration");
	break;

	case HOST_USER_DISCONNECTION:
		LOG_DEBUG(LOG_USB, "USB Disconnected");
	break;

	case HOST_USER_CLASS_ACTIVE:
		LOG_DEBUG(LOG_USB, "USB Active");
		INSTANCE->procActive();
	break;

	case HOST_USER_CONNECTION:
		LOG_DEBUG(LOG_USB, "USB Connected");
	break;

	default:
		LOG_WARN(LOG_USB, "USBH_UserProcess, id: " << id);
	break;
	}
#endif
}

extern "C" void USBH_PAX_D200_TransmitCallback(USBH_HandleTypeDef *phost) {
#if 0
	PAX_D200_HandleTypeDef *handle = (PAX_D200_HandleTypeDef *) phost->pActiveClass->pData;

	LOG_DEBUG(LOG_USB, "USBH_PAX_D200_TransmitCallback, len: " << handle->wlen);

	EventObserver *eventObserver = INSTANCE->getEventObserver();
	if (eventObserver)
	{
		Event event(Usb2Engine::Event_USB_Data_Transmitted, handle->wlen);
		eventObserver->proc(&event);
	}
#else
	INSTANCE->procTransmitCallback(phost);
#endif
}

extern "C" void USBH_PAX_D200_ReceiveCallback(USBH_HandleTypeDef *phost, uint8_t *data, uint32_t len, uint32_t available) {
#if 0
	LOG_DEBUG(LOG_USB, "USBH_PAX_D200_ReceiveCallback, len: " << len << ", available: " << available << ", data: " << data[0] << " " << data[1] << " " << data[2] << " " << data[3]);

	Fifo<uint8_t> *receiveBuffer = INSTANCE->getReceiveBuffer();
	if (receiveBuffer == NULL)
	{
		LOG_ERROR(LOG_USB, "USB receive buffer doesn't initialized!");
		return;
	}

	for (uint32_t i = 0; i < len; i++)
	{
		if (!receiveBuffer->push(data[i]))
		{
			LOG_ERROR(LOG_USB, "USB receive buffer is full! Received data was lost!");
			break;
		}
	}

	EventObserver *eventObserver = INSTANCE->getEventObserver();
	if (eventObserver)
	{
		Event event(Usb2Engine::Event_USB_Data_Received, len);
		eventObserver->proc(&event);
	}
#else
	INSTANCE->procReceiveCallback(phost);
#endif
}

extern "C" void USBH_CDC_TransmitCallback(USBH_HandleTypeDef *phost) {
#if 0
	CDC_HandleTypeDef *handle = (CDC_HandleTypeDef *) phost->pActiveClass->pData;

	uint32_t len = handle->TxDataLength;

	LOG_DEBUG(LOG_USB, "CDC_TransmitCallback, len: " << len);
	Logger::get()->toWiresharkHex(handle->pTxData, 36);

	EventObserver *eventObserver = INSTANCE->getEventObserver();
	if (eventObserver)
	{
		Event event(Usb2Engine::Event_USB_Data_Transmitted, len);
		eventObserver->proc(&event);
	}
#else
	INSTANCE->procTransmitCallback(phost);
#endif
}

extern "C" void USBH_CDC_ReceiveCallback(USBH_HandleTypeDef *phost) {
#if 0
	CDC_HandleTypeDef *handle = (CDC_HandleTypeDef *) phost->pActiveClass->pData;
	uint32_t len1 = handle->RxDataLength;
	uint32_t len2 = USBH_CDC_GetLastReceivedDataSize(phost);

	LOG_DEBUG(LOG_USB, "CDC_ReceiveCallback: len1: " << len1 << ", len2: " << len2);

	Fifo<uint8_t> *receiveBuffer = INSTANCE->getReceiveBuffer();
	if (receiveBuffer == NULL)
	{
		LOG_ERROR(LOG_USB, "USB receive buffer doesn't initialized!");
		return;
	}

	if (!len2)
	{
		LOG_WARN(LOG_USB, "CDC_ReceiveCallback: Data is empty !");
		return;
	}

	uint8_t *data = handle->pRxData;

	for (uint32_t i = 0; i < len2; i++)
	{
		if (!receiveBuffer->push(data[i]))
		{
			LOG_ERROR(LOG_USB, "USB receive buffer is full! Received data was lost!");
			break;
		}
	}

	EventObserver *eventObserver = INSTANCE->getEventObserver();
	if (eventObserver)
	{
		Event event(Usb2Engine::Event_USB_Data_Received, len2);
		eventObserver->proc(&event);
	}
#else
	INSTANCE->procReceiveCallback(phost);
#endif
}

extern "C" void USBH_CDC_LineCodingChanged(USBH_HandleTypeDef *phost) {
	LOG_DEBUG(LOG_USB, "CDC_LineCodingChanged");
}

extern "C" uint32_t USB_GetCurrentAndLastTimeDiff(uint32_t lastTimeMs) {
	return SystemTimer::get()->getCurrentAndLastTimeDiff(lastTimeMs);
}

extern "C" uint32_t USB_GetCurrentMs() {
	return SystemTimer::get()->getMs();
}

// FIXME: USB, переделать на асинхронное ожидание!
extern "C" void HAL_Delay(uint32_t delay) {
	SystemTimer::get()->delay_ms(delay);
}

extern "C" void HAL_Delay_us(uint32_t delay) {
	SystemTimer::get()->delay_us(delay);
}

extern "C" void OTG_FS_IRQHandler(void) {
	HAL_HCD_IRQHandler(&hhcd_USB_OTG_FS);
}

extern "C" void TIM7_IRQHandler(void) {
	TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
	if(INSTANCE == NULL) return;
	INSTANCE->execute();
}
#endif
