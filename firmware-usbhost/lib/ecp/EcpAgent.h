#ifndef LIB_ECP_AGENT_H_
#define LIB_ECP_AGENT_H_

#include "EcpConfigParser.h"
#include "EcpEventTable.h"

#include "lib/modem/ConfigMaster.h"
#include "lib/modem/ConfigEraser.h"

#include "common/config/include/ConfigModem.h"
#include "common/dex/include/DexDataParser.h"
#include "common/dex/include/DexDataGenerator.h"
#include "common/timer/include/Timer.h"
#include "common/utils/include/Event.h"
#include "common/utils/include/Buffer.h"
#include "common/uart/include/interface.h"
#include "common/evadts/EvadtsParserAdapter.h"
#include "common/config/include/ConfigEvadts.h"
#include "common/ecp/include/EcpServer.h"

class ConfigConfigParserDex;
class TimerEngine;
class StringBuilder;
class Screen;
class EcpScreenUpdater;
namespace Dex { class Server; }

#define AUDIT_LOAD

class EcpInterface {
public:
	enum EventType {
		Event_AuditComplete	 = GlobalId_Dex | 0x01, // ѕосле выхода из обработчика событи€ данные аудита могут быть затерты.
		Event_AuditError	 = GlobalId_Dex | 0x02,
	};
	virtual bool loadAudit(EventObserver *observer) = 0;
};

class EcpAgent : public EcpInterface, public EventObserver {
public:
	EcpAgent(ConfigMaster *config, TimerEngine *timers, EventEngineInterface *eventEngine, AbstractUart *uart, RealTimeInterface *realtime, Screen *screen);
	virtual ~EcpAgent();
	virtual bool loadAudit(EventObserver *observer) override;
	virtual void proc(Event *event);
	void procTimer();

private:
	class AuditReceiver : public Dex::DataParser {
	public:
		AuditReceiver(StringBuilder *buf);
		virtual ~AuditReceiver() {}
		void setObserver(EventObserver *observer);
		virtual Result start(uint32_t dataSize);
		virtual Result procData(const uint8_t *data, const uint16_t dataLen);
		virtual Result complete();
		virtual void error();

	private:
		StringBuilder *buf;
		EventCourier courier;
	};

	class Observer : public Dex::CommandResult {
	public:
		void setObserver(EventObserver *observer);
		virtual void success();
		virtual void error();

	private:
		EventCourier courier;
	};

	enum State {
		State_Dex = 0,
		State_AuditLoad,
		State_EcpConnecting,
		State_Ecp
	};

	ConfigMaster *config;
	TimerEngine *timers;
	Timer *timer;
	State state;
	EcpScreenUpdater *screenUpdater;
	EcpConfigParser *configReceiver;
	ConfigEraser *configEraser;
	ConfigConfigGenerator *configGenerator;
	EcpEventTable *eventTable;
#ifdef AUDIT_LOAD
	AuditReceiver *auditLoader;
#else
	EvadtsParserAdapter *auditFiller;
#endif
	ConfigAuditGeneratorVendmax *auditGenerator;
	Dex::Server *dex;
	Ecp::Server *ecp;
	Observer observer;

	void stateDexEvent(Event *event);
	void gotoStateEcp();
	void stateEcpConnectingEvent(Event *event);
	void stateEcpEvent(Event *event);
};

#endif
