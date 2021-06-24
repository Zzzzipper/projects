#ifndef COMMON_LOGGER_LOGTARGETRAM_H_
#define COMMON_LOGGER_LOGTARGETRAM_H_

#include "utils/include/Buffer.h"
#include "logger/include/Logger.h"

class LogTargetRam : public LogTarget {
public:
	LogTargetRam(uint16_t size);
	virtual void send(const uint8_t *data, const uint16_t len);
	void clear();
	uint8_t *getData();
	uint16_t getLen();

private:
	Buffer buf;
};

#endif
