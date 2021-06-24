#ifndef COMMON_GSM_COMMANDRECV_H_
#define COMMON_GSM_COMMANDRECV_H_

#include "GsmCommandParser.h"

namespace Gsm {

class CommandRecv : public StreamParser::Customer {
public:
	CommandRecv(StreamParser *parser, TimerEngine *timerEngine, StringBuilder *data, Command::Observer *observer);
	~CommandRecv();
	void start(Command *command);

	virtual void procLine(const char *line, uint16_t lineLen);
	virtual bool procData(uint8_t);
	virtual void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Delay,
		State_RecvInfo,
		State_RecvData,
		State_Result
	};

	StreamParser *parser;
	TimerEngine *timerEngine;
	Timer *timer;
	StringBuilder *data;
	Command::Observer *observer;
	Command *command;
	State state;
	uint8_t *recvBuf;
	uint16_t recvBufSize;
	uint16_t recvLen;

	void stateDelayTimeout();
	void stateRecvInfoLine(const char *line, uint16_t lineLen);
	void stateResultLine(const char *line, uint16_t lineLen);

	void deliverResult(Command::Result result, const char *line);
	void deliverEvent(const char *line);
	void procErrorTimeout();
};

}
#endif
