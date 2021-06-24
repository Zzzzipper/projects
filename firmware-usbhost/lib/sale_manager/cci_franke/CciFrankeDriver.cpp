#include <lib/sale_manager/cci_franke/CciFrankeT3Driver.h>
#include "CciFrankeCommandLayer.h"

#include "common/ccicsi/CciCsiPacketLayer.h"

namespace Cci {
namespace Franke {

Cashless::Cashless(AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine) {
	packetLayer = new CciCsi::PacketLayer(timers, uart);
	commandLayer = new CommandLayer(packetLayer, timers, eventEngine);
}

Cashless::~Cashless() {
	delete this->commandLayer;
	delete this->packetLayer;
}

#if 1
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
	commandLayer->denyVend(close);
}

void Cashless::cancelVend() {
	commandLayer->cancelVend();
}
#else
void Cashless::reset() {
	commandLayer->reset();
}

void Cashless::disableProducts() {
	commandLayer->disableProducts();
}

void Cashless::enableProducts() {
	commandLayer->enableProducts();
}

void Cashless::approveVend(uint16_t productId) {
	commandLayer->approveVend(productId);
}

void Cashless::denyVend() {
	commandLayer->denyVend();
}
#endif

}
}
