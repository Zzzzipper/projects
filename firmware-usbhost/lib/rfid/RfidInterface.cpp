#include "RfidInterface.h"
#include "logger/include/Logger.h"

namespace Rfid {

Uid::Uid() {
	for(uint16_t i = 0; i < RFID_UID_SIZE; i++) {
		val[i] = 0;
	}
	len = 0;
}

void Uid::clear() {

}

void Uid::set(uint8_t *val, uint16_t len) {
	uint16_t i = 0;
	for(; i < RFID_UID_SIZE && i < len; i++) {
		this->val[i] = val[i];
	}
	for(; i < RFID_UID_SIZE; i++) {
		this->val[i] = 0;
	}
	this->len = i;
}

void Uid::setLen(uint16_t len) {
	for(uint16_t i = len; i < RFID_UID_SIZE; i++) {
		this->val[i] = 0;
	}
	this->len = len;
}

bool Uid::equal(uint8_t *val, uint16_t valLen) {
	if(len != valLen) {
		return false;
	}
	for(uint16_t i = 0; i < len; i++) {
		if(this->val[i] != val[i]) {
			return false;
		}
	}
	return true;
}

enum EventCardParam {
	EventCardParam_Uid = 0,
};

EventCard::EventCard() :
	EventInterface(Event_Card)
{

}

EventCard::EventCard(
	EventDeviceId deviceId,
	uint8_t *uid,
	uint16_t uidLen
) :
	EventInterface(deviceId, Event_Card)
{
	this->uid.set(uid, uidLen);
}

bool EventCard::open(EventEnvelope *envelope) {
	if(EventInterface::open(envelope) == false) { return false; }
	uint16_t len = 0;
	if(envelope->getData(EventCardParam_Uid, uid.getVal(), uid.getSize(), &len) == false) { return false; }
	uid.setLen(len);
	return true;
}

bool EventCard::pack(EventEnvelope *envelope) {
	if(EventInterface::pack(envelope) == false) { return false; }
	if(envelope->addData(EventCardParam_Uid, uid.getVal(), uid.getLen()) == false) { return false; }
	return true;
}

}
