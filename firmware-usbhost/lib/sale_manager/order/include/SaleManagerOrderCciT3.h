#ifndef LIB_SALEMANAGER_ORDER_CCIT3_H_
#define LIB_SALEMANAGER_ORDER_CCIT3_H_

#include "lib/client/ClientContext.h"
#include "lib/sale_manager/include/SaleManager.h"
#include "lib/modem/EventRegistrar.h"

#include "common/config/include/ConfigModem.h"
#include "common/timer/include/Timer.h"
#include "common/fiscal_register/include/FiscalRegister.h"
#include "common/utils/include/Event.h"

class TimerEngine;
class TcpIp;
class Uart;
class SaleManagerOrderCore;
class OrderDeviceInterface;
class OrderMasterInterface;
class PinCodeDeviceInterface;
class LedInterface;
namespace Cci { namespace T3 { class Driver; } }

class SaleManagerOrderCciT3 : public SaleManager {
public:
	SaleManagerOrderCciT3(ConfigModem *config, ClientContext *client, TimerEngine *timerEngine, EventEngine *eventEngine, Uart *deviceUart, Uart *pincodeUart, TcpIp *tcpConn, Fiscal::Register *fiscalRegister, LedInterface *leds, EventRegistrar *chronicler);
	~SaleManagerOrderCciT3();
	virtual void reset();
	virtual void shutdown();

private:
	SaleManagerOrderCore *core;

	OrderMasterInterface *initMaster(ConfigModem *config, TimerEngine *timerEngine, EventEngine *eventEngine, TcpIp *tcpConn);
	OrderDeviceInterface *initDevice(TimerEngine *timerEngine, EventEngine *eventEngine, Uart *deviceUart, Uart *pincodeUart);
};

#endif
