
#ifndef COMMON_RTT_UARTLOGTARGET_H_
#define COMMON_RTT_UARTLOGTARGET_H_

#include "logger/include/Logger.h"

class AbstractUart;

class RttLogTarget : public LogTarget {
public:
	RttLogTarget(AbstractUart *uart);
	void send(const uint8_t *data, const uint16_t len);

private:
	AbstractUart *uart;
};

#endif
