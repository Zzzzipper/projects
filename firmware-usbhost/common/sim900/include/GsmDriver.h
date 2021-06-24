#ifndef COMMON_GSM_DRIVER_H
#define COMMON_GSM_DRIVER_H

#include "GsmDriverInterface.h"

#include "sim900/GsmHardware.h"
#include "sim900/tcp/GsmTcpDispatcher.h"
#include "sim900/command/GsmCommand.h"
#include "event/include/EventEngine.h"
#include "config/include/ConfigModem.h"
#include "config/include/StatStorage.h"
#include "utils/include/Fifo.h"
#include "utils/include/Event.h"
#include "utils/include/StringBuilder.h"

#include <stdint.h>

class TimerEngine;
class Timer;

namespace Gsm {

class Driver : public DriverInterface, public CommandProcessor, public Command::Observer {
public:
	enum EventType {
		Event_NetworkUp		= GlobalId_Sim900 | 0x01,
		Event_NetworkDown	= GlobalId_Sim900 | 0x02,
		Event_SignalQuality = GlobalId_Sim900 | 0x03,
	};
	Driver(ConfigBoot *config, HardwareInterface *hardware, CommandParserInterface *parser, TimerEngine *timers, EventEngineInterface *eventEngine, StatStorage *stat);
	virtual ~Driver();
	bool init(TcpServerProcessor *serverProcessor);
	void restart() override;
	TcpIp *getTcpConnection1() { return dispatcher->getConnection1(); }
	TcpIp *getTcpConnection2() { return dispatcher->getConnection2(); }
	TcpIp *getTcpConnection3() { return dispatcher->getConnection3(); }
	TcpIp *getTcpConnection4() { return dispatcher->getConnection4(); }
	TcpIp *getTcpConnection5() { return dispatcher->getConnection5(); }
	bool isNetworkUp() { return state->get() >= State_CommandReady; }

	virtual bool execute(Command *command) override;
	virtual bool executeOutOfTurn(Command *command) override;
	virtual void reset() override;
	virtual void dump(StringBuilder *data) override;

private:
	enum State {
		State_Idle = 0,
		State_PowerOff,
		State_PowerOffPause,
		State_PowerOn,
		State_PowerOnWait,
		State_Hello,
		State_ResetToDefault,
		State_EchoOff,
		State_CopsFormat,
		State_VersionGet,
		State_ImeiGet,
		State_SimCheck,
		State_SimTryDelay,
		State_SimId,
		State_NetworkEventOn,
		State_NetworkCheck,
		State_NetworkSearch,
		State_GprsShutdown,
		State_GprsCheck,
		State_GprsUpTryDelay,
		State_GprsRecvOff,
		State_GprsMultiConnOn,
		State_GprsCSTT,
		State_GprsCIICR,
		State_GprsCIFSR,
		State_GprsCops,
		State_GprsServer,
		State_CommandReady,
		State_CommandExecution,
		State_CommandNext,
		State_CommandReset,
		State_ResetCfun0,
		State_ResetCfun1,
	};

	ConfigBoot *config;
	HardwareInterface *hardware;
	CommandParserInterface *parser;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	StatStorage *stat;
	TcpDispatcher *dispatcher;
	EventDeviceId deviceId;
	StatNode *state;
	StatNode *hardResetCount;
	StatNode *softResetCount;
	StatNode *gprsResetCount;
	StatNode *commandMax;
	Timer *timer;
	bool hard;
	uint8_t atTryNumber;
	uint8_t simTryNumber;
	uint8_t gprsResetNumber;
	uint8_t softResetNumber;
	Command *command;
	Command *currentCommand;
	Fifo<Command*> *commands;

	void procTimer();
	virtual void procResponse(Command::Result result, const char *data);
	virtual void procEvent(const char *data);

	void gotoStatePowerOff();
	void statePowerOffTimeout();
	void gotoStatePowerOffPause();
	void statePowerOffPauseTimeout();
	void gotoStatePowerOn();
	void statePowerOnTimeout();
	void gotoStatePowerOnWait();
	void statePowerOnWaitTimeout();

	void gotoStateHello();
	void stateHelloResponse(Command::Result result);
	void gotoStateResetToDefault();
	void stateResetToDefaultResponse(Command::Result result);
	void gotoStateEchoOff();
	void stateEchoOffResponse(Command::Result result);
	void gotoStateCopsFormat();
	void stateCopsFormatResponse(Command::Result result);
	void gotoStateVersionGet();
	void stateVersionGetResponse(Command::Result result, const char *data);
	void gotoStateImeiGet();
	void stateImeiGetResponse(Command::Result result, const char *data);

	void gotoStateSimCheck();
	void stateSimCheckResponse(Command::Result result);
	void gotoStateSimTryDelay();
	void gotoStateSimId();
	void stateSimIdResponse(Command::Result result, const char *data);

	void gotoStateNetworkEventOn();
	void stateNetworkEventOnResponse(Command::Result result);
	void gotoStateNetworkCheck();
	void stateNetworkCheckResponse(Command::Result result, const char *data);
	void gotoStateNetworkSearch();
	void stateNetworkSearchEvent(const char *data);

	void gotoStateGprsShutdown();
	void stateGprsShutdownResponse(Command::Result result);
	void gotoStateGprsCheck();
	void gotoStateGprsUpTryDelay();
	void stateGprsCheckResponse(Command::Result result, const char *data);
	void gotoStateGprsRecvOff();
	void stateGprsRecvOffResponse(Command::Result result);
	void gotoStateGprsMultiConnOn();
	void stateGprsMultiConnOnResponse(Command::Result result);
	void gotoStateGprsCSTT();
	void stateStateGprsCSTTResponse(Command::Result result);
	void gotoStateGprsCIICR();
	void stateStateGprsCIICRResponse(Command::Result result);
	void gotoStateGprsCIFSR();
	void stateStateGprsCIFSRResponse(Command::Result result, const char *data);
	void gotoStateGprsCops();
	void stateStateGprsCopsResponse(Command::Result result, const char *data);
	void gotoStateGprsServer();
	void stateStateGprsServerResponse(Command::Result result, const char *data);

	void gotoStateCommandReady();
	bool stateCommandReadyExecute(Command *command);
	void stateCommandReadyReset();
	void gotoStateCommandExecution();
	bool stateCommandExecutionExecute(Command *command);
	void stateCommandExecutionReset();
	void stateCommandExecutionResponse(Command::Result result, const char *data);
	void gotoStateCommandNext();
	bool stateCommandNextExecute(Command *command);
	void stateCommandNextReset();
	void stateCommandNextTimeout();
	void gotoStateCommandReset();
	void stateCommandResetResponse(Command::Result result, const char *data);
	void stateCommandEvent(const char *data);
	void resetCommandQueue();

	void gotoStateResetCfun0();
	void stateResetCfun0Response(Command::Result result);
	void gotoStateResetCfun1();
	void stateResetCfun1Response(Command::Result result);

	void deliverEvent(EventType type);
};

}

#endif
