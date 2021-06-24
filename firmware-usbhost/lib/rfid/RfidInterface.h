#ifndef LIB_RFID_INTERFACE_H
#define LIB_RFID_INTERFACE_H

#include "event/include/EventEngine.h"
#include "utils/include/Event.h"

namespace Rfid {

#define RFID_UID_SIZE 7

class Uid {
public:
	Uid();
	void clear();
	void set(uint8_t *val, uint16_t len);
	void setLen(uint16_t len);
	bool equal(uint8_t *val, uint16_t len);
	uint8_t *getVal() { return val; }
	uint16_t getLen() { return len; }
	uint16_t getSize() { return RFID_UID_SIZE; }

private:
	uint8_t val[RFID_UID_SIZE];
	uint16_t len;
};

enum EventType {
	Event_Card	= GlobalId_Rfid | 0x01, // EventCard
};

class EventCard : public EventInterface {
public:
	EventCard();
	EventCard(EventDeviceId deviceId, uint8_t *uid, uint16_t uidLen);
	Uid *getUid() { return &uid; }
	virtual bool open(EventEnvelope *event);
	virtual bool pack(EventEnvelope *event);
private:
	Uid uid;
};

}

#endif
