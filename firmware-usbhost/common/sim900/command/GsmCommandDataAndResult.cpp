#include "GsmCommandDataAndResult.h"

#include "timer/include/TimerEngine.h"
#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"

namespace Gsm {

CommandDataAndResult::CommandDataAndResult(StreamParser *parser, TimerEngine *timerEngine, StringBuilder *data, Command::Observer *observer) :
	parser(parser),
	timerEngine(timerEngine),
	data(data),
	observer(observer)
{
	this->timer = timerEngine->addTimer<CommandDataAndResult, &CommandDataAndResult::procTimer>(this);
}

CommandDataAndResult::~CommandDataAndResult() {
	timerEngine->deleteTimer(timer);
}

void CommandDataAndResult::start(Command *command) {
	LOG_DEBUG(LOG_SIM, "start");
	this->command = command;
	this->data->clear();
	this->parser->readLine(this);
	this->timer->start(AT_COMMAND_DELAY);
	state = State_Delay;
}

void CommandDataAndResult::procLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "procLine " << state);
	switch(state) {
	case State_Delay: deliverEvent(line); return;
	case State_GsnData: stateGsnDataLine(line, lineLen); return;
	case State_CgmrData: stateCgmrDataLine(line, lineLen); return;
	case State_CcidData: stateCcidDataLine(line, lineLen); return;
	case State_CsqData: stateCsqDataLine(line, lineLen); return;
	case State_CopsData: stateCopsDataLine(line, lineLen); return;
	case State_CregData: stateCregDataLine(line, lineLen); return;
	case State_CgattData: stateCgattDataLine(line, lineLen); return;
	case State_CifsrData: stateCifsrDataLine(line, lineLen); return;
	case State_CipPing: stateCipPingLine(line, lineLen); return;
	case State_CipClose: stateCipCloseLine(line, lineLen); return;
	case State_Result: stateResultLine(line, lineLen); return;
	default: LOG_ERROR(LOG_SIM, "Unwaited line: " << state << "," << line);
	}
}

bool CommandDataAndResult::procData(uint8_t) {
	return false;
}

void CommandDataAndResult::procTimer() {
	LOG_DEBUG(LOG_SIM, "procTimer " << state);
	switch(state) {
	case State_Delay: stateDelayTimeout(); return;
	default: procErrorTimeout();
	}
}

void CommandDataAndResult::stateDelayTimeout() {
	LOG_DEBUG(LOG_SIM, "stateDelayTimeout");
	parser->sendLine(command->getText());
	timer->start(command->getTimeout());
	switch(command->getType()) {
	case Command::Type_Result: state = State_Result; return;
	case Command::Type_GsnData: state = State_GsnData; return;
	case Command::Type_CgmrData: state = State_CgmrData; return;
	case Command::Type_CcidData: state = State_CcidData; return;
	case Command::Type_CsqData: state = State_CsqData; return;
	case Command::Type_CopsData: state = State_CopsData; return;
	case Command::Type_CregData: state = State_CregData; return;
	case Command::Type_CgattData: state = State_CgattData; return;
	case Command::Type_CifsrData: state = State_CifsrData; return;
	case Command::Type_CipPing: gotoStateCipPing(); return;
	case Command::Type_CipClose: state = State_CipClose; return;
	default: {
		LOG_ERROR(LOG_SIM, "Unwaited type " << command->getType());
		state = State_Result;
		return;
	}
	}
}

void CommandDataAndResult::stateGsnDataLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateGsnDataLine");
	StringParser parser(line, lineLen);
	parser.skipEqual("0123456789");
	if(parser.unparsedLen() != 0) {
		deliverEvent(line);
		return;
	}
	data->set(line, lineLen);
	state = State_Result;
}

void CommandDataAndResult::stateCgmrDataLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateCgmrDataLine");
	StringParser parser(line, lineLen);
	if(parser.compareAndSkip("Revision:") == false) {
		deliverEvent(line);
		return;
	}
	parser.skipSpace();
	data->set(parser.unparsed(), parser.unparsedLen());
	state = State_Result;
}

void CommandDataAndResult::stateCcidDataLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateCcidDataLine");
	StringParser parser(line, lineLen);
	parser.skipEqual("0123456789fF");
	if(parser.unparsedLen() != 0) {
		deliverEvent(line);
		return;
	}
	data->set(line, lineLen);
	state = State_Result;
}

void CommandDataAndResult::stateCregDataLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateCregDataLine");
	StringParser parser(line, lineLen);
	if(parser.compareAndSkip("+CREG:") == false) {
		deliverEvent(line);
		return;
	}
	parser.skipSpace();
	uint16_t mode;
	if(parser.getNumber(&mode) == false) {
		deliverEvent(line);
		return;
	}
	parser.skipSpace();
	if(parser.compareAndSkip(",") == false) {
		deliverEvent(line);
		return;
	}
	parser.skipSpace();
	uint16_t stat;
	if(parser.getNumber(&stat) == false) {
		deliverEvent(line);
		return;
	}
	data->set(line, lineLen);
	state = State_Result;
}

void CommandDataAndResult::stateCsqDataLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateCsqDataLine");
	StringParser parser(line, lineLen);
	if(parser.compareAndSkip("+CSQ:") == false) {
		deliverEvent(line);
		return;
	}
	parser.skipSpace();
	data->set(parser.unparsed(), parser.unparsedLen());
	state = State_Result;
}

// +COPS: 0,1,"MegaFon"
void CommandDataAndResult::stateCopsDataLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateCopsDataLine");
	StringParser parser(line, lineLen);
	if(parser.compareAndSkip("+COPS:") == false) {
		deliverEvent(line);
		return;
	}
	parser.skipNotEqual(",");
	parser.skipEqual(",");
	parser.skipNotEqual(",");
	parser.skipEqual(",\"");
	uint16_t valueLen = parser.getValue("\"", (char*)data->getData(), data->getSize());
	data->setLen(valueLen);
	state = State_Result;
}

void CommandDataAndResult::stateCgattDataLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateCgattDataLine");
	StringParser parser(line, lineLen);
	if(parser.compareAndSkip("+CGATT:") == false) {
		deliverEvent(line);
		return;
	}
	parser.skipSpace();
	data->set(parser.unparsed(), parser.unparsedLen());
	state = State_Result;
}

void CommandDataAndResult::stateCifsrDataLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateCifsrDataLine");
	StringParser parser(line, lineLen);
	if(parser.compare("ERROR") == true) {
		deliverResult(Command::Result_ERROR, line);
		return;
	}

	parser.skipEqual("0123456789.");
	if(parser.unparsedLen() != 0) {
		deliverEvent(line);
		return;
	}
	data->set(line, lineLen);
	deliverResult(Command::Result_OK, line);
}

void CommandDataAndResult::gotoStateCipPing() {
	LOG_DEBUG(LOG_SIM, "gotoStateCipPing");
	cipPing = false;
	state = State_CipPing;
}

void CommandDataAndResult::stateCipPingLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateCipPingLine");
	StringParser parser(line, lineLen);
	if(parser.compare("OK") == true) {
		if(cipPing == true) {
			deliverResult(Command::Result_OK, line);
		} else {
			deliverResult(Command::Result_ERROR, line);
		}
		return;
	}
	if(parser.compare("ERROR") == true) {
		deliverResult(Command::Result_ERROR, line);
		return;
	}

	if(parser.compareAndSkip("+CIPPING:") == false) {
		deliverEvent(line);
		return;
	}
	parser.skipSpace();
	parser.skipEqual("0123456789");
	parser.skipSpace();
	parser.skipEqual(",\"");
	parser.skipNotEqual(",\"");
	parser.skipEqual(",\"");
	uint16_t replyTime;
	if(parser.getNumber(&replyTime) == false) {
		deliverEvent(line);
		return;
	}

	data->clear();
	*data << replyTime;
	if(replyTime < 600) { cipPing = true; }
}

void CommandDataAndResult::stateCipCloseLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateCipCloseLine");
	StringParser parser(line, lineLen);
	if(parser.compare("ERROR") == true) {
		deliverResult(Command::Result_ERROR, line);
		return;
	}

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
	if(parser.compareAndSkip("CLOSE OK") == false) {
		deliverEvent(line);
		return;
	}
	data->set(line, lineLen);
	deliverResult(Command::Result_OK, line);
}

void CommandDataAndResult::stateResultLine(const char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_SIM, "stateResultLine");
	StringParser parser(line, lineLen);
	if(parser.compare("OK") == true) {
		deliverResult(Command::Result_OK, line);
		return;
	}
	if(parser.compare("SHUT OK") == true) {
		deliverResult(Command::Result_OK, line);
		return;
	}
	if(parser.compare("ERROR") == true) {
		deliverResult(Command::Result_ERROR, line);
		return;
	}

	deliverEvent(line);
	return;
}

void CommandDataAndResult::deliverResult(Command::Result result, const char *line) {
	LOG_INFO(LOG_SIM, "<< " << line << " [" << data->getString() << "]");
	timer->stop();
	state = State_Idle;
	if(observer == NULL) {
		LOG_ERROR(LOG_SIM, "Observer not defined.");
		return;
	}
	observer->procResponse(result, data->getString());
}

void CommandDataAndResult::deliverEvent(const char *line) {
	LOG_INFO(LOG_SIM, "<< EVENT [" << line << "]");
	if(observer == NULL) {
		LOG_ERROR(LOG_SIM, "Observer not defined.");
		return;
	}
	observer->procEvent(line);
}

void CommandDataAndResult::procErrorTimeout() {
	LOG_DEBUG(LOG_SIM, "procErrorTimeout " << state);
	state = State_Idle;
	observer->procResponse(Command::Result_TIMEOUT, NULL);
}

}
