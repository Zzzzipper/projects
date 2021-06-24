#ifndef LIB_SALEMANAGER_EXEMASTER_H_
#define LIB_SALEMANAGER_EXEMASTER_H_

#include "lib/sale_manager/include/SaleManager.h"
#include "lib/modem/EventRegistrar.h"
#include "lib/client/ClientContext.h"

#include "common/config/include/ConfigModem.h"
#include "common/mdb/master/cashless/MdbMasterCashlessStack.h"
#include "common/fiscal_register/include/FiscalRegister.h"

class TimerEngine;
class Uart;
class LedInterface;
class ExeMaster;
class MdbMasterEngine;
class SaleManagerExeMasterCore;
class SaleManagerExeMasterParams;
class MdbMasterCashlessInterface;

class SaleManagerExeMaster : public SaleManager {
public:
	SaleManagerExeMaster(ConfigModem *config, ClientContext *client, TimerEngine *timers, EventEngine *eventEngine, Uart *slaveUart, Uart *masterUart, MdbMasterCashlessStack *masterCashlessStack, Fiscal::Register *fiscalRegister, LedInterface *leds, EventRegistrar *chronicler);
	~SaleManagerExeMaster();
	virtual void reset();
	virtual void shutdown();

private:
	ExeMaster *executive;
	MdbMasterEngine *masterEngine;
	SaleManagerExeMasterCore *core;

	void initExeMaster(TimerEngine *timers, EventEngine *eventEngine, Uart *uart, SaleManagerExeMasterParams *params);
	void initMdbMaster(ConfigModem *config, TimerEngine *timers, EventEngine *eventEngine, Uart *uart, MdbMasterCashlessStack *masterCashlessStack, SaleManagerExeMasterParams *params);
};

#endif
