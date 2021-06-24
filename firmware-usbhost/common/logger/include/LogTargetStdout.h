#ifndef COMMON_LOGGER_LOGTARGETSTDOUT_H
#define COMMON_LOGGER_LOGTARGETSTDOUT_H

#include <stdio.h>

class LogTargetStdout : public LogTarget {
public:
	virtual void send(const uint8_t *data, const uint16_t len) {
		printf("%.*s", len, data);
		fflush(stdout);
	}
};

#endif
