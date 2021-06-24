#include "ErpProtocol.h"
#include "ErpSyncRequest.h"

#include "common/mdb/MdbProtocolBillValidator.h"
#include "common/utils/include/sha1.h"
#include "common/utils/include/Buffer.h"
#include "common/utils/include/Number.h"
#include "common/utils/include/DecimalPoint.h"
#include "common/logger/include/Logger.h"

#include <string.h>

#define REQUEST_PATH_MAX_SIZE 128
#define REQUEST_DATA_MAX_SIZE 256
#define RESPONSE_DATA_MAX_SIZE 256
//#define RESPONSE_DATA_MAX_SIZE 536 // выровнено на LWIP::TCP_MSS
#define PHP_SESSION_ID_MAX_SIZE 64
#define JSON_NODE_MAX 30

ErpProtocol::ErpProtocol() :
	http(NULL),
	state(State_Idle),
	jsonParser(JSON_NODE_MAX)
{
	this->reqPath = new StringBuilder(REQUEST_PATH_MAX_SIZE, REQUEST_PATH_MAX_SIZE);
	this->sessionId = new StringBuilder(PHP_SESSION_ID_MAX_SIZE, PHP_SESSION_ID_MAX_SIZE);
	this->respData = new StringBuilder(RESPONSE_DATA_MAX_SIZE, RESPONSE_DATA_MAX_SIZE);
}

ErpProtocol::~ErpProtocol() {
	delete respData;
	delete sessionId;
	delete reqPath;
}

void ErpProtocol::init(ConfigModem *config, Http::ClientInterface *http, RealTimeInterface *realtime) {
	LOG_DEBUG(LOG_MODEM, "init");
	this->config = config;
	this->realtime = realtime;
	this->http = http;
	this->http->setObserver(this);
	this->req.serverName = config->getBoot()->getServerDomain();
	this->req.serverPort = config->getBoot()->getServerPort();
}

void ErpProtocol::setObserver(EventObserver *observer) {
	this->courier.setRecipient(observer);
}

void ErpProtocol::proc(Event *event) {
	LOG_DEBUG(LOG_MODEM, "proc " << event->getType());
	switch(state) {
	case State_Audit: stateAuditEvent(event); break;
	case State_Config: stateConfigEvent(event); break;
	case State_Event: stateEventEvent(event); break;
	case State_Ping: statePingEvent(event); break;
	default: LOG_ERROR(LOG_MODEM, "Unwaited event " << state);
	}
}

bool ErpProtocol::sendAuditRequest(const char *login, bool auditType, StringBuilder *reqData) {
	LOG_DEBUG(LOG_MODEM, "sendAuditRequest " << reqData->getLen());
	if(http == NULL) {
		LOG_ERROR(LOG_MODEM, "Internet not inited");
		return false;
	}

	reqPath->clear();
	*reqPath << "/api/1.0/Modem.php?action=";
	if(auditType) { *reqPath << "ImportAudit"; } else { *reqPath << "ImportStat"; }
	*reqPath << "&login=" << login << "&password=" << config->getBoot()->getServerPassword() << "&_dc=" << realtime->getUnixTimestamp();

	req.method = Http::Request::Method_POST;
	req.serverPath = reqPath->getString();
	req.keepAlive = false;
	req.phpSessionId = NULL;
	req.data = reqData;
	resp.phpSessionId = NULL;
	resp.data = respData;

	if(http->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_MODEM, "sendRequest failed");
		return false;
	}

	state = State_Audit;
	return true;
}

void ErpProtocol::procError() {
	LOG_DEBUG(LOG_MODEM, "procError");
	state = State_Idle;
	courier.deliver(Http::ClientInterface::Event_RequestError);
}

void ErpProtocol::stateAuditEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "stateAuditEvent");
	switch(event->getType()) {
		case Http::ClientInterface::Event_RequestComplete: stateAuditEventRequestComplete(); return;
		case Http::ClientInterface::Event_RequestError: {
			LOG_ERROR(LOG_MODEM, "Request failed");
			procError();
			return;
		}
	}
}

void ErpProtocol::stateAuditEventRequestComplete() {
	LOG_DEBUG(LOG_MODEM, "stateAuditEventRequestComplete");
	LOG_DEBUG_STR(LOG_MODEM, (char*)resp.data->getData(), resp.data->getLen());
	if(jsonParser.parse((char*)resp.data->getData(), resp.data->getLen()) == false) {
		LOG_ERROR(LOG_MODEM, "Json parse failed");
		procError();
		return;
	}

	JsonNode *nodeRoot = jsonParser.getRoot();
	if(nodeRoot == NULL || nodeRoot->getType() != JsonNode::Type_Object) {
		LOG_ERROR(LOG_MODEM, "Wrong json format");
		procError();
		return;
	}

	if(checkResponseSuccess(nodeRoot) == false) {
		procError();
		return;
	}

	JsonNode *nodeData = nodeRoot->getField("data", JsonNode::Type_Array);
	if(nodeData == NULL) {
		LOG_ERROR(LOG_MODEM, "Wrong response format");
		procError();
		return;
	}

    JsonNode *nodeEntry = nodeData->getChildByIndex(0);
    if(nodeEntry == NULL || nodeEntry->getType() != JsonNode::Type_Object) {
        LOG_ERROR(LOG_MODEM, "Wrong response format");
		procError();
        return;
    }

	uint32_t configId = 0;
	if(nodeEntry->getNumberField("config_id", &configId) == NULL) {
		LOG_DEBUG(LOG_MODEM, "Update field not found");
		procError();
		return;
	}

	state = State_Idle;
	Event event(Http::ClientInterface::Event_RequestComplete, configId);
	courier.deliver(&event);
}

bool ErpProtocol::sendConfigRequest(const char *login, StringBuilder *respBuf) {
	LOG_DEBUG(LOG_MODEM, "sendConfigRequest " << respBuf->getSize());
	if(http == NULL) {
		LOG_ERROR(LOG_MODEM, "Internet not inited");
		return false;
	}

	reqPath->clear();
	*reqPath << "/api/1.0/Modem.php?action=ReadConfig&login=" << login << "&password=" << config->getBoot()->getServerPassword() << "&_dc=" << realtime->getUnixTimestamp();

	req.method = Http::Request::Method_POST;
	req.serverPath = reqPath->getString();
	req.keepAlive = false;
	req.phpSessionId = NULL;
	req.data = NULL;
	resp.phpSessionId = NULL;
	resp.data = respBuf;

	if(http->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_MODEM, "sendRequest failed");
		return false;
	}

	state = State_Config;
	return true;
}

void ErpProtocol::stateConfigEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "stateConfigEvent");
	switch(event->getType()) {
		case Http::ClientInterface::Event_RequestComplete: {
			state = State_Idle;
			Event event(Http::ClientInterface::Event_RequestComplete);
			courier.deliver(&event);
			return;
		}
		case Http::ClientInterface::Event_RequestError: {
			LOG_ERROR(LOG_MODEM, "Request failed");
			procError();
			return;
		}
	}
}

bool ErpProtocol::sendSyncRequest(const char *login, uint32_t decimalPoint, uint16_t signalQuality, ConfigEventList *events, StringBuilder *reqData) {
	LOG_DEBUG(LOG_MODEM, "sendSyncRequest");
	if(http == NULL) {
		LOG_ERROR(LOG_MODEM, "Internet not inited");
		return false;
	}

	this->events = events;

	reqPath->clear();
	*reqPath << "/api/1.0/Modem.php?action=AddEvent&login=" << login << "&password=" << config->getBoot()->getServerPassword() << "&_dc=" << realtime->getUnixTimestamp();

	ErpSyncRequest syncReq(config);
	syncReq.make(decimalPoint, signalQuality, events, reqData);
	syncIndex = syncReq.getSyncIndex();

	req.method = Http::Request::Method_POST;
	req.serverPath = reqPath->getString();
	req.keepAlive = false;
	req.phpSessionId = NULL;
	req.data = reqData;
	resp.phpSessionId = NULL;
	resp.data = respData;

	if(http->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_MODEM, "sendRequest failed");
		return false;
	}

	state = State_Event;
	return true;
}

void ErpProtocol::stateEventEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "stateEventEvent");
	switch(event->getType()) {
		case Http::ClientInterface::Event_RequestComplete: stateEventEventRequestComplete(); return;
		case Http::ClientInterface::Event_RequestError: {
			LOG_ERROR(LOG_MODEM, "Server event failed");
			procError();
			return;
		}
	}
}

void ErpProtocol::stateEventEventRequestComplete() {
	LOG_DEBUG(LOG_MODEM, "stateEventEventRequestComplete");
	LOG_DEBUG_STR(LOG_MODEM, (char*)resp.data->getData(), resp.data->getLen());
	if(jsonParser.parse((char*)resp.data->getData(), resp.data->getLen()) == false) {
		LOG_ERROR(LOG_MODEM, "Json parse failed");
		procError();
		return;
	}

	JsonNode *nodeRoot = jsonParser.getRoot();
	if(nodeRoot == NULL || nodeRoot->getType() != JsonNode::Type_Object) {
		LOG_ERROR(LOG_MODEM, "Wrong json format");
		procError();
		return;
	}

	if(checkResponseSuccess(nodeRoot) == false) {
		procError();
		return;
	}

	JsonNode *nodeData = nodeRoot->getField("data", JsonNode::Type_Array);
	if(nodeData == NULL) {
		LOG_ERROR(LOG_MODEM, "Wrong response format");
		procError();
		return;
	}

    JsonNode *nodeEntry = nodeData->getChildByIndex(0);
    if(nodeEntry == NULL || nodeEntry->getType() != JsonNode::Type_Object) {
        LOG_ERROR(LOG_MODEM, "Wrong response format");
		procError();
        return;
    }

	DateTime datetime;
	if(nodeEntry->getDateTimeField("date", &datetime) == NULL) {
		LOG_ERROR(LOG_MODEM, "Wrong date format");
		procError();
		return;
	}

	uint8_t update = 0;
	if(nodeEntry->getNumberField("update", &update) == NULL) {
		LOG_DEBUG(LOG_MODEM, "Update field not found");
	}

	uint8_t command = 0;
	if(nodeEntry->getNumberField("command", &command) == NULL) {
		LOG_DEBUG(LOG_MODEM, "Command field not found");
	}

	if(syncIndex != CONFIG_EVENT_UNSET) { events->setSync(syncIndex); }
	state = State_Idle;
	EventSync event(Http::ClientInterface::Event_RequestComplete, datetime, update, command);
	courier.deliver(&event);
}

bool ErpProtocol::sendPingRequest(const char *login, uint32_t decimalPoint, StringBuilder *reqData) {
	LOG_DEBUG(LOG_MODEM, "sendSyncRequest");
	if(http == NULL) {
		LOG_ERROR(LOG_MODEM, "Internet not inited");
		return false;
	}

	this->events = events;

	reqPath->clear();
	*reqPath << "/api/1.0/Modem.php?action=Sync&login=" << login << "&password=" << config->getBoot()->getServerPassword() << "&_dc=" << realtime->getUnixTimestamp();

	reqData->set("{}");

	req.method = Http::Request::Method_POST;
	req.serverPath = reqPath->getString();
	req.keepAlive = false;
	req.phpSessionId = NULL;
	req.data = reqData;
	resp.phpSessionId = NULL;
	resp.data = respData;

	if(http->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_MODEM, "sendRequest failed");
		return false;
	}

	state = State_Ping;
	return true;
}

void ErpProtocol::statePingEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "statePingtEvent");
	switch(event->getType()) {
		case Http::ClientInterface::Event_RequestComplete: statePingEventRequestComplete(); return;
		case Http::ClientInterface::Event_RequestError: {
			LOG_ERROR(LOG_MODEM, "Server event failed");
			procError();
			return;
		}
	}
}

void ErpProtocol::statePingEventRequestComplete() {
	LOG_DEBUG(LOG_MODEM, "statePingEventRequestComplete");
	LOG_DEBUG_STR(LOG_MODEM, (char*)resp.data->getData(), resp.data->getLen());
	if(jsonParser.parse((char*)resp.data->getData(), resp.data->getLen()) == false) {
		LOG_ERROR(LOG_MODEM, "Json parse failed");
		procError();
		return;
	}

	JsonNode *nodeRoot = jsonParser.getRoot();
	if(nodeRoot == NULL || nodeRoot->getType() != JsonNode::Type_Object) {
		LOG_ERROR(LOG_MODEM, "Wrong json format");
		procError();
		return;
	}

	if(checkResponseSuccess(nodeRoot) == false) {
		procError();
		return;
	}

	JsonNode *nodeData = nodeRoot->getField("data", JsonNode::Type_Array);
	if(nodeData == NULL) {
		LOG_ERROR(LOG_MODEM, "Wrong response format");
		procError();
		return;
	}

    JsonNode *nodeEntry = nodeData->getChildByIndex(0);
    if(nodeEntry == NULL || nodeEntry->getType() != JsonNode::Type_Object) {
        LOG_ERROR(LOG_MODEM, "Wrong response format");
		procError();
        return;
    }

	DateTime datetime;
	if(nodeEntry->getDateTimeField("date", &datetime) == NULL) {
		LOG_ERROR(LOG_MODEM, "Wrong date format");
		procError();
		return;
	}

	uint8_t update = 0;
	uint8_t command = 0;
	if(nodeEntry->getNumberField("command", &command) == NULL) {
		LOG_DEBUG(LOG_MODEM, "Command field not found");
	}

	state = State_Idle;
	EventSync event(Http::ClientInterface::Event_RequestComplete, datetime, update, command);
	courier.deliver(&event);
}

bool ErpProtocol::checkResponseSuccess(JsonNode *nodeRoot) {
	JsonNode *nodeResult = nodeRoot->getChild("success");
	if(nodeResult == NULL || nodeResult->getType() != JsonNode::Type_Member) {
		LOG_ERROR(LOG_MODEM, "Wrong response format");
		return false;
	}

	JsonNode *nodeResultValue = nodeResult->getChild();
	if(nodeResultValue == NULL || nodeResultValue->getType() != JsonNode::Type_TRUE) {
		LOG_ERROR(LOG_MODEM, "Wrong result");
		return false;
	}

	return true;
}
