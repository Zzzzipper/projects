#include "include/OrangeData.h"
#include "WolfSslRsaSign.h"
#include "WolfSslConnection.h"
#include "OrangeDataCommandLayer.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace OrangeData {

FiscalRegister::FiscalRegister(
	ConfigModem *config,
	Fiscal::Context *context,
	TcpIp *conn,
	TimerEngine *timers,
	EventEngineInterface *eventEngine,
	RealTimeInterface *realtime,
	LedInterface *leds
) {
	this->sign = new WolfSslRsaSign;
	this->sign->init(config->getFiscal()->getSignPrivateKey());
	this->sslAdapter = new WolfSslAdapter(conn);
	this->sslConn = new WolfSslConnection(sslAdapter);
	this->sslConn->init(config->getFiscal()->getAuthPublicKey(), config->getFiscal()->getAuthPrivateKey());
	this->commandLayer = new CommandLayer(config, context, sign, sslConn, timers, eventEngine, realtime, leds);
}

FiscalRegister::~FiscalRegister() {
	delete this->commandLayer;
	delete this->sslConn;
	delete this->sslAdapter;
	delete this->sign;
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
