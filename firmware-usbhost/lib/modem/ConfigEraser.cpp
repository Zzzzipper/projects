#include "ConfigEraser.h"

#include "common/logger/include/Logger.h"

ConfigEraser::ConfigEraser(ConfigModem *config, TimerEngine *timers, EventEngineInterface *eventEngine) :
	config(config),
	timers(timers),
	eventEngine(eventEngine),
	state(State_Idle)
{
	this->timer = timers->addTimer<ConfigEraser, &ConfigEraser::procTimer>(this);
}

ConfigEraser::~ConfigEraser() {
	timers->deleteTimer(timer);
}

void ConfigEraser::setObserver(EventObserver *observer) {
	courier.setRecipient(observer);
}

Dex::DataParser::Result ConfigEraser::start(uint32_t size) {
	timer->start(1);
	state = State_Delay;
	return Dex::DataParser::Result_Ok;
}

Dex::DataParser::Result ConfigEraser::procData(const uint8_t *data, const uint16_t len) {
	return Dex::DataParser::Result_Error;
}

Dex::DataParser::Result ConfigEraser::complete() {
	return Dex::DataParser::Result_Error;
}

void ConfigEraser::error() {

}

void ConfigEraser::procTimer() {
	LOG_INFO(LOG_CFG, "procTimer");
	switch(state) {
	case State_Delay: {
		config->getAutomat()->shutdown();
		config->init();
		config->getEvents()->add(ConfigEvent::Type_ConfigEdited);
		timer->start(2000);
		state = State_Reboot;
		return;
	}
	case State_Reboot: {
		EventInterface event(SystemEvent_Reboot);
		eventEngine->transmit(&event);
		return;
	}
	default: {
		LOG_ERROR(LOG_SM, "Unwaited timeout " << state);
		return;
	}
	}
}
