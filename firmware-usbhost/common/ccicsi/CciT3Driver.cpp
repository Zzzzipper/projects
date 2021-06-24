#include <ccicsi/CciT3CommandLayer.h>
#include <ccicsi/CciT3Driver.h>
#include "CciCsiPacketLayer.h"

namespace Cci {
namespace T3 {

Driver::Driver(AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine) {
	packetLayer = new CciCsi::PacketLayer(timers, uart);
	commandLayer = new CommandLayer(packetLayer, timers, eventEngine);
}

Driver::~Driver() {
	delete this->commandLayer;
	delete this->packetLayer;
}

EventDeviceId Driver::getDeviceId() {
	return commandLayer->getDeviceId();
}

void Driver::setOrder(Order *order) {
	commandLayer->setOrder(order);
}

void Driver::reset() {
	commandLayer->reset();
}

void Driver::disable() {
	commandLayer->disable();
}

void Driver::enable() {
	commandLayer->enable();
}

void Driver::approveVend() {
	commandLayer->approveVend();
}

void Driver::requestPinCode() {

}

void Driver::denyVend() {
	commandLayer->denyVend();
}

}
}
