#ifndef COMMON_DDCMP_H
#define COMMON_DDCMP_H

#include "timer/include/TimerEngine.h"
#include "dex/include/DexDataParser.h"
#include "dex/include/DexDataGenerator.h"
#include "uart/include/interface.h"

namespace Ddcmp {

class CommandLayer;
class PacketLayer;

class Ddcmp {
public:
	Ddcmp(AbstractUart *uart, TimerEngine *timers);
	virtual ~Ddcmp();
	void recvAudit(Dex::DataParser *dataParser, Dex::CommandResult *commandResult);

private:
	PacketLayer *packetLayer;
	CommandLayer *commandLayer;
};

}

#endif
