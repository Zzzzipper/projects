#include "include/ChekOnline.h"
#include "ChekOnlineCommandLayer.h"

#include "lib/fiscal_register/orange_data/WolfSslConnection.h"

#include "common/logger/include/Logger.h"

#include <string.h>

namespace ChekOnline {

FiscalRegister::FiscalRegister(
	ConfigModem *config,
	Fiscal::Context *context,
	TcpIp *conn,
	TimerEngine *timers,
	EventEngineInterface *eventEngine,
	RealTimeInterface *realtime,
	LedInterface *leds
) {
	this->sslAdapter = new WolfSslAdapter(conn);
	this->sslConn = new WolfSslConnection(sslAdapter);
	this->sslConn->init(config->getFiscal()->getAuthPublicKey(), config->getFiscal()->getAuthPrivateKey());
	this->commandLayer = new CommandLayer(config, context, sslConn, timers, eventEngine, realtime, leds);
}

FiscalRegister::~FiscalRegister() {
	delete this->commandLayer;
	delete this->sslConn;
	delete this->sslAdapter;
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
