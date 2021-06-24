#include "GsmCommandCipStatus.h"

#include "timer/include/TimerEngine.h"
#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"

namespace Gsm {

CommandCipStatus::CommandCipStatus(StreamParser *parser, TimerEngine *timerEngine, StringBuilder *data, Command::Observer *observer) :
	parser(parser),
	timerEngine(timerEngine),
	data(data),
	observer(observer)
{
	this->timer = timerEngine->addTimer<CommandCipStatus, &CommandCipStatus::procTimer>(this);
}

CommandCipStatus::~CommandCipStatus() {
	timerEngine->deleteTimer(timer);
}

void CommandCipStatus::start(Command *command) {
	LOG_DEBUG(LOG_SIM, "start");
	this->command = command;
	this->data->clear();
	this->parser->readLine(this);
	this->timer->start(AT_COMMAND_DELAY);
	state = State_Delay;
}

void CommandCipStatus::procLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "procLine " << state);
	switch(state) {
	case State_Delay: deliverEvent(line); return;
	case State_Result: stateResultLine(line, lineLen); return;
	case State_Status: stateStatusLine(line, lineLen); return;
	case State_Info: stateInfoLine(line, lineLen); return;
	default: LOG_ERROR(LOG_SIM, "Unwaited line: " << state << "," << line);
	}
}

bool CommandCipStatus::procData(uint8_t) {
	return false;
}

void CommandCipStatus::procTimer() {
	LOG_DEBUG(LOG_SIM, "procTimer " << state);
	switch(state) {
	case State_Delay: stateDelayTimeout(); return;
	default: procErrorTimeout();
	}
}

void CommandCipStatus::stateDelayTimeout() {
	LOG_DEBUG(LOG_SIM, "stateDelayTimeout");
	parser->sendLine(command->getText());
	timer->start(command->getTimeout());
	state = State_Result;
}

void CommandCipStatus::stateResultLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateResultLine");
	StringParser parser(line, lineLen);
	if(parser.compare("OK") == true) {
		state = State_Status;
		return;
	} else if(parser.compare("ERROR") == true) {
		state = State_Idle;
		observer->procResponse(Command::Result_ERROR, line);
		return;
	} else {
		deliverEvent(line);
		return;
	}
}

void CommandCipStatus::stateStatusLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateStatusLine");
	StringParser parser(line, lineLen);
	if(parser.compareAndSkip("STATE: ") != true) {
		deliverResult(Command::Result_ERROR, line);
		return;
	}
	data->set(parser.unparsed(), parser.unparsedLen());
	connCount = 6;
	state = State_Info;
}

void CommandCipStatus::stateInfoLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateInfoLine");
	LOG_INFO_STR(LOG_SIM, line, lineLen);
	connCount--;
	if(connCount > 0) {
		return;
	}
	state = State_Idle;
	deliverResult(Command::Result_OK, line);
}

void CommandCipStatus::deliverResult(Command::Result result, const char *line) {
	LOG_INFO(LOG_SIM, "<< " << line << " [" << data->getString() << "]");
	timer->stop();
	state = State_Idle;
	if(observer == NULL) {
		LOG_ERROR(LOG_SIM, "Observer not defined.");
		return;
	}
	observer->procResponse(result, data->getString());
}

void CommandCipStatus::deliverEvent(const char *line) {
	LOG_INFO(LOG_SIM, "<< EVENT [" << line << "]");
	if(observer == NULL) {
		LOG_ERROR(LOG_SIM, "Observer not defined.");
		return;
	}
	observer->procEvent(line);
}

void CommandCipStatus::procErrorTimeout() {
	LOG_DEBUG(LOG_SIM, "procErrorTimeout " << state);
	state = State_Idle;
	observer->procResponse(Command::Result_TIMEOUT, NULL);
}

}
