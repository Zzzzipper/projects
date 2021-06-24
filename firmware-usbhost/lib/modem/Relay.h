#ifndef LIB_MODEM_RELAY_H_
#define LIB_MODEM_RELAY_H_

#include "lib/battery/PowerAgent.h"

#include "common/timer/include/TimerEngine.h"

class RelayInterface {
public:
	virtual ~RelayInterface() {}
	virtual void on() = 0;
};

class Relay : public RelayInterface {
public:
	Relay(PowerAgent *powerAgent, TimerEngine *timerEngine);
	~Relay() override;
	void on();

private:
	PowerAgent *powerAgent;
	TimerEngine *timerEngine;
	Timer *timer;

	void procTimer();
};

#endif
