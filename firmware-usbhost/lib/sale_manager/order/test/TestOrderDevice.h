#ifndef LIB_SALEMANAGER_ORDER_TEST_ORDERDEVICE_H_
#define LIB_SALEMANAGER_ORDER_TEST_ORDERDEVICE_H_

#include "common/ccicsi/Order.h"
#include "common/event/include/Event2.h"

class TestOrderDevice : public OrderDeviceInterface {
public:
	TestOrderDevice(uint16_t deviceId, StringBuilder *str) : deviceId(deviceId), str(str) {}
	EventDeviceId getDeviceId() { return deviceId; }
	void setOrder(Order *order) override { *str << "<OM(" <<  deviceId.getValue() << ")::setOrder>"; }
	void reset() override { *str << "<OD(" <<  deviceId.getValue() << ")::reset>"; }
	void disable() override { *str << "<OD(" <<  deviceId.getValue() << ")::disable>"; }
	void enable() override { *str << "<OD(" <<  deviceId.getValue() << ")::enable>"; }
	void approveVend() override { *str << "<OD(" <<  deviceId.getValue() << ")::approveVend>"; }
	void requestPinCode() override { *str << "<OD(" <<  deviceId.getValue() << ")::requestPinCode>"; }
	void denyVend() override { *str << "<OD(" <<  deviceId.getValue() << ")::denyVend>"; }

private:
	EventDeviceId deviceId;
	StringBuilder *str;
};

#endif
