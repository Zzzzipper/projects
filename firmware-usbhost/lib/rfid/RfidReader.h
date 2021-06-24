#ifndef LIB_RFID_READER_H
#define LIB_RFID_READER_H

#include "RfidInterface.h"

#include "event/include/EventEngine.h"
#include "utils/include/Event.h"
#include "timer/include/TimerEngine.h"
#include "mdb/MdbProtocol.h"

class RFID;

class RfidReader {
public:
	RfidReader(Mdb::DeviceContext *context, TimerEngine *timerEngine, EventEngineInterface *eventEngine);
	void reset();

private:
	enum State {
		State_Idle = 0,
		State_Init,
		State_Poll,
	};

	Mdb::DeviceContext *context;
	TimerEngine *timerEngine;
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	Timer *timer;
	RFID *rfid;
	State state;
	uint16_t initCount;

	void procTimer();

	void gotoStateInit();
	void stateInitTimeout();

	void gotoStatePoll();
	void statePollTimeout();
};

#endif
