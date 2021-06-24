#include "EcpConfigParser.h"

#include "common/logger/include/Logger.h"

#define REBOOT_DELAY 500

EcpConfigParser::EcpConfigParser(ConfigMaster *config, TimerEngine *timerEngine, EventEngineInterface *eventEngine) :
	config(config),
	timerEngine(timerEngine),
	eventEngine(eventEngine),
	state(State_Idle)
{
	timer = timerEngine->addTimer<EcpConfigParser, &EcpConfigParser::procTimer>(this);
	updater = new ConfigUpdater(timerEngine, config->getConfig(), this);
}

EcpConfigParser::~EcpConfigParser() {
	delete updater;
	timerEngine->deleteTimer(timer);
}

Dex::DataParser::Result EcpConfigParser::start(uint32_t dataSize) {
	LOG_INFO(LOG_MODEM, "start " << dataSize);
	if(config->lock() == false) {
		LOG_ERROR(LOG_MODEM, "Config locked");
		return Result_Error;
	}
	if(state != State_Idle && state != State_Updated) {
		LOG_ERROR(LOG_MODEM, "Parser busy");
		return Result_Busy;
	}
	buf = config->getBuffer();
	buf->clear();
	state = State_Upload;
	return Result_Ok;
}

Dex::DataParser::Result EcpConfigParser::procData(const uint8_t *data, const uint16_t len) {
	LOG_INFO(LOG_MODEM, "procData " << len << "/" << buf->getLen() << "/" << buf->getSize());
	LOG_DEBUG_STR(LOG_MODEM, data, len);
	switch(state) {
	case State_Upload: return stateUploadProcData(data, len);
	default: return Result_Error;
	}
}

Dex::DataParser::Result EcpConfigParser::complete() {
	LOG_INFO(LOG_MODEM, "complete " << buf->getLen());
	switch(state) {
	case State_Upload: return stateUploadComplete();
	case State_Update: return stateUpdateComplete();
	case State_Updated: return stateUpdatedComplete();
	default: return Result_Error;
	}
}

void EcpConfigParser::error() {
	LOG_ERROR(LOG_MODEM, "error");
	config->unlock();
}

void EcpConfigParser::proc(Event *event) {
	switch(state) {
	case State_Update: stateUpdateEvent(event); return;
	default: LOG_ERROR(LOG_MODEM, "Unwaited event " << event->getType() << "," << state);
	}
}

void EcpConfigParser::procTimer() {
	LOG_DEBUG(LOG_MODEM, "procTimer");
	switch(state) {
	case State_Updated: stateUpdatedTimeout(); break;
	default: LOG_DEBUG(LOG_MODEM, "Wrong state " << state);
	}
}

Dex::DataParser::Result EcpConfigParser::stateUploadProcData(const uint8_t *data, const uint16_t len) {
	buf->addStr((const char*)data, len);
	return Result_Ok;
}

Dex::DataParser::Result EcpConfigParser::stateUploadComplete() {
	updater->resize(buf, false);
	state = State_Update;
	return Result_Busy;
}

Dex::DataParser::Result EcpConfigParser::stateUpdateComplete() {
	return Result_Busy;
}

void EcpConfigParser::stateUpdateEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "stateUpdateEvent");
	switch(event->getType()) {
	case Dex::DataParser::Event_AsyncOk: stateUpdateEventAsyncOk(); return;
	case Dex::DataParser::Event_AsyncError: stateUpdateEventAsyncError(); return;
	default: LOG_ERROR(LOG_MODEM, "Unwaited event " << event->getType() << "," << state);
	}
}

void EcpConfigParser::stateUpdateEventAsyncOk() {
	LOG_DEBUG(LOG_MODEM, "stateUpdateEventAsyncOk");
	config->getConfig()->getEvents()->add(ConfigEvent::Type_ConfigEdited);
	config->unlock();
	timer->start(2000);
	state = State_Updated;
}

void EcpConfigParser::stateUpdateEventAsyncError() {
	LOG_DEBUG(LOG_MODEM, "stateUpdateEventAsyncError");
	config->getConfig()->getEvents()->add(ConfigEvent::Type_ConfigParseFailed);
	config->unlock();
	state = State_Updated;
}

Dex::DataParser::Result EcpConfigParser::stateUpdatedComplete() {
	return updater->getResult();
}

void EcpConfigParser::stateUpdatedTimeout() {
	LOG_DEBUG(LOG_MODEM, "stateUpdatedTimeout");
	EventInterface event(SystemEvent_Reboot);
	eventEngine->transmit(&event);
}

