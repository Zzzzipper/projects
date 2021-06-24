#ifndef LIB_SALEMANAGER_CCIT3_H_
#define LIB_SALEMANAGER_CCIT3_H_

#include "lib/client/ClientContext.h"
#include "lib/sale_manager/include/SaleManager.h"
#include "lib/modem/EventRegistrar.h"

#include "common/config/include/ConfigModem.h"
#include "common/timer/include/Timer.h"
#include "common/fiscal_register/include/FiscalRegister.h"
#include "common/code_scanner/CodeScanner.h"
#include "common/utils/include/Event.h"

class TimerEngine;
class TcpIp;
class Uart;
class MdbMasterCashlessStack;
class SaleManagerCciT3Core;
class LedInterface;
namespace Cci::T3 { class Driver; }

class SaleManagerCciT3 : public SaleManager {
public:
	SaleManagerCciT3(ConfigModem *config, ClientContext *client, TimerEngine *timerEngine, EventEngine *eventEngine, Uart *dexUart, CodeScannerInterface *scanner, TcpIp *tcpConn, MdbMasterCashlessStack *masterCashlessStack, Fiscal::Register *fiscalRegister, LedInterface *leds, EventRegistrar *chronicler);
	~SaleManagerCciT3();
	virtual void reset();
	virtual void shutdown();

private:
	Cci::T3::Driver *driver;
	SaleManagerCciT3Core *core;

	void initCsiDriver(TimerEngine *timerEngine, EventEngine *eventEngine, Uart *uart);
};

#endif
