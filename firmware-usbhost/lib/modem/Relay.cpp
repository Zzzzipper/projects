#include "Relay.h"

#include "defines.h"

Relay::Relay(PowerAgent *powerAgent, TimerEngine *timerEngine) :
	powerAgent(powerAgent),
	timerEngine(timerEngine)
{
	timer = timerEngine->addTimer<Relay, &Relay::procTimer>(this);
}

Relay::~Relay() {
	timerEngine->deleteTimer(timer);
}

void Relay::on() {
	RELE1_ON
	powerAgent->stop();
	timer->start(7000);
}

void Relay::procTimer() {
	RELE1_OFF
	powerAgent->start();
}
