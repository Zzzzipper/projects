#ifndef LIB_ERP_AGENTCORE_H_
#define LIB_ERP_AGENTCORE_H_

#include "ErpProtocol.h"
#include "ErpCashless.h"

#include "lib/modem/ConfigMaster.h"
#include "lib/modem/ConfigUpdater.h"
#include "lib/modem/Relay.h"
#include "lib/network/include/Network.h"
#include "lib/ecp/EcpAgent.h"

#include "common/config/include/ConfigModem.h"
#include "common/config/include/ConfigEvadts.h"
#include "common/beeper/include/Gramophone.h"
#include "common/timer/include/Timer.h"
#include "common/timer/include/TimerEngine.h"
#include "common/event/include/EventEngine.h"
#include "common/timer/include/RealTime.h"
#include "common/utils/include/Event.h"
#include "common/utils/include/LedInterface.h"
#include "common/sim900/include/GsmSignalQuality.h"

class ErpAgentCore : public EventObserver, public EventSubscriber {
public:
	ErpAgentCore(ConfigMaster *config, TimerEngine *timers, EventEngineInterface *eventEngine, Network *network, Gsm::SignalQualityInterface *signalQuality, ErpProtocolInterface *erp, EcpInterface *ecp, ErpCashless *cashless, VerificationInterface *verification, GramophoneInterface *gramophone, RealTimeInterface *realtime, RelayInterface *relay, PowerInterface *power, LedInterface *leds);
	~ErpAgentCore();
	void reset();
	void syncEvents();
	void ping();
	void powerDown();
	void sendAudit();
	void loadConfig();
	void sendDebug();
	void procTimer();
	void procSyncTimer();
	virtual void proc(Event *event);
	virtual void proc(EventEnvelope *envelope);

private:
	enum State {
		State_Idle = 0,
		State_Wait,
		State_SignalQuality,
		State_Sync,
		State_Ping,
		State_AuditGenerate,
		State_AuditSend,
		State_AuditMelody,
		State_ConfigLoad,
		State_ConfigUpdate,
		State_ConfigMelody,
		State_Reboot,
		State_AuditLoad,
	};

	enum CommandType {
		CommandType_None	 = 0x00,
		CommandType_Sync	 = 0x01,
		CommandType_Audit	 = 0x02,
		CommandType_Debug	 = 0x03,
	};

	ConfigMaster *config;
	TimerEngine *timers;
	EventEngineInterface *eventEngine;
	Network *network;
	Gsm::SignalQualityInterface *signalQuality;
	ErpProtocolInterface *erp;
	EcpInterface *ecp;
	ErpCashless *cashless;
	VerificationInterface *verification;
	GramophoneInterface *gramophone;
	RealTimeInterface *realtime;
	RelayInterface *relay;
	PowerInterface *power;
	LedInterface *leds;
	StatStorage *stat;
	ConfigUpdater *updater;
	Melody *melodyAudit;
	Melody *melodyConfig;
	EventDeviceId deviceId;
	StatNode *state;
	StatNode *syncErrorCount;
	StatNode *syncErrorMax;
	CommandType command;
	Timer *timer;
	Timer *syncTimer;
	uint16_t syncCount;
	uint16_t syncError;
	ConfigAuditGenerator generator;
	uint32_t configId;
	uint16_t signalValue;
	bool poweroff;
	bool auditType;

	bool procRemoteCommand(uint32_t command);
	void procLockError();
	void procServerError();
	void procSyncUnwaitedTimeout();

	void gotoStateWait();
	void stateWaitTimeout();

	void gotoStateSignalQuality();
	void stateSignalQualityEvent(EventEnvelope *envelope);
	void stateSignalQualityEventComplete(EventEnvelope *envelope);
	void gotoStateSync();
	void stateSyncEvent(Event *event);
	void stateSyncEventComplete(Event *event);
	bool updateFirmware(uint8_t update);
	void updateDateTime(DateTime *datetime);
	void stateSyncEventError();

	void gotoStatePing();
	void statePingEvent(Event *event);
	void statePingEventComplete(Event *event);
	void statePingEventError();

	void gotoStateAuditLoad(bool auditType);
	void stateAuditLoadEvent(Event *event);
	void stateAuditLoadEventCompete();
	void stateAuditLoadEventError();

	void gotoStateAuditGenerate();
	void stateAuditGenerateTimeout();

	void gotoStateAuditSend();
	void stateAuditSendEvent(Event *event);
	void stateAuditSendEventComplete(uint32_t configId);

	void gotoStateAuditMelody();
	void stateAuditMelodyEvent(Event *event);

	void gotoSendDebug();

	void gotoStateConfigLoad();
	void stateConfigLoadEvent(Event *event);
	void stateConfigLoadEventComplete();
	void stateConfigLoadEventError();

	void gotoStateConfigUpdate();
	void stateConfigUpdateEvent(Event *event);
	void stateConfigUpdateEventComplete();
	void stateConfigUpdateEventError(Event *event);

	void gotoStateConfigMelody();
	void stateConfigMelodyEvent(Event *event);

	void gotoStateReboot();
};

#endif
