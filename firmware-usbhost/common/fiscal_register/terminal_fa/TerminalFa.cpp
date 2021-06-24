#include "fiscal_register/terminal_fa/include/TerminalFa.h"
#include "TerminalFaCommandLayer.h"
#include "TerminalFaPacketLayer.h"
#include "logger/include/Logger.h"

namespace TerminalFa {

FiscalRegister::FiscalRegister(AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine) {
	this->packetLayer = new PacketLayer(timers, uart);
	this->commandLayer = new CommandLayer(packetLayer, timers, eventEngine);
}

FiscalRegister::~FiscalRegister() {
	delete this->commandLayer;
	delete this->packetLayer;
}

EventDeviceId FiscalRegister::getDeviceId() {
	return commandLayer->getDeviceId();
}

void FiscalRegister::sale(Fiscal::Sale *saleData, uint32_t decimalPoint) {
	commandLayer->sale(saleData, decimalPoint);
}

void FiscalRegister::getLastSale() {
	commandLayer->getLastSale();
}

void FiscalRegister::closeShift() {
	commandLayer->closeShift();
}

}
