#ifndef LIB_SALEMANAGER_MDBMASTER_CORE2_H_
#define LIB_SALEMANAGER_MDBMASTER_CORE2_H_

#include "lib/modem/EventRegistrar.h"

#include "common/config/include/ConfigModem.h"
#include "common/timer/include/Timer.h"
#include "common/timer/include/TimerEngine.h"
#include "common/fiscal_register/include/FiscalRegister.h"
#include "common/utils/include/LedInterface.h"
#include "common/mdb/slave/cashless/MdbSlaveCashless3.h"
#include "common/mdb/slave/coin_changer/MdbSlaveCoinChanger.h"
#include "common/mdb/slave/bill_validator/MdbSlaveBillValidator.h"
#include "common/mdb/master/coin_changer/MdbMasterCoinChanger.h"
#include "common/mdb/master/bill_validator/MdbMasterBillValidator.h"
#include "common/mdb/master/cashless/MdbMasterCashlessStack.h"

#define MDBMASTER_APPROVING_TIMEUT 1000

class SaleManagerMdbMasterParams {
public:
	ConfigModem *config;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	Fiscal::Register *fiscalRegister;
	LedInterface *leds;
	EventRegistrar *chronicler;
	MdbSlaveCashlessInterface *slaveCashless1;
	MdbSlaveCoinChangerInterface *slaveCoinChanger;
	MdbSlaveBillValidatorInterface *slaveBillValidator;
	MdbMasterCoinChangerInterface *masterCoinChanger;
	MdbMasterBillValidatorInterface *masterBillValidator;
	MdbMasterCashlessStack *masterCashlessStack;
};

class SaleManagerMdbMasterCore : public EventSubscriber {
public:
	SaleManagerMdbMasterCore(SaleManagerMdbMasterParams *config);
	virtual ~SaleManagerMdbMasterCore();
	void reset();
	void service();
	void shutdown();
	void testCredit();

	void procTimer();
	virtual void proc(EventEnvelope *envelope);

private:
	enum State {
		State_Idle = 0,
		State_Init,
		State_Sale,
		State_CashCredit,
		State_CashVending,
		State_CashChange,
		State_CashCheckPrinting,
		State_CashRefund,
		State_TokenCredit,
		State_TokenVending,
		State_TokenCheckPrinting,
		State_CashlessStackApproving,
		State_CashlessStackHoldingApproving,
		State_CashlessCredit,
		State_CashlessRevaluing,
		State_CashlessApproving,
		State_CashlessHoldingApproving,
		State_CashlessHoldingCredit,
		State_CashlessVending,
		State_CashlessCheckPrinting,
		State_CashlessClosing,
		State_Service,
		State_Shutdown,
	};

	ConfigModem *config;
	ConfigProductIterator *product;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	Fiscal::Sale saleData;
	Fiscal::Register *fiscalRegister;
	LedInterface *leds;
	EventRegistrar *chronicler;
	MdbSlaveCashlessInterface *slaveCashless1;
	MdbSlaveCoinChangerInterface *slaveCoinChanger;
	MdbSlaveBillValidatorInterface *slaveBillValidator;
	MdbMasterCoinChangerInterface *masterCoinChanger;
	MdbMasterBillValidatorInterface *masterBillValidator;
	MdbMasterCashlessStack *masterCashlessStack;
	MdbMasterCashlessInterface *masterCashless;
	EventDeviceId deviceId;
	Mdb::DeviceContext *context;
	Timer *timer;
	uint32_t price;
	uint32_t approvedPrice;
	uint32_t revalue;
	uint32_t cashCredit;
	uint32_t cashlessCredit;
	uint16_t saleNum;

	void stateInitEvent(EventEnvelope *envelope);
	void stateInitEventSlaveCashlessReset();
	void stateInitTimeout();

	void gotoStateSale();
	void stateSaleEvent(EventEnvelope *envelope);
	void stateSaleEventCashlessEnable();
	void stateSaleEventCashlessDisable();
	bool parseCashCredit(EventEnvelope *envelope, uint32_t *incomingCredit);
	void stateSaleEventCashCredit(EventEnvelope *envelope);
	void stateSaleEventTokenCredit();
	void stateSaleEventCashlessReady();
	void stateSaleEventCashlessCredit(EventEnvelope *envelope);
	void stateSaleEventCashlessVendRequest(EventEnvelope *envelope);

	void gotoStateCashCredit();
	void stateCashCreditEvent(EventEnvelope *envelope);
	void stateCashCreditEventCredit(EventEnvelope *envelope);
	void stateCashCreditEventEscrowRequest();
	void stateCashCreditEventVendRequest(EventEnvelope *envelope);
	void stateCashCreditEventVendCancel();

	void stateCashVendingEvent(EventEnvelope *envelope);
	void unwaitedEventCoinDeposite(EventEnvelope *envelope);
	void unwaitedEventBillDeposite(EventEnvelope *envelope);
	void unwaitedEventSessionBegin(EventEnvelope *envelope);
	void stateCashVendingEventComplete();
	void stateCashVendingEventCancel();

	void gotoStateCashChange();
	void stateCashChangeEvent(EventEnvelope *envelope);

	void gotoStateCashCheckPrinting();
	void stateCashCheckPrintingEvent(EventEnvelope *envelope);
	void stateCashCheckPrintingEventRegisterComplete();

	void gotoStateCashRefund();
	void stateCashRefundEvent(EventEnvelope *envelope);

	void gotoStateTokenCredit();
	void stateTokenCredit(EventEnvelope *envelope);
	void stateTokenCreditEventVendRequest(EventEnvelope *envelope);
	void stateTokenCreditEventVendCancel();

	void stateTokenVendingEvent(EventEnvelope *envelope);
	void stateTokenVendingEventComplete();
	void stateTokenVendingEventCancel();

	void gotoStateTokenCheckPrinting();
	void stateTokenCheckPrintingEvent(EventEnvelope *envelope);

	void gotoStateCashlessStackApproving();
	void stateCashlessStackApprovingEvent(EventEnvelope *envelope);
	void stateCashlessStackApprovingEventVendApproved(EventEnvelope *envelope);
	void stateCashlessStackApprovingEventVendDenied();
	void stateCashlessStackApprovingEventVendCancel();
	void stateCashlessStackApprovingFailed();
	void stateCashlessStackApprovingTimeout();

	void stateCashlessStackHoldingApprovingEvent(EventEnvelope *envelope);
	void stateCashlessStackHoldingApprovingEventVendApproved(EventEnvelope *envelope);
	void stateCashlessStackHoldingApprovingEventVendDenied();
	void stateCashlessStackHoldingApprovingFailed();
	void stateCashlessStackHoldingApprovingEventVendCancel();

	void stateCashlessCreditEvent(EventEnvelope *envelope);
	void stateCashlessCreditEventDeposite(EventEnvelope *envelope);
	void stateCashlessCreditEventVendRequest(EventEnvelope *envelope);
	void stateCashlessCreditEventSessionEnd(EventEnvelope *envelope);
	void stateCashlessCreditEventVendCancel();

	void stateCashlessRevaluingEvent(EventEnvelope *envelope);
	void stateCashlessRevaluingEventRevalueApproved();
	void stateCashlessRevaluingEventRevalueDenied();

	void gotoStateCashlessApproving();
	void stateCashlessApprovingEvent(EventEnvelope *envelope);
	void stateCashlessApprovingEventVendApproved(EventEnvelope *envelope);
	void stateCashlessApprovingEventVendDenied();
	void stateCashlessApprovingFailed();
	void stateCashlessApprovingEventSessionEnd(EventEnvelope *envelope);
	void stateCashlessApprovingEventVendCancel();
	void stateCashlessApprovingTimeout();

	void stateCashlessHoldingApprovingEvent(EventEnvelope *envelope);
	void stateCashlessHoldingApprovingEventVendApproved(EventEnvelope *envelope);
	void stateCashlessHoldingApprovingEventVendDenied();
	void stateCashlessHoldingApprovingFailed();
	void stateCashlessHoldingApprovingEventSessionEnd(EventEnvelope *envelope);
	void stateCashlessHoldingApprovingEventVendCancel();

	void stateCashlessHoldingCreditEvent(EventEnvelope *envelope);
	void stateCashlessHoldingCreditEventDeposite(EventEnvelope *envelope);
	void stateCashlessHoldingCreditEventSessionEnd(EventEnvelope *envelope);
	void stateCashlessHoldingCreditEventVendRequest(EventEnvelope *envelope);
	void stateCashlessHoldingCreditEventVendCancel();

	void stateCashlessVendingEvent(EventEnvelope *envelope);
	void stateCashlessVendingEventVendComplete();
	void stateCashlessVendingEventVendCancel();

	void gotoStateCashlessCheckPrinting();
	void stateCashlessCheckPrintingEvent(EventEnvelope *envelope);

	void gotoStateCashlessClosing();
	void stateCashlessClosingEvent(EventEnvelope *envelope);
	void stateCashlessClosingEventSessionEnd(EventEnvelope *envelope);

	void stateServiceEvent(EventEnvelope *envelope);
	void stateServiceEventDeposite(EventEnvelope *envelope);
	void stateServiceEventDispenseCoin(EventEnvelope *envelope);
	void stateServiceTimeout();

	void stateShutdownTimeout();

	bool findProduct(uint16_t eventId, uint32_t eventPrice, const char *deviceId, uint8_t priceListNumber);
	void approveProduct();
	void setMaxBill();
};

#endif
