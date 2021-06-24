#include "config.h"
#ifdef DEVICE_USB2
#include "Usb2Ingenico.h"
#include "Usb2Engine.h"

#include "logger/include/Logger.h"

#define INGENICO_RX_BUFFER_SIZE 256
#define INGENICO_TX_BUFFER_SIZE 256

extern USBH_HandleTypeDef hUsbHostFS;
CDC_LineCodingTypeDef cdcFrameFormat;

static uint8_t usb_cdc_rx_buff[INGENICO_RX_BUFFER_SIZE];
static uint8_t usb_cdc_tx_buff[INGENICO_TX_BUFFER_SIZE];

Usb2Ingenico::Usb2Ingenico() :
	timerEngine(NULL),
	rxBuffer(NULL),
	txBuffer(NULL),
	rxLock(true),
	txLock(true)
{

}

void Usb2Ingenico::init(TimerEngine *timerEngine) {
	LOG_DEBUG(LOG_USB, "init");
	this->timerEngine = timerEngine;
	this->timer = timerEngine->addTimer<Usb2Ingenico, &Usb2Ingenico::procTimer>(this);

	if(rxBuffer == NULL) { rxBuffer = new Fifo<uint8_t>(INGENICO_RX_BUFFER_SIZE); }
	if(txBuffer == NULL) { txBuffer = new Fifo<uint8_t>(INGENICO_TX_BUFFER_SIZE); }

	this->rxLock = true;
	this->txLock = true;

	cdcFrameFormat.b.dwDTERate = 115200;
	cdcFrameFormat.b.bCharFormat = 0;
	cdcFrameFormat.b.bDataBits = 8;
	cdcFrameFormat.b.bParityType = 0;

	USBH_RegisterClass(&hUsbHostFS, USBH_CDC_CLASS);
}

void Usb2Ingenico::deinit() {
	if(txBuffer) delete txBuffer;
	if(rxBuffer) delete rxBuffer;
	txBuffer = NULL;
	rxBuffer = NULL;
}

void Usb2Ingenico::setReceiveHandler(UartReceiveHandler *handler) {
	this->rxHandler = handler;
}

void Usb2Ingenico::setTransmitHandler(UartTransmitHandler *handler) {
	this->txHandler = handler;
}

void Usb2Ingenico::send(uint8_t b) {
	if(txBuffer == NULL) { return; }
	txBuffer->push(b);
}

void Usb2Ingenico::sendAsync(uint8_t b) {
	if(txBuffer == NULL) { return; }
	txBuffer->push(b);
}

uint8_t Usb2Ingenico::receive() {
	if(rxBuffer == NULL) { return 0; }
	return rxBuffer->pop();
}

bool Usb2Ingenico::isEmptyReceiveBuffer() {
	if(rxBuffer == NULL) { return true; }
	return rxBuffer->isEmpty();
}

bool Usb2Ingenico::isFullTransmitBuffer() {
	if(txBuffer == NULL) { return false; }
	return txBuffer->isFull();
}

void Usb2Ingenico::execute() {
#if 0
	CDC_HandleTypeDef *handle = (CDC_HandleTypeDef*) phost->pActiveClass->pData;
	if(handle->txLock == false) {
		if((handle->state == CDC_IDLE_STATE) || (handle->state == CDC_TRANSFER_DATA))
		{
			uint32_t len = USBH_ALL_TransmitRequest(usb_cdc_tx_buff, sizeof(usb_cdc_tx_buff));
			if(len) {
				if(USBH_CDC_Transmit(phost, usb_cdc_tx_buff, len) == USBH_OK) {
					USBH_DbgLog ("t %d", len);
					handle->txLock = true;
				}
			}
		}
	}

	if(handle->rxLock == false) {
		if(USBH_CDC_Receive(phost, usb_cdc_rx_buff, sizeof(usb_cdc_rx_buff)) == USBH_OK) {
			USBH_DbgLog ("r");
			handle->rxLock = true;
		}
	}
#else
	CDC_HandleTypeDef *handle = (CDC_HandleTypeDef*)hUsbHostFS.pActiveClass->pData;
	if(txLock == false) {
		if((handle->state == CDC_IDLE_STATE) || (handle->state == CDC_TRANSFER_DATA)) {
			if(txBuffer == NULL) { return; }
			uint32_t txLen = 0;
			while(txBuffer->isEmpty() == false && txLen < sizeof(usb_cdc_tx_buff)) {
				usb_cdc_tx_buff[txLen] = txBuffer->pop();
				txLen++;
			}
			if(txLen > 0) {
				if(USBH_CDC_Transmit(&hUsbHostFS, usb_cdc_tx_buff, txLen) == USBH_OK) {
					LOG_DEBUG(LOG_USB, "t " << txLen);
					txLock = true;
				}
			}
		}
	}

	if(rxLock == false) {
		if(USBH_CDC_Receive(&hUsbHostFS, usb_cdc_rx_buff, sizeof(usb_cdc_rx_buff)) == USBH_OK) {
			LOG_DEBUG(LOG_USB, "r");
			rxLock = true;
		}
	}
#endif
}

void Usb2Ingenico::procActive() {
#if 0
	if(INSTANCE->getTransmitBuffer()) INSTANCE->getTransmitBuffer()->clear();
	if(INSTANCE->getReceiveBuffer()) INSTANCE->getReceiveBuffer()->clear();
	INSTANCE->startTimer();
#else
	timer->start(30000);
#endif
}

void Usb2Ingenico::procTimer() {
	LOG_DEBUG(LOG_USB, "Start transmiting");
	txBuffer->clear();
	rxBuffer->clear();
	rxLock = false;
	txLock = false;
}

void Usb2Ingenico::procTransmitCallback(USBH_HandleTypeDef *phost) {
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
	LOG_DEBUG(LOG_USB, "CDC_TransmitCallback");
	txLock = false;
#endif
}

void Usb2Ingenico::procReceiveCallback(USBH_HandleTypeDef *phost) {
#if 0
	CDC_HandleTypeDef *handle = (CDC_HandleTypeDef *) phost->pActiveClass->pData;
	uint32_t len1 = handle->RxDataLength;
	uint32_t len = USBH_CDC_GetLastReceivedDataSize(phost);

	LOG_DEBUG(LOG_USB, "CDC_ReceiveCallback: len1: " << len1 << ", len2: " << len);

	Fifo<uint8_t> *receiveBuffer = INSTANCE->getReceiveBuffer();
	if (receiveBuffer == NULL)
	{
		LOG_ERROR(LOG_USB, "USB receive buffer doesn't initialized!");
		return;
	}

	if (!len)
	{
		LOG_WARN(LOG_USB, "CDC_ReceiveCallback: Data is empty !");
		return;
	}

	uint8_t *data = handle->pRxData;

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
	CDC_HandleTypeDef *handle = (CDC_HandleTypeDef *)phost->pActiveClass->pData;
	uint32_t len = USBH_CDC_GetLastReceivedDataSize(phost);
	LOG_DEBUG(LOG_USB, "CDC_ReceiveCallback: len=" << len);
	rxLock = false;

	if(rxBuffer == NULL) {
		LOG_ERROR(LOG_USB, "USB receive buffer doesn't initialized!");
		return;
	}

	if(len == 0) {
		LOG_WARN(LOG_USB, "CDC_ReceiveCallback: Data is empty !");
		return;
	}

	uint8_t *data = handle->pRxData;
	for(uint32_t i = 0; i < len; i++) {
		if(rxBuffer->push(data[i]) == false) {
			LOG_ERROR(LOG_USB, "USB receive buffer is full! Received data was lost!");
			break;
		}
	}

	rxHandler->handle();
#endif
}
#endif
