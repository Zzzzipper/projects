#ifndef uart_interface_H_
#define uart_interface_H_

#include <stdint.h>

class UartReceiveHandler {
public:
	UartReceiveHandler() : len(1) {}
	UartReceiveHandler(uint16_t len) : len(len) {}
	virtual ~UartReceiveHandler() {}
	void setLen(uint16_t len) {	this->len = len; }		
	uint16_t getLen() {	return len;	}		
	virtual void handle() = 0;
	virtual bool isInterruptMode() { return false; }

private:
	uint16_t len;
};

class UartTransmitHandler {
public:
	virtual void emptyTransmitBuffer() = 0;
};

class AbstractUart {
public:
	virtual ~AbstractUart() {}
	virtual void setReceiveHandler(UartReceiveHandler *handler) = 0;
	virtual void setTransmitHandler(UartTransmitHandler *handler) = 0;
	virtual void send(uint8_t b) = 0;
	virtual void sendAsync(uint8_t b) = 0;
	virtual uint8_t receive() = 0;
	virtual bool isEmptyReceiveBuffer() = 0;
	virtual bool isFullTransmitBuffer() = 0;
	virtual void execute() = 0;
};

#endif
