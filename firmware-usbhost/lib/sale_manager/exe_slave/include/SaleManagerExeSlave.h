#ifndef LIB_SALEMANAGER_EXESLAVE_H_
#define LIB_SALEMANAGER_EXESLAVE_H_

#include "lib/sale_manager/include/SaleManager.h"

#include "common/config/include/ConfigModem.h"
#include "common/timer/include/Timer.h"
#include "common/fiscal_register/include/FiscalRegister.h"
#include "common/utils/include/Event.h"

class TimerEngine;
class Uart;
class ExeSniffer;
class SaleManagerExeSlaveCore;

class SaleManagerExeSlave : public SaleManager {
public:
	SaleManagerExeSlave(ConfigModem *config, TimerEngine *timers, EventEngine *eventEngine, Uart *slaveUart, Uart *masterUart, Fiscal::Register *fiscalRegister);
	~SaleManagerExeSlave();
	virtual void reset();
	virtual void shutdown();

private:
	ExeSniffer *exeSniffer;
	SaleManagerExeSlaveCore *core;

	void initSniffer(EventEngine *eventEngine, Uart *slaveUart, Uart *masterUart, StatStorage *stat);
};

#endif
