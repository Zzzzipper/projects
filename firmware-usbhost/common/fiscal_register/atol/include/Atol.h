#ifndef COMMON_ATOL_H
#define COMMON_ATOL_H

#include "fiscal_register/include/FiscalRegister.h"
#include "tcpip/include/TcpIp.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/LedInterface.h"

namespace Atol {

class CommandLayer;
class TaskLayer;
class PacketLayer;

class FiscalRegister : public Fiscal::Register {
public:
	FiscalRegister(Fiscal::Context *context, const char *ipaddr, uint16_t port, TcpIp *conn, TimerEngine *timers, EventEngineInterface *eventEngine, LedInterface *leds);
	virtual ~FiscalRegister();
	virtual EventDeviceId getDeviceId();
	virtual void reset();
	virtual void sale(Fiscal::Sale *saleData, uint32_t decimalPoint);
	virtual void getLastSale();
	virtual void closeShift();

private:
	CommandLayer *commandLayer;
	TaskLayer *taskLayer;
	PacketLayer *packetLayer;
};

}

#endif
