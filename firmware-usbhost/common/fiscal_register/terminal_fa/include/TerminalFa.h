#ifndef COMMON_TERMINALFA_H
#define COMMON_TERMINALFA_H

#include "fiscal_register/include/FiscalRegister.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"

namespace TerminalFa {

class CommandLayer;
class PacketLayer;

class FiscalRegister : public Fiscal::Register {
public:
	FiscalRegister(AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine);
	virtual ~FiscalRegister();
	virtual EventDeviceId getDeviceId();
	virtual void sale(Fiscal::Sale *saleData, uint32_t decimalPoint);
	virtual void getLastSale();
	virtual void closeShift();

private:
	CommandLayer *commandLayer;
	PacketLayer *packetLayer;
};

}

#endif
