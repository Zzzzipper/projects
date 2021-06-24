#ifndef LIB_ERP_AGENT_H_
#define LIB_ERP_AGENT_H_

#include "ErpAgentCore.h"
#include "ErpProtocol.h"

#include "common/http/include/HttpClient.h"
#include "common/http/include/HttpTransport.h"
#include "common/beeper/include/Gramophone.h"

class ModemLed;

class ErpAgent {
public:
	ErpAgent(ConfigMaster *config, TimerEngine *timerEngine, EventEngineInterface *eventEngine, Network *network, Gsm::SignalQuality *signalQuality, EcpInterface *ecp, VerificationInterface *verification, RealTimeInterface *realtime, RelayInterface *relay, PowerInterface *power, ModemLed *leds);
	~ErpAgent();
	MdbMasterCashlessInterface *getCashless() { return cashless; }
	void reset();
	void syncEvents();
	void ping();
	void sendAudit();
	void loadConfig();
	void sendDebug();
	void powerDown();
	void proc(Event *event);

private:
	Http::Client *httpClient;
	Http::Transport *httpTransport;
	ErpProtocol *erp;
	ErpCashless *cashless;
	GramophoneInterface *gramophone;
	ErpAgentCore *core;
};

#endif
