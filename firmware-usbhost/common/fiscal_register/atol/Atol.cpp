#include "fiscal_register/atol/include/Atol.h"
#include "AtolCommandLayer.h"
#include "AtolTaskLayer.h"
#include "AtolPacketLayer.h"
#include "logger/include/Logger.h"

namespace Atol {

FiscalRegister::FiscalRegister(
	Fiscal::Context *context,
	const char *ipaddr,
	uint16_t port,
	TcpIp *conn,
	TimerEngine *timers,
	EventEngineInterface *eventEngine,
	LedInterface *leds
) {
	this->packetLayer = new PacketLayer(timers, conn);
	this->taskLayer = new TaskLayer(timers, packetLayer);
	this->commandLayer = new CommandLayer(context, ipaddr, port, timers, taskLayer, eventEngine, leds);
}

FiscalRegister::~FiscalRegister() {
	delete this->commandLayer;
	delete this->taskLayer;
	delete this->packetLayer;
}

EventDeviceId FiscalRegister::getDeviceId() {
	return this->commandLayer->getDeviceId();
}

void FiscalRegister::reset() {
	this->commandLayer->reset();
}

void FiscalRegister::sale(Fiscal::Sale *saleData, uint32_t decimalPoint) {
	this->commandLayer->sale(saleData);
}

void FiscalRegister::getLastSale() {
	this->commandLayer->getLastSale();
}

void FiscalRegister::closeShift() {
	this->commandLayer->closeShift();
}

}
