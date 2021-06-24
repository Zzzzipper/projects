#ifndef LIB_ORANGEDATA_COMMANDLAYER_H
#define LIB_ORANGEDATA_COMMANDLAYER_H

#include "OrangeDataResponseParser.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "config/include/ConfigModem.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/LedInterface.h"
#include "tcpip/include/TcpIp.h"
#include "http/HttpResponseParser.h"
#include "utils/include/Buffer.h"
#include "utils/include/StringBuilder.h"
#include "WolfSslRsaSign.h"

namespace OrangeData {

class CommandLayer : public EventObserver, public EventSubscriber {
public:
	CommandLayer(ConfigModem *config, Fiscal::Context *context, RsaSignInterface *sign, TcpIp *conn, TimerEngine *timers, EventEngineInterface *eventEngine, RealTimeInterface *realtime, LedInterface *leds);
	virtual ~CommandLayer();
	uint16_t getDeviceId();
	void reset();
	void sale(Fiscal::Sale *saleData);
	void getLastSale();
	void closeShift();

	virtual void proc(Event *event);
	virtual void proc(EventEnvelope *envelope);
	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_CheckConnect,
		State_CheckSend,
		State_CheckRecv,
		State_CheckTryDisconnect,
		State_CheckTryDelay,
		State_CheckDisconnect,
		State_PollDelay,
		State_PollConnect,
		State_PollSend,
		State_PollRecv,
		State_PollTryDisconnect,
		State_PollTryDelay,
		State_PollDisconnect,
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
	RsaSignInterface *sign;
	TcpIp *conn;
	TimerEngine *timers;
	Timer *timer;
	EventEngineInterface *eventEngine;
	RealTimeInterface *realtime;
	LedInterface *leds;
	uint16_t deviceId;
	Command command;
	Fiscal::Sale *saleData;
	StringBuilder id;
	StringBuilder buf;
	EventEnvelope envelope;
	OrangeData::CheckResponseParser *checkParser;
	OrangeData::PollResponseParser *pollParser;
	uint16_t tryCount;

	void gotoStateCheckConnect();
	void stateCheckConnectEvent(Event *event);

	void gotoStateCheckSend();
	void makeMoney(uint32_t value);
	void generateId();
	void makeCheckRequest();
	void makeCheckBody();
	void stateCheckSendEvent(Event *event);

	void gotoStateCheckRecv();
	void stateCheckRecvEvent(Event *event);
	void stateCheckRecvTimeout();

	void gotoStateCheckDisconnect();
	void stateCheckDisconnectEvent(Event *event);

	void gotoStateCheckTryDisconnect(uint16_t errorCode, uint32_t line, const char *suffix = NULL);
	void stateCheckTryDisconnectEvent(Event *event);

	void gotoStateCheckTryDelay();
	void stateCheckTryDelayEnvelope(EventEnvelope *envelope);
	void stateCheckTryDelayTimeout();

	void gotoStatePollDelay();
	void statePollDelayTimeout();

	void gotoStatePollConnect();
	void statePollConnectEvent(Event *event);

	void gotoStatePollSend();
	void makePollRequest();
	void statePollSendEvent(Event *event);

	void gotoStatePollRecv();
	void statePollRecvEvent(Event *event);
	void statePollRecvEventRecv(Event *event);
	void statePollRecvTimeout();

	void gotoStatePollDisconnect();
	void statePollDisconnectEvent(Event *event);

	void gotoStatePollTryDisconnect(uint16_t errorCode, uint32_t line, const char *suffix = NULL);
	void statePollTryDisconnectEvent(Event *event);

	void gotoStatePollTryDelay();
	void statePollTryDelayEnvelope(EventEnvelope *envelope);
	void statePollTryDelayTimeout();

	void procError(uint16_t errorCode, uint32_t line, const char *suffix = NULL);
	uint8_t getPaymentType(uint8_t paymentType);
	uint8_t convertTaxSystem(uint8_t taxSystem);
};

}

#endif
