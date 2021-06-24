#ifndef LIB_SALEMANAGER_CCIT4_H_
#define LIB_SALEMANAGER_CCIT4_H_

#include "lib/client/ClientContext.h"
#include "lib/sale_manager/include/SaleManager.h"
#include "common/config/include/ConfigModem.h"
#include "common/timer/include/Timer.h"
#include "common/fiscal_register/include/FiscalRegister.h"
#include "common/utils/include/Event.h"

class TimerEngine;
class Uart;
class MdbMasterCashlessStack;
class SaleManagerCciT4Core;
namespace CciCsi { class Cashless; }

class SaleManagerCciT4 : public SaleManager {
public:
	SaleManagerCciT4(ConfigModem *config, ClientContext *client, TimerEngine *timerEngine, EventEngine *eventEngine, Uart *uart, MdbMasterCashlessStack *masterCashlessStack, Fiscal::Register *fiscalRegister);
	~SaleManagerCciT4();
	virtual void reset();
	virtual void shutdown();

private:
	CciCsi::Cashless *slaveCashless;
	SaleManagerCciT4Core *core;

	void initSlaveCashless(TimerEngine *timerEngine, EventEngine *eventEngine, Uart *uart);
};

#endif
