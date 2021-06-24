#if 0
#ifndef LIB_SALEMANAGER_MDBDUMB_CORE_H_
#define LIB_SALEMANAGER_MDBDUMB_CORE_H_

#include "lib/modem/EventRegistrar.h"

#include "common/config/include/ConfigModem.h"
#include "common/timer/include/TimerEngine.h"
#include "common/event/include/EventEngine.h"
#include "common/utils/include/LedInterface.h"
#include "common/mdb/slave/bill_validator/MdbSlaveBillValidator.h"
#include "common/mdb/slave/coin_changer/MdbSlaveCoinChanger.h"
#include "common/mdb/master/bill_validator/MdbMasterBillValidator.h"
#include "common/mdb/master/coin_changer/MdbMasterCoinChanger.h"
#include "common/fiscal_register/include/FiscalRegister.h"

#include "lib/utils/stm32/Reboot.h"

class SaleManagerMdbNoCashlessParams {
public:
	ConfigModem *config;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	LedInterface *leds;
	EventRegistrar *chronicler;
	MdbSlaveCoinChanger *slaveCoinChanger;
	MdbSlaveBillValidator *slaveBillValidator;
	MdbMasterCoinChanger *masterCoinChanger;
	MdbMasterBillValidator *masterBillValidator;
	Fiscal::Register *fiscalRegister;
	Reboot::Reason rebootReason;
};

class SaleManagerMdbNoCashlessCore : public EventSubscriber {
public:
	SaleManagerMdbNoCashlessCore(SaleManagerMdbNoCashlessParams *params);
	virtual ~SaleManagerMdbNoCashlessCore();
	void reset();
	void service();
	void shutdown();

	void procTimer();
	virtual void proc(EventEnvelope *envelope);

private:
	enum State {
		State_Idle = 0,
		State_Wait,
		State_CashCredit,
		State_CashVending,
		State_CashChange,
	};

	ConfigModem *config;
	ConfigProductIterator *product;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	LedInterface *leds;
	EventRegistrar *chronicler;
	MdbSlaveCoinChanger *slaveCoinChanger;
	MdbSlaveBillValidator *slaveBillValidator;
	MdbMasterCoinChanger *masterCoinChanger;
	MdbMasterBillValidator *masterBillValidator;
	Fiscal::Sale saleData;
	Fiscal::Register *fiscalRegister;
	Reboot::Reason rebootReason;
	EventDeviceId deviceId;
	Mdb::DeviceContext *context;
	Timer *timer;
	uint32_t price;
	uint32_t cashCredit;
	bool enabled;

	void procDeviceStateChange();
	bool procDepositeCoin(EventEnvelope *envelope);
	bool procDepositeBill(EventEnvelope *envelope);

	void gotoStateDelay();
	void stateDelayTimeout();

	void gotoStateWait();
	void stateWaitEvent(EventEnvelope *envelope);
	void stateWaitEventDepositeCoin(EventEnvelope *envelope);
	void stateWaitEventDepositeBill(EventEnvelope *envelope);

	void gotoStateCashCredit();
	void stateCashCreditEvent(EventEnvelope *envelope);
	void stateCashCreditEventDispenseCoin(EventEnvelope *envelope);
	void stateCashCreditEventDisable();

	void stateCashVendingEvent(EventEnvelope *envelope);
	void stateCashVendingEventDispenseCoin(EventEnvelope *envelope);
	void stateCashVendingEventEnable();

	void stateCashChangeEvent(EventEnvelope *envelope);
	void stateCashChangeEventDispenseCoin(EventEnvelope *envelope);
	void stateCashChangeTimeout();
};

#endif
#else
#ifndef LIB_SALEMANAGER_MDBDUMB_CORE_H_
#define LIB_SALEMANAGER_MDBDUMB_CORE_H_

#include "lib/sale_manager/mdb_slave/MdbSnifferCoinChanger.h"
#include "lib/sale_manager/mdb_slave/MdbSnifferBillValidator.h"
#include "lib/sale_manager/mdb_slave/MdbSnifferCashless.h"
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

class SaleManagerMdbNoCashlessParams {
public:
	ConfigModem *config;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	LedInterface *leds;
	EventRegistrar *chronicler;
	MdbSniffer *slaveCoinChanger;
	MdbSniffer *slaveBillValidator;
	Fiscal::Register *fiscalRegister;
	Reboot::Reason rebootReason;
};

class SaleManagerMdbNoCashlessCore : public EventSubscriber {
public:
	SaleManagerMdbNoCashlessCore(SaleManagerMdbNoCashlessParams *params);
	virtual ~SaleManagerMdbNoCashlessCore();
	void reset();
	void service();
	void shutdown();

	void procTimer();
	virtual void proc(EventEnvelope *envelope);

private:
	enum State {
		State_Idle = 0,
		State_Delay,
		State_Wait,
		State_CashCredit,
		State_CashVending,
		State_CashChange,
	};

	ConfigModem *config;
	ConfigProductIterator *product;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	LedInterface *leds;
	EventRegistrar *chronicler;
	MdbSniffer *slaveCoinChanger;
	MdbSniffer *slaveBillValidator;
	Fiscal::Sale saleData;
	Fiscal::Register *fiscalRegister;
	Reboot::Reason rebootReason;
	EventDeviceId deviceId;
	Mdb::DeviceContext *context;
	Timer *timer;
	uint32_t price;
	uint32_t cashCredit;
	bool enabled;

	void procDeviceStateChange();
	bool procDepositeCoin(EventEnvelope *envelope);
	bool procDepositeBill(EventEnvelope *envelope);
	void procCashSale();

	void gotoStateDelay();
	void stateDelayTimeout();

	void gotoStateWait();
	void stateWaitEvent(EventEnvelope *envelope);
	void stateWaitEventDepositeCoin(EventEnvelope *envelope);
	void stateWaitEventDepositeBill(EventEnvelope *envelope);

	void gotoStateCashCredit();
	void stateCashCreditEvent(EventEnvelope *envelope);
	void stateCashCreditEventDispenseCoin(EventEnvelope *envelope);
	void stateCashCreditEventDisable();

	void gotoStateCashVending();
	void stateCashVendingEvent(EventEnvelope *envelope);
	void stateCashVendingEventDispenseCoin(EventEnvelope *envelope);
	void stateCashVendingEventEnable();

	void gotoStateCashChange();
	void stateCashChangeEvent(EventEnvelope *envelope);
	void stateCashChangeEventDepositeCoin(EventEnvelope *envelope);
	void stateCashChangeEventDispenseCoin(EventEnvelope *envelope);
	void stateCashChangeTimeout();
};

#endif
#endif
