#ifndef LIB_ERP_CASHLESS_H_
#define LIB_ERP_CASHLESS_H_

#include "common/mdb/master/cashless/MdbMasterCashless.h"
#include "common/event/include/EventEngine.h"

class ErpCashless : public MdbMasterCashlessInterface {
public:
	ErpCashless(Mdb::DeviceContext *context, TimerEngine *timerEngine, EventEngineInterface *eventEngine);
	bool deposite(uint32_t credit);

	virtual EventDeviceId getDeviceId() override;
	virtual void reset() override;
	virtual bool isRefundAble() override;
	virtual void disable() override;
	virtual void enable() override;
	virtual bool revalue(uint32_t credit) override;
	virtual bool sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) override;
	virtual bool saleComplete() override;
	virtual bool saleFailed() override;
	virtual bool closeSession() override;

	void procTimer();

public:
	enum State {
		State_Idle = 0,
		State_Disabled,
		State_Enabled,
		State_Session,
		State_Vending,
	};

	Mdb::DeviceContext *context;
	TimerEngine *timerEngine;
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	Timer *timer;
	bool enabled;
	uint32_t credit;

	void gotoStateEnabled();
	void gotoStateSession();
	void stateSessionTimeout();
};

#endif
