#include "Order.h"

#include "common/logger/include/Logger.h"

enum EventPinCodeCompletedParam {
	EventPinCodeCompleted_PinCode = 0,
};

OrderDeviceInterface::EventPinCodeCompleted::EventPinCodeCompleted() :
	EventInterface(Event_PinCodeCompleted),
	pincode(PINCODE_SIZE, PINCODE_SIZE)
{
}

OrderDeviceInterface::EventPinCodeCompleted::EventPinCodeCompleted(EventDeviceId deviceId) :
	EventInterface(deviceId, Event_PinCodeCompleted),
	pincode(PINCODE_SIZE, PINCODE_SIZE)
{
}

OrderDeviceInterface::EventPinCodeCompleted::EventPinCodeCompleted(EventDeviceId deviceId, const char *pincode) :
	EventInterface(deviceId, Event_PinCodeCompleted),
	pincode(PINCODE_SIZE, PINCODE_SIZE)
{
	this->pincode.set(pincode);
}

void OrderDeviceInterface::EventPinCodeCompleted::setPinCode(const char *pincode) {
	this->pincode.set(pincode);
}

bool OrderDeviceInterface::EventPinCodeCompleted::open(EventEnvelope *envelope) {
	if(EventInterface::open(envelope) == false) { return false; }
	if(envelope->getString(EventPinCodeCompleted_PinCode, &pincode) == false) { return false; }
	return true;
}

bool OrderDeviceInterface::EventPinCodeCompleted::pack(EventEnvelope *envelope) {
	if(EventInterface::pack(envelope) == false) { return false; }
	if(envelope->addString(EventPinCodeCompleted_PinCode, pincode.getString()) == false) { return false; }
	return true;
}

Order::Order() {
	clear();
}

void Order::clear() {
	len = 0;
	quantity = 0;
}

void Order::set(uint16_t cid) {
	clear();
	add(cid, 1);
}

void Order::add(uint16_t cid, uint16_t quantity) {
	if(len >= size) {
		return;
	}
	OrderCell *cell = list + len;
	cell->cid = cid;
	cell->quantity = quantity;
	this->len++;
	this->quantity += quantity;
}

void Order::remove(uint16_t cid) {
	for(uint16_t i = 0; i < len; i++) {
		OrderCell *cell = getByIndex(i);
		if(cell->cid == cid) {
			if(cell->quantity > 1) {
				cell->quantity--;
				quantity--;
			} else {
				removeCellByIndex(i);
			}
			break;
		}
	}
#if 0
	LOG_DEBUG(LOG_SM, "remove " << cid);
	for(uint16_t i = 0; i < len; i++) {
		OrderCell *cell = getByIndex(i);
		LOG_DEBUG(LOG_SM, "cell " << i << "=" << cell->cid << "/" << cell->quantity);
	}
#endif
}

uint16_t Order::getLen() {
	return len;
}

uint16_t Order::getQuantity() {
	return quantity;
}

uint16_t Order::getFirstCid() {
	OrderCell *cell = getByIndex(0);
	if(cell == NULL) {
		return OrderCell::CidUndefined;
	}
	return cell->cid;
}

OrderCell *Order::getById(uint16_t cid) {
	for(uint16_t i = 0; i < len; i++) {
		OrderCell *cell = getByIndex(i);
		if(cell->cid == cid) {
			return cell;
		}
	}
	return NULL;
}

OrderCell *Order::getByIndex(uint16_t index) {
	if(index >= len) {
		return NULL;
	}
	return (list + index);
}

bool Order::hasId(uint16_t cid) {
	return (getById(cid) != NULL);
}

void Order::removeCellByIndex(uint16_t index) {
	OrderCell *cell = getByIndex(index);
	if(cell == NULL) {
		return;
	}
	quantity -= cell->quantity;
	len--;
	for(uint16_t i = index; i < len; i++) {
		OrderCell *prev = list + i;
		OrderCell *next = list + i + 1;
		prev->cid = next->cid;
		prev->quantity = next->quantity;
	}
}
