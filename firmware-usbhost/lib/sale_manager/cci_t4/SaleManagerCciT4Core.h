#ifndef LIB_SALEMANAGER_CCIT4_CORE_H_
#define LIB_SALEMANAGER_CCIT4_CORE_H_

#include "lib/client/ClientContext.h"

#include "common/config/include/ConfigModem.h"
#include "common/timer/include/TimerEngine.h"
#include "common/ccicsi/include/CciCsi.h"
#include "common/mdb/master/cashless/MdbMasterCashlessStack.h"
#include "common/fiscal_register/include/FiscalRegister.h"
#include "common/utils/include/Event.h"
#include "common/executive/include/ExeSniffer.h"

class SaleManagerCciT4Core : public EventSubscriber {
public:
	SaleManagerCciT4Core(ConfigModem *config, ClientContext *client, TimerEngine *timers, EventEngineInterface *eventEngine, MdbSlaveCashlessInterface *slaveCashless, MdbMasterCashlessStack *masterCashlessStack, Fiscal::Register *fiscalRegister);
	~SaleManagerCciT4Core();
	virtual void reset();
	virtual void shutdown();

	void procTimer();
	void proc(EventEnvelope *envelope) override;

private:
	enum State {
		State_Idle = 0,
		State_Sale,
		State_FreeVending,
		State_CashlessApproving,
		State_CashlessVending,
		State_CashlessClosing,
	};

	ConfigModem *config;
	ClientContext *client;
	ConfigProductIterator *product;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	MdbSlaveCashlessInterface *slaveCashless;
	MdbMasterCashlessStack *masterCashlessStack;
	MdbMasterCashlessInterface *masterCashless;
	Fiscal::Sale saleData;
	Fiscal::Register *fiscalRegister;
	EventDeviceId deviceId;
	Mdb::DeviceContext *context;
	Timer *timer;
	uint32_t price;

	void gotoStateSale();
	void stateSaleEvent(EventEnvelope *envelope);
	void stateSaleEventVendRequest(EventEnvelope *envelope);

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

	void registerCashlessIdNotFoundEvent(uint16_t cashlessId);
	void registerPriceListNotFoundEvent(const char *deviceId, uint8_t priceListNumber);
};

#endif
