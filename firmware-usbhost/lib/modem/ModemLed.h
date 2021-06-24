#ifndef LIB_MODEM_LED_H
#define LIB_MODEM_LED_H

#include "common/timer/include/TimerEngine.h"
#include "utils/include/LedInterface.h"

class Led;

class ModemLedEntry {
public:
	void setState(LedInterface::State state);
	LedInterface::State getState(uint8_t count);
	void reset();
	void tick(uint8_t count);
private:
	LedInterface::State state;
};

class ModemLed : public LedInterface {
public:
	ModemLed(Led *leds, TimerEngine *timerEngine);
	~ModemLed();
	void reset();
	virtual void setInternet(State state);
	virtual void setPayment(State state);
	virtual void setFiscal(State state);
	virtual void setServer(State state);

private:
	Led *leds;
	TimerEngine *timerEngine;
	Timer *timer;
	ModemLedEntry entries[4];
	uint8_t count;

	void procTimer();
};

#endif
