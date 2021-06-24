#include "config.h"
#ifdef DEVICE_USB
#ifndef LIB_USB_UART_H_
#define LIB_USB_UART_H_

#include "common/uart/include/interface.h"
#include "common/utils/include/Buffer.h"
#include "lib/usb/USB.h"

class UsbUart : public AbstractUart, public EventObserver {
public:
	UsbUart(USB *usb);

	void setReceiveHandler(UartReceiveHandler *handler) override;
	void setTransmitHandler(UartTransmitHandler *handler) override;
	void send(uint8_t b) override;
	void sendAsync(uint8_t b) override;
	uint8_t receive() override;
	bool isEmptyReceiveBuffer() override;
	bool isFullTransmitBuffer() override;
	void execute() override;

	void proc(Event *event) override;

private:
	USB *usb;
	Buffer rxBuf;
	Buffer txBuf;
	UartReceiveHandler *rxHandler;
	UartTransmitHandler *txHandler;

	void procEventUsbActive();
	void procEventUsbDisconnected();
	void procEventUsbDataReceived();
	void procEventUsbDataTransmitted();
};

#endif
#endif
