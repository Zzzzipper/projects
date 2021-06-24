#include "config.h"
#ifdef DEVICE_USB2
#include "Usb2PaxD200.h"
#include "Usb2Engine.h"

#include "logger/include/Logger.h"

#define INGENICO_RX_BUFFER_SIZE 256
#define INGENICO_TX_BUFFER_SIZE 256

extern USBH_HandleTypeDef hUsbHostFS;
static uint8_t usb_cdc_rx_buff[INGENICO_RX_BUFFER_SIZE];
static uint8_t usb_cdc_tx_buff[INGENICO_TX_BUFFER_SIZE];

Usb2PaxD200::Usb2PaxD200() :
	timerEngine(NULL),
	rxBuffer(NULL),
	txBuffer(NULL),
	rxLock(true),
	txLock(true)
{

}

void Usb2PaxD200::init(TimerEngine *timerEngine) {
	LOG_DEBUG(LOG_USB, "init");
	this->timerEngine = timerEngine;
	this->timer = timerEngine->addTimer<Usb2PaxD200, &Usb2PaxD200::procTimer>(this);

	if(rxBuffer == NULL) { rxBuffer = new Fifo<uint8_t>(INGENICO_RX_BUFFER_SIZE); }
	if(txBuffer == NULL) { txBuffer = new Fifo<uint8_t>(INGENICO_TX_BUFFER_SIZE); }

	this->rxLock = true;
	this->txLock = true;

//	USBH_RegisterClass(&hUsbHostFS, USBH_CDC_CLASS);
	USBH_RegisterClass(&hUsbHostFS, &Vendor_PAX_D200_Class);
}

void Usb2PaxD200::deinit() {
	if(txBuffer) delete txBuffer;
	if(rxBuffer) delete rxBuffer;
	txBuffer = NULL;
	rxBuffer = NULL;
}

void Usb2PaxD200::setReceiveHandler(UartReceiveHandler *handler) {
	this->rxHandler = handler;
}

void Usb2PaxD200::setTransmitHandler(UartTransmitHandler *handler) {
	this->txHandler = handler;
}

void Usb2PaxD200::send(uint8_t b) {
	if(txBuffer == NULL) { return; }
	txBuffer->push(b);
}

void Usb2PaxD200::sendAsync(uint8_t b) {
	if(txBuffer == NULL) { return; }
	txBuffer->push(b);
}

uint8_t Usb2PaxD200::receive() {
	if(rxBuffer == NULL) { return 0; }
	return rxBuffer->pop();
}

bool Usb2PaxD200::isEmptyReceiveBuffer() {
	if(rxBuffer == NULL) { return true; }
	return rxBuffer->isEmpty();
}

bool Usb2PaxD200::isFullTransmitBuffer() {
	if(txBuffer == NULL) { return false; }
	return txBuffer->isFull();
}

void Usb2PaxD200::execute() {
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
}

void Usb2PaxD200::procActive() {
	timer->start(30000);
}

void Usb2PaxD200::procTimer() {
	LOG_DEBUG(LOG_USB, "Start transmiting");
	txBuffer->clear();
	rxBuffer->clear();
	rxLock = false;
	txLock = false;
}

void Usb2PaxD200::procTransmitCallback(USBH_HandleTypeDef *phost) {
	LOG_DEBUG(LOG_USB, "CDC_TransmitCallback");
	txLock = false;
}

void Usb2PaxD200::procReceiveCallback(USBH_HandleTypeDef *phost) {
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
}
#endif
