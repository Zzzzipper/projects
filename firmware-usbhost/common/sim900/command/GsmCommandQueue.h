#ifndef COMMON_GSM_COMMANDQUEUE_H
#define COMMON_GSM_COMMANDQUEUE_H

#include "GsmCommand.h"
#include "GsmCommandParser.h"

#include "utils/include/Fifo.h"

namespace Gsm {

class CommandQueue {
public:
	CommandQueue(CommandParser *receiver);
	void push(Command *command);

private:
	CommandParser *receiver;
	Command *currentCommand;
	Fifo<Command*> *commands;
};

}

#endif
