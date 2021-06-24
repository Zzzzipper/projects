#ifndef LIB_FISCALREGISTER_EPHOR_COMMANDLAYER_H
#define LIB_FISCALREGISTER_EPHOR_COMMANDLAYER_H

#include "EphorResponseParser.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "config/include/ConfigModem.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/LedInterface.h"
#include "http/include/HttpClient.h"
#include "utils/include/Buffer.h"
#include "utils/include/StringBuilder.h"

namespace Ephor {

class CommandLayer : public EventObserver, public EventSubscriber {
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
	virtual void proc(EventEnvelope *envelope);

private:
	enum State {
		State_Idle = 0,
		State_Check,
		State_CheckTryDelay,
		State_PollDelay,
		State_Poll,
		State_PollTryDelay,
	};

	ConfigBoot *boot;
	ConfigFiscal *fiscal;
	Fiscal::Context *context;
	TimerEngine *timers;
	Timer *timer;
	EventEngineInterface *eventEngine;
	RealTimeInterface *realtime;
	LedInterface *leds;
	EventDeviceId deviceId;
	State state;
	Http::Client *conn;
	Http::Request req;
	Http::Response resp;
	StringBuilder reqPath;
	StringBuilder reqData;
	StringBuilder respData;
	uint32_t id;
	uint16_t tryCount;
	Ephor::CheckResponseParser respParser;
	Fiscal::Sale *saleData;

	void gotoStateCheck();
	void makeCheckRequest();
	void makeCheckBody();
	void stateCheckEvent(Event *event);
	void stateCheckEventRequestComplete();
	void gotoStateCheckTryDelay(uint16_t errorCode, uint32_t line, const char *suffix = NULL);
	void stateCheckTryDelayEnvelope(EventEnvelope *envelope);
	void stateCheckTryDelayTimeout();
	void procCheckError(uint16_t errorCode, uint32_t line, const char *suffix = NULL);

	void gotoStatePollDelay();
	void statePollDelayTimeout();

	void gotoStatePoll();
	void makePollBody();
	void statePollEvent(Event *event);
	void statePollEventRequestComplete();
	void gotoStatePollTryDelay(uint16_t errorCode, uint32_t line, const char *suffix = NULL);
	void statePollTryDelayEnvelope(EventEnvelope *envelope);
	void statePollTryDelayTimeout();
	void procPollError();
};

}

#endif
