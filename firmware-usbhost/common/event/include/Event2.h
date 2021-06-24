#ifndef COMMON_EVENT2_H
#define COMMON_EVENT2_H

#include "EventEnvelope.h"

class EventEngineInterface;

class EventDeviceId {
public:
	EventDeviceId(uint16_t value);
	EventDeviceId(EventEngineInterface *eventEngine);
	void setValue(uint16_t value);
	uint16_t getValue();

private:
	uint16_t value;
};

class EventDevice {
public:
	EventDeviceId getDeviceId();

private:
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
};

class EventInterface {
public:
	EventInterface(uint16_t type) : device((uint16_t)0), type(type) {}
	EventInterface(EventDeviceId device, uint16_t type) : device(device), type(type) {}
	uint16_t getDevice() { return device.getValue(); }
	uint16_t getType() { return type; }
	virtual bool open(EventEnvelope *envelope);
	virtual bool pack(EventEnvelope *envelope);

protected:
	EventDeviceId device;
	uint16_t type;
};

class EventUint8Interface : public EventInterface {
public:
	EventUint8Interface(uint16_t type) : EventInterface(type) {}
	EventUint8Interface(EventDeviceId device, uint16_t type, uint8_t value) : EventInterface(device, type), value(value) {}
	uint8_t getValue() { return value; }
	virtual bool open(EventEnvelope *envelope);
	virtual bool pack(EventEnvelope *envelope);

protected:
	uint8_t value;
};

class EventUint16Interface : public EventInterface {
public:
	EventUint16Interface(uint16_t type) : EventInterface(type) {}
	EventUint16Interface(EventDeviceId device, uint16_t type, uint16_t value) : EventInterface(device, type), value(value) {}
	uint16_t getValue() { return value; }
	virtual bool open(EventEnvelope *envelope);
	virtual bool pack(EventEnvelope *envelope);

protected:
	uint16_t value;
};

class EventUint32Interface : public EventInterface {
public:
	EventUint32Interface(uint16_t type) : EventInterface(type) {}
	EventUint32Interface(EventDeviceId device, uint16_t type, uint32_t value) : EventInterface(device, type), value(value) {}
	uint32_t getValue() { return value; }
	virtual bool open(EventEnvelope *envelope);
	virtual bool pack(EventEnvelope *envelope);

protected:
	uint32_t value;
};

#endif
