#include "include/EventEnvelope.h"

#include "logger/include/Logger.h"

#include <string.h>

enum EventParamType {
	EventParamType_Uint8 = 0,
	EventParamType_Uint16,
	EventParamType_Uint32,
	EventParamType_String,
	EventParamType_Data
};

#pragma pack(push,1)
struct EventParamHeader {
	uint16_t id;
	uint8_t type;
};

struct EventParamUint8 {
	uint16_t id;
	uint8_t type;
	uint8_t value;

	static uint16_t getParamSize() {
		return sizeof(EventParamUint8);
	}

	void set(uint16_t id, uint8_t value) {
		this->id = id;
		this->type = EventParamType_Uint8;
		this->value = value;
	}
};

struct EventParamUint16 {
	uint16_t id;
	uint8_t type;
	uint16_t value;

	static uint16_t getParamSize() {
		return sizeof(EventParamUint16);
	}

	void set(uint16_t id, uint16_t value) {
		this->id = id;
		this->type = EventParamType_Uint16;
		this->value = value;
	}
};

struct EventParamUint32 {
	uint16_t id;
	uint8_t type;
	uint32_t value;

	static uint16_t getParamSize() {
		return sizeof(EventParamUint32);
	}

	void set(uint16_t id, uint32_t value) {
		this->id = id;
		this->type = EventParamType_Uint32;
		this->value = value;
	}
};

struct EventParamString {
	uint16_t id;
	uint8_t type;
	uint16_t len;
	char value[0];

	static uint16_t getParamSize(const char *str) {
		uint16_t i = 0;
		for(; str[i] != '\0'; i++);
		return (sizeof(EventParamString) + i + 1);
	}

	uint16_t getParamSize() {
		return (sizeof(EventParamString) + this->len);
	}

	void set(uint16_t id, const char *str) {
		this->id = id;
		this->type = EventParamType_String;
		uint16_t i = 0;
		for(; str[i] != '\0'; i++) {
			value[i] = str[i];
		}
		this->len = i + 1;
		this->value[i] = '\0';
	}

	bool get(char *buf, uint16_t bufSize) {
		for(uint16_t i = 0; i < this->len; i++) {
			if(i > bufSize) { return false; }
			buf[i] = this->value[i];
		}
		return true;
	}
};

struct EventParamData {
	uint16_t id;
	uint8_t type;
	uint16_t len;
	uint8_t value[0];

	static uint16_t getParamSize(uint16_t dataLen) {
		return (sizeof(EventParamData) + dataLen);
	}

	uint16_t getParamSize() {
		return (sizeof(EventParamData) + this->len);
	}

	void set(uint16_t id, const uint8_t *data, uint16_t dataLen) {
		this->id = id;
		this->type = EventParamType_Data;
		for(uint16_t i = 0; i < dataLen; i++) {
			value[i] = data[i];
		}
		this->len = dataLen;
	}

	bool get(uint8_t *buf, uint16_t bufSize, uint16_t *bufLen) {
		for(uint16_t i = 0; i < this->len; i++) {
			if(i > bufSize) { return false; }
			buf[i] = this->value[i];
		}
		if(bufLen != NULL) { *bufLen = this->len; }
		return true;
	}
};
#pragma pack(pop)

EventEnvelope::EventEnvelope(uint16_t size) : device(0), type(0), buf(size) {

}

bool EventEnvelope::copy(EventEnvelope *envelope) {
	if(this->buf.getSize() < envelope->buf.getLen()) {
		return false;
	}
	this->device = envelope->device;
	this->type = envelope->type;
	this->buf.clear();
	this->buf.add(envelope->buf.getData(), envelope->buf.getLen());
	return true;
}

void EventEnvelope::setDevice(uint16_t device) {
	this->device = device;
}

void EventEnvelope::setType(uint16_t type) {
	this->type = type;
}

bool EventEnvelope::addUint8(uint16_t id, uint8_t value) {
	uint16_t dataLen = buf.getLen();
	uint16_t freeSpace = buf.getSize() - dataLen;
	uint16_t paramSize = EventParamUint8::getParamSize();
	if(freeSpace < paramSize) {
		return false;
	}
	uint8_t *data = buf.getData() + dataLen;
	EventParamUint8 *param = (EventParamUint8*)data;
	param->set(id, value);
	buf.setLen(dataLen + paramSize);
	return true;
}

bool EventEnvelope::addUint16(uint16_t id, uint16_t value) {
	uint16_t dataLen = buf.getLen();
	uint16_t freeSpace = buf.getSize() - dataLen;
	uint16_t paramSize = EventParamUint16::getParamSize();
	if(freeSpace < paramSize) {
		return false;
	}
	uint8_t *data = buf.getData() + dataLen;
	EventParamUint16 *param = (EventParamUint16*)data;
	param->set(id, value);
	buf.setLen(dataLen + paramSize);
	return true;
}

bool EventEnvelope::addUint32(uint16_t id, uint32_t value) {
	uint16_t dataLen = buf.getLen();
	uint16_t freeSpace = buf.getSize() - dataLen;
	uint16_t paramSize = EventParamUint32::getParamSize();
	if(freeSpace < paramSize) {
		return false;
	}
	uint8_t *data = buf.getData() + dataLen;
	EventParamUint32 *param = (EventParamUint32*)data;
	param->set(id, value);
	buf.setLen(dataLen + paramSize);
	return true;
}

bool EventEnvelope::addString(uint16_t id, const char *str) {
	uint16_t dataLen = buf.getLen();
	uint16_t freeSpace = buf.getSize() - dataLen;
	uint16_t paramSize = EventParamString::getParamSize(str);
	if(freeSpace < paramSize) {
		return false;
	}
	uint8_t *data = buf.getData() + buf.getLen();
	EventParamString *param = (EventParamString*)data;
	param->set(id, str);
	buf.setLen(dataLen + paramSize);
	return true;
}

bool EventEnvelope::addString(uint16_t id, const char *str, uint16_t strLen) {
	uint16_t dataLen = buf.getLen();
	uint16_t freeSpace = buf.getSize() - dataLen;
	uint16_t paramSize = EventParamString::getParamSize(str);
	if(freeSpace < paramSize) {
		return false;
	}
	uint8_t *data = buf.getData() + buf.getLen();
	EventParamString *param = (EventParamString*)data;
	param->set(id, str);
	buf.setLen(dataLen + paramSize);
	return true;
}

bool EventEnvelope::addData(uint16_t id, const uint8_t *val, uint16_t valLen) {
	uint16_t dataLen = buf.getLen();
	uint16_t freeSpace = buf.getSize() - dataLen;
	uint16_t paramSize = EventParamData::getParamSize(valLen);
	if(freeSpace < paramSize) {
		return false;
	}
	uint8_t *data = buf.getData() + buf.getLen();
	EventParamData *param = (EventParamData*)data;
	param->set(id, val, valLen);
	buf.setLen(dataLen + paramSize);
	return true;
}

uint16_t EventEnvelope::getDevice() {
	return device;
}

uint16_t EventEnvelope::getType() {
	return type;
}

bool EventEnvelope::getUint8(uint16_t id, uint8_t *buf) {
	EventParamUint8 *param = (EventParamUint8*)find(id, EventParamType_Uint8);
	if(param == NULL) {
		return false;
	}
	*buf = param->value;
	return true;
}

bool EventEnvelope::getUint16(uint16_t id, uint16_t *buf) {
	EventParamUint16 *param = (EventParamUint16*)find(id, EventParamType_Uint16);
	if(param == NULL) {
		return false;
	}
	*buf = param->value;
	return true;
}

bool EventEnvelope::getUint32(uint16_t id, uint32_t *buf) {
	EventParamUint32 *param = (EventParamUint32*)find(id, EventParamType_Uint32);
	if(param == NULL) {
		return false;
	}
	*buf = param->value;
	return true;
}

bool EventEnvelope::getString(uint16_t id, char *buf, uint16_t bufSize) {
	EventParamString *param = (EventParamString*)find(id, EventParamType_String);
	if(param == NULL) {
		return false;
	}
	return param->get(buf, bufSize);
}

bool EventEnvelope::getString(uint16_t id, StringBuilder *buf) {
	EventParamString *param = (EventParamString*)find(id, EventParamType_String);
	if(param == NULL) {
		return false;
	}
	buf->clear();
	buf->addStr(param->value, param->len);
	return true;
}

bool EventEnvelope::getData(uint16_t id, uint8_t *buf, uint16_t bufSize, uint16_t *bufLen) {
	EventParamData *param = (EventParamData*)find(id, EventParamType_Data);
	if(param == NULL) {
		return false;
	}
	return param->get(buf, bufSize, bufLen);
}

bool EventEnvelope::setParam(const uint8_t *data, uint16_t dataLen) {
	if(buf.getSize() < dataLen) {
		return false;
	}
	buf.clear();
	buf.add(data, dataLen);
	return true;
}

bool EventEnvelope::getParam(uint8_t *data, uint16_t dataLen) {
	if(buf.getLen() != dataLen) {
		return false;
	}
	memcpy(data, buf.getData(), buf.getLen());
	return true;
}

EventParamHeader *EventEnvelope::find(uint16_t id, uint16_t type) {
	uint16_t i = 0;
	for(uint16_t o = 0; o < buf.getLen() && i < 10; i++) {
		EventParamHeader *hdr = (EventParamHeader*)(buf.getData() + o);
		if(hdr->id == id && hdr->type == type) {
			return hdr;
		}
		switch(hdr->type) {
		case EventParamType_Uint8: o += EventParamUint8::getParamSize(); break;
		case EventParamType_Uint16: o += EventParamUint16::getParamSize(); break;
		case EventParamType_Uint32: o += EventParamUint32::getParamSize(); break;
		case EventParamType_String: o += ((EventParamString*)hdr)->getParamSize(); break;
		default: return NULL;
		}
	}
	return NULL;
}

void EventEnvelope::clear() {
	buf.clear();
}
