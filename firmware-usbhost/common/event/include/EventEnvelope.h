#ifndef COMMON_EVENT_ENVELOPE_H
#define COMMON_EVENT_ENVELOPE_H

#include "utils/include/Buffer.h"
#include "utils/include/StringBuilder.h"

struct EventParamHeader;

class EventEnvelope {
public:
	EventEnvelope(uint16_t size);
	bool copy(EventEnvelope *envelope);
	void setDevice(uint16_t device);
	void setType(uint16_t type);
	bool addUint8(uint16_t id, uint8_t value);
	bool addUint16(uint16_t id, uint16_t value);
	bool addUint32(uint16_t id, uint32_t value);
	bool addString(uint16_t id, const char *str);
	bool addString(uint16_t id, const char *str, uint16_t strLen);
	bool addData(uint16_t id, const uint8_t *data, uint16_t dataLen);
	uint16_t getDevice();
	uint16_t getType();
	bool getUint8(uint16_t id, uint8_t *buf);
	bool getUint16(uint16_t id, uint16_t *buf);
	bool getUint32(uint16_t id, uint32_t *buf);
	bool getString(uint16_t id, char *buf, uint16_t bufSize);
	bool getString(uint16_t id, StringBuilder *buf);
	bool getData(uint16_t id, uint8_t *buf, uint16_t bufSize, uint16_t *bufLen);
	void clear();

//+++
	bool setParam(const uint8_t *data, uint16_t dataLen);
	bool getParam(uint8_t *data, uint16_t dataLen);
//+++

private:
	uint16_t device;
	uint16_t type;
	Buffer buf;

	EventParamHeader *find(uint16_t id);
	EventParamHeader *find(uint16_t id, uint16_t type);
};

#endif
