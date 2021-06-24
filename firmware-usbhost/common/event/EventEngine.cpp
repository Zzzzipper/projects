#include "include/EventEngine.h"

#include "logger/include/Logger.h"

EventEngine::EventEngine(uint16_t poolSize, uint16_t envelopeSize, uint16_t subNumber) :
	deviceCounter(0),
	errorCounter(0)
{
	this->pool = new Fifo<EventEnvelope*>(poolSize);
	for(uint16_t i = 0; i < poolSize; i++) {
		EventEnvelope *envelope = new EventEnvelope(envelopeSize);
		this->pool->push(envelope);
	}
	this->fifo = new Fifo<EventEnvelope*>(poolSize);
	this->subs = new List<EventSubscription>(subNumber);
}

EventEngine::~EventEngine() {
	delete subs;
	delete fifo;
	delete pool;
}

uint16_t EventEngine::getDeviceId() {
	deviceCounter++;
	return deviceCounter;
}

bool EventEngine::subscribe(EventSubscriber *sub, uint16_t source) {
	uint16_t device = 0;
	if(findSub(sub, source, device) != NULL) {
		LOG_ERROR(LOG_EVENT, "Subscription already exist " << source);
		errorCounter++;
		return false;
	}
	EventSubscription *subscription = new EventSubscription(sub, source, device);
	subs->add(subscription);
	LOG_DEBUG(LOG_EVENT, "subscribe " << subs->getSize() /*<< "," << (long)(subscription->sub)*/ << "," << subscription->source << "," << subscription->device);
	return true;
}

bool EventEngine::subscribe(EventSubscriber *sub, uint16_t source, EventDeviceId deviceId) {
	uint16_t device = deviceId.getValue();
	if(findSub(sub, source, device) != NULL) {
		LOG_ERROR(LOG_EVENT, "Subscription already exist " << source);
		errorCounter++;
		return false;
	}
	EventSubscription *subscription = new EventSubscription(sub, source, device);
	subs->add(subscription);
	LOG_DEBUG(LOG_EVENT, "subscribe " << subs->getSize() /*<< "," << (int)(subscription->sub)*/ << "," << subscription->source << "," << subscription->device);
	return true;
}

bool EventEngine::transmit(EventInterface *event) {
	EventEnvelope *envelope = pool->pop();
	if(envelope == NULL) {
		LOG_ERROR(LOG_EVENT, "No free event in pool");
		errorCounter++;
		return false;
	}
	if(event->pack(envelope) == false) {
		LOG_ERROR(LOG_EVENT, "Save event failed");
		pool->push(envelope);
		errorCounter++;
		return false;
	}
	if(fifo->push(envelope) == false) {
		LOG_ERROR(LOG_EVENT, "Fifo overflow");
		pool->push(envelope);
		errorCounter++;
		return false;
	}
	LOG_DEBUG(LOG_EVENT, "transmit " << event->getType());
	return true;
}

bool EventEngine::transmit(EventEnvelope *envelope2) {
	EventEnvelope *envelope = pool->pop();
	if(envelope == NULL) {
		LOG_ERROR(LOG_EVENT, "No free event in pool");
		errorCounter++;
		return false;
	}
	if(envelope->copy(envelope2) == false) {
		LOG_ERROR(LOG_EVENT, "Envelope copy failed");
		pool->push(envelope);
		errorCounter++;
		return false;
	}
	if(fifo->push(envelope) == false) {
		LOG_ERROR(LOG_EVENT, "Fifo overflow");
		pool->push(envelope);
		errorCounter++;
		return false;
	}
	LOG_DEBUG(LOG_EVENT, "transmit " << envelope->getType());
	return true;
}

void EventEngine::execute() {
	uint16_t num = fifo->getSize();
	for(uint16_t i = 0; i < num; i++) {
		EventEnvelope *envelope = fifo->pop();
		deliver(envelope);
		pool->push(envelope);
	}
}

uint16_t EventEngine::getErrorCount() {
	return errorCounter;
}

EventEngine::EventSubscription *EventEngine::findSub(EventSubscriber *sub, uint16_t source, uint16_t device) {
	for(uint16_t i = 0; i < subs->getSize(); i++) {
		EventSubscription *subscription = subs->get(i);
		if(subscription->sub == sub && subscription->source == source && subscription->device == device) {
			return subscription;
		}
	}
	return NULL;
}

void EventEngine::deliver(EventEnvelope *envelope) {
	uint16_t source = envelope->getType() & 0xFF00;
	uint16_t device = envelope->getDevice();
	LOG_TRACE(LOG_EVENT, "deliver " << source << "," << device);
	for(uint16_t i = 0; i < subs->getSize(); i++) {
		EventSubscription *subscription = subs->get(i);
		if(subscription == NULL) {
			LOG_ERROR(LOG_EVENT, "Unwaited list end");
			errorCounter++;
			return;
		}
		LOG_TRACE(LOG_EVENT, "check " /*<< (int)(subscription->sub) << ","*/ << subscription->source << "," << subscription->device);
		if(subscription->device == 0) {
			if(subscription->source == source) {
				LOG_DEBUG(LOG_EVENT, "deliver " << envelope->getType() << "," << envelope->getDevice());
				subscription->sub->proc(envelope);
			}
		} else {
			if(subscription->device == device) {
				LOG_DEBUG(LOG_EVENT, "deliver " << envelope->getType() << "," << envelope->getDevice());
				subscription->sub->proc(envelope);
			}
		}
	}
}
