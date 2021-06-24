#include "config.h"
#ifdef DEVICE_USB

#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "stm32f4xx_conf.h"
#include "config.h"
#include "common/logger/include/Logger.h"
#include "common/timer/stm32/include/SystemTimer.h"
#include "common/platform/arm/include/platform.h"


#include "USB.h"

#define APB_SPEED				84		// —корость шины таймера, в Mhz

extern USBH_HandleTypeDef 		hUsbHostFS;
extern HCD_HandleTypeDef 		hhcd_USB_OTG_FS;

#ifndef USB_ACCESSORY_MODE
USBH_HandleTypeDef hUsbHostFS;
#endif
CDC_LineCodingTypeDef cdcFrameFormat;

#ifndef USB_ACCESSORY_MODE

typedef enum
{
	APPLICATION_IDLE = 0,
	APPLICATION_START,
	APPLICATION_READY,
	APPLICATION_DISCONNECT
} ApplicationTypeDef;

ApplicationTypeDef usb_state = APPLICATION_IDLE;

#endif

static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);


static USB *INSTANCE = NULL;

USB *USB::get()
{
	if (INSTANCE == NULL) INSTANCE = new USB();
	return INSTANCE;
}

USB::USB() :
	timerEngine(NULL),
	transmitBuffer(NULL),
	receiveBuffer(NULL),
	eventObserver(NULL),
	initialized(false),
	count(0),
	controller(NULL)
{
	LOG_DEBUG(LOG_USB, "USB constructor");
}

void USB::init(TimerEngine *timerEngine)
{
#ifdef USB_ACCESSORY_MODE
	LOG_DEBUG(LOG_USB, "USB host init");

	this->timerEngine = timerEngine;
	this->timer = timerEngine->addTimer<USB, &USB::procTimer>(this);

	MX_USB_HOST_Init();

#else
	if (initialized)
	{
		LOG_DEBUG(LOG_USB, "reinit");
		USBH_ReEnumerate(&hUsbHostFS);
		return;
	}

	LOG_DEBUG(LOG_USB, "init");
	this->timerEngine = timerEngine;
	this->timer = timerEngine->addTimer<USB, &USB::procTimer>(this);
	if (!controller)
		controller = new UsbController(this, &hUsbHostFS, timerEngine);

	start();

//	initTimer(USB_PROCESSING_PERIOD);

//	PROBE_INIT(B, 3, GPIO_OType_PP);
//	PROBE_INIT(B, 4, GPIO_OType_PP);
//	PROBE_INIT(B, 5, GPIO_OType_PP);

	if (!receiveBuffer)
		receiveBuffer = new Fifo<uint8_t>(USB_CDC_DEFAULT_RX_TX_BUFFER_SIZE);

	if (!transmitBuffer)
		transmitBuffer = new Fifo<uint8_t>(USB_CDC_DEFAULT_RX_TX_BUFFER_SIZE);
#endif
	initialized = true;
}

void USB::start() {

	USBH_Init(&hUsbHostFS, USBH_UserProcess, HOST_FS);

	USBH_RegisterClass(&hUsbHostFS, USBH_PAX_D200_CLASS);
	USBH_RegisterClass(&hUsbHostFS, USBH_CDC_CLASS);

	cdcFrameFormat.b.dwDTERate = 115200;
	cdcFrameFormat.b.bCharFormat = 0;
	cdcFrameFormat.b.bDataBits = 8;
	cdcFrameFormat.b.bParityType = 0;

	//USBH_RegisterClass(&hUsbHostFS, USBH_HID_CLASS);
	//USBH_RegisterClass(&hUsbHostFS, USBH_INGENICO_CLASS);

	USBH_Start(&hUsbHostFS);
}

void USB::deinit()
{
	LOG_DEBUG(LOG_USB, "deinit");

	TIM_Cmd(TIM7, DISABLE);

	stop();

	initialized = false;
	ATOMIC
	{
		if (transmitBuffer)	delete transmitBuffer;
		if(receiveBuffer)	delete receiveBuffer;
		transmitBuffer = NULL;
		receiveBuffer = NULL;
	}
}

void USB::stop()
{
	USBH_Stop(&hUsbHostFS);
	USBH_DeInit(&hUsbHostFS);
}

void USB::reload() {
#ifdef USB_ACCESSORY_MODE
	MX_USB_HOST_Reload();
#endif
}

void USB::setTransmitBuffer(uint8_t *buf, uint32_t bufSize)
{
	ATOMIC
	{
		if (transmitBuffer)	delete transmitBuffer;
		transmitBuffer = new Fifo<uint8_t>(bufSize, buf);
	}
}

void USB::setReceiveBuffer(uint8_t *buf, uint32_t bufSize)
{
	ATOMIC
	{
		if(receiveBuffer)	delete receiveBuffer;
		receiveBuffer = new Fifo<uint8_t>(bufSize, buf);
	}
}

void USB::setEventObserver(EventObserver *eventObserver)
{
	this->eventObserver = eventObserver;
}

EventObserver *USB::getEventObserver()
{
	return eventObserver;
}

UsbController *USB::getController()
{
	return controller;
}

void USB::initTimer(int period)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);
	TIM_TimeBaseInitTypeDef timerStruct;

	period *= 1000;
	int prescaler = APB_SPEED;
	while (period >= 0xffff)
	{
		period /= 10;
		prescaler *= 10;
	}

	timerStruct.TIM_Period = period - 1;
	timerStruct.TIM_Prescaler = prescaler - 1;
	timerStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	timerStruct.TIM_CounterMode = TIM_CounterMode_Up;
	timerStruct.TIM_RepetitionCounter = 0x0000;

	TIM_TimeBaseInit(TIM7, &timerStruct);

	TIM_ITConfig(TIM7, TIM_DIER_UIE, ENABLE);

	// TODO: NVIC, TIM7
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = IRQ_PRIORITY_TIM7;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = IRQ_SUB_PRIORITY_TIM7;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM7, ENABLE);
}

void USB::execute() {
	if (!initialized)
		return;

	USBH_Process(&hUsbHostFS);
	// –аздельно, потому что USB может быть больше одного
	// и если они в разном режиме работают, то нужно делать так
	MX_USBH_AA_Process(&hUsbHostFS);

	if (count++ % 1000000 == 0) {
		printf("+");
		fflush(stdout);
	}
}

Fifo<uint8_t>* USB::getTransmitBuffer()
{
	return transmitBuffer;
}

Fifo<uint8_t>* USB::getReceiveBuffer()
{
	return receiveBuffer;
}

#ifdef DEBUG
void USB::test()
{
	uint8_t test_send_cdc[] = {0x02,0x23,0x41,0x42,0x50,0x41,0x44,0x41,0x41,0x64,0x52,0x41,0x6F,0x41,0x30,0x77,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x42,0x41,0x41,0x41,0x41,0x32,0x39,0x55,0x3D,0x03};
	for(uint8_t b: test_send_cdc) {
		transmitBuffer->push(b);
	}
}
#endif

void USB::startTimer() {
	timer->start(30000);
}

void USB::procTimer() {
	LOG_DEBUG(LOG_USB, "Start transmiting");
	//+++ todo: pax
	CDC_HandleTypeDef *handle = (CDC_HandleTypeDef*) hUsbHostFS.pActiveClass->pData;
	handle->rxLock = false;
	handle->txLock = false;

	EventObserver *eventObserver = INSTANCE->getEventObserver();
	if(eventObserver) {
		Event event(USB::Event_USB_Active, (uint8_t) USB::Event_USB_Device_PAX_D200);
		eventObserver->proc(&event);
	}
}

#ifdef USB_ACCESSORY_MODE
	void USB::setResetCounterBuf(uint8_t* buf) {
		_buf = buf;
		if(*((uint8_t*)_buf) != 0 && *((uint8_t*)_buf) > MAX_RESET_COUNT) {
			*((uint8_t*)_buf) = 0;
		}
	}

	bool USB::incReset() {
		if(_buf != nullptr && _buf[0] < MAX_RESET_COUNT) {
			*((uint8_t*)_buf) = *((uint8_t*)_buf) + 1;
			return true;
		}
		return false;
	}
#endif

#ifndef USB_ACCESSORY_MODE
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id) {
	USB::get()->getController()->userProcess(id);

	EventObserver *eventObserver = INSTANCE->getEventObserver();
	switch (id) {
	case HOST_USER_SELECT_CONFIGURATION:
		LOG_DEBUG(LOG_USB, "USB Select configuration");
		break;

	case HOST_USER_CLASS_SELECTED:
		LOG_DEBUG(LOG_USB, "USB Class selected");
		break;

	case HOST_USER_DISCONNECTION:
		LOG_DEBUG(LOG_USB, "USB Disconnection");

		usb_state = APPLICATION_DISCONNECT;
		if (eventObserver) {
			Event event(USB::Event_USB_Disconnected);
			eventObserver->proc(&event);
		}
		break;

	case HOST_USER_CLASS_ACTIVE:
		LOG_DEBUG(LOG_USB, "USB Active");

		usb_state = APPLICATION_READY;
		if (INSTANCE->getTransmitBuffer())
			INSTANCE->getTransmitBuffer()->clear();

		if (INSTANCE->getReceiveBuffer())
			INSTANCE->getReceiveBuffer()->clear();

		INSTANCE->startTimer();
		break;

	case HOST_USER_CONNECTION:
		LOG_DEBUG(LOG_USB, "USB Connection");

		usb_state = APPLICATION_START;
		break;

	default:
		LOG_ERROR(LOG_USB, "USB, unknown userProcess id: " << id);
		break;
	}
}
#endif

extern "C" uint32_t USBH_ALL_TransmitRequest(uint8_t *writeData, const uint32_t maxLen)
{
	if (INSTANCE == NULL) return 0;

	Fifo<uint8_t> *fifo = INSTANCE->getTransmitBuffer();
	if (fifo == NULL) return 0;

	uint32_t cnt = 0;
	while (!fifo->isEmpty() && cnt < maxLen)
		writeData[cnt++] = fifo->pop();

	if(cnt > 0) {
		LOG_DEBUG(LOG_USB, "Fire USBH_ALL_TransmitRequest, len = " << cnt);
	}

	return cnt;
}

extern "C" void USBH_Ibox_TransmitCallback(USBH_HandleTypeDef *phost)
{
	AA_HandleTypeDef *handle = (AA_HandleTypeDef*) phost->pActiveClass->pData;

	LOG_DEBUG(LOG_USB, "Ibox_TransmitCallback, len: " << handle->OutEpSize);

	EventObserver *eventObserver = INSTANCE->getEventObserver();
	if (eventObserver)
	{
		Event event(USB::Event_USB_Data_Transmitted, handle->OutEpSize);
		eventObserver->proc(&event);
	}
}

extern "C" void USBH_Ibox_ReceiveCallback(uint8_t *data, uint32_t len)
{
	LOG_DEBUG(LOG_USB, "USBH_Ibox_ReceiveCallback, len: " << len << ", data: " << data[0] << " " << data[1] << " " << data[2] << " " << data[3]);

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
		Event event(USB::Event_USB_Data_Received, len);
		eventObserver->proc(&event);
	}
}

extern "C" void USBH_PAX_D200_TransmitCallback(USBH_HandleTypeDef *phost)
{
	PAX_D200_HandleTypeDef *handle = (PAX_D200_HandleTypeDef *) phost->pActiveClass->pData;

	LOG_DEBUG(LOG_USB, "USBH_PAX_D200_TransmitCallback, len: " << handle->wlen);

	EventObserver *eventObserver = INSTANCE->getEventObserver();
	if (eventObserver)
	{
		Event event(USB::Event_USB_Data_Transmitted, handle->wlen);
		eventObserver->proc(&event);
	}
}

extern "C" void USBH_PAX_D200_ReceiveCallback(USBH_HandleTypeDef *phost, uint8_t *data, uint32_t len, uint32_t available)
{
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
		Event event(USB::Event_USB_Data_Received, len);
		eventObserver->proc(&event);
	}
}

extern "C" void USBH_CDC_TransmitCallback(USBH_HandleTypeDef *phost)
{
	CDC_HandleTypeDef *handle = (CDC_HandleTypeDef *) phost->pActiveClass->pData;

	uint32_t len = handle->TxDataLength;

	LOG_DEBUG(LOG_USB, "CDC_TransmitCallback, len: " << len);
	Logger::get()->toWiresharkHex(handle->pTxData, 36);

	EventObserver *eventObserver = INSTANCE->getEventObserver();
	if (eventObserver)
	{
		Event event(USB::Event_USB_Data_Transmitted, len);
		eventObserver->proc(&event);
	}
}

extern "C" void USBH_CDC_ReceiveCallback(USBH_HandleTypeDef *phost)
{
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
		Event event(USB::Event_USB_Data_Received, len2);
		eventObserver->proc(&event);
	}
}

extern "C" void USBH_CDC_LineCodingChanged(USBH_HandleTypeDef *phost)
{
	LOG_DEBUG(LOG_USB, "CDC_LineCodingChanged");
}

//extern "C" void USBH_HID_EventCallback(USBH_HandleTypeDef *phost)
//{
//	 switch (usb_state)
//	  {
//	  case HOST_USER_CLASS_ACTIVE:
//	  {
//		  HID_KEYBD_Info_TypeDef *k_pinfo;
//		  HID_MOUSE_Info_TypeDef *m_pinfo;
//		  HID_TypeTypeDef devtype = USBH_HID_GetDeviceType(&hUsbHostFS);
//
//		  switch (devtype)
//		   {
//		   case HID_KEYBOARD:
//		   	   {
//		   		   k_pinfo = USBH_HID_GetKeybdInfo(&hUsbHostFS);
//		   		   if (k_pinfo != NULL)
//		   		   {
//		   			   LOG_DEBUG(LOG_USB, "Keyboard, state: " << k_pinfo->state << ", lctrl: " << k_pinfo->lctrl );
//		   		   }
//		   	   }
//		   break;
//
//		   case HID_MOUSE:
//		   {
//			   m_pinfo = USBH_HID_GetMouseInfo(&hUsbHostFS);
//			   if (m_pinfo != NULL)
//			   {
//	   			   LOG_DEBUG(LOG_USB, "Mouse, x: " << m_pinfo->x << ", y: " << m_pinfo->y);
//			   }
//		   }
//		   break;
//
//		   default:
//			   LOG_DEBUG(LOG_USB, "Unknown devtype: " << devtype);
//		   break;
//		   }
//
//	  }
//	  break;
//
//	  default:
//		   LOG_DEBUG(LOG_USB, "Unknown usb_state: " << usb_state);
//	  break;
//	  }
//}

extern "C" uint32_t USB_GetCurrentAndLastTimeDiff(uint32_t lastTimeMs)
{
	return SystemTimer::get()->getCurrentAndLastTimeDiff(lastTimeMs);
}

extern "C" uint32_t USB_GetCurrentMs()
{
	return SystemTimer::get()->getMs();
}

// FIXME: USB, переделать на асинхронное ожидание!
extern "C" void HAL_Delay(uint32_t delay)
{
	SystemTimer::get()->delay_ms(delay);
}

extern "C" void HAL_Delay_us(uint32_t delay)
{
	SystemTimer::get()->delay_us(delay);
}

extern "C" void OTG_FS_IRQHandler(void)
{
//	USB_Probe_B(5, 1);

	HAL_HCD_IRQHandler(&hhcd_USB_OTG_FS);

//	USB_Probe_B(5, 0);
}

extern "C" void TIM7_IRQHandler(void)
{
	TIM_ClearITPendingBit(TIM7, TIM_IT_Update);

	if(INSTANCE == NULL) return;
	INSTANCE->execute();
}

//extern "C" void USB_Probe_B(int port, int stat)
//{
//	if (stat == 0)
//	{
//		GPIO_ResetBits(GPIOB, 1 << port);
//	}
//	else if (stat == 1)
//	{
//		GPIO_SetBits(GPIOB, 1 << port);
//	}
//	else
//	{
//		GPIO_ToggleBits(GPIOB, 1 << port);
//	}
//}
#endif
