#ifndef COMMON_EXECUTIVE_SLAVERECEIVER_H_
#define COMMON_EXECUTIVE_SLAVERECEIVER_H_

#include "uart/include/interface.h"

#include <stddef.h>

class ExeSlaveReceiver : public UartReceiveHandler {
public:
	class Customer {
	public:
		virtual void recvRequest(uint8_t b) = 0;
	};

	ExeSlaveReceiver(Customer *customer);
	void setUart(AbstractUart *uart, AbstractUart *sendUart = NULL);
	virtual void handle();
	virtual bool isInterruptMode();

protected:
	Customer *customer;
	AbstractUart *uart;
	AbstractUart *sendUart;
};

#endif
