#ifndef LIB_SALEMANAGER_EXESLAVE_CORE_H_
#define LIB_SALEMANAGER_EXESLAVE_CORE_H_

#include "common/config/include/ConfigModem.h"
#include "common/timer/include/TimerEngine.h"
#include "common/uart/stm32/include/uart.h"
#include "common/fiscal_register/include/FiscalRegister.h"
#include "common/utils/include/Event.h"
#include "common/executive/include/ExeSniffer.h"

class SaleManagerExeSlaveCore : public EventSubscriber {
public:
	SaleManagerExeSlaveCore(ConfigModem *config, TimerEngine *timers, EventEngine *eventEngine, ExeSnifferInterface *exeSniffer, Fiscal::Register *fiscalRegister);
	~SaleManagerExeSlaveCore();
	virtual void reset();
	virtual void shutdown();

	void procTimer();
	virtual void proc(EventEnvelope *envelope);

private:
	enum State {
		State_Idle = 0,
		State_Wait,
		State_PrintCheck,
	};

	ConfigModem *config;
	ConfigProductIterator *product;
	ConfigPrice *price;
	TimerEngine *timers;
	EventEngine *eventEngine;
	ExeSnifferInterface *exeSniffer;
	Fiscal::Sale saleData;
	Fiscal::Register *fiscalRegister;
	EventDeviceId deviceId;
	Mdb::DeviceContext *context;
	Timer *timer;

	void gotoStateWait();
	void stateWaitEvent(EventEnvelope *envelope);
	void stateWaitEventSale(EventEnvelope *envelope);
	void gotoStatePrintCheck();
	void statePrintCheckEvent(EventEnvelope *envelope);
};

#endif
