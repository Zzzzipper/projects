#ifndef COMMON_LOGGER_REMOTELOGGER_H_
#define COMMON_LOGGER_REMOTELOGGER_H_

#include "include/Logger.h"
#include "timer/include/RealTime.h"

#ifdef REMOTE_LOGGING
#define REMOTE_LOG(LOG_CATEGORY, __args...) if(LOG_CATEGORY >= LOG_LEVEL_ERROR) { RemoteLogger::get()->str(__args); }
#define REMOTE_LOG_STATE(LOG_CATEGORY, __args...) if(LOG_CATEGORY >= LOG_LEVEL_ERROR) { RemoteLogger::get()->state(__args); }
#define REMOTE_LOG_HEX(LOG_CATEGORY, __args...) if(LOG_CATEGORY >= LOG_LEVEL_ERROR) { RemoteLogger::get()->hex(__args); }
#define REMOTE_LOG_IN(LOG_CATEGORY, __args...) if(LOG_CATEGORY >= LOG_LEVEL_ERROR) { RemoteLogger::get()->hex("<", __args); }
#define REMOTE_LOG_OUT(LOG_CATEGORY, __args...) if(LOG_CATEGORY >= LOG_LEVEL_ERROR) { RemoteLogger::get()->hex(">", __args); }
#else
#define REMOTE_LOG(__args...)
#define REMOTE_LOG_STATE(__args...)
#define REMOTE_LOG_HEX(__args...)
#define REMOTE_LOG_IN(__args...)
#define REMOTE_LOG_OUT(__args...)
#endif

class FifoBuf {
public:
	FifoBuf(uint32_t dataSize);
	~FifoBuf();
	void push(char b);
	char *getData();
	uint32_t getDataLen();
	void clear();

private:
	char *data;
	uint32_t dataSize;
	uint32_t dataLen;
	uint32_t index;
};

class RemoteLogger : public LogTarget {
public:
	static RemoteLogger *get();
	void registerRealTime(RealTimeInterface *realtime);
	void time();
	void state(const char *prefix, uint32_t state);
	void str(const char *str);
	void hex(const uint8_t symbol);
	void hex(const char *prefix, const uint8_t b1);
	void hex(const char *prefix, const void *data, uint16_t len);
	char *getData();
	uint32_t getDataLen();

	void send(const uint8_t *data, const uint16_t len) override;

private:
	static RemoteLogger *instance;
	RealTimeInterface *realtime;
	FifoBuf buf;

	RemoteLogger();
	~RemoteLogger();
};

#endif
