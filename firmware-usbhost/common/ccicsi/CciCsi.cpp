#include "include/CciCsi.h"
#include "CciCsiPacketLayer.h"
#include "CciCsiCommandLayer.h"

namespace CciCsi {

Cashless::Cashless(AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine) {
	packetLayer = new CciCsi::PacketLayer(timers, uart);
	commandLayer = new CommandLayer(packetLayer, timers, eventEngine);
}

Cashless::~Cashless() {
	delete this->commandLayer;
	delete this->packetLayer;
}

void Cashless::reset() {
	commandLayer->reset();
}

bool Cashless::isInited() {
	return commandLayer->isInited();
}

bool Cashless::isEnable() {
	return commandLayer->isEnable();
}

void Cashless::setCredit(uint32_t credit) {
	commandLayer->setCredit(credit);
}

void Cashless::approveVend(uint32_t productPrice) {
	commandLayer->approveVend(productPrice);
}

void Cashless::denyVend(bool close) {
	commandLayer->denyVend();
}

void Cashless::cancelVend() {
	commandLayer->cancelVend();
}

}
