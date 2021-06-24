#ifndef COMMON_MDB_MASTER_ENGINE_H
#define COMMON_MDB_MASTER_ENGINE_H

#include "mdb/master/MdbMaster.h"
#include "mdb/master/MdbMasterSender.h"
#include "mdb/master/MdbMasterReceiver.h"
#include "utils/include/List.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"

class MdbMasterEngine : public MdbMasterReceiver::Customer {
public:
	MdbMasterEngine(StatStorage *stat);
	~MdbMasterEngine();
	void init(AbstractUart* uart, TimerEngine* timers);
	void add(MdbMaster *slave);
	MdbMaster *getDevice(Mdb::Device deviceId);
	void reset();
	void procResetTimer();
	void procPollTimer();
	void procRecvTimer();

private:
	enum State {
		State_Idle = 0,
		State_Reset,
		State_Poll,
		State_Recv
	};
	State state;
	AbstractUart *uart;
	MdbMasterSender sender;
	MdbMasterReceiver receiver;
	List<MdbMaster> *devices;
	Timer *timerReset;
	Timer *timerPoll;
	Timer *timerRecv;
	uint16_t current;
	StatNode *timeoutCount;
	StatNode *wrongState1;
	StatNode *wrongState2;
	StatNode *wrongState3;
	StatNode *wrongState4;
	StatNode *otherError;

	void pollSlave();
	void procResponse(const uint8_t *data, uint16_t len, bool crc);

	friend MdbMasterReceiver;
};

#endif
