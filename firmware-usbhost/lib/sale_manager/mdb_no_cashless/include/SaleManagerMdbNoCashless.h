#if 0
#ifndef LIB_SALEMANAGER_MDBNOCASHLESS_H_
#define LIB_SALEMANAGER_MDBNOCASHLESS_H_

#include "lib/sale_manager/include/SaleManager.h"
#include "lib/utils/stm32/Reboot.h"
#include "lib/modem/EventRegistrar.h"

#include "common/config/include/ConfigModem.h"
#include "common/mdb/master/cashless/MdbMasterCashlessStack.h"
#include "common/fiscal_register/include/FiscalRegister.h"

class TimerEngine;
class EventEngine;
class Uart;
class LedInterface;
class MdbMasterCashlessInterface;
class MdbSlaveEngine;
class MdbMasterEngine;
class SaleManagerMdbNoCashlessCore;

class SaleManagerMdbNoCashless : public SaleManager {
public:
	SaleManagerMdbNoCashless(ConfigModem *config, TimerEngine *timers, EventEngine *events, Uart *slaveUart, Uart *masterUart, MdbMasterCashlessStack *externCashless, Fiscal::Register *fiscalRegister, LedInterface *leds, EventRegistrar *chronicler, Reboot::Reason rebootReason);
	~SaleManagerMdbNoCashless();
	virtual void reset();
	virtual void service();
	virtual void shutdown();

private:
	MdbSlaveEngine *slaveEngine;
	MdbMasterEngine *masterEngine;
	SaleManagerMdbNoCashlessCore *core;
};

#endif
#else
#ifndef LIB_SALEMANAGER_MDBNOCASHLESS_H_
#define LIB_SALEMANAGER_MDBNOCASHLESS_H_

#include "lib/sale_manager/include/SaleManager.h"
#include "lib/utils/stm32/Reboot.h"
#include "lib/modem/EventRegistrar.h"

#include "common/config/include/ConfigModem.h"
#include "common/mdb/master/cashless/MdbMasterCashlessStack.h"
#include "common/fiscal_register/include/FiscalRegister.h"

class TimerEngine;
class EventEngine;
class Uart;
class LedInterface;
class MdbMasterCashlessInterface;
class MdbSnifferEngine;
class SaleManagerMdbNoCashlessCore;

class SaleManagerMdbNoCashless : public SaleManager {
public:
	SaleManagerMdbNoCashless(ConfigModem *config, TimerEngine *timers, EventEngine *events, Uart *slaveUart, Uart *masterUart, MdbMasterCashlessStack *externCashless, Fiscal::Register *fiscalRegister, LedInterface *leds, EventRegistrar *chronicler, Reboot::Reason rebootReason);
	~SaleManagerMdbNoCashless();
	virtual void reset();
	virtual void service();
	virtual void shutdown();

private:
	MdbSnifferEngine *engine;
	SaleManagerMdbNoCashlessCore *core;
};

#endif
#endif
