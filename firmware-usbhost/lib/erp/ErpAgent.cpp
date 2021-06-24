#include "ErpAgent.h"

#include "lib/modem/ModemLed.h"
#include "lib/utils/stm32/Beeper.h"

#include "common/logger/include/Logger.h"

#define HTTP_MAX_TRY_NUMBER 3

ErpAgent::ErpAgent(
	ConfigMaster *config,
	TimerEngine *timerEngine,
	EventEngineInterface *eventEngine,
	Network *network,
	Gsm::SignalQuality *signalQuality,
	EcpInterface *ecp,
	VerificationInterface *verification,
	RealTimeInterface *realtime,
	RelayInterface *relay,
	PowerInterface *power,
	ModemLed *leds
) {
	this->httpClient = new Http::Client(timerEngine, network->getTcpConnection1());
	this->httpTransport = new Http::Transport(httpClient, HTTP_MAX_TRY_NUMBER);
	this->erp = new ErpProtocol;
	this->erp->init(config->getConfig(), this->httpTransport, realtime);
	this->cashless = new ErpCashless(config->getConfig()->getAutomat()->getRemoteCashlessContext(), timerEngine, eventEngine);
	this->gramophone = new Gramophone(Beeper::get(), timerEngine);
	this->core = new ErpAgentCore(config, timerEngine, eventEngine, network, signalQuality, erp, ecp, cashless, verification, gramophone, realtime, relay, power, leds);
}

ErpAgent::~ErpAgent() {
	delete core;
	delete gramophone;
	delete cashless;
	delete erp;
	delete httpTransport;
	delete httpClient;
}

void ErpAgent::reset() {
	core->reset();
}

void ErpAgent::syncEvents() {
	core->syncEvents();
}

void ErpAgent::ping() {
	core->ping();
}

void ErpAgent::sendAudit() {
	core->sendAudit();
}

void ErpAgent::loadConfig() {
	core->loadConfig();
}

void ErpAgent::sendDebug() {
	core->sendDebug();
}

void ErpAgent::powerDown() {
	core->powerDown();
}


void ErpAgent::proc(Event *event) {
	core->proc(event);
}
