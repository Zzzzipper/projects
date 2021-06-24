#ifndef LIB_SALEMANAGER_ORDER_DUMMYORDERMASTER_H_
#define LIB_SALEMANAGER_ORDER_DUMMYORDERMASTER_H_

#include "SaleManagerOrderCore.h"

class DummyOrderMaster : public OrderMasterInterface {
public:
	DummyOrderMaster(EventEngineInterface *eventEngine);
	void setOrder(Order *order) override;
	void reset() override;
	void connect() override;
	void check() override;
	void checkPinCode(const char *pincode) override;
	void distribute(uint16_t cid) override;
	void complete() override;

private:
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	Order *order;
};

#endif
