#ifndef COMMON_GSM_COMMANDSEND_H_
#define COMMON_GSM_COMMANDSEND_H_

#include "GsmCommandParser.h"

namespace Gsm {

class CommandSend : public StreamParser::Customer {
public:
	CommandSend(StreamParser *parser, TimerEngine *timerEngine, Command::Observer *observer);
	~CommandSend();
	void start(Command *command);

	virtual void procLine(const char *line, uint16_t lineLen);
	virtual bool procData(uint8_t);
	virtual void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Delay,
		State_RecvStarter,
		State_SkipSpace,
		State_SendDelay,
		State_Result
	};

	StreamParser *parser;
	TimerEngine *timerEngine;
	Timer *timer;
	Command::Observer *observer;
	Command *command;
	State state;
	const uint8_t *sendBuf;
	uint16_t sendLen;
	uint16_t recvLen;

	void stateDelayTimeout();
	void stateEchoLine(const char *line, uint16_t lineLen);
	bool stateRecvStarterData(uint8_t b);
	bool stateSkipSpaceData(uint8_t b);
	void gotoStateSendDelay();
	bool stateSendDelayTimeout();
	void gotoStateResult();
	void stateResultLine(const char *line, uint16_t lineLen);

	void deliverResult(Command::Result result, const char *line);
	void deliverEvent(const char *line);
	void procErrorTimeout();
};

}
#endif
