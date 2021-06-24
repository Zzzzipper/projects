#ifndef LIB_ERP_PROTOCOL_H_
#define LIB_ERP_PROTOCOL_H_

#include "common/config/include/ConfigModem.h"
#include "common/http/include/Http.h"
#include "common/timer/include/RealTime.h"
#include "common/utils/include/Json.h"

#include <stdint.h>

class ErpProtocolInterface {
public:
	enum Command {
		Command_LoadConfig = 1,
		Command_ReloadModem = 2,
		Command_ReloadAutomat = 3,
		Command_ResetErrors = 4,
		Command_ChargeCash = 5,
		Command_LoadAudit = 6,
		Command_UpdateFirmware = 7,
		Command_SverkaItogov = 8,
		Command_ResetAuditCounts = 9,
		Command_LoadStat = 10,
	};

	class EventSync : public Event {
	public:
		EventSync(uint16_t type, DateTime &dt, uint8_t update, uint8_t command) : Event(type), dt(dt), update(update), command(command) {}
		DateTime dt;
		uint8_t update;
		uint8_t command;
	};

	virtual ~ErpProtocolInterface() {}
	virtual void setObserver(EventObserver *observer) = 0;
	virtual bool sendAuditRequest(const char *login, bool auditType, StringBuilder *reqData) = 0;
	virtual bool sendConfigRequest(const char *login, StringBuilder *respBuf) = 0;
	virtual bool sendSyncRequest(const char *login, uint32_t decimalPoint, uint16_t signalQuality, ConfigEventList *events, StringBuilder *reqData) = 0;
	virtual bool sendPingRequest(const char *login, uint32_t decimalPoint, StringBuilder *reqData) = 0;
};

class ErpProtocol : public ErpProtocolInterface, public EventObserver {
public:
	ErpProtocol();
	virtual ~ErpProtocol();
	void init(ConfigModem *config, Http::ClientInterface *http, RealTimeInterface *realtime);
	virtual void setObserver(EventObserver *observer);
	virtual bool sendAuditRequest(const char *login, bool auditType, StringBuilder *reqData);
	virtual bool sendConfigRequest(const char *login, StringBuilder *respBuf);
	virtual bool sendSyncRequest(const char *login, uint32_t decimalPoint, uint16_t signalQuality, ConfigEventList *events, StringBuilder *reqData);
	virtual bool sendPingRequest(const char *login, uint32_t decimalPoint, StringBuilder *reqData);
	void proc(Event *event);

private:
	enum State {
		State_Idle = 0,
		State_Audit,
		State_Event,
		State_Ping,
		State_Config
	};

	ConfigModem *config;
	Http::ClientInterface *http;
	RealTimeInterface *realtime;
	State state;
	Http::Request req;
	Http::Response resp;
	StringBuilder *reqPath;
	StringBuilder *sessionId;
	StringBuilder *respData;
	EventCourier courier;
	JsonParser jsonParser;
	ConfigEventList *events;
	uint16_t syncIndex;

	void stateAuditEvent(Event *event);
	void stateAuditEventRequestComplete();
	void stateEventEvent(Event *event);
	void stateEventEventRequestComplete();
	void statePingEvent(Event *event);
	void statePingEventRequestComplete();
	void stateConfigEvent(Event *event);

	bool checkResponseSuccess(JsonNode *nodeRoot);
	void procError();
};

#endif
