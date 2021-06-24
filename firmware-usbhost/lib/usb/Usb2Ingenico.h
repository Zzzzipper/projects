#include "config.h"
#ifdef DEVICE_USB2
#pragma once

#include "common/uart/include/interface.h"
#include "common/timer/include/TimerEngine.h"
#include "common/utils/include/Fifo.h"
#include "common/utils/include/Event.h"

#include "usb2h_cdc.h"

class Usb2Ingenico : public AbstractUart {
public:
	Usb2Ingenico();
	void init(TimerEngine *timerEngine);
	void deinit();

	void setReceiveHandler(UartReceiveHandler *handler) override;
	void setTransmitHandler(UartTransmitHandler *handler) override;
	void send(uint8_t b) override;
	void sendAsync(uint8_t b) override;
	uint8_t receive() override;
	bool isEmptyReceiveBuffer() override;
	bool isFullTransmitBuffer() override;
	void execute() override;

	void procActive();
	void procTimer();
	void procReceiveCallback(USBH_HandleTypeDef *phost);
	void procTransmitCallback(USBH_HandleTypeDef *phost);

private:
	TimerEngine *timerEngine;
	Timer *timer;
	Fifo<uint8_t> *rxBuffer;
	Fifo<uint8_t> *txBuffer;
	uint8_t rxLock;
	uint8_t txLock;
	UartReceiveHandler *rxHandler;
	UartTransmitHandler *txHandler;
};
#endif
