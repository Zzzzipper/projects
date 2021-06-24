#ifndef LIB_FISCALREGISTER_CHEKONLINE_COMMANDLAYER_H
#define LIB_FISCALREGISTER_CHEKONLINE_COMMANDLAYER_H

#include "ChekOnlineResponseParser.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "config/include/ConfigModem.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "tcpip/include/TcpIp.h"
#include "utils/include/LedInterface.h"
#include "utils/include/Buffer.h"
#include "utils/include/StringBuilder.h"

namespace ChekOnline {

class CommandLayer : public EventObserver {
public:
	CommandLayer(ConfigModem *config, Fiscal::Context *context, TcpIp *conn, TimerEngine *timers, EventEngineInterface *eventEngine, RealTimeInterface *realtime, LedInterface *leds);
	virtual ~CommandLayer();
	EventDeviceId getDeviceId();
	void reset();
	void sale(Fiscal::Sale *saleData);
	void getLastSale();
	void closeShift();

	void procTimer();
	virtual void proc(Event *event);

private:
	enum State {
		State_Idle = 0,
		State_Connect,
		State_CheckSend,
		State_CheckRecv,
		State_Disconnect,
	};
	enum Command {
		Command_None = 0,
		Command_Reset,
		Command_Sale,
		Command_GetLastSale,
		Command_CloseShift,
	};

	ConfigBoot *boot;
	ConfigFiscal *fiscal;
	Fiscal::Context *context;
	TcpIp *conn;
	TimerEngine *timers;
	Timer *timer;
	EventEngineInterface *eventEngine;
	RealTimeInterface *realtime;
	LedInterface *leds;
	EventDeviceId deviceId;
	State state;
	Command command;
	Fiscal::Sale *saleData;
	StringBuilder id;
	StringBuilder buf;
	EventEnvelope envelope;
	ChekOnline::ResponseParser parser;

	void gotoStateConnect();
	void stateConnectEvent(Event *event);

	void gotoStateCheckSend();
	void generateId();
	void makeCheck();
	void makeRequest();
	void stateCheckSendEvent(Event *event);

	void gotoStateCheckRecv();
	void stateCheckRecvEvent(Event *event);
	void stateCheckRecvTimeout();

	void gotoStateDisconnect();
	void stateDisconnectEvent(Event *event);

	void procError(uint16_t errorCode);
};

}

#endif
