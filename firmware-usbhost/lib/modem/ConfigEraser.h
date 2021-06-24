#ifndef LIB_MODEM_CONFIGERASER_H_
#define LIB_MODEM_CONFIGERASER_H_

#include "ConfigMaster.h"

#include "common/dex/include/DexDataParser.h"
#include "common/timer/include/TimerEngine.h"
#include "common/utils/include/Event.h"

class ConfigEraser : public Dex::DataParser {
public:
	ConfigEraser(ConfigModem *config, TimerEngine *timers, EventEngineInterface *eventEngine);
	virtual ~ConfigEraser();
	virtual void setObserver(EventObserver *);
	virtual Result start(uint32_t size);
	virtual Result procData(const uint8_t *data, const uint16_t len);
	virtual Result complete();
	virtual void error();

	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Delay,
		State_Reboot,
	};

	ConfigModem *config;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	Timer *timer;
	State state;
	EventCourier courier;
};

#endif
