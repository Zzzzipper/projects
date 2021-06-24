#ifndef COMMON_GSM_COMMANDDATAANDRESULT_H_
#define COMMON_GSM_COMMANDDATAANDRESULT_H_

#include "GsmCommandParser.h"

namespace Gsm {

class CommandDataAndResult : public StreamParser::Customer {
public:
	CommandDataAndResult(StreamParser *parser, TimerEngine *timerEngine, StringBuilder *data, Command::Observer *observer);
	~CommandDataAndResult();
	void start(Command *command);

	virtual void procLine(const char *line, uint16_t lineLen);
	virtual bool procData(uint8_t);
	virtual void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Delay,
		State_GsnData,
		State_CgmrData,
		State_CcidData,
		State_CsqData,
		State_CopsData,
		State_CregData,
		State_CgattData,
		State_CifsrData,
		State_CipPing,
		State_CipClose,
		State_Result
	};

	StreamParser *parser;
	TimerEngine *timerEngine;
	Timer *timer;
	StringBuilder *data;
	Command::Observer *observer;
	Command *command;
	State state;
	bool cipPing;

	void stateDelayTimeout();
	void stateEchoLine(const char *line, uint16_t lineLen);
	void stateDataLine(const char *line, uint16_t lineLen);
	void stateGsnDataLine(const char *line, uint16_t lineLen);
	void stateCgmrDataLine(const char *line, uint16_t lineLen);
	void stateCcidDataLine(const char *line, uint16_t lineLen);
	void stateCsqDataLine(const char *line, uint16_t lineLen);
	void stateCopsDataLine(const char *line, uint16_t lineLen);
	void stateCregDataLine(const char *line, uint16_t lineLen);
	void stateCgattDataLine(const char *line, uint16_t lineLen);
	void stateCifsrDataLine(const char *line, uint16_t lineLen);
	void gotoStateCipPing();
	void stateCipPingLine(const char *line, uint16_t lineLen);
	void stateCipCloseLine(const char *line, uint16_t lineLen);
	void stateResultLine(const char *line, uint16_t lineLen);

	void deliverResult(Command::Result result, const char *line);
	void deliverEvent(const char *line);
	void procErrorTimeout();
};

}
#endif
