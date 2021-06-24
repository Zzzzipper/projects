#include "ModemLed.h"

#include "common/utils/stm32/include/Led.h"
#include "common/logger/include/Logger.h"

#define STATE_MAX 7
#define COUNT_MAX 8

uint8_t blink[STATE_MAX][COUNT_MAX] = {
	{ 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 1, 0, 1, 0, 1, 0, 1, 0 },
	{ 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 0, 0, 0, 0, 0, 0, 0 },
	{ 1, 0, 1, 0, 0, 0, 0, 0 },
	{ 1, 0, 1, 0, 1, 0, 0, 0 },
};

void ModemLedEntry::setState(LedInterface::State state) {
	this->state = state;
}

LedInterface::State ModemLedEntry::getState(uint8_t count) {
	return blink[state][count] > 0 ? state : LedInterface::State_Off;
}

void ModemLedEntry::reset() {
	state = LedInterface::State_Off;
}

ModemLed::ModemLed(Led *leds, TimerEngine *timerEngine) :
	leds(leds),
	timerEngine(timerEngine),
	count(0)
{
	this->timer = timerEngine->addTimer<ModemLed, &ModemLed::procTimer>(this, TimerEngine::ProcInTick);
}

ModemLed::~ModemLed() {
	timerEngine->deleteTimer(timer);
}

void ModemLed::reset() {
	setServer(LedInterface::State_Off);
	setPayment(LedInterface::State_Off);
	setFiscal(LedInterface::State_Off);
	setInternet(LedInterface::State_Off);
	timer->start(500);
}

void ModemLed::setServer(State state) {
	entries[3].setState(state);
}

void ModemLed::setFiscal(State state) {
	entries[2].setState(state);
}

void ModemLed::setPayment(State state) {
	entries[1].setState(state);
}

void ModemLed::setInternet(State state) {
	entries[0].setState(state);
}

void ModemLed::procTimer() {
	switch(entries[3].getState(count)) {
	case State_Off: leds->setLed1(0, 0); break;
	case State_InProgress: leds->setLed1(0, 1); break;
	case State_Success: leds->setLed1(0, 1); break;
	case State_Failure1:
	case State_Failure2:
	case State_Failure3:
	case State_Failure: leds->setLed1(1, 0); break;
	default:;
	}
	switch(entries[2].getState(count)) {
	case State_Off: leds->setLed2(0, 0); break;
	case State_InProgress: leds->setLed2(0, 1); break;
	case State_Success: leds->setLed2(0, 1); break;
	case State_Failure1:
	case State_Failure2:
	case State_Failure3:
	case State_Failure: leds->setLed2(1, 0); break;
	default:;
	}
	switch(entries[1].getState(count)) {
	case State_Off: leds->setLed3(0, 0); break;
	case State_InProgress: leds->setLed3(0, 1); break;
	case State_Success: leds->setLed3(0, 1); break;
	case State_Failure1:
	case State_Failure2:
	case State_Failure3:
	case State_Failure: leds->setLed3(1, 0); break;
	default:;
	}
	switch(entries[0].getState(count)) {
	case State_Off: leds->setLed4(0, 0); break;
	case State_InProgress: leds->setLed4(0, 1); break;
	case State_Success: leds->setLed4(0, 1); break;
	case State_Failure1:
	case State_Failure2:
	case State_Failure3:
	case State_Failure: leds->setLed4(1, 0); break;
	default:;
	}

	count++;
	if(count >= COUNT_MAX) { count = 0; }
	timer->start(500);
}
