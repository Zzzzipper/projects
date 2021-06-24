#include "EcpAgent.h"

#include "common/config/include/ConfigEvadts.h"
#include "common/timer/include/TimerEngine.h"
#include "common/dex/include/DexServer.h"
#include "common/utils/include/Utils.h"
#include "common/logger/include/Logger.h"

#include "lib/screen/EcpScreenUpdater.h"

#define ECP_CONNECTING_TIMEOUT 5000

EcpAgent::AuditReceiver::AuditReceiver(StringBuilder *buf) : buf(buf) {
}

void EcpAgent::AuditReceiver::setObserver(EventObserver *observer) {
	courier.setRecipient(observer);
}

Dex::DataParser::Result EcpAgent::AuditReceiver::start(uint32_t dataSize) {
	LOG_INFO(LOG_MODEM, "start " << dataSize);
	buf->clear();
	return Result_Ok;
}

Dex::DataParser::Result EcpAgent::AuditReceiver::procData(const uint8_t *data, const uint16_t len) {
	LOG_INFO(LOG_MODEM, "procData " << len << "/" << buf->getLen() << "/" << buf->getSize());
	LOG_DEBUG_STR(LOG_MODEM, data, len);
	buf->addStr((const char*)data, len);
	return Result_Ok;
}

Dex::DataParser::Result EcpAgent::AuditReceiver::complete() {
	LOG_INFO(LOG_MODEM, "complete " << buf->getLen());
	courier.deliver(Event_AuditComplete);
	return Result_Ok;
}

void EcpAgent::AuditReceiver::error() {
	LOG_ERROR(LOG_MODEM, "error");
	courier.deliver(Event_AuditError);
}

void EcpAgent::Observer::setObserver(EventObserver *observer) {
	courier.setRecipient(observer);
}

void EcpAgent::Observer::success() {
	courier.deliver(Dex::DataParser::Event_AsyncOk);
}

void EcpAgent::Observer::error() {
	courier.deliver(Dex::DataParser::Event_AsyncError);
}

EcpAgent::EcpAgent(
	ConfigMaster *config,
	TimerEngine *timers,
	EventEngineInterface *eventEngine,
	AbstractUart *uart,
	RealTimeInterface *realtime,
	Screen *screen
) :
	config(config),
	timers(timers),
	state(State_Dex)
{
	this->timer = timers->addTimer<EcpAgent, &EcpAgent::procTimer>(this);
	this->screenUpdater = new EcpScreenUpdater(screen);
	this->configReceiver = new EcpConfigParser(config, timers, eventEngine);
	this->configGenerator = new ConfigConfigGenerator(config->getConfig());
	this->configEraser = new ConfigEraser(config->getConfig(), timers, eventEngine);
	this->eventTable = new EcpEventTable(config->getConfig(), realtime);
#ifdef AUDIT_LOAD
	this->auditLoader = new AuditReceiver(config->getBuffer());
#else
	this->auditFiller = new EvadtsParserAdapter(new ConfigAuditFiller(config->getConfig()->getAutomat()));
#endif
	this->auditGenerator = new ConfigAuditGeneratorVendmax(config->getConfig());
	this->dex = new Dex::Server(this);
	this->dex->init(uart, timers, auditGenerator, NULL);
	this->ecp = new Ecp::Server(uart, timers);
	this->ecp->setObserver(this);
	this->ecp->setScreenParser(screenUpdater);
	this->ecp->setConfigParser(configReceiver);
	this->ecp->setConfigGenerator(configGenerator);
	this->ecp->setConfigEraser(configEraser);
	this->ecp->setTableProcessor(eventTable);
}

EcpAgent::~EcpAgent() {
	delete ecp;
	delete dex;
	delete eventTable;
	delete configEraser;
	delete auditGenerator;
#ifdef AUDIT_LOAD
	delete auditLoader;
#else
	delete auditFiller;
#endif
	delete configGenerator;
	delete configReceiver;
	timers->deleteTimer(timer);
}

void EcpAgent::proc(Event *event) {
	LOG_DEBUG(LOG_MODEM, "proc");
	switch(state) {
	case State_Dex: stateDexEvent(event); break;
//	case State_AuditLoad: stateAuditLoadEvent(event); break;
	case State_EcpConnecting: stateEcpConnectingEvent(event); break;
	case State_Ecp: stateEcpEvent(event); break;
	default: LOG_ERROR(LOG_MODEM, "Unwaited event " << state << "," << event->getType());
	}
}

void EcpAgent::procTimer() {
	LOG_DEBUG(LOG_MODEM, "procTimer");
	dex->reset();
	state = State_Dex;
}

bool EcpAgent::loadAudit(EventObserver *observer) {
	if(state != State_Dex || dex->getState() != Dex::Server::State_Wait) {
		LOG_ERROR(LOG_MODEM, "Wrong DEX server state.");
		return false;
	}
	this->observer.setObserver(observer);
#ifdef AUDIT_LOAD
	auditLoader->setObserver(observer);
	dex->recvData(auditLoader, &this->observer);
#else
	auditFiller->setObserver(observer);
	dex->recvData(auditFiller, &this->observer);
#endif
//	state = State_AuditLoad;
	return true;
}

void EcpAgent::stateDexEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "procTimer");
	switch(event->getType()) {
	case Dex::Server::Event_ManufacturerMode: gotoStateEcp(); return;
	default: LOG_ERROR(LOG_MODEM, "Unwaited event " << event->getType() << "," << state);
	}
}

void EcpAgent::gotoStateEcp() {
	LOG_INFO(LOG_MODEM, "gotoStateEcp");
	timer->start(ECP_CONNECTING_TIMEOUT);
	ecp->reset();
	state = State_EcpConnecting;
}

void EcpAgent::stateEcpConnectingEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "stateEcpConnectingEvent");
	switch(event->getType()) {
	case Ecp::Server::Event_Connect: {
		timer->stop();
		state = State_Ecp;
		return;
	}
	default: LOG_ERROR(LOG_MODEM, "Unwaited event " << event->getType() << "," << state);
	}
}

void EcpAgent::stateEcpEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "stateEcpEvent");
	switch(event->getType()) {
	case Ecp::Server::Event_Disconnect: {
		timer->stop();
		dex->reset();
		state = State_Dex;
		return;
	}
	default: LOG_ERROR(LOG_MODEM, "Unwaited event " << event->getType() << "," << state);
	}
}
