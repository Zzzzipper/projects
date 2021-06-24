#ifndef COMMON_CCICSI_ORDER_H_
#define COMMON_CCICSI_ORDER_H_

#include "common/event/include/Event2.h"
#include "common/utils/include/Event.h"
#include "common/utils/include/StringBuilder.h"

#define PINCODE_SIZE 10

#if 0
class OrderCell {
public:
	uint16_t cid;
	uint16_t result;
};

class Order {
public:
	Order();
	void clear();
	void set(uint16_t cid);
	void add(uint16_t cid);
	void remove(uint16_t cid);
	void complete(uint16_t cid, uint16_t result);
	uint16_t getLen();
	OrderCell *getById(uint16_t cid);
	OrderCell *getByIndex(uint16_t index);
	bool hasId(uint16_t cid);

private:
	static const uint16_t size = 20;
	uint16_t len;
	OrderCell list[size];
};
#else
class OrderCell {
public:
	static const uint16_t CidUndefined = 0xFFFF;
	uint16_t cid;
	uint16_t quantity;
};

class Order {
public:
	Order();
	void clear();
	void set(uint16_t cid);
	void add(uint16_t cid, uint16_t quantity);
	void remove(uint16_t cid);
	uint16_t getLen();
	uint16_t getQuantity();
	uint16_t getFirstCid();
	OrderCell *getById(uint16_t cid);
	OrderCell *getByIndex(uint16_t index);
	bool hasId(uint16_t cid);

private:
	static const uint16_t size = 20;
	uint16_t len;
	uint16_t quantity;
	OrderCell list[size];

	void removeCellByIndex(uint16_t index);
};
#endif

class OrderDeviceInterface {
public:
	enum EventType {
		Event_PinCodeCompleted	 = GlobalId_OrderDevice | 0x01, // EventPinCodeCompleted
		Event_PinCodeCancelled	 = GlobalId_OrderDevice | 0x02,
		Event_VendRequest		 = GlobalId_OrderDevice | 0x03, // EventUint16Interface
		Event_VendCompleted		 = GlobalId_OrderDevice | 0x04, // EventUint16Interface
		Event_VendSkipped		 = GlobalId_OrderDevice | 0x05, // EventUint16Interface
		Event_VendCancelled		 = GlobalId_OrderDevice | 0x06,
	};

	class EventPinCodeCompleted : public EventInterface {
	public:
		EventPinCodeCompleted();
		EventPinCodeCompleted(EventDeviceId deviceId);
		EventPinCodeCompleted(EventDeviceId deviceId, const char *pincode);
		void setPinCode(const char *pincode);
		const char *getPinCode() const { return pincode.getString(); }
		uint32_t getPinCodeLen() { return pincode.getLen(); }
		virtual bool open(EventEnvelope *event);
		virtual bool pack(EventEnvelope *event);
	private:
		String pincode;
	};

	virtual ~OrderDeviceInterface() {}
	virtual void setOrder(Order *order) = 0;
	virtual void reset() = 0;
	virtual void disable() = 0;
	virtual void enable() = 0;
	virtual void approveVend() = 0;
	virtual void requestPinCode() = 0;
	virtual void denyVend() = 0;
};

#endif
