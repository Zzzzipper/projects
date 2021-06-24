#ifndef LIB_SALEMANAGER_CCIFRANKE_H_
#define LIB_SALEMANAGER_CCIFRANKE_H_

#include "lib/client/ClientContext.h"
#include "lib/sale_manager/include/SaleManager.h"
#include "lib/modem/EventRegistrar.h"

#include "common/config/include/ConfigModem.h"
#include "common/timer/include/Timer.h"
#include "common/fiscal_register/include/FiscalRegister.h"
#include "common/utils/include/Event.h"

class TimerEngine;
class Uart;
class MdbMasterCashlessStack;
#if 0
class SaleManagerCciFrankeCore;
#else
//class SaleManagerOrderCore;
class SaleManagerCciT4Core;
#endif
class LedInterface;
namespace Cci { namespace Franke { class Cashless; } }

class SaleManagerCciFranke : public SaleManager {
public:
	SaleManagerCciFranke(ConfigModem *config, ClientContext *client, TimerEngine *timerEngine, EventEngine *eventEngine, Uart *uart, MdbMasterCashlessStack *masterCashlessStack, Fiscal::Register *fiscalRegister, LedInterface *leds, EventRegistrar *chronicler);
	~SaleManagerCciFranke();
	virtual void reset();
	virtual void shutdown();

private:
	Cci::Franke::Cashless *slaveCashless;
#if 0
	SaleManagerCciFrankeCore *core;
#else
//	SaleManagerOrderCore *core;
	SaleManagerCciT4Core *core;
#endif

	void initSlaveCashless(TimerEngine *timerEngine, EventEngine *eventEngine, Uart *uart);
};

#endif
