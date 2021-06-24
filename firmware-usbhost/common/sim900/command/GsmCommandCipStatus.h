#ifndef COMMON_GSM_COMMANDCIPSTATUS_H_
#define COMMON_GSM_COMMANDCIPSTATUS_H_

#include "GsmCommandParser.h"

namespace Gsm {

class CommandCipStatus : public StreamParser::Customer {
public:
	CommandCipStatus(StreamParser *parser, TimerEngine *timerEngine, StringBuilder *data, Command::Observer *observer);
	~CommandCipStatus();
	void start(Command *command);

	virtual void procLine(const char *line, uint16_t lineLen);
	virtual bool procData(uint8_t);
	virtual void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Delay,
		State_Result,
		State_Status,
		State_Info
	};

	StreamParser *parser;
	TimerEngine *timerEngine;
	Timer *timer;
	StringBuilder *data;
	Command::Observer *observer;
	Command *command;
	State state;
	uint8_t connCount;

	void stateDelayTimeout();
	void stateEchoLine(const char *line, uint16_t lineLen);
	void stateResultLine(const char *line, uint16_t lineLen);
	void stateStatusLine(const char *line, uint16_t lineLen);
	void stateInfoLine(const char *line, uint16_t lineLen);

	void deliverResult(Command::Result result, const char *line);
	void deliverEvent(const char *line);
	void procErrorTimeout();
};

}
#endif
