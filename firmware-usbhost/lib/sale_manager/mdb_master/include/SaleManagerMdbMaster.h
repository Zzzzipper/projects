#ifndef LIB_SALEMANAGER_MDBMASTER_H_
#define LIB_SALEMANAGER_MDBMASTER_H_

#include "lib/sale_manager/include/SaleManager.h"
#include "lib/modem/EventRegistrar.h"

#include "common/config/include/ConfigModem.h"
#include "common/mdb/master/cashless/MdbMasterCashlessStack.h"
#include "common/fiscal_register/include/FiscalRegister.h"

class TimerEngine;
class Uart;
class LedInterface;
class MdbSlaveEngine;
class MdbMasterEngine;
class MdbMasterCashlessInterface;
class SaleManagerMdbMasterCore;
class SaleManagerMdbMasterCore;
class SaleManagerMdbMasterParams;

class SaleManagerMdbMaster : public SaleManager {
public:
	SaleManagerMdbMaster(ConfigModem *config, TimerEngine *timers, EventEngine *events, Uart *slaveUart, Uart *masterUart, MdbMasterCashlessStack *masterCashlessStack, Fiscal::Register *fiscalRegister, LedInterface *leds, EventRegistrar *chronicler);
	~SaleManagerMdbMaster();
	virtual void reset();
	virtual void service();
	virtual void shutdown();
	void testCredit();

private:
	MdbSlaveEngine *slaveEngine;
	MdbMasterEngine *masterEngine;
//	SaleManagerMdbMasterCore *core;
	SaleManagerMdbMasterCore *core;

	void initMdbSlave(ConfigModem *config, TimerEngine *timers, EventEngine *eventEngine, Uart *uart, SaleManagerMdbMasterParams *params);
	void initMdbMaster(ConfigModem *config, TimerEngine *timers, EventEngine *eventEngine, Uart *uart, MdbMasterCashlessStack *masterCashlessStack, SaleManagerMdbMasterParams *params);
};

#endif
