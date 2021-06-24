#include "GsmCommandQueue.h"

namespace Gsm {

CommandQueue::CommandQueue(CommandParser *receiver) : receiver(receiver) {
	this->commands = new Fifo<Command*>(10);
}

}
