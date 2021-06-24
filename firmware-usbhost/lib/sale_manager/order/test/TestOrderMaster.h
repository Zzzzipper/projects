#ifndef LIB_SALEMANAGER_ORDER_TEST_ORDERMASTER_H_
#define LIB_SALEMANAGER_ORDER_TEST_ORDERMASTER_H_

#include "lib/sale_manager/order/SaleManagerOrderCore.h"

class TestOrderMaster : public OrderMasterInterface {
public:
	TestOrderMaster(uint16_t deviceId, StringBuilder *str) : deviceId(deviceId), str(str) {}
	EventDeviceId getDeviceId() { return deviceId; }
	void setOrder(Order *order) override { this->order = order; *str << "<OM(" <<  deviceId.getValue() << ")::setOrder>"; }
	void reset() override { *str << "<OM(" <<  deviceId.getValue() << ")::reset>"; }
	void connect() override { *str << "<OM(" <<  deviceId.getValue() << ")::connect>"; }
	void check() override { *str << "<OM(" <<  deviceId.getValue() << ")::check>"; }
	void checkPinCode(const char *pincode) override { *str << "<OM(" <<  deviceId.getValue() << ")::checkPinCode(" << pincode << ">"; }
	void distribute(uint16_t cid) override { *str << "<OM(" <<  deviceId.getValue() << ")::distribute>"; }
	void complete() override { *str << "<OM(" <<  deviceId.getValue() << ")::complete>"; }

private:
	EventDeviceId deviceId;
	StringBuilder *str;
	Order *order;
};

#endif
