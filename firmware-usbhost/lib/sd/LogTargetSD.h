#ifndef COMMON_LOGGER_LOGTARGETSD_H_
#define COMMON_LOGGER_LOGTARGETSD_H_

#include "utils/include/Buffer.h"
#include "logger/include/Logger.h"

class LogTargetSD : public LogTarget {
public:
	LogTargetSD();
	~LogTargetSD();
	bool init();
	void shutdown();
	virtual void send(const uint8_t *data, const uint16_t len);

private:
	void *file;
	uint8_t c;
};

#endif
