#include "GsmCommandSend.h"

#include "timer/include/TimerEngine.h"
#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"

namespace Gsm {

CommandSend::CommandSend(StreamParser *parser, TimerEngine *timerEngine, Command::Observer *observer) :
	parser(parser),
	timerEngine(timerEngine),
	observer(observer)
{
	this->timer = timerEngine->addTimer<CommandSend, &CommandSend::procTimer>(this);
}

CommandSend::~CommandSend() {
	timerEngine->deleteTimer(timer);
}

void CommandSend::start(Command *command) {
	LOG_DEBUG(LOG_SIM, "start");
	this->command = command;
	this->sendBuf = command->getData();
	this->sendLen = command->getDataLen();
	this->parser->readLine(this);
	this->timer->start(AT_COMMAND_DELAY);
	state = State_Delay;
}

void CommandSend::procLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "procLine " << state);
	switch(state) {
	case State_Delay: deliverEvent(line); return;
	case State_RecvStarter: deliverEvent(line); return;
	case State_SendDelay: deliverEvent(line); return;
	case State_Result: stateResultLine(line, lineLen); return;
	default: LOG_ERROR(LOG_SIM, "Unwaited line: " << state << "," << line);
	}
}

// тут возможно понадобиться оптимизация
bool CommandSend::procData(uint8_t b) {
	LOG_DEBUG(LOG_SIM, "procData " << state);
	switch(state) {
	case State_RecvStarter: return stateRecvStarterData(b);
	case State_SkipSpace: return stateSkipSpaceData(b);
	default: LOG_ERROR(LOG_SIM, "Unwaited symbol: " << state << "," << b); return false;
	}
}

void CommandSend::procTimer() {
	LOG_DEBUG(LOG_SIM, "procTimer " << state);
	switch(state) {
	case State_Delay: stateDelayTimeout(); return;
	case State_SendDelay: stateSendDelayTimeout(); return;
	default: procErrorTimeout();
	}
}

void CommandSend::stateDelayTimeout() {
	LOG_DEBUG(LOG_SIM, "stateDelayTimeout");
	parser->sendLine(command->getText());
	timer->start(command->getTimeout());
	parser->waitSymbol('>', this);
	state = State_RecvStarter;
}

bool CommandSend::stateRecvStarterData(uint8_t b) {
	LOG_DEBUG(LOG_SIM, "stateRecvStarterData");
	if(b != '>') {
		LOG_ERROR(LOG_SIM, "Unwaited symbol " << b);
		return true;
	}

	LOG_DEBUG(LOG_SIM, "FIRESTARTER" << sendBuf[0]);
	parser->readData(this);
	state = State_SkipSpace;
	return true;
}

bool CommandSend::stateSkipSpaceData(uint8_t b) {
	LOG_DEBUG(LOG_SIM, "stateSkipSpaceData " << b);
	gotoStateSendDelay();
	return true;
}

void CommandSend::gotoStateSendDelay() {
	LOG_DEBUG(LOG_SIM, "gotoStateSendDelay");
	parser->readLine(this);
	timer->start(AT_CIPSEND_SEND_DELAY);
	state = State_SendDelay;
}

bool CommandSend::stateSendDelayTimeout() {
	LOG_DEBUG(LOG_SIM, "stateSendDelayTimeout");
	LOG_TRACE_HEX(LOG_SIM, sendBuf, sendLen);
	parser->sendData(sendBuf, sendLen);
	timer->start(command->getTimeout());
	gotoStateResult();
	return true;
}

void CommandSend::gotoStateResult() {
	LOG_DEBUG(LOG_SIM, "gotoStateResult");
	state = State_Result;
}

void CommandSend::stateResultLine(const char *line, uint16_t lineLen) {
	StringParser parser(line, lineLen);
	uint16_t conn;
	if(parser.getNumber(&conn) == false) {
		deliverEvent(line);
		return;
	}
	if(parser.compareAndSkip(",") == false) {
		deliverEvent(line);
		return;
	}
	parser.skipSpace();
	if(parser.compareAndSkip("SEND OK") == true) {
		deliverResult(Command::Result_OK, line);
		return;
	} else if(parser.compareAndSkip("SEND FAIL") == true) {
		deliverResult(Command::Result_ERROR, line);
		return;
	} else {
		deliverEvent(line);
		return;
	}
}

void CommandSend::deliverResult(Command::Result result, const char *line) {
	LOG_INFO(LOG_SIM, "<< " << line << " []");
	timer->stop();
	state = State_Idle;
	if(observer == NULL) {
		LOG_ERROR(LOG_SIM, "Observer not defined.");
		return;
	}
	observer->procResponse(result, line);
}

void CommandSend::deliverEvent(const char *line) {
	LOG_INFO(LOG_SIM, "<< EVENT [" << line << "]");
	if(observer == NULL) {
		LOG_ERROR(LOG_SIM, "Observer not defined.");
		return;
	}
	observer->procEvent(line);
}

void CommandSend::procErrorTimeout() {
	LOG_DEBUG(LOG_SIM, "procErrorTimeout " << state);
	state = State_Idle;
	observer->procResponse(Command::Result_TIMEOUT, NULL);
}

}
