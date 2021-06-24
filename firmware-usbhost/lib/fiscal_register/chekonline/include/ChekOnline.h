#ifndef LIB_FISCALREGISTER_CHEKONLINE_H
#define LIB_FISCALREGISTER_CHEKONLINE_H

#include "fiscal_register/include/FiscalRegister.h"
#include "config/include/ConfigModem.h"
#include "tcpip/include/TcpIp.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/LedInterface.h"

class WolfSslConnection;
class WolfSslAdapter;

namespace ChekOnline {

class CommandLayer;

class FiscalRegister : public Fiscal::Register {
public:
	FiscalRegister(ConfigModem *config, Fiscal::Context *context, TcpIp *conn, TimerEngine *timers, EventEngineInterface *eventEngine, RealTimeInterface *realtime, LedInterface *leds);
	virtual ~FiscalRegister();
	virtual EventDeviceId getDeviceId();
	virtual void reset();
	virtual void sale(Fiscal::Sale *saleData, uint32_t decimalPoint);
	virtual void getLastSale();
	virtual void closeShift();

private:
	WolfSslConnection *sslConn;
	WolfSslAdapter *sslAdapter;
	CommandLayer *commandLayer;
};

}

#endif
