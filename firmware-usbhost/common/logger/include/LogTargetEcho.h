#ifndef COMMON_LOGGER_LOGTARGETECHO_H_
#define COMMON_LOGGER_LOGTARGETECHO_H_

#include "logger/include/Logger.h"

class AbstractUart;

class LogTargetEcho : public LogTarget {
public:
	LogTargetEcho(uint8_t *buf, uint32_t bufSize);
	void send(const uint8_t *data, const uint16_t len);

private:
	uint8_t *buf;
	uint32_t bufSize;
	uint32_t index;
};

#endif
