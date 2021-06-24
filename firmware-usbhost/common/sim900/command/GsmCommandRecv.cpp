#include "GsmCommandRecv.h"

#include "timer/include/TimerEngine.h"
#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"

namespace Gsm {

CommandRecv::CommandRecv(StreamParser *parser, TimerEngine *timerEngine, StringBuilder *data, Command::Observer *observer) :
	parser(parser),
	timerEngine(timerEngine),
	data(data),
	observer(observer)
{
	this->timer = timerEngine->addTimer<CommandRecv, &CommandRecv::procTimer>(this);
}

CommandRecv::~CommandRecv() {
	timerEngine->deleteTimer(timer);
}

void CommandRecv::start(Command *command) {
	LOG_DEBUG(LOG_SIM, "start");
	this->command = command;
	this->recvBuf = command->getData();
	this->recvBufSize = command->getDataLen();
	this->recvLen = 0;
	this->data->clear();
	this->parser->readLine(this);
	this->timer->start(AT_COMMAND_DELAY);
	state = State_Delay;
}

void CommandRecv::procLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "procLine " << state);
	switch(state) {
	case State_Delay: deliverEvent(line); return;
	case State_RecvInfo: stateRecvInfoLine(line, lineLen); return;
	case State_RecvData: procErrorTimeout(); return;
	case State_Result: stateResultLine(line, lineLen); return;
	default: LOG_ERROR(LOG_SIM, "Unwaited line: " << state << "," << line);
	}
}

bool CommandRecv::procData(uint8_t b) {
	recvBuf[recvLen] = b;
	recvLen++;
	if(recvLen < recvBufSize) {
		return true;
	} else {
		LOG_DEBUG(LOG_SIM, "Receiving data complete");
		state = State_Result;
		return false;
	}
}

void CommandRecv::procTimer() {
	LOG_DEBUG(LOG_SIM, "procTimer " << state);
	switch(state) {
	case State_Delay: stateDelayTimeout(); return;
	default: procErrorTimeout();
	}
}

void CommandRecv::stateDelayTimeout() {
	LOG_DEBUG(LOG_SIM, "stateDelayTimeout");
	parser->sendLine(command->getText());
	timer->start(command->getTimeout());
	if(command->getType() == Command::Type_RecvData) {
		state = State_RecvInfo;
		return;
	} else {
		recvBufSize = command->getDataLen();
		parser->readData(this);
		state = State_RecvData;
		return;
	}
}

void CommandRecv::stateRecvInfoLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateRecvInfoLine");
	StringParser parser(line, lineLen);
	if(parser.compare("ERROR") == true) {
		deliverResult(Command::Result_ERROR, line);
		return;
	}

	if(parser.compareAndSkip("+CIPRXGET:") == false) {
		deliverEvent(line);
		return;
	}
	parser.skipSpace();
	uint16_t rxMode = 0;
	if(parser.getNumber(&rxMode) == false) {
		deliverEvent(line);
		return;
	}
	parser.skipEqual(" ,");
	uint16_t rxId = 0;
	if(parser.getNumber(&rxId) == false) {
		deliverEvent(line);
		return;
	}
	parser.skipEqual(" ,");
	uint16_t rxRecvLen  = 0;
	if(parser.getNumber(&rxRecvLen) == false) {
		deliverEvent(line);
		return;
	}
	parser.skipEqual(" ,");
	uint16_t rxTailLen = 0;
	if(parser.getNumber(&rxTailLen) == false) {
		deliverEvent(line);
		return;
	}

	LOG_INFO(LOG_SIM, "parsed CIPRXGET: " << rxRecvLen << "," << rxTailLen);
	recvBufSize = rxRecvLen;
	data->set(line, lineLen);
	if(rxRecvLen == 0) {
		state = State_Result;
		return;
	} else {
		this->parser->readData(this);
		state = State_RecvData;
		return;
	}
}

void CommandRecv::stateResultLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateResultLine");
	StringParser parser(line, lineLen);
	if(parser.compare("OK") == true) {
		deliverResult(Command::Result_OK, line);
		return;
	} else if(parser.compare("ERROR") == true) {
		deliverResult(Command::Result_ERROR, line);
		return;
	} else {
		deliverEvent(line);
		return;
	}
}

void CommandRecv::deliverResult(Command::Result result, const char *line) {
	LOG_INFO(LOG_SIM, "<< " << line << " [" << data->getString() << "]");
	timer->stop();
	state = State_Idle;
	if(observer == NULL) {
		LOG_ERROR(LOG_SIM, "Observer not defined.");
		return;
	}
	observer->procResponse(result, data->getString());
}

void CommandRecv::deliverEvent(const char *line) {
	LOG_INFO(LOG_SIM, "<< EVENT [" << line << "]");
	if(observer == NULL) {
		LOG_ERROR(LOG_SIM, "Observer not defined.");
		return;
	}
	observer->procEvent(line);
}

void CommandRecv::procErrorTimeout() {
	LOG_DEBUG(LOG_SIM, "procErrorTimeout " << state);
	state = State_Idle;
	observer->procResponse(Command::Result_TIMEOUT, NULL);
}

}
