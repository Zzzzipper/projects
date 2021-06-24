#include "PowerAgent.h"
#include "Battery.h"

#include "lib/adc/Adc.h"

#include "common/logger/include/Logger.h"

#define POWER_POLL_TIMEOUT 5000
#define POWER_MINIMUM_VALUE 10000

PowerAgent::PowerAgent(ConfigModem *config, TimerEngine *timerEngine, EventEngineInterface *eventEngine) :
	config(config),
	timerEngine(timerEngine),
	eventEngine(eventEngine),
	deviceId(eventEngine)
{
	timer = timerEngine->addTimer<PowerAgent, &PowerAgent::procTimer>(this);
}

PowerAgent::~PowerAgent() {
	timerEngine->deleteTimer(timer);
}

void PowerAgent::reset() {
	Battery::init();
	Battery::enable();
	timer->start(POWER_POLL_TIMEOUT);
	LOG_INFO(LOG_MODEM, "Battery OK");
}

void PowerAgent::start() {
	LOG_DEBUG(LOG_MODEM, "start");
	timer->start(POWER_POLL_TIMEOUT);
}

void PowerAgent::stop() {
	LOG_DEBUG(LOG_MODEM, "stop");
	timer->stop();
}

void PowerAgent::on() {
	Battery::enable();
}

void PowerAgent::off() {
	Battery::disable();
}

void PowerAgent::procTimer() {
	LOG_TRACE(LOG_MODEM, "procTimer");
	if(Adc::get()->getInputVoltage() > POWER_MINIMUM_VALUE) {
		timer->start(POWER_POLL_TIMEOUT);
		return;
	}

	LOG_INFO(LOG_MODEM, "Power down");
	config->getAutomat()->getSMContext()->getErrors()->add(ConfigEvent::Type_PowerDown, "");
	config->getEvents()->add(ConfigEvent::Type_PowerDown);
	EventInterface event(deviceId, Event_PowerDown);
	eventEngine->transmit(&event);
}
