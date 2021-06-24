#ifndef LIB_MODEM_CONFIGUPDATER_H_
#define LIB_MODEM_CONFIGUPDATER_H_

#include "ConfigMaster.h"

#include "common/dex/include/DexDataParser.h"
#include "common/timer/include/Timer.h"
#include "common/timer/include/TimerEngine.h"
#include "common/utils/include/Buffer.h"
#include "common/config/include/ConfigEvadts.h"
#include "common/evadts/EvadtsChecker.h"
#include "common/utils/include/Event.h"

class ConfigUpdater {
public:
	ConfigUpdater(TimerEngine *timers, ConfigModem *config, EventObserver *observer = NULL);
	~ConfigUpdater();
	bool checkCrc(StringBuilder *data);
	void resize(StringBuilder *data, bool fixedDecimalPoint);
	void update(StringBuilder *data, bool fixedDecimalPoint);
	Dex::DataParser::Result getResult();
	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Init,
		State_InitLast,
		State_Compare,
		State_Resize,
		State_Update,
		State_UpdateLast,
	};
	ConfigModem *config;
	TimerEngine *timers;
	Timer *timer;
	EventCourier courier;
	State state;
	bool fixedDecimalPoint;
	StringBuilder *data;
	uint16_t offset;
	uint16_t stepSize;
	Evadts::Checker checker;
	ConfigConfigIniter initer;
	ConfigConfigParser parser;
	Dex::DataParser::Result result;

	void gotoStateInit();
	void stateInitTimeout();
	void stateInitLastTimeout();
	void gotoStateCompare();
	void stateCompareTimeout();
	void gotoStateResize();
	void stateResizeTimeout();
	void stateResizeLastTimeout();
	void gotoStateUpdate();
	void stateUpdateTimeout();
	void stateUpdateLastTimeout();
};

#endif
