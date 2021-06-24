#include "include/Event2.h"
#include "include/EventEngine.h"

#include "logger/include/Logger.h"

EventDeviceId::EventDeviceId(EventEngineInterface *eventEngine) {
	value = eventEngine->getDeviceId();
}

EventDeviceId::EventDeviceId(uint16_t value) : value(value) {

}

void EventDeviceId::setValue(uint16_t value) {
	this->value = value;
}

uint16_t EventDeviceId::getValue() {
	return value;
}

bool EventInterface::open(EventEnvelope *envelope) {
	if(envelope->getType() != type) { return false; }
	device.setValue(envelope->getDevice());
	return true;
}

bool EventInterface::pack(EventEnvelope *envelope) {
	envelope->clear();
	envelope->setType(type);
	envelope->setDevice(device.getValue());
	return true;
}

enum EventUint8InterfaceParam {
	EventUint8Interface_value = 0,
};

bool EventUint8Interface::open(EventEnvelope *envelope) {
	if(EventInterface::open(envelope) == false) { return false; }
	if(envelope->getUint8(EventUint8Interface_value, &value) == false) { return false; }
	return true;
}

bool EventUint8Interface::pack(EventEnvelope *envelope) {
	if(EventInterface::pack(envelope) == false) { return false; }
	if(envelope->addUint8(EventUint8Interface_value, value) == false) { return false; }
	return true;
}

enum EventUint16InterfaceParam {
	EventUint16Interface_value = 0,
};

bool EventUint16Interface::open(EventEnvelope *envelope) {
	if(EventInterface::open(envelope) == false) { return false; }
	if(envelope->getUint16(EventUint16Interface_value, &value) == false) { return false; }
	return true;
}

bool EventUint16Interface::pack(EventEnvelope *envelope) {
	if(EventInterface::pack(envelope) == false) { return false; }
	if(envelope->addUint16(EventUint16Interface_value, value) == false) { return false; }
	return true;
}

enum EventUint32InterfaceParam {
	EventUint32Interface_value = 0,
};

bool EventUint32Interface::open(EventEnvelope *envelope) {
	if(EventInterface::open(envelope) == false) { return false; }
	if(envelope->getUint32(EventUint32Interface_value, &value) == false) { return false; }
	return true;
}

bool EventUint32Interface::pack(EventEnvelope *envelope) {
	if(EventInterface::pack(envelope) == false) { return false; }
	if(envelope->addUint32(EventUint32Interface_value, value) == false) { return false; }
	return true;
}
