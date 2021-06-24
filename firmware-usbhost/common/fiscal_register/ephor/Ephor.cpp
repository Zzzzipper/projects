#include "include/Ephor.h"
#include "EphorCommandLayer.h"

#include "common/logger/include/Logger.h"

#include <string.h>

namespace Ephor {

FiscalRegister::FiscalRegister(
	ConfigModem *config,
	Fiscal::Context *context,
	TcpIp *conn,
	TimerEngine *timers,
	EventEngineInterface *eventEngine,
	RealTimeInterface *realtime,
	LedInterface *leds
) {
	this->commandLayer = new CommandLayer(config, context, conn, timers, eventEngine, realtime, leds);
}

FiscalRegister::~FiscalRegister() {
	delete this->commandLayer;
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
