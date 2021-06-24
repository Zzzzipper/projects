#ifndef LIB_ECP_CONFIGPARSER_H_
#define LIB_ECP_CONFIGPARSER_H_

#include "lib/modem/ConfigMaster.h"
#include "lib/modem/ConfigUpdater.h"

#include "common/dex/include/DexDataParser.h"
#include "common/utils/include/Event.h"
#include "common/utils/include/StringBuilder.h"

class EcpConfigParser : public Dex::DataParser, public EventObserver {
public:
	EcpConfigParser(ConfigMaster *config, TimerEngine *timers, EventEngineInterface *eventEngine);
	virtual ~EcpConfigParser();
	virtual Result start(uint32_t dataSize);
	virtual Result procData(const uint8_t *data, const uint16_t dataLen);
	virtual Result complete();
	virtual void error();

	virtual void proc(Event *event);
	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Upload,
		State_Update,
		State_Updated
	};

	ConfigMaster *config;
	TimerEngine *timerEngine;
	EventEngineInterface *eventEngine;
	Timer *timer;
	StringBuilder *buf;
	ConfigUpdater *updater;
	State state;

	Result stateUploadProcData(const uint8_t *data, const uint16_t len);
	Result stateUploadComplete();

	Result stateUpdateComplete();
	void stateUpdateEvent(Event *event);
	void stateUpdateEventAsyncOk();
	void stateUpdateEventAsyncError();

	Result stateUpdatedComplete();
	void stateUpdatedTimeout();
};

#endif
