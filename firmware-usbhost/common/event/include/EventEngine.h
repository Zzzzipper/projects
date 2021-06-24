#ifndef COMMON_EVENT_ENGINE_H
#define COMMON_EVENT_ENGINE_H

#include "Event2.h"
#include "utils/include/List.h"
#include "utils/include/Fifo.h"

#define EVENT_POOL_SIZE 50
#define EVENT_DATA_SIZE 50
#define EVENT_SUB_NUMBER 25

class EventSubscriber {
public:
	virtual ~EventSubscriber() {}
	virtual void proc(EventEnvelope *envelope) = 0;
};

class EventEngineInterface {
public:
	virtual ~EventEngineInterface() {}
	virtual uint16_t getDeviceId() = 0;
	virtual bool subscribe(EventSubscriber *sub, uint16_t source) = 0;
	virtual bool subscribe(EventSubscriber *sub, uint16_t source, EventDeviceId deviceId) = 0;
	virtual bool transmit(EventInterface *event) = 0;
	virtual bool transmit(EventEnvelope *event) = 0;
};

class EventEngine : public EventEngineInterface {
public:
	EventEngine(uint16_t poolSize, uint16_t eventSize, uint16_t subNumber);
	virtual ~EventEngine();
	virtual uint16_t getDeviceId();
	virtual bool subscribe(EventSubscriber *sub, uint16_t source);
	virtual bool subscribe(EventSubscriber *sub, uint16_t source, EventDeviceId deviceId);
	virtual bool transmit(EventInterface *event);
	virtual bool transmit(EventEnvelope *envelope);
	void execute();
	uint16_t getErrorCount();

private:
	class EventSubscription {
	public:
		EventSubscription(EventSubscriber *sub, uint16_t source, uint16_t device) : sub(sub), source(source), device(device) {}
		EventSubscriber *sub;
		uint16_t source;
		uint16_t device;
	};

	uint16_t deviceCounter;
	Fifo<EventEnvelope*> *pool;
	Fifo<EventEnvelope*> *fifo;
	List<EventSubscription> *subs;
	uint16_t errorCounter;

	EventSubscription *findSub(EventSubscriber *sub, uint16_t source, uint16_t device);
	void deliver(EventEnvelope *envelope);
};

#endif
