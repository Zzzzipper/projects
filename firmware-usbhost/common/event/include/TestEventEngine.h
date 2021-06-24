#ifndef COMMON_EVENT_TESTENGINE_H
#define COMMON_EVENT_TESTENGINE_H

#include "EventEngine.h"
#include "utils/include/StringBuilder.h"
#include "logger/include/Logger.h"

class TestEventEngine : public EventEngineInterface {
public:
	TestEventEngine(StringBuilder *result) : result(result), deviceCounter(0) {}
	uint16_t getDeviceId() override {
		deviceCounter++;
		return deviceCounter;
	}

	bool subscribe(EventSubscriber *subscriber, uint16_t source) override {
		(void)subscriber;
		*result << "<subscribe=" << source << ">";
		return true;
	}

	bool subscribe(EventSubscriber *subscriber, uint16_t source, EventDeviceId deviceId) override {
		(void)subscriber;
		(void)deviceId;
		*result << "<subscribe=" << source << ">";
		return true;
	}

	bool transmit(EventEnvelope *envelope) override {
		uint16_t type = envelope->getType();
		*result << "<event=" << envelope->getDevice() << ",0x";
		result->addHex(type >> 8);
		result->addHex(type & 0xFF);
		*result << ">";
		return true;
	}

	bool transmit(EventInterface *event) override {
		EventEnvelope envelope(50);
		if(event->pack(&envelope) == false) {
			*result << "<event-pack-error>";
			return false;
		}
		return transmit(&envelope);
	}

protected:
	StringBuilder *result;
	uint16_t deviceCounter;

	void procEvent(EventEnvelope *envelope, uint16_t type, const char *name) {
		EventInterface event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { *result << "<event-pack-error>"; return; }
		*result << "<event=" << event.getDevice() << "," << name << ">";
	}

	void procEventUint8(EventEnvelope *envelope, uint16_t type, const char *name) {
		EventUint8Interface event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { *result << "<event-pack-error>"; return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.getValue() << ">";
	}

	void procEventUint16(EventEnvelope *envelope, uint16_t type, const char *name) {
		EventUint16Interface event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { *result << "<event-pack-error>"; return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.getValue() << ">";
	}

	void procEventUint32(EventEnvelope *envelope, uint16_t type, const char *name) {
		EventUint32Interface event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { *result << "<event-pack-error>"; return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.getValue() << ">";
	}
};

#endif
