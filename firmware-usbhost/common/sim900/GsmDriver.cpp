#include "include/GsmDriver.h"
#include "GsmHardware.h"
#include "command/GsmCommandQueue.h"

#include "timer/include/TimerEngine.h"
#include "mdb/MdbProtocol.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace Gsm {

#define COMMAND_QUEUE_SIZE	20
#define COMMAND_QUEUE_DELAY 1
#define HELLO_TIMEOUT		500
#define HELLO_TRY_NUMBER	10
#define SIM_TRY_NUMBER		5
#define SIM_TRY_DELAY		1000
#define GPRS_TRY_NUMBER		40
#define GPRS_TRY_DELAY		1000

Driver::Driver(
	ConfigBoot *config,
	HardwareInterface *hardware,
	CommandParserInterface *parser,
	TimerEngine *timers,
	EventEngineInterface *eventEngine,
	StatStorage *stat
) :
	config(config),
	hardware(hardware),
	parser(parser),
	timers(timers),
	eventEngine(eventEngine),
	stat(stat),
	deviceId(eventEngine)
{
	this->state = stat->add(Mdb::DeviceContext::Info_Gsm_State, State_Idle);
	this->hardResetCount = stat->add(Mdb::DeviceContext::Info_Gsm_HardResetCount, 0);
	this->softResetCount = stat->add(Mdb::DeviceContext::Info_Gsm_SoftResetCount, 0);
	this->gprsResetCount = stat->add(Mdb::DeviceContext::Info_Gsm_GprsResetCount, 0);
	this->commandMax = stat->add(Mdb::DeviceContext::Info_Gsm_CommandMax, 0);
	this->timer = timers->addTimer<Driver, &Driver::procTimer>(this);
	this->parser->setObserver(this);
	this->command = new Command(NULL);
	this->commands = new Fifo<Command*>(COMMAND_QUEUE_SIZE);
	this->currentCommand = NULL;
	this->dispatcher = new TcpDispatcher(timers, this, stat);
	this->hardware->init();
}

Driver::~Driver() {
	delete this->dispatcher;
	delete this->commands;
	delete this->command;
	this->timers->deleteTimer(this->timer);
}

bool Driver::init(TcpServerProcessor *serverProcessor) {
	if(state->get() != State_Idle) {
		LOG_ERROR(LOG_SIMD, "Wrong state " << state->get());
		return false;
	}
	dispatcher->bind(serverProcessor);
	hardResetCount->reset();
	softResetCount->reset();
	softResetNumber = 0;
	gprsResetCount->reset();
	commandMax->reset();
	gotoStatePowerOff();
	return true;
}

void Driver::restart() {
	LOG_INFO(LOG_SIMD, "restart " << state->get());
	hard = true;
	switch(state->get()) {
	case State_CommandReady: stateCommandReadyReset(); return;
	case State_CommandExecution: stateCommandExecutionReset(); return;
	case State_CommandNext: stateCommandNextReset(); return;
	default: LOG_ERROR(LOG_SIMD, "Wrong state " << state->get()); return;
	}
}

bool Driver::execute(Command *command) {
	switch(state->get()) {
	case State_CommandReady: return stateCommandReadyExecute(command);
	case State_CommandExecution: return stateCommandExecutionExecute(command);
	case State_CommandNext: return stateCommandNextExecute(command);
	default: LOG_ERROR(LOG_SIMD, "Wrong state " << state->get()); return false;
	}
}

bool Driver::executeOutOfTurn(Command *command) {
	if(state->get() != State_CommandReady && state->get() != State_CommandNext) {
		LOG_ERROR(LOG_SIMD, "Wrong state " << state->get());
		return false;
	}

	currentCommand = command;
	if(parser->execute(currentCommand) == false) {
		LOG_ERROR(LOG_SIMD, "Execute command failed" << currentCommand->getText());
		gotoStateCommandNext();
		currentCommand->deliverResponse(Command::Result_LOGIC_ERROR, NULL);
		return false;
	}

	timer->stop();
	gotoStateCommandExecution();
	return true;
}

void Driver::reset() {
	LOG_INFO(LOG_SIMD, "reset " << state->get());
	hard = false;
	switch(state->get()) {
	case State_CommandReady: stateCommandReadyReset(); return;
	case State_CommandExecution: stateCommandExecutionReset(); return;
	case State_CommandNext: stateCommandNextReset(); return;
	default: LOG_ERROR(LOG_SIMD, "Wrong state " << state->get()); return;
	}
}

void Driver::dump(StringBuilder *data) {
	*data << "gsm:" << state->get();
}

void Driver::procTimer() {
	switch(state->get()) {
	case State_PowerOff: statePowerOffTimeout(); break;
	case State_PowerOffPause: statePowerOffPauseTimeout(); break;
	case State_PowerOn: statePowerOnTimeout(); break;
	case State_PowerOnWait: statePowerOnWaitTimeout(); break;
	case State_SimTryDelay: gotoStateSimCheck(); break;
	case State_NetworkSearch: gotoStateResetCfun0(); break;
	case State_GprsUpTryDelay: gotoStateGprsCheck(); break;
	case State_CommandNext: stateCommandNextTimeout(); break;
	default: LOG_ERROR(LOG_SIMD, "Unwaited timeout: " << state->get()); return;
	}
}

void Driver::procResponse(Command::Result result, const char *data) {
	switch(state->get()) {
	case State_Hello: stateHelloResponse(result); break;
	case State_ResetToDefault: stateResetToDefaultResponse(result); break;
	case State_EchoOff: stateEchoOffResponse(result); break;
	case State_CopsFormat: stateCopsFormatResponse(result); break;
	case State_VersionGet: stateVersionGetResponse(result, data); break;
	case State_ImeiGet: stateImeiGetResponse(result, data); break;
	case State_SimCheck: stateSimCheckResponse(result); break;
	case State_SimId: stateSimIdResponse(result, data); break;
	case State_NetworkEventOn: stateNetworkEventOnResponse(result); break;
	case State_NetworkCheck: stateNetworkCheckResponse(result, data); break;
	case State_GprsShutdown: stateGprsShutdownResponse(result); break;
	case State_GprsCheck: stateGprsCheckResponse(result, data); break;
	case State_GprsRecvOff: stateGprsRecvOffResponse(result); break;
	case State_GprsMultiConnOn: stateGprsMultiConnOnResponse(result); break;
	case State_GprsCSTT: stateStateGprsCSTTResponse(result); break;
	case State_GprsCIICR: stateStateGprsCIICRResponse(result); break;
	case State_GprsCIFSR: stateStateGprsCIFSRResponse(result, data); break;
	case State_GprsCops: stateStateGprsCopsResponse(result, data); break;
	case State_GprsServer: stateStateGprsServerResponse(result, data); break;
	case State_CommandExecution: stateCommandExecutionResponse(result, data); break;
	case State_CommandReset: stateCommandResetResponse(result, data); break;
	case State_ResetCfun0: stateResetCfun0Response(result); break;
	case State_ResetCfun1: stateResetCfun1Response(result); break;
	default: LOG_ERROR(LOG_SIMD, "Unwaited response: " << state->get() << "," << result); return;
	}
}

void Driver::procEvent(const char *data) {
	switch(state->get()) {
	case State_NetworkSearch: stateNetworkSearchEvent(data); break;
	case State_CommandReady:
	case State_CommandExecution:
	case State_CommandNext:
	case State_CommandReset: stateCommandEvent(data); break;
	default: LOG_ERROR(LOG_SIMD, "Unwaited event: " << state->get() << "," << data);
	}
}

void Driver::gotoStatePowerOff() {
	LOG_DEBUG(LOG_SIMD, "gotoStatePowerOff");
	if(hardware->isStatusUp() == false) {
		gotoStatePowerOn();
		return;
	}
	LOG_INFO(LOG_SIMD, "Modem power off");
	hardware->pressPowerButton();
	timer->start(POWER_SHUTDOWN_CHECK);
	atTryNumber = 0;
	state->set(State_PowerOff);
}

void Driver::statePowerOffTimeout() {
	LOG_DEBUG(LOG_SIMD, "statePowerOffTimeout");
	if(hardware->isStatusUp() == false) {
		gotoStatePowerOffPause();
		return;
	} else {
		atTryNumber++;
		if(atTryNumber < POWER_SHUTDOWN_TRY_NUMBER) {
			timer->start(POWER_SHUTDOWN_CHECK);
			return;
		} else {
			hardware->releasePowerButton();
			gotoStateHello();
			return;
		}
	}
}

void Driver::gotoStatePowerOffPause() {
	LOG_DEBUG(LOG_SIMD, "gotoStatePowerOffPause");
	hardware->releasePowerButton();
	timer->start(POWER_SHUTDOWN_DELAY);
	state->set(State_PowerOffPause);
}

void Driver::statePowerOffPauseTimeout() {
	LOG_DEBUG(LOG_SIMD, "statePowerOffPauseTimeout");
	gotoStatePowerOn();
}

void Driver::gotoStatePowerOn() {
	LOG_DEBUG(LOG_SIMD, "gotoStatePowerOn");
	if(hardware->isStatusUp() == true) {
		gotoStatePowerOff();
		return;
	}
	LOG_INFO(LOG_SIMD, "Modem power on");
	softResetNumber = 0;
	hardResetCount->inc();
	hardware->pressPowerButton();
	timer->start(POWER_BUTTON_PRESS);
	atTryNumber = 0;
	state->set(State_PowerOn);
}

void Driver::statePowerOnTimeout() {
	LOG_DEBUG(LOG_SIMD, "statePowerOnTimeout");
	hardware->releasePowerButton();
	gotoStatePowerOnWait();
}

void Driver::gotoStatePowerOnWait() {
	LOG_DEBUG(LOG_SIMD, "gotoStatePowerOnWait");
	timer->start(POWER_SHUTDOWN_CHECK);
	state->set(State_PowerOnWait);
}

void Driver::statePowerOnWaitTimeout() {
	LOG_DEBUG(LOG_SIMD, "statePowerOnWaitTimeout");
	if(hardware->isStatusUp() == true) {
		gotoStateHello();
		return;
	} else {
		atTryNumber++;
		if(atTryNumber < POWER_SHUTDOWN_TRY_NUMBER) {
			timer->start(POWER_SHUTDOWN_CHECK);
			return;
		} else {
			gotoStateHello();
			return;
		}
	}
}

void Driver::gotoStateHello() {
	LOG_DEBUG(LOG_SIMD, "gotoStateHello");
	parser->reset();
	atTryNumber = 0;
	command->set(Command::Type_Result, "AT", HELLO_TIMEOUT);
	parser->execute(command);
	state->set(State_Hello);
}

void Driver::stateHelloResponse(Command::Result result) {
	LOG_DEBUG(LOG_SIMD, "stateHelloResponse");
	if(result == Command::Result_TIMEOUT) {
		atTryNumber++;
		if(atTryNumber > HELLO_TRY_NUMBER) {
			LOG_ERROR(LOG_SIMD, "Too many AT tries");
			gotoStatePowerOff();
			return;
		}
		command->set(Command::Type_Result, "AT", HELLO_TIMEOUT);
		parser->execute(command);
		return;
	} else if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStatePowerOff();
		return;
	};
	gotoStateResetToDefault();
}

void Driver::gotoStateResetToDefault() {
	LOG_DEBUG(LOG_SIMD, "gotoStateResetToDefault");
	command->set(Command::Type_Result, "ATZ");
	parser->execute(command);
	state->set(State_ResetToDefault);
}

void Driver::stateResetToDefaultResponse(Command::Result result) {
	LOG_DEBUG(LOG_SIMD, "stateResetToDefault");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	};
	gotoStateEchoOff();
}

void Driver::gotoStateEchoOff() {
	LOG_DEBUG(LOG_SIMD, "gotoStateEchoOff");
	command->set(Command::Type_Result, "ATE0");
	parser->execute(command);
	state->set(State_EchoOff);
}

void Driver::stateEchoOffResponse(Command::Result result) {
	LOG_DEBUG(LOG_SIMD, "stateEchoOffResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	};
	gotoStateCopsFormat();
}

void Driver::gotoStateCopsFormat() {
	LOG_DEBUG(LOG_SIMD, "gotoStateCopsFormat");
	command->set(Command::Type_Result, "AT+COPS=3,1");
	parser->execute(command);
	state->set(State_CopsFormat);
}

void Driver::stateCopsFormatResponse(Command::Result result) {
	LOG_DEBUG(LOG_SIMD, "stateCopsFormatResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	};
	gotoStateVersionGet();
}

void Driver::gotoStateVersionGet() {
	LOG_DEBUG(LOG_SIMD, "gotoStateImeiGet");
	command->set(Command::Type_CgmrData, "AT+CGMR");
	parser->execute(command);
	state->set(State_VersionGet);
}

void Driver::stateVersionGetResponse(Command::Result result, const char *data) {
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	}
	config->setGsmFirmwareVersion(data);
	gotoStateImeiGet();
}

//>AT+COPS?
//<+COPS: 0,0,"RUS MTS"
//<OK
//>AT+CUSD=1,\"*205#\"
//<OK
//<+CUSD: 0, "0412043004480020043D043E043C04350440003A0020002B00370039003300300031003600300031003300360035", 72
void Driver::gotoStateImeiGet() {
	LOG_DEBUG(LOG_SIMD, "gotoStateImeiGet");
	command->set(Command::Type_GsnData, "AT+GSN");
	parser->execute(command);
	state->set(State_ImeiGet);
}

void Driver::stateImeiGetResponse(Command::Result result, const char *data) {
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	}
	config->setImei(data);
	simTryNumber = 0;
	gotoStateSimCheck();
}

void Driver::gotoStateSimCheck() {
	LOG_DEBUG(LOG_SIMD, "gotoStateNetworkEventOn");
	simTryNumber++;
	if(simTryNumber > SIM_TRY_NUMBER) {
		LOG_ERROR(LOG_SIMD, "Too many GPRS tries");
		gotoStateResetCfun0();
		return;
	}
	command->set(Command::Type_Result, "AT+CPIN?");
	parser->execute(command);
	state->set(State_SimCheck);
}

void Driver::stateSimCheckResponse(Command::Result result) {
	LOG_DEBUG(LOG_SIMD, "stateSimCheckResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateSimTryDelay();
		return;
	};
	gotoStateSimId();
}

void Driver::gotoStateSimTryDelay() {
	LOG_DEBUG(LOG_SIMD, "gotoStateSimTryDelay");
	timer->start(SIM_TRY_DELAY);
	state->set(State_SimTryDelay);
}

void Driver::gotoStateSimId() {
	LOG_DEBUG(LOG_SIMD, "gotoStateSimId");
	command->set(Command::Type_CcidData, "AT+CCID");
	parser->execute(command);
	state->set(State_SimId);
}

void Driver::stateSimIdResponse(Command::Result result, const char *data) {
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	}
	config->setIccid(data);
	gotoStateNetworkEventOn();
}

void Driver::gotoStateNetworkEventOn() {
	LOG_DEBUG(LOG_SIMD, "gotoStateNetworkEventOn");
	command->set(Command::Type_Result, "AT+CREG=1");
	parser->execute(command);
	state->set(State_NetworkEventOn);
}

void Driver::stateNetworkEventOnResponse(Command::Result result) {
	LOG_DEBUG(LOG_SIMD, "stateNetworkEventOnResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	};
	gotoStateNetworkCheck();
}

void Driver::gotoStateNetworkCheck() {
	LOG_DEBUG(LOG_SIMD, "gotoStateNetworkCheck");
	command->set(Command::Type_CregData, "AT+CREG?");
	parser->execute(command);
	state->set(State_NetworkCheck);
}

void Driver::stateNetworkCheckResponse(Command::Result result, const char *data) {
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	}
	if(strcmp("+CREG: 1,1", data) != 0 && strcmp("+CREG: 1,5", data) != 0) {
		LOG_ERROR(LOG_SIMD, "Unwaited answer " << data);
		gotoStateNetworkSearch();
		return;
	}
	gotoStateGprsShutdown();
}

void Driver::gotoStateNetworkSearch() {
	LOG_DEBUG(LOG_SIMD, "gotoStateNetworkSearch");
	timer->start(AT_CREG_EVENT_TIMEOUT);
	state->set(State_NetworkSearch);
}

void Driver::stateNetworkSearchEvent(const char *data) {
	if(strcmp(data, "+CREG: 1") != 0 && strcmp(data, "+CREG: 5") != 0) {
		return;
	}
	timer->stop();
	gotoStateGprsShutdown();
}

void Driver::gotoStateGprsShutdown() {
	LOG_DEBUG(LOG_SIMD, "gotoStateGprsShutdown");
	command->set(Command::Type_Result, "AT+CIPSHUT", AT_CIPSHUT_TIMEOUT);
	parser->execute(command);
	gprsResetCount->inc();
	gprsResetNumber = 0;
	state->set(State_GprsShutdown);
}

void Driver::stateGprsShutdownResponse(Command::Result result) {
	LOG_DEBUG(LOG_SIMD, "stateGprsShutdownResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	};
	gprsResetNumber = 0;
	gotoStateGprsCheck();
}

void Driver::gotoStateGprsCheck() {
	LOG_DEBUG(LOG_SIMD, "gotoStateGprsCheck");
	gprsResetNumber++;
	if(gprsResetNumber > GPRS_TRY_NUMBER) {
		LOG_ERROR(LOG_SIMD, "Too many GPRS tries");
		gotoStateResetCfun0();
		return;
	}
	command->set(Command::Type_CgattData, "AT+CGATT?");
	parser->execute(command);
	state->set(State_GprsCheck);
}

void Driver::stateGprsCheckResponse(Command::Result result, const char *data) {
	LOG_DEBUG(LOG_SIMD, "stateGprsCheckResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	}
	if(strcmp("1", data) != 0) {
		LOG_ERROR(LOG_SIMD, "Unwaited answer " << data);
		gotoStateGprsUpTryDelay();
		return;
	}
	gotoStateGprsRecvOff();
}

void Driver::gotoStateGprsUpTryDelay() {
	LOG_DEBUG(LOG_SIMD, "gotoStateGprsUpTryDelay");
	timer->start(GPRS_TRY_DELAY);
	state->set(State_GprsUpTryDelay);
}

void Driver::gotoStateGprsRecvOff() {
	LOG_DEBUG(LOG_SIMD, "gotoStateGprsRecvOff");
	command->set(Command::Type_Result, "AT+CIPRXGET=1");
	parser->execute(command);
	timer->stop();
	state->set(State_GprsRecvOff);
}

void Driver::stateGprsRecvOffResponse(Command::Result result) {
	LOG_DEBUG(LOG_SIMD, "stateGprsRecvOffResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	};
	gotoStateGprsMultiConnOn();
}

void Driver::gotoStateGprsMultiConnOn() {
	LOG_DEBUG(LOG_SIMD, "gotoStateMultiConnectionOn");
	command->set(Command::Type_Result, "AT+CIPMUX=1");
	parser->execute(command);
	state->set(State_GprsMultiConnOn);
}

void Driver::stateGprsMultiConnOnResponse(Command::Result result) {
	LOG_DEBUG(LOG_SIMD, "stateMultiConnectionOnResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	};
	gotoStateGprsCSTT();
}

void Driver::gotoStateGprsCSTT() {
	LOG_DEBUG(LOG_SIMD, "gotoStateGprsCSTT");
	command->set(Command::Type_Result);
	command->setText() << "AT+CSTT=" << config->getGprsApn() << "," << config->getGprsUsername() << "," << config->getGprsPassword();
	parser->execute(command);
	state->set(State_GprsCSTT);
}

void Driver::stateStateGprsCSTTResponse(Command::Result result) {
	LOG_DEBUG(LOG_SIMD, "stateStateGprsCSTTResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	};
	gotoStateGprsCIICR();
}

void Driver::gotoStateGprsCIICR() {
	LOG_DEBUG(LOG_SIMD, "gotoStateGprsCIICR");
	command->set(Command::Type_Result, "AT+CIICR", AT_CIICR_TIMEOUT);
	parser->execute(command);
	state->set(State_GprsCIICR);
}

void Driver::stateStateGprsCIICRResponse(Command::Result result) {
	LOG_DEBUG(LOG_SIMD, "stateStateGprsCIICRResponse");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	};
	gotoStateGprsCIFSR();
}

void Driver::gotoStateGprsCIFSR() {
	LOG_DEBUG(LOG_SIMD, "gotoStateGprsCIFSR");
	command->set(Command::Type_CifsrData, "AT+CIFSR");
	parser->execute(command);
	state->set(State_GprsCIFSR);
}

void Driver::stateStateGprsCIFSRResponse(Command::Result result, const char *data) {
	LOG_DEBUG(LOG_SIMD, "stateStateGprsCIFSRResponse " << data);
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	};
	gotoStateGprsCops();
}

void Driver::gotoStateGprsCops() {
	LOG_DEBUG(LOG_SIMD, "gotoStateGprsCops");
	command->set(Command::Type_CopsData, "AT+COPS?");
	parser->execute(command);
	state->set(State_GprsCops);
}

void Driver::stateStateGprsCopsResponse(Command::Result result, const char *data) {
	LOG_DEBUG(LOG_SIMD, "stateStateGprsCopsResponse " << data);
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	};
	config->setOper(data);
#ifdef GSM_STATIC_IP
	gotoStateGprsServer();
#else
	gotoStateCommandReady();
	LOG_INFO(LOG_SIMD, "Network up");
	deliverEvent(Event_NetworkUp);
#endif
}

void Driver::gotoStateGprsServer() {
	LOG_DEBUG(LOG_SIMD, "gotoStateGprsServer");
	command->set(Command::Type_Result, "AT+CIPSERVER=1,80");
	parser->execute(command);
	state->set(State_GprsServer);
}

void Driver::stateStateGprsServerResponse(Command::Result result, const char *data) {
	LOG_DEBUG(LOG_SIMD, "stateStateGprsServerResponse " << data);
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStateResetCfun0();
		return;
	};
	gotoStateCommandReady();
	LOG_INFO(LOG_SIMD, "Network up");
	deliverEvent(Event_NetworkUp);
}

void Driver::gotoStateCommandReady() {
	LOG_DEBUG(LOG_SIMD, "gotoStateCommandReady");
	softResetNumber = 0;
	state->set(State_CommandReady);
}

bool Driver::stateCommandReadyExecute(Command *command) {
	LOG_DEBUG(LOG_SIMD, "stateCommandReadyExecute " << command->getText());
	currentCommand = command;
	if(parser->execute(currentCommand) == false) {
		LOG_ERROR(LOG_SIMD, "Execute command failed" << currentCommand->getText());
		return false;
	}
	gotoStateCommandExecution();
	return true;
}

void Driver::stateCommandReadyReset() {
	LOG_INFO(LOG_SIMD, "stateCommandReadyReset");
	if(hard == true) {
		gotoStatePowerOff();
	} else {
		gotoStateResetCfun0();
	}
	LOG_INFO(LOG_SIMD, "Network down");
	deliverEvent(Event_NetworkDown);
}

void Driver::gotoStateCommandExecution() {
	LOG_DEBUG(LOG_SIMD, "gotoStateCommandExecution");
	state->set(State_CommandExecution);
}

bool Driver::stateCommandExecutionExecute(Command *command) {
	LOG_DEBUG(LOG_SIMD, "stateCommandExecutionExecute " << command->getText());
	bool result = commands->push(command);
	commandMax->max(commands->getSize());
	return result;
}

void Driver::stateCommandExecutionReset() {
	LOG_INFO(LOG_SIMD, "stateCommandExecutionReset");
	gotoStateCommandReset();
}

void Driver::stateCommandExecutionResponse(Command::Result result, const char *data) {
	LOG_DEBUG(LOG_SIMD, "stateCommandExecutionResponse");
	gotoStateCommandNext();
	currentCommand->deliverResponse(result, data);
}

void Driver::gotoStateCommandNext() {
	LOG_DEBUG(LOG_SIMD, "gotoStateCommandNext");
	timer->start(COMMAND_QUEUE_DELAY);
	state->set(State_CommandNext);
}

bool Driver::stateCommandNextExecute(Command *command) {
	LOG_DEBUG(LOG_SIMD, "stateCommandNextExecute");
	bool result = commands->push(command);
	commandMax->max(commands->getSize());
	return result;
}

void Driver::stateCommandNextReset() {
	LOG_INFO(LOG_SIMD, "stateCommandNextReset");
	gotoStateResetCfun0();
	resetCommandQueue();
	LOG_INFO(LOG_SIMD, "Network down");
	deliverEvent(Event_NetworkDown);
}

void Driver::stateCommandNextTimeout() {
	LOG_DEBUG(LOG_SIMD, "stateCommandExecutionTimeout");
	if(commands->isEmpty() == true) {
		LOG_DEBUG(LOG_SIMD, "Command queue is empty");
		gotoStateCommandReady();
		return;
	}

	currentCommand = commands->pop();
	if(parser->execute(currentCommand) == false) {
		LOG_ERROR(LOG_SIMD, "Execute command failed" << currentCommand->getText());
		gotoStateCommandNext();
		currentCommand->deliverResponse(Command::Result_LOGIC_ERROR, NULL);
		return;
	}

	gotoStateCommandExecution();
}

void Driver::gotoStateCommandReset() {
	LOG_DEBUG(LOG_SIMD, "gotoStateCommandReset");
	state->set(State_CommandReset);
}

void Driver::stateCommandResetResponse(Command::Result result, const char *data) {
	LOG_INFO(LOG_SIMD, "stateCommandResetResponse");
	gotoStateResetCfun0();
	currentCommand->deliverResponse(result, data);
	resetCommandQueue();
	LOG_INFO(LOG_SIMD, "Network down");
	deliverEvent(Event_NetworkDown);
}

void Driver::gotoStateResetCfun0() {
	LOG_DEBUG(LOG_SIMD, "gotoStateResetCfun0");
	softResetNumber++;
	if(softResetNumber > AT_SOFT_RESET_MAX) {
		LOG_ERROR(LOG_SIMD, "Too many soft resets " << softResetNumber);
		gotoStatePowerOff();
		return;
	}
	softResetCount->inc();
	command->set(Command::Type_Result, "AT+CFUN=0", AT_CFUN_TIMEOUT);
	parser->execute(command);
	state->set(State_ResetCfun0);
}

void Driver::stateResetCfun0Response(Command::Result result) {
	LOG_DEBUG(LOG_SIMD, "stateResetCfun0Response");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStatePowerOff();
		return;
	}
	gotoStateResetCfun1();
}

void Driver::gotoStateResetCfun1() {
	LOG_DEBUG(LOG_SIMD, "gotoStateResetCfun1");
	command->set(Command::Type_Result, "AT+CFUN=1", AT_CFUN_TIMEOUT);
	parser->execute(command);
	state->set(State_ResetCfun1);
}

void Driver::stateResetCfun1Response(Command::Result result) {
	LOG_DEBUG(LOG_SIMD, "stateResetCfun1Response");
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIMD, "Error code " << result);
		gotoStatePowerOff();
		return;
	}
	gotoStateHello();
}

// Так как неизвестно когда придет событие, то все события пробрасываются в режиме команд всем соединениям.
void Driver::stateCommandEvent(const char *data) {
	LOG_DEBUG(LOG_SIMD, "stateCommandEvent");
	dispatcher->procEvent(data);
}

void Driver::resetCommandQueue() {
	LOG_DEBUG(LOG_SIMD, "resetCommandQueue");
	while(commands->isEmpty() == false) {
		Command *nextCommand = commands->pop();
		nextCommand->deliverResponse(Command::Result_RESET, NULL);
	}
}

void Driver::deliverEvent(EventType type) {
	EventInterface event1(deviceId, type);
	eventEngine->transmit(&event1);
	Event event2(type);
	dispatcher->proc(&event2);
}

}
