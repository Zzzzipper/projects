#ifndef LIB_BATTERY_POWERAGENT_H_
#define LIB_BATTERY_POWERAGENT_H_

#include "common/config/include/ConfigModem.h"
#include "common/timer/include/TimerEngine.h"
#include "common/event/include/EventEngine.h"

class PowerInterface {
public:
	virtual ~PowerInterface() {}
	virtual void on() = 0;
	virtual void off() = 0;
};

class PowerAgent : public PowerInterface {
public:
	enum EventType {
		Event_PowerDown = GlobalId_Battery | 0x01,
	};

	PowerAgent(ConfigModem *config, TimerEngine *timers, EventEngineInterface *eventEngine);
	~PowerAgent();
	void reset();
	void start();
	void stop();
	void on() override;
	void off() override;

private:
	ConfigModem *config;
	TimerEngine *timerEngine;
	EventEngineInterface *eventEngine;
	Timer *timer;
	EventDeviceId deviceId;

	void procTimer();
};

#endif
