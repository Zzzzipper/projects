#include "Ddcmp.h"
#include "DdcmpCommandLayer.h"
#include "DdcmpPacketLayer.h"
#include "logger/include/Logger.h"

namespace Ddcmp {

Ddcmp::Ddcmp(AbstractUart *uart, TimerEngine *timers) {
	this->packetLayer = new PacketLayer(timers, uart);
	this->commandLayer = new CommandLayer(packetLayer, timers);
}

Ddcmp::~Ddcmp() {
	delete this->commandLayer;
	delete this->packetLayer;
}

void Ddcmp::recvAudit(Dex::DataParser *dataParser, Dex::CommandResult *commandResult) {
	commandLayer->recvAudit(dataParser, commandResult);
}

}
