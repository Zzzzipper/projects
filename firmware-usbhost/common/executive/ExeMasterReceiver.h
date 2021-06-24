#ifndef COMMON_EXECUTIVE_MASTERRECEIVER_H_
#define COMMON_EXECUTIVE_MASTERRECEIVER_H_

#include "uart/include/interface.h"

#include <stddef.h>

class ExeMasterReceiver : public UartReceiveHandler {
public:
	class Customer {
	public:
		virtual void recvResponse(uint8_t b) = 0;
	};

	ExeMasterReceiver(Customer *customer);
	void setUart(AbstractUart *uart, AbstractUart *sendUart = NULL);
	virtual void handle();
	virtual bool isInterruptMode();

protected:
	Customer *customer;
	AbstractUart *uart;
	AbstractUart *sendUart;
};

#endif
