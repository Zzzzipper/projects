#ifndef LIB_SALEMANAGER_MDBSLAVE_CORE_H_
#define LIB_SALEMANAGER_MDBSLAVE_CORE_H_

#include "MdbSnifferCoinChanger.h"
#include "MdbSnifferBillValidator.h"
#include "MdbSnifferCashless.h"

#include "lib/modem/EventRegistrar.h"

#include "common/config/include/ConfigModem.h"
#include "common/timer/include/TimerEngine.h"
#include "common/event/include/EventEngine.h"
#include "common/utils/include/LedInterface.h"
#include "common/mdb/slave/bill_validator/MdbSlaveBillValidator.h"
#include "common/mdb/slave/coin_changer/MdbSlaveCoinChanger.h"
#include "common/mdb/slave/cashless/MdbSlaveCashless3.h"
#include "common/mdb/slave/com_gateway/MdbSlaveComGateway.h"
#include "common/mdb/master/cashless/MdbMasterCashlessStack.h"
#include "common/fiscal_register/include/FiscalRegister.h"

#include "lib/utils/stm32/Reboot.h"

class SaleManagerMdbSlaveParams {
public:
	ConfigModem *config;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	LedInterface *leds;
	EventRegistrar *chronicler;
	MdbSniffer *slaveCoinChanger;
	MdbSniffer *slaveBillValidator;
	MdbSniffer *slaveCashless1;
	MdbSlaveCashlessInterface *slaveCashless2;
	MdbSlaveComGatewayInterface *slaveGateway;
	MdbMasterCashlessStack *externCashless;
	Fiscal::Register *fiscalRegister;
	Reboot::Reason rebootReason;
};

class SaleManagerMdbSlaveCore : public EventSubscriber {
public:
	SaleManagerMdbSlaveCore(SaleManagerMdbSlaveParams *params);
	virtual ~SaleManagerMdbSlaveCore();
	void reset();
	void service();
	void shutdown();

	void procTimer();
	virtual void proc(EventEnvelope *envelope);

private:
	enum State {
		State_Idle = 0,
		State_Delay,
		State_Sale,
		State_CashlessStackApproving,
		State_ExternCredit,
		State_ExternRevaluing,
		State_ExternApproving,
		State_ExternVending,
		State_ExternClosing,
	};

	ConfigModem *config;
	ConfigProductIterator *product;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	LedInterface *leds;
	EventRegistrar *chronicler;
	MdbSniffer *slaveCoinChanger;
	MdbSniffer *slaveBillValidator;
	MdbSniffer *slaveCashless1;
	MdbSlaveCashlessInterface *slaveCashless2;
	MdbSlaveComGatewayInterface *slaveGateway;
	MdbMasterCashlessStack *masterCashlessStack;
	MdbMasterCashlessInterface *masterCashless;
	Fiscal::Sale saleData;
	Fiscal::Register *fiscalRegister;
	Reboot::Reason rebootReason;
	EventDeviceId deviceId;
	Mdb::DeviceContext *context;
	Timer *timer;
	uint32_t price;
	uint32_t approvePrice;
	uint32_t cashlessCredit;
	bool enabled;

	void procEventCashSale(EventEnvelope *envelope);
	void procReportTransaction(EventEnvelope *envelope);
	void procReportTransactionPaidVend(MdbSlaveComGateway::ReportTransaction *event);
	void procReportTransactionTokenVend(MdbSlaveComGateway::ReportTransaction *event);
	void procReportEvent(EventEnvelope *envelope);
	void procDepositeBill(EventEnvelope *envelope);
	void procDepositeCoin(EventEnvelope *envelope);
	void procDeviceStateChange();

	void gotoStateDelay();
	void stateDelayTimeout();

	void gotoStateSale();
	void stateSaleEvent(EventEnvelope *envelope);
	void stateSaleEventCashlessEnable();
	void stateSaleEventCashlessDisable();
	void stateSaleEventVendRequest(EventEnvelope *envelope);
	void stateSaleEventCashlessSale(EventEnvelope *envelope);
	void stateSaleEventSessionBegin(EventEnvelope *envelope);

	void gotoStateCashlessStackApproving();
	void stateCashlessStackApprovingEvent(EventEnvelope *envelope);
	void stateCashlessStackApprovingEventVendApproved(EventEnvelope *envelope);
	void stateCashlessStackApprovingEventVendDenied();
	void stateCashlessStackApprovingFailed();

	void stateExternCreditEvent(EventEnvelope *envelope);
	void stateExternCreditEventSessionEnd(EventEnvelope *envelope);
	void stateExternCreditEventVendRequest(EventEnvelope *envelope);
	void stateExternCreditEventVendCancel();

	void stateExternApprovingEvent(EventEnvelope *envelope);
	void stateExternApprovingEventVendApproved(EventEnvelope *envelope);
	void stateExternApprovingEventVendDenied();
	void stateExternApprovingFailed();
	void stateExternApprovingEventVendCancel();
	void stateExternApprovingEventSessionEnd(EventEnvelope *envelope);

	void stateExternVendingEvent(EventEnvelope *envelope);
	void stateExternVendingEventVendComplete();
	void stateExternVendingEventVendCancel();

	void gotoStateExternClosing();
	void stateExternClosingEvent(EventEnvelope *envelope);
	void stateExternClosingEventSessionEnd(EventEnvelope *envelope);

	void unwaitedEventSessionBegin(EventEnvelope *envelope);
};

#endif
