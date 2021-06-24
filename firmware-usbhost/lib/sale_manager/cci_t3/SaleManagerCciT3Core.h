#ifndef LIB_SALEMANAGER_CCIT3_CORE_H_
#define LIB_SALEMANAGER_CCIT3_CORE_H_

#include <ccicsi/CciT3Driver.h>
#include "lib/sale_manager/cci_t3/ErpOrderMaster.h"
#include "lib/modem/EventRegistrar.h"

#include "common/config/include/ConfigModem.h"
#include "common/timer/include/TimerEngine.h"
#include "common/mdb/master/cashless/MdbMasterCashlessStack.h"
#include "common/fiscal_register/include/FiscalRegister.h"
#include "common/utils/include/Event.h"
#include "common/utils/include/LedInterface.h"
#include "common/executive/include/ExeSniffer.h"

class SaleManagerCciT3Core : public EventSubscriber {
public:
	SaleManagerCciT3Core(ConfigModem *config, ClientContext *client, TimerEngine *timerEngine, EventEngineInterface *eventEngine, OrderDeviceInterface *slaveCashless, OrderInterface *scanner, MdbMasterCashlessStack *masterCashlessStack, Fiscal::Register *fiscalRegister, LedInterface *leds, EventRegistrar *chronicler);
	~SaleManagerCciT3Core();
	virtual void reset();
	virtual void shutdown();

	void procTimer();
	virtual void proc(EventEnvelope *envelope);

private:
	enum State {
		State_Idle = 0,
		State_Sale,
		State_OrderApproving,
		State_OrderVending,
		State_OrderClosing,
		State_FreeVending,
		State_CashlessApproving,
		State_CashlessVending,
		State_CashlessClosing,
	};

	ConfigModem *config;
	ClientContext *client;
	ConfigProductIterator *product;
	TimerEngine *timerEngine;
	EventEngineInterface *eventEngine;
	OrderDeviceInterface *slaveCashless;
	OrderInterface *scanner;
	MdbMasterCashlessStack *masterCashlessStack;
	MdbMasterCashlessInterface *masterCashless;
	Fiscal::Sale saleData;
	Fiscal::Register *fiscalRegister;
	LedInterface *leds;
	EventRegistrar *chronicler;
	EventDeviceId deviceId;
	Mdb::DeviceContext *context;
	Timer *timer;
	Order order;
	uint32_t price;

	void gotoStateSale();
	void stateSaleEvent(EventEnvelope *envelope);
	void stateSaleEventVendRequest(EventEnvelope *envelope);
	void stateSaleEventOrderApprove(EventEnvelope *envelope);

	void stateOrderApprovingEvent(EventEnvelope *envelope);
	void stateOrderApprovingEventOrderRequest(EventEnvelope *envelope);
	void stateOrderApprovingEventOrderCancel();

	void stateOrderVendingEvent(EventEnvelope *envelope);
	void stateOrderVendingEventVendComplete();
	void stateOrderVendingEventVendCancel();

	void gotoStateOrderClosing();
	void stateOrderClosingEvent(EventEnvelope *envelope);
	void stateOrderClosingEventOrderEnd(EventEnvelope *envelope);

	void gotoStateFreeVending();
	void stateFreeVendingEvent(EventEnvelope *envelope);
	void stateFreeVendingEventVendComplete();
	void stateFreeVendingEventVendCancel();

	void stateCashlessApprovingEvent(EventEnvelope *envelope);
	void stateCashlessApprovingEventVendApproved(EventEnvelope *envelope);
	void stateCashlessApprovingEventVendDenied();
	void stateCashlessApprovingEventVendCancel();

	void stateCashlessVendingEvent(EventEnvelope *envelope);
	void stateCashlessVendingEventVendComplete();
	void stateCashlessVendingEventVendCancel();

	void gotoStateCashlessClosing();
	void stateCashlessClosingEvent(EventEnvelope *envelope);
	void stateCashlessClosingEventSessionEnd(EventEnvelope *envelope);

	void unwaitedEventSessionBegin(EventEnvelope *envelope);
};

#endif
