#include "config.h"
#ifdef DEVICE_USB
#include "UsbUart.h"

#include "logger/include/Logger.h"

UsbUart::UsbUart(USB *usb) :
	usb(usb),
	rxBuf(1024),
	txBuf(1024),
	rxHandler(NULL),
	txHandler(NULL)

{
	usb->setTransmitBuffer(txBuf.getData(), txBuf.getSize());
	usb->setReceiveBuffer(rxBuf.getData(), txBuf.getSize());
	usb->setEventObserver(this);
}

void UsbUart::setReceiveHandler(UartReceiveHandler *handler) {
	LOG_DEBUG(LOG_USB_UART, "setReceiveHandler");
	rxHandler = handler;
}

void UsbUart::setTransmitHandler(UartTransmitHandler *handler) {
	txHandler = handler;
}

void UsbUart::send(uint8_t b) {
	usb->getTransmitBuffer()->push(b);
}

void UsbUart::sendAsync(uint8_t b) {
	usb->getTransmitBuffer()->push(b);
}

uint8_t UsbUart::receive() {
	return usb->getReceiveBuffer()->pop();
}

bool UsbUart::isEmptyReceiveBuffer() {
	return usb->getReceiveBuffer()->isEmpty();
}

bool UsbUart::isFullTransmitBuffer() {
	return usb->getTransmitBuffer()->isFull();
}

void UsbUart::execute() {

}

void UsbUart::proc(Event *event) {
	switch(event->getType()) {
		case USB::Event_USB_Active: procEventUsbActive(); return;
		case USB::Event_USB_Disconnected: procEventUsbDisconnected(); return;
		case USB::Event_USB_Data_Received: procEventUsbDataReceived(); return;
		case USB::Event_USB_Data_Transmitted: procEventUsbDataTransmitted(); return;
		default: LOG_ERROR(LOG_USB_UART, "Unwaited event: event=" << event->getType());
	}
}

void UsbUart::procEventUsbActive() {
	LOG_DEBUG(LOG_USB_UART, "procEventUsbActive");
}

void UsbUart::procEventUsbDisconnected() {
	LOG_DEBUG(LOG_USB_UART, "procEventUsbDisconnected");
}

void UsbUart::procEventUsbDataReceived() {
	LOG_DEBUG(LOG_USB_UART, "procEventUsbDataReceived");
	if(rxHandler) { rxHandler->handle(); }
}

void UsbUart::procEventUsbDataTransmitted() {
	LOG_DEBUG(LOG_USB_UART, "procEventUsbDataTransmitted");
}

/*
 * INPAS
USB Device Attached
PID: 101h
VID: 1234h
Address (#1) assigned.
Manufacturer : PAX
Product : d200
Serial Number : N/A
Enumeration done.
This device has only 1 configuration.
Default configuration set.
Switching to Interface (#0)
Class    : ffh
SubClass : 0h
Protocol : 0h
DEBUG : PAX_D200_Reset
D200 class started.
W 19.07.14 17:52:51 USB.cpp#241 USBH_UserProcess, id: 3
DEBUG : ”стройство инициализировано, начинаем опрос
ERROR: Len!=2: 0

 SBERBANK
USB Device Attached
PID: 101h
VID: 1234h
Address (#1) assigned.
Manufacturer : PAX
Product : d200
Serial Number : N/A
Enumeration done.
This device has only 1 configuration.
Default configuration set.
Switching to Interface (#0)
Class    : ffh
SubClass : 0h
Protocol : 0h
DEBUG : PAX_D200_Reset
D200 class started.
W 19.07.14 17:53:07 USB.cpp#241 USBH_UserProcess, id: 3
DEBUG : ”стройство инициализировано, начинаем опрос
DEBUG : maxdata: 508

PID: 83h
VID: b00h
Address (#1) assigned.
Manufacturer : INGENICO
Product : Link2500
Serial Number : 000000000000000000000000
Enumeration done.
This device has only 1 configuration.
Default configuration set.
USB Check Class
USB Check Class Code=0x2, Pid=0x0083, Vid=0x0B00
Switching to Interface (#0)
Class    : 2h
SubClass : 2h
Protocol : 1h
CDC class started.
 */
#endif
