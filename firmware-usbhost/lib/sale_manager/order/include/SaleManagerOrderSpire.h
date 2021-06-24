#ifndef LIB_SALEMANAGER_ORDER_SPIRE_H_
#define LIB_SALEMANAGER_ORDER_SPIRE_H_

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
class MdbSnifferEngine;
class SaleManagerOrderCore;
class OrderDeviceInterface;
class OrderMasterInterface;
class PinCodeDeviceInterface;
class LedInterface;
namespace Cci { namespace T3 { class Driver; } }

class SaleManagerOrderSpire : public SaleManager {
public:
	SaleManagerOrderSpire(ConfigModem *config, ClientContext *client, TimerEngine *timerEngine, EventEngine *eventEngine, Uart *slaveUart, Uart *masterUart, Uart *pincodeUart, TcpIp *tcpConn, Fiscal::Register *fiscalRegister, LedInterface *leds, EventRegistrar *chronicler);
	~SaleManagerOrderSpire();
	void reset() override;
	void shutdown() override;

private:
	MdbSnifferEngine *engine;
	SaleManagerOrderCore *core;

	OrderMasterInterface *initMaster(ConfigModem *config, TimerEngine *timerEngine, EventEngine *eventEngine, TcpIp *tcpConn);
	OrderDeviceInterface *initDevice(ConfigModem *config, TimerEngine *timerEngine, EventEngine *eventEngine, Uart *slaveUart, Uart *masterUart);
};

#endif
