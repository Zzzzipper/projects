#include "GsmCommandParser.h"
#include "GsmCommandDataAndResult.h"
#include "GsmCommandSend.h"
#include "GsmCommandRecv.h"
#include "GsmCommandCipStatus.h"

#include "timer/include/TimerEngine.h"
#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace Gsm {

/*
 * Примеры команд:
 * Если включено эхо, <CR><LF><command><CR><LF><answer><CR><LF>
 * Если ответ на <command>? -> <CR><LF><answer><CR><LF>OK<CR><LF>
 * Если ответ на <command>  -> <CR><LF> OK<CR><LF>
 * AT<CR><LF> -> AT<CR><LF><CR><LF>OK<CR><LF>
 *
 * Типы ответов:
 * - Unsolicited Result Code - асинхронные события
 * - Final Result Code - ответы на AT-команды
 *
 * Ответы не разрывают друг друга.
 * Лучше всего не отключать echo.
 *
 * Final Result Code состоят из данных и кода результата.
 * Допустимые коды результатов:
 * - OK
 * - ERROR
 * - NO CARRIER
 * - NO ANSWER
 * - NO DIALTONE
 * - BUSY
 *
 * https://embeddedfreak.wordpress.com/2008/08/19/handling-urc-unsolicited-result-code-in-hayes-at-command/
 */

CommandNone::CommandNone(StreamParser *parser, Command::Observer *observer) :
	parser(parser),
	observer(observer)
{

}

void CommandNone::start() {
	parser->readLine(this);
}

void CommandNone::procLine(const char *line, uint16_t) {
	LOG_INFO(LOG_SIM, "<< EVENT [" << line << "]");
	observer->procEvent(line);
}

CommandParser::CommandParser(AbstractUart *uart, TimerEngine *timerEngine) :
	state(State_Wait),
	data(AT_DATA_MAX_SIZE, AT_DATA_MAX_SIZE),
	observer(NULL)
{
	this->parser = new StreamParser(uart);
	this->commandNone = new CommandNone(parser, this);
	this->commandDataAndResult = new CommandDataAndResult(parser, timerEngine, &data, this);
	this->commandSend = new CommandSend(parser, timerEngine, this);
	this->commandRecv = new CommandRecv(parser, timerEngine, &data, this);
	this->commandCipStatus = new CommandCipStatus(parser, timerEngine, &data, this);
}

CommandParser::~CommandParser() {
	delete commandCipStatus;
	delete commandRecv;
	delete commandSend;
	delete commandDataAndResult;
	delete commandNone;
	delete parser;
}

void CommandParser::setObserver(Command::Observer *observer) {
	this->observer = observer;
}

void CommandParser::reset() {
	LOG_INFO(LOG_SIM, "reset");
	commandNone->start();
	parser->reset();
	state = State_Wait;
}

bool CommandParser::execute(Command *command) {
	if(state != State_Wait) {
		LOG_ERROR(LOG_SIM, "Wrong state: " << state << "," << command->getText());
		return false;
	}

	LOG_INFO(LOG_SIM, ">> " << command->getText());
	switch(command->getType()) {
	case Command::Type_Result:
	case Command::Type_GsnData:
	case Command::Type_CgmrData:
	case Command::Type_CcidData:
	case Command::Type_CsqData:
	case Command::Type_CopsData:
	case Command::Type_CregData:
	case Command::Type_CgattData:
	case Command::Type_CifsrData:
	case Command::Type_CipPing:
	case Command::Type_CipClose: commandDataAndResult->start(command); break;
	case Command::Type_CipStatus: commandCipStatus->start(command); break;
	case Command::Type_SendData: commandSend->start(command); break;
	case Command::Type_RecvData: commandRecv->start(command); break;
	default: {
		LOG_ERROR(LOG_SIM, "Unsupported command type " << command->getType());
		return false;
	}
	}

	state = State_Command;
	return true;
}

void CommandParser::procResponse(Command::Result result, const char *data) {
	commandNone->start();
	state = State_Wait;
	observer->procResponse(result, data);
}

void CommandParser::procEvent(const char *data) {
	observer->procEvent(data);
}

}
