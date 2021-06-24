#ifndef COMMON_GSM_COMMANDPROCESSOR_H_
#define COMMON_GSM_COMMANDPROCESSOR_H_

#include "GsmCommand.h"
#include "sim900/GsmStreamParser.h"
#include "utils/include/StringBuilder.h"

class AbstractUart;
class TimerEngine;
class Timer;

namespace Gsm {

class CommandNone : public StreamParser::Customer {
public:
	CommandNone(StreamParser *parser, Command::Observer *observer);
	void start();
	virtual void procLine(const char *line, uint16_t lineLen);
	virtual bool procData(uint8_t) { return false; }

private:
	StreamParser *parser;
	Command::Observer *observer;
};

class CommandDataAndResult;
class CommandSend;
class CommandRecv;
class CommandCipStatus;

class CommandParser : public CommandParserInterface, public Command::Observer {
public:
	CommandParser(AbstractUart *uart, TimerEngine *timers);
	virtual ~CommandParser();

	virtual void setObserver(Command::Observer *observer);
	virtual bool execute(Command *command);
	virtual void reset();

	virtual void procResponse(Command::Result result, const char *data);
	virtual void procEvent(const char *data);

private:
	enum State {
		State_Wait = 0,
		State_Command,
	};

	State state;
	StreamParser *parser;
	CommandNone *commandNone;
	CommandDataAndResult *commandDataAndResult;
	CommandSend *commandSend;
	CommandRecv *commandRecv;
	CommandCipStatus *commandCipStatus;
	StringBuilder data;
	Command::Observer *observer;
};

}
#endif
