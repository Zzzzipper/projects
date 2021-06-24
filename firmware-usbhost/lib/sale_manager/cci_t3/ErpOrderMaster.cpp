#include <lib/sale_manager/cci_t3/ErpOrderMaster.h>
#include "common/timer/include/TimerEngine.h"
#include "common/utils/include/StringParser.h"
#include "common/logger/include/Logger.h"

#define JSON_NODE_MAX 30
#define ORDER_ID_SIZE 20
#define ORDER_TIMEOUT 60000
//+++
#define NFC_MAX_TRY_NUMBER 3
#define REQUEST_PATH_MAX_SIZE 368
#define RESPONSE_DATA_MAX_SIZE 1024
#define JSON_NODE_MAX 30
//+++

ErpOrderMaster::ErpOrderMaster(
	ConfigModem *config,
	CodeScannerInterface *scanner,
	TcpIp *tcpConn,
	TimerEngine *timerEngine,
	EventEngineInterface *eventEngine
) :
	config(config),
	scanner(scanner),
	timerEngine(timerEngine),
	eventEngine(eventEngine),
	deviceId(eventEngine),
	state(State_Idle),
	enabled(false),
	searchCid(CID_UNDEFINED),
	oid(ORDER_ID_SIZE, ORDER_ID_SIZE),
	jsonParser(JSON_NODE_MAX)
{
//	timer = timerEngine->addTimer<OrderCashless, &OrderCashless::procTimer>(this);
	scanner->addObserver(this);

	httpClient = new Http::Client(timerEngine, tcpConn);
	httpTransport = new Http::Transport(httpClient, NFC_MAX_TRY_NUMBER);
	httpTransport->setObserver(this);

	req.serverName = config->getBoot()->getServerDomain();
	req.serverPort = config->getBoot()->getServerPort();
	reqPath = new StringBuilder(REQUEST_PATH_MAX_SIZE, REQUEST_PATH_MAX_SIZE);
	reqData = new StringBuilder(REQUEST_PATH_MAX_SIZE, REQUEST_PATH_MAX_SIZE);
	respData = new StringBuilder(RESPONSE_DATA_MAX_SIZE, RESPONSE_DATA_MAX_SIZE);
}

ErpOrderMaster::~ErpOrderMaster() {

}

void ErpOrderMaster::reset() {
	LOG_DEBUG(LOG_ORDERP, "reset");
	enabled = false;
	gotoStateWait();
}

void ErpOrderMaster::disable() {
	LOG_DEBUG(LOG_ORDERP, "disable");
	if(enabled == true) {
		scanner->off();
		enabled = false;
	}
}

void ErpOrderMaster::enable() {
	LOG_DEBUG(LOG_ORDERP, "enable");
	if(enabled == false) {
		scanner->on();
		enabled = true;
	}
}

void ErpOrderMaster::setProductId(uint16_t searchCid) {
	LOG_DEBUG(LOG_ORDERP, "setProductId");
	this->searchCid = searchCid;
}

bool ErpOrderMaster::saleComplete() {
	LOG_DEBUG(LOG_ORDERP, "saleComplete");
	if(state != State_OrderProcessing) {
		LOG_ERROR(LOG_ORDERP, "Wrong state " << state);
		return false;
	}
	gotoStateOrderComplete();
	return true;
}

bool ErpOrderMaster::saleFailed() {
	LOG_DEBUG(LOG_ORDERP, "saleFailed");
	if(state != State_OrderProcessing) {
		LOG_ERROR(LOG_ORDERP, "Wrong state " << state);
		return false;
	}
	state = State_Wait;
	EventInterface event(deviceId, OrderInterface::Event_OrderEnd);
	eventEngine->transmit(&event);
	return true;
}

bool ErpOrderMaster::closeSession() {
	LOG_DEBUG(LOG_ORDERP, "closeSession");
	EventInterface event(deviceId, OrderInterface::Event_OrderEnd);
	eventEngine->transmit(&event);
	return true;
}

bool ErpOrderMaster::procCode(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ORDERP, "procCode");
	LOG_INFO_HEX(LOG_ORDERP, data, dataLen);
	if(state != State_Wait || enabled == false) {
		LOG_ERROR(LOG_ORDERP, "Wrong state " << state);
		return false;
	}

	StringParser parser((char*)data, dataLen);
	if(parser.compareAndSkip("10050") == false) {
		LOG_DEBUG(LOG_ORDERP, "Unsupported prefix");
		return false;
	}

	LOG_DEBUG(LOG_ORDERP, "Order code");
	scanner->off();
	oid.set(parser.unparsed(), parser.unparsedLen());
	gotoStateOrderStart();
	EventInterface event(deviceId, OrderInterface::Event_OrderApprove);
	eventEngine->transmit(&event);
	return true;
}

void ErpOrderMaster::proc(Event *event) {
	LOG_DEBUG(LOG_ORDERP, "proc " << event->getType());
	switch(state) {
		case State_OrderStart: stateOrderStartEvent(event); break;
		case State_OrderComplete: stateOrderCompleteEvent(event); break;
		default: LOG_ERROR(LOG_ORDERP, "Unwaited event " << state);
	}
}

void ErpOrderMaster::procTimer() {

}

void ErpOrderMaster::gotoStateWait() {
	LOG_DEBUG(LOG_ORDERP, "gotoStateWait");
	if(enabled == true) { scanner->on(); } else { scanner->off(); }
	state = State_Wait;
}

void ErpOrderMaster::gotoStateOrderStart() {
	LOG_DEBUG(LOG_ORDERP, "makeRequest");
	reqPath->clear();
	*reqPath << "/api/1.0/Modem.php?action=StartOrder"
			 << "&login=" << config->getBoot()->getImei()
			 << "&password=" << config->getBoot()->getServerPassword()
			 << "&_dc=" << config->getRealTime()->getUnixTimestamp();

	DateTime datetime;
	config->getRealTime()->getDateTime(&datetime);
	reqData->clear();
	*reqData << "{\"sid\":\"" << oid << "\"";
	if(searchCid != CID_UNDEFINED) { *reqData << ",\"cid\":" << searchCid; }
	*reqData << ",\"date\":\""; datetime2string(&datetime, reqData); *reqData << "\"}";

	req.method = Http::Request::Method_POST;
	req.serverPath = reqPath->getString();
	req.keepAlive = false;
	req.phpSessionId = NULL;
	req.data = reqData;

	resp.phpSessionId = NULL;
	resp.data = respData;

	if(httpTransport->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_ORDERP, "sendRequest failed");
		return;
	}

//	timer->start(ORDER_TIMEOUT);
	state = State_OrderStart;
}

void ErpOrderMaster::stateOrderStartEvent(Event *event) {
	LOG_DEBUG(LOG_ORDERP, "stateOrderStartEvent");
	switch(event->getType()) {
		case Http::ClientInterface::Event_RequestComplete: stateOrderStartEventRequestComplete(); return;
		case Http::ClientInterface::Event_RequestError: stateOrderStartEventRequestError(); return;
	}
}

void ErpOrderMaster::stateOrderStartEventRequestComplete() {
	LOG_INFO(LOG_ORDERP, "stateOrderStartEventRequestComplete");
	LOG_INFO_HEX(LOG_ORDERP, resp.data->getData(), resp.data->getLen());
	if(jsonParser.parse((char*)resp.data->getData(), resp.data->getLen()) == false) {
		LOG_ERROR(LOG_ORDERP, "Json parse failed");
		stateOrderStartEventRequestError();
		return;
	}

	JsonNode *nodeRoot = jsonParser.getRoot();
	if(nodeRoot == NULL || nodeRoot->getType() != JsonNode::Type_Object) {
		LOG_ERROR(LOG_ORDERP, "Wrong json format");
		stateOrderStartEventRequestError();
		return;
	}

	JsonNode *nodeResult = nodeRoot->getChild("success");
	if(nodeResult == NULL || nodeResult->getType() != JsonNode::Type_Member) {
		LOG_ERROR(LOG_ORDERP, "Wrong response format");
		stateOrderStartEventRequestError();
		return;
	}

	JsonNode *nodeResultValue = nodeResult->getChild();
	if(nodeResultValue == NULL || nodeResultValue->getType() != JsonNode::Type_TRUE) {
		LOG_ERROR(LOG_ORDERP, "Wrong result");
		stateOrderStartEventRequestError();
		return;
	}

	JsonNode *nodeData = nodeRoot->getField("data", JsonNode::Type_Array);
	if(nodeData == NULL) {
		LOG_ERROR(LOG_ORDERP, "Wrong response format");
		stateOrderStartEventRequestError();
		return;
	}

	JsonNode *nodeEntry = nodeData->getChildByIndex(0);
	if(nodeEntry == NULL || nodeEntry->getType() != JsonNode::Type_Object) {
		LOG_ERROR(LOG_ORDERP, "Wrong response format");
		stateOrderStartEventRequestError();
		return;
	}

	JsonNode *nodeResult2 = nodeEntry->getChild("success");
	if(nodeResult2 == NULL || nodeResult2->getType() != JsonNode::Type_Member) {
		LOG_ERROR(LOG_ORDERP, "Wrong response format");
		stateOrderStartEventRequestError();
		return;
	}

	JsonNode *nodeResultValue2 = nodeResult2->getChild();
	if(nodeResultValue == NULL || nodeResultValue2->getType() != JsonNode::Type_TRUE) {
		LOG_ERROR(LOG_ORDERP, "Wrong result");
		stateOrderStartEventRequestError();
		return;
	}

	if(nodeEntry->getNumberField("cid", &cid) == NULL) {
		LOG_ERROR(LOG_ORDERP, "CID not found");
		stateOrderStartEventRequestError();
		return;
	}

	LOG_INFO(LOG_ORDERP, "OrderStart " << cid);
	state = State_OrderProcessing;
	EventUint16Interface event(deviceId, OrderInterface::Event_OrderRequest, cid);
	eventEngine->transmit(&event);
}

void ErpOrderMaster::stateOrderStartEventRequestError() {
	LOG_INFO(LOG_ORDERP, "stateOrderStartEventRequestError");
	LOG_INFO_HEX(LOG_ORDERP, resp.data->getData(), resp.data->getLen());
	gotoStateWait();
	EventInterface event(deviceId, OrderInterface::Event_OrderCancel);
	eventEngine->transmit(&event);
}

void ErpOrderMaster::gotoStateOrderComplete() {
	LOG_DEBUG(LOG_ORDERP, "gotoStateOrderComplete");
	reqPath->clear();
	*reqPath << "/api/1.0/Modem.php?action=CompleteOrder"
			 << "&login=" << config->getBoot()->getImei()
			 << "&password=" << config->getBoot()->getServerPassword()
			 << "&_dc=" << config->getRealTime()->getUnixTimestamp();

	DateTime datetime;
	config->getRealTime()->getDateTime(&datetime);
	reqData->clear();
	*reqData << "{\"sid\":\"" << oid << "\"";
	*reqData << ",\"result\":0}";

	req.method = Http::Request::Method_POST;
	req.serverPath = reqPath->getString();
	req.keepAlive = false;
	req.phpSessionId = NULL;
	req.data = reqData;

	resp.phpSessionId = NULL;
	resp.data = respData;

	if(httpTransport->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_ORDERP, "sendRequest failed");
		return;
	}

//	timer->start(ORDER_TIMEOUT);
	state = State_OrderComplete;
}

void ErpOrderMaster::stateOrderCompleteEvent(Event *event) {
	LOG_DEBUG(LOG_ORDERP, "stateOrderCompleteEvent");
	switch(event->getType()) {
		case Http::ClientInterface::Event_RequestComplete: stateOrderCompleteEventRequestComplete(); return;
		case Http::ClientInterface::Event_RequestError: stateOrderCompleteEventRequestError(); return;
	}
}

void ErpOrderMaster::stateOrderCompleteEventRequestComplete() {
	LOG_INFO(LOG_ORDERP, "stateOrderCompleteEventRequestComplete");
	LOG_INFO_HEX(LOG_ORDERP, resp.data->getData(), resp.data->getLen());
	if(jsonParser.parse((char*)resp.data->getData(), resp.data->getLen()) == false) {
		LOG_ERROR(LOG_ORDERP, "Json parse failed");
		stateOrderCompleteEventRequestError();
		return;
	}

	JsonNode *nodeRoot = jsonParser.getRoot();
	if(nodeRoot == NULL || nodeRoot->getType() != JsonNode::Type_Object) {
		LOG_ERROR(LOG_ORDERP, "Wrong json format");
		stateOrderCompleteEventRequestError();
		return;
	}

	JsonNode *nodeResult = nodeRoot->getChild("success");
	if(nodeResult == NULL || nodeResult->getType() != JsonNode::Type_Member) {
		LOG_ERROR(LOG_ORDERP, "Wrong response format");
		stateOrderCompleteEventRequestError();
		return;
	}

	JsonNode *nodeResultValue = nodeResult->getChild();
	if(nodeResultValue == NULL || nodeResultValue->getType() != JsonNode::Type_TRUE) {
		LOG_ERROR(LOG_ORDERP, "Wrong result");
		stateOrderCompleteEventRequestError();
		return;
	}

	JsonNode *nodeData = nodeRoot->getField("data", JsonNode::Type_Array);
	if(nodeData == NULL) {
		LOG_ERROR(LOG_ORDERP, "Wrong response format");
		stateOrderCompleteEventRequestError();
		return;
	}

	JsonNode *nodeEntry = nodeData->getChildByIndex(0);
	if(nodeEntry == NULL || nodeEntry->getType() != JsonNode::Type_Object) {
		LOG_ERROR(LOG_ORDERP, "Wrong response format");
		stateOrderCompleteEventRequestError();
		return;
	}

	JsonNode *nodeResult2 = nodeEntry->getChild("success");
	if(nodeResult2 == NULL || nodeResult2->getType() != JsonNode::Type_Member) {
		LOG_ERROR(LOG_ORDERP, "Wrong response format");
		stateOrderCompleteEventRequestError();
		return;
	}

	JsonNode *nodeResultValue2 = nodeResult2->getChild();
	if(nodeResultValue == NULL || nodeResultValue2->getType() != JsonNode::Type_TRUE) {
		LOG_ERROR(LOG_ORDERP, "Wrong result");
		stateOrderCompleteEventRequestError();
		return;
	}

	LOG_INFO(LOG_ORDERP, "OrderComplete");
	gotoStateWait();
	EventInterface event(deviceId, OrderInterface::Event_OrderEnd);
	eventEngine->transmit(&event);
}

void ErpOrderMaster::stateOrderCompleteEventRequestError() {
	LOG_INFO(LOG_ORDERP, "stateOrderCompleteEventRequestError");
	LOG_INFO_HEX(LOG_ORDERP, resp.data->getData(), resp.data->getLen());
	gotoStateWait();
	EventInterface event(deviceId, OrderInterface::Event_OrderEnd);
	eventEngine->transmit(&event);
}
