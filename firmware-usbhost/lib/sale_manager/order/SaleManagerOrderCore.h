#ifndef LIB_SALEMANAGER_ORDER_CORE_H_
#define LIB_SALEMANAGER_ORDER_CORE_H_

#include "lib/sale_manager/cci_t3/ErpOrderMaster.h"
#include "lib/modem/EventRegistrar.h"

#include "common/ccicsi/CciT3Driver.h"
#include "common/ccicsi/Order.h"
#include "common/config/include/ConfigModem.h"
#include "common/timer/include/TimerEngine.h"
#include "common/mdb/master/cashless/MdbMasterCashlessStack.h"
#include "common/fiscal_register/include/FiscalRegister.h"
#include "common/utils/include/Event.h"
#include "common/utils/include/LedInterface.h"
#include "common/executive/include/ExeSniffer.h"

class OrderMasterInterface {
public:
	enum EventType {
		Event_Connected		 = GlobalId_OrderMaster | 0x01,
		Event_Approved		 = GlobalId_OrderMaster | 0x02,
		Event_PinCode		 = GlobalId_OrderMaster | 0x03,
		Event_Denied		 = GlobalId_OrderMaster | 0x04,
		Event_Distributed	 = GlobalId_OrderMaster | 0x05,
	};

	virtual ~OrderMasterInterface() {}
	virtual void setOrder(Order *order) = 0;
	virtual void reset() = 0;
	virtual void connect() = 0;
	virtual void check() = 0;
	virtual void checkPinCode(const char *pincode) = 0;
	virtual void distribute(uint16_t cid) = 0;
	virtual void complete() = 0;
};

class SaleManagerOrderCore : public EventSubscriber {
public:
	SaleManagerOrderCore(ConfigModem *config, ClientContext *client, TimerEngine *timerEngine, EventEngineInterface *eventEngine, OrderMasterInterface *master, OrderDeviceInterface *device, Fiscal::Register *fiscalRegister, LedInterface *leds, EventRegistrar *chronicler);
	~SaleManagerOrderCore();
	virtual void reset();
	virtual void shutdown();

	void procTimer();
	void proc(EventEnvelope *envelope) override;

private:
	enum State {
		State_Idle = 0,
		State_Init,
		State_Sale,
		State_Approving,
		State_PinCode,
		State_Vending,
		State_Saving,
	};

	ConfigModem *config;
	ClientContext *client;
	ConfigProductIterator *product;
	TimerEngine *timerEngine;
	EventEngineInterface *eventEngine;
	OrderMasterInterface *master;
	OrderDeviceInterface *device;
	Fiscal::Sale saleData;
	Fiscal::Register *fiscalRegister;
	LedInterface *leds;
	EventRegistrar *chronicler;
	Mdb::DeviceContext *context;
	EventDeviceId deviceId;
	Order order;
	Timer *timer;
	uint32_t price;

	void gotoStateInit();
	void stateInitEvent(EventEnvelope *envelope);
	void stateInitEventOrderMasterConnected(EventEnvelope *envelope);

	void gotoStateSale();
	void stateSaleEvent(EventEnvelope *envelope);
	void stateSaleEventOrderDeviceRequest(EventEnvelope *envelope);

	void gotoStateApproving();
	void stateApprovingEvent(EventEnvelope *envelope);
	void stateApprovingEventOrderMasterApproved();
	void stateApprovingEventOrderMasterPinCode();
	void stateApprovingEventOrderMasterDenied();

	void gotoStatePinCode();
	void statePinCodeEvent(EventEnvelope *envelope);
	void statePinCodeEventPinCodeCompleted(EventEnvelope *envelope);
	void statePinCodeEventPinCodeCancelled();

	void gotoStateVending();
	void stateVendingEvent(EventEnvelope *envelope);
	void stateVendingEventVendCompleted(EventEnvelope *envelope);
	void stateVendingEventVendSkipped(EventEnvelope *envelope);
	void stateVendingEventVendCancelled();
	void stateVendingTimeout();

	void gotoStateSaving(uint16_t cid);
	void stateSavingEvent(EventEnvelope *envelope);
	void stateSavingEventOrderMasterSaved(EventEnvelope *envelope);
};

#endif
