#ifndef COMMON_LOGGER_LOGTARGETUART_H_
#define COMMON_LOGGER_LOGTARGETUART_H_

#include "logger/include/Logger.h"

class AbstractUart;

class LogTargetUart : public LogTarget {
public:
	LogTargetUart(AbstractUart *uart);
	void send(const uint8_t *data, const uint16_t len);

private:
	AbstractUart *uart;	
};

#endif
