#ifndef COMMON_LOGGER_LOGTARGETFILE_H_
#define COMMON_LOGGER_LOGTARGETFILE_H_

#include "logger/include/Logger.h"

#include <QFile>

class LogTargetFile : public LogTarget {
public:
	LogTargetFile();
	~LogTargetFile();
	bool open(const char *filename);
	virtual void send(const uint8_t *data, const uint16_t len);
	void close();

private:
	QFile file;
};

#endif
