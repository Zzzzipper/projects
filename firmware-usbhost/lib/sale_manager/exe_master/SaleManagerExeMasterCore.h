#if 1
#ifndef LIB_SALEMANAGER_EXEMASTER_CORE_H_
#define LIB_SALEMANAGER_EXEMASTER_CORE_H_

#include "lib/sale_manager/exe_master/ExeMaster.h"
#include "lib/modem/EventRegistrar.h"
#include "lib/client/ClientContext.h"

#include "common/config/include/ConfigModem.h"
#include "common/timer/include/Timer.h"
#include "common/fiscal_register/include/FiscalRegister.h"
#include "common/utils/include/LedInterface.h"
#include "common/mdb/master/coin_changer/MdbMasterCoinChanger.h"
#include "common/mdb/master/bill_validator/MdbMasterBillValidator.h"
#include "common/mdb/master/cashless/MdbMasterCashless.h"
#include "common/mdb/master/cashless/MdbMasterCashlessStack.h"
#define EXEMASTER_APPROVING_TIMEOUT 1000
#define EXEMASTER_HOLDING_TIMEOUT 20000

class SaleManagerExeMasterParams {
public:
	ConfigModem *config;
	ClientContext *client;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	Fiscal::Register *fiscalRegister;
	LedInterface *leds;
	EventRegistrar *chronicler;
	ExeMasterInterface *executive;
	MdbMasterCoinChangerInterface *masterCoinChanger;
	MdbMasterBillValidatorInterface *masterBillValidator;
	MdbMasterCashlessStack *masterCashlessStack;
};

class SaleManagerExeMasterCore : public EventSubscriber {
public:
	SaleManagerExeMasterCore(SaleManagerExeMasterParams *config);
	virtual ~SaleManagerExeMasterCore();
	EventDeviceId getDeviceId();
	void reset();
	void shutdown();
	void procTimer();
	virtual void proc(EventEnvelope *envelope);

private:
	enum State {
		State_Idle = 0,
		State_Disabled,
		State_Enabled,
		State_CashCredit,
		State_CashPrice,
		State_CashVending,
		State_CashChange,
		State_CashCheckPrinting,
		State_CashRefund,
		State_TokenCredit,
		State_TokenPrice,
		State_TokenVending,
		State_TokenCheckPrinting,
		State_CashlessStackPrice,
		State_CashlessStackApproving,
		State_CashlessStackHolding,
		State_CashlessCredit,
		State_CashlessRevaluing,
		State_CashlessPrice,
		State_CashlessApproving,
		State_CashlessHoldingApproving,
		State_CashlessHolding,
		State_CashlessVending,
		State_CashlessCheckPrinting,
		State_CashlessClosing,
		State_Shutdown,
	};

	ConfigModem *config;
	ClientContext *client;
	ConfigProductIterator *product;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	Fiscal::Sale saleData;
	Fiscal::Register *fiscalRegister;
	LedInterface *leds;
	EventRegistrar *chronicler;
	ExeMasterInterface *executive;
	MdbMasterCoinChangerInterface *masterCoinChanger;
	MdbMasterBillValidatorInterface *masterBillValidator;
	MdbMasterCashlessStack *masterCashlessStack;
	MdbMasterCashlessInterface *masterCashless;
	EventDeviceId deviceId;
	Mdb::DeviceContext *context;
	Timer *timer;
	uint16_t productId;
	uint32_t price;
	uint32_t approvedPrice;
	uint32_t revalue;
	uint32_t cashCredit;
	uint32_t cashlessCredit;
	uint16_t vendFailed;
	uint16_t saleNum;

	void updateChangeStatus();

	void gotoStateDisabled();
	void stateDisabledEvent(EventEnvelope *evnelope);
	void stateDisabledEventExeMasterReady();

	void gotoStateEnabled();
	void stateEnabledEvent(EventEnvelope *evnelope);
	void stateEnabledEventNotReady();
	void stateEnabledEventVendRequest(EventEnvelope *evnelope);
	void stateEnabledEventCashCredit(EventEnvelope *evnelope);
	void stateEnabledEventTokenCredit();

	void gotoStateCashCredit();
	void stateCashCreditEvent(EventEnvelope *evnelope);
	bool parseCashCredit(EventEnvelope *envelope, uint32_t *incomingCredit);
	void stateCashCreditEventCredit(EventEnvelope *evnelope);
	void stateCashCreditEventEscrowRequest();
	void stateCashCreditEventVendRequest(EventEnvelope *evnelope);

	void gotoStateCashPrice();
	void stateCashPriceEvent(EventEnvelope *evnelope);
	void stateCashPriceEventVendPrice();

	void stateCashVendingEvent(EventEnvelope *evnelope);
	void unwaitedEventCashCredit(EventEnvelope *evnelope);
	void unwaitedEventSessionBegin(EventEnvelope *envelope);
	void stateCashVendingEventComplete();
	void stateCashVendingEventVendFailed();

	void gotoStateCashChange();
	void stateCashChangeEvent(EventEnvelope *envelope);

	void gotoStateCashCheckPrinting();
	void stateCashCheckPrintingEvent(EventEnvelope *evnelope);

	void gotoStateCashRefund();
	void stateCashRefundEvent(EventEnvelope *evnelope);

	void stateEnabledEventCashlessCredit(EventEnvelope *evnelope);
	void stateCashCreditEventCashlessCredit(EventEnvelope *evnelope);

	void gotoStateTokenCredit();
	void stateTokenCredit(EventEnvelope *envelope);
	void stateTokenCreditEventCashCredit(EventEnvelope *envelope);
	void stateTokenCreditEventVendRequest(EventEnvelope *envelope);

	void stateTokenPriceEvent(EventEnvelope *evnelope);
	void stateTokenPriceEventVendPrice();

	void stateTokenVendingEvent(EventEnvelope *envelope);
	void stateTokenVendingEventComplete();
	void stateTokenVendingEventFailed();

	void gotoStateTokenCheckPrinting();
	void stateTokenCheckPrintingEvent(EventEnvelope *envelope);

	void gotoStateCashlessStackPrice();
	void stateCashlessStackPriceEvent(EventEnvelope *envelope);
	void stateCashlessStackPriceEventVendPrice();
	void stateCashlessStackPriceEventSessionEnd();

	void gotoStateCashlessStackApproving();
	void stateCashlessStackApprovingEvent(EventEnvelope *envelope);
	void stateCashlessStackApprovingEventCashCredit(EventEnvelope *envelope);
	void stateCashlessStackApprovingEventTokenCredit();
	void stateCashlessStackApprovingEventVendApproved(EventEnvelope *envelope);
	void stateCashlessStackApprovingEventVendDenied();
	void stateCashlessStackApprovingFailed();
	void stateCashlessStackApprovingTimeout();

	void stateCashlessStackHoldingEvent(EventEnvelope *envelope);
	void stateCashlessStackHoldingEventCashCredit(EventEnvelope *envelope);
	void stateCashlessStackHoldingEventTokenCredit();
	void stateCashlessStackHoldingEventVendApproved(EventEnvelope *envelope);
	void stateCashlessStackHoldingEventVendDenied();
	void stateCashlessStackHoldingFailed();

	void stateCashlessCreditEvent(EventEnvelope *evnelope);
	void stateCashlessCreditEventVendRequest(EventEnvelope *evnelope);
	void stateCashlessCreditEventSessionEnd();
	void stateCashlessCreditEventDeposite(EventEnvelope *evnelope);

	void stateCashlessRevaluingEvent(EventEnvelope *evnelope);
	void stateCashlessRevaluingEventRevalueApproved();
	void stateCashlessRevaluingEventRevalueDenied();

	void stateCashlessPriceEvent(EventEnvelope *envelope);
	void stateCashlessPriceEventVendPrice();
	void stateCashlessPriceEventSessionEnd();

	void gotoStateCashlessApproving();
	void stateCashlessApprovingEvent(EventEnvelope *evnelope);
	void stateCashlessApprovingEventVendApproved(EventEnvelope *envelope);
	void stateCashlessApprovingEventVendDenied();
	void stateCashlessApprovingFailed();
	void stateCashlessAprrovingTimeout();

	void gotoStateCashlessHoldingApproving();
	void stateCashlessHoldingApprovingEvent(EventEnvelope *envelope);
	void stateCashlessHoldingApprovingEventVendApproved(EventEnvelope *envelope);
	void stateCashlessHoldingApprovingEventVendDenied();
	void stateCashlessHoldingApprovingFailed();

	void gotoStateCashlessHolding();
	void stateCashlessHoldingEvent(EventEnvelope *envelope);
	void stateCashlessHoldingEventVendRequest(EventEnvelope *evnelope);
	void stateCashlessHoldingTimeout();

	void gotoStateCashlessVending();
	void stateCashlessVendingEvent(EventEnvelope *evnelope);
	void stateCashlessVendingEventVendComplete();
	void stateCashlessVendingEventVendFailed();

	void gotoStateCashlessCheckPrinting();
	void stateCashlessCheckPrintingEvent(EventEnvelope *evnelope);

	void gotoStateCashlessClosing();
	void stateCashlessClosingEvent(EventEnvelope *evnelope);
	void stateCashlessClosingEventSessionEnd();

	void stateShutdownTimeout();
};

#endif
#else
#ifndef LIB_SALEMANAGER_EXEMASTER_CORE_H_
#define LIB_SALEMANAGER_EXEMASTER_CORE_H_

#include "lib/sale_manager/exe_master/ExeMaster.h"

#include "common/config/ConfigModem.h"
#include "common/timer/include/Timer.h"
#include "common/fiscal_register/include/FiscalRegister.h"
#include "common/utils/include/LedInterface.h"
#include "common/mdb/master/coin_changer/MdbMasterCoinChanger.h"
#include "common/mdb/master/bill_validator/MdbMasterBillValidator.h"
#include "common/mdb/master/cashless/MdbMasterCashless.h"

class SaleManagerExeMasterParams {
public:
	ConfigModem *config;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	Fiscal::Register *fiscalRegister;
	LedInterface *leds;
	ExeMasterInterface *executive;
	MdbMasterCoinChangerInterface *masterCoinChanger;
	MdbMasterBillValidatorInterface *masterBillValidator;
	MdbMasterCashlessInterface *masterCashless1;
	MdbMasterCashlessInterface *masterCashless2;
	MdbMasterCashlessInterface *externCashless1;
};

class SaleManagerExeMasterCore : public EventSubscriber {
public:
	SaleManagerExeMasterCore(SaleManagerExeMasterParams *config);
	virtual ~SaleManagerExeMasterCore();
	EventDeviceId getDeviceId();
	void reset();
	void shutdown();
	void procTimer();
	virtual void proc(EventEnvelope *envelope);

private:
	enum State {
		State_Idle = 0,
		State_Disabled,
		State_Enabled,
		State_CashCredit,
		State_CashVending,
		State_CashChange,
		State_CashCheckPrinting,
		State_CashRefund,
		State_CashlessCredit,
		State_CashlessRevaluing,
		State_CashlessApproving,
		State_CashlessVending,
		State_CashlessCheckPrinting,
		State_CashlessClosing,
		State_Shutdown,
	};

	ConfigModem *config;
	ConfigProductIterator *product;
	ConfigPrice *price;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	Fiscal::Sale saleData;
	Fiscal::Register *fiscalRegister;
	LedInterface *leds;
	ExeMasterInterface *executive;
	MdbMasterCoinChangerInterface *masterCoinChanger;
	MdbMasterBillValidatorInterface *masterBillValidator;
	MdbMasterCashlessInterface *masterCashless1;
	MdbMasterCashlessInterface *masterCashless2;
	MdbMasterCashlessInterface *externCashless1;
	MdbMasterCashlessInterface *masterCashlessStack;
	EventDeviceId deviceId;
	Mdb::DeviceContext *context;
	Timer *timer;
	uint32_t revalue;
	uint32_t cashCredit;
	uint32_t cashlessCredit;

	void updateChangeStatus();

	void gotoStateDisabled();
	void stateDisabledEvent(EventEnvelope *evnelope);
	void stateDisabledEventExeMasterReady();

	void gotoStateEnabled();
	void stateEnabledEvent(EventEnvelope *evnelope);
	void stateEnabledEventNotReady();
	void stateEnabledEventVendRequest(EventEnvelope *evnelope);
	void stateEnabledEventCashCredit(EventEnvelope *evnelope);

	void stateCashCreditEvent(EventEnvelope *evnelope);
	bool parseCashCredit(EventEnvelope *envelope, uint32_t *incomingCredit);
	void stateCashCreditEventCredit(EventEnvelope *evnelope);
	void stateCashCreditEventEscrowRequest();
	void stateCashCreditEventVendRequest(EventEnvelope *evnelope);

	void stateCashVendingEvent(EventEnvelope *evnelope);
	void stateCashVendingEventCredit(EventEnvelope *evnelope);
	void stateCashVendingEventComplete();
	void stateCashVendingEventVendFailed();

	void gotoStateCashChange();
	void stateCashChangeEvent(EventEnvelope *envelope);

	void gotoStateCashCheckPrinting();
	void stateCashCheckPrintingEvent(EventEnvelope *evnelope);

	void gotoStateCashRefund();
	void stateCashRefundEvent(EventEnvelope *evnelope);

	void stateEnabledEventCashlessCredit(EventEnvelope *evnelope);

	void stateCashlessCreditEvent(EventEnvelope *evnelope);
	void stateCashlessCreditEventVendRequest(EventEnvelope *evnelope);
	void stateCashlessCreditEventSessionEnd();
	void stateCashlessCreditEventDeposite(EventEnvelope *evnelope);

	void stateCashlessRevaluingEvent(EventEnvelope *evnelope);
	void stateCashlessRevaluingEventRevalueApproved();
	void stateCashlessRevaluingEventRevalueDenied();

	void stateCashlessApprovingEvent(EventEnvelope *evnelope);
	void stateCashlessApprovingEventVendApproved();
	void stateCashlessApprovingEventVendDenied();

	void stateCashlessVendingEvent(EventEnvelope *evnelope);
	void stateCashlessVendingEventVendComplete();
	void stateCashlessVendingEventVendFailed();

	void gotoStateCashlessCheckPrinting();
	void stateCashlessCheckPrintingEvent(EventEnvelope *evnelope);

	void gotoStateCashlessClosing();
	void stateCashlessClosingEvent(EventEnvelope *evnelope);
	void stateCashlessClosingEventSessionEnd();

	void stateShutdownTimeout();

	void registerCashlessIdNotFoundEvent(uint16_t cashlessId);
	void registerPriceListNotFoundEvent(const char *deviceId, uint8_t priceListNumber);
	void registerCoinInEvent(uint32_t value);
	void registerBillInEvent(uint32_t value);
	void registerChangeEvent(uint32_t value);
	MdbMasterCashlessInterface *findCashless(uint16_t deviceId);
};

#endif
#endif
