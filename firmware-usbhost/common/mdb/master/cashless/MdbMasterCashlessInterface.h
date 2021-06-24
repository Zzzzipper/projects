#ifndef MDB_MASTER_CASHLESS_INTERFACE_H_
#define MDB_MASTER_CASHLESS_INTERFACE_H_

#include "event/include/Event2.h"
#include "utils/include/Event.h"

class VerificationInterface {
public:
	virtual bool verification() = 0;
};

class MdbMasterCashlessInterface {
public:
	enum EventType {
		Event_Ready				= GlobalId_MasterCashless | 0x01,
		Event_Error				= GlobalId_MasterCashless | 0x02, // uint16_t errorCode
		Event_SessionBegin		= GlobalId_MasterCashless | 0x03, // uint32_t credit
		Event_RevalueApproved	= GlobalId_MasterCashless | 0x04,
		Event_RevalueDenied		= GlobalId_MasterCashless | 0x05,
		Event_VendApproved		= GlobalId_MasterCashless | 0x06, // uint32_t approved price
		Event_VendDenied		= GlobalId_MasterCashless | 0x07,
		Event_SessionEnd		= GlobalId_MasterCashless | 0x08,
	};

	class EventApproved : public EventInterface {
	public:
		EventApproved();
		EventApproved(EventDeviceId deviceId);
		EventApproved(EventDeviceId deviceId, uint8_t type1, uint32_t nominal1);
		EventApproved(EventDeviceId deviceId, uint8_t type1, uint32_t nominal1, uint8_t type2, uint32_t nominal2);
		void set(uint8_t type1, uint32_t nominal1, uint8_t type2, uint32_t nominal2);
		uint8_t getType1() { return type1; }
		uint32_t getValue1() { return value1; }
		uint8_t getType2() { return type2; }
		uint32_t getValue2() { return value2; }
		virtual bool open(EventEnvelope *event);
		virtual bool pack(EventEnvelope *event);
	private:
		uint8_t type1;
		uint32_t value1;
		uint8_t type2;
		uint32_t value2;
	};

	virtual ~MdbMasterCashlessInterface() {}
	virtual EventDeviceId getDeviceId() = 0;
	virtual void reset() = 0;
	virtual bool isRefundAble() = 0;
	virtual void disable() = 0;
	virtual void enable() = 0;
	virtual bool revalue(uint32_t credit) = 0;
	virtual bool sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) = 0;
	virtual bool saleComplete() = 0;
	virtual bool saleFailed() = 0;
	virtual bool closeSession() = 0;
};

#endif
