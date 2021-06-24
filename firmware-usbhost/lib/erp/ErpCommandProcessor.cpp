#include "ErpCommandProcessor.h"
#include "logger/include/Logger.h"

enum RemoteCommand {
	RemoteCommand_None			 = 0x00,
	RemoteCommand_Setting		 = 0x01,
	RemoteCommand_ReloadModem	 = 0x02,
	RemoteCommand_ReloadAutomat	 = 0x03,
	RemoteCommand_ResetErrors	 = 0x04,
	RemoteCommand_ChargeCash	 = 0x05,
};

ErpCommandProcessor::ErpCommandProcessor(Mdb::DeviceContext *context, TimerEngine *timerEngine, EventEngineInterface *eventEngine) :
	state(State_Idle),
	server(NULL),
	data(256, 256),
	jsonParser(10)
{
	req.data = &data;
	resp.data = &data;
	cashless = new ErpCashless(context, timerEngine, eventEngine);
}

ErpCommandProcessor::~ErpCommandProcessor() {
	if(server != NULL) { delete server; }
}

void ErpCommandProcessor::bind(Http::ServerInterface *server) {
	LOG_DEBUG(LOG_HTTP, "bind");
	this->server = server;
	this->server->setObserver(this);
}

void ErpCommandProcessor::accept() {
	LOG_DEBUG(LOG_HTTP, "accept");
	if(server == NULL) { return; }
	server->accept(&req);
	state = State_WaitRequest;
}

void ErpCommandProcessor::proc(Event *event) {
	LOG_DEBUG(LOG_HTTP, "Event: state=" << state << ", event=" << event->getType());
	if(server == NULL) { return; }
	switch(state) {
		case State_WaitRequest: stateWaitRequestEvent(event); break;
		case State_SendResponse: stateSendResponseEvent(event); break;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void ErpCommandProcessor::stateWaitRequestEvent(Event *event) {
	LOG_DEBUG(LOG_HTTP, "stateWaitRequestEvent");
	switch(event->getType()) {
		case Http::Server::Event_RequestIncoming: stateWaitRequestEventRequestIncoming(); return;
		case Http::Server::Event_ResponseError: procUnwaitedClose(); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void ErpCommandProcessor::stateWaitRequestEventRequestIncoming() {
	LOG_DEBUG(LOG_HTTP, "stateWaitRequestEventRequestIncoming");
	LOG_DEBUG_STR(LOG_HTTP, data.getString(), data.getLen());
	if(jsonParser.parse(data.getString(), data.getLen()) == false) {
		LOG_ERROR(LOG_HTTP, "Json parse failed");
		sendFailureResponse();
		return;
	}

	JsonNode *nodeRoot = jsonParser.getRoot();
	if(nodeRoot == NULL || nodeRoot->getType() != JsonNode::Type_Object) {
		LOG_ERROR(LOG_FR, "Wrong json format");
		sendFailureResponse();
		return;
	}

	if(nodeRoot->getNumberField("command", &command) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'command' not found");
		sendFailureResponse();
		return;
	}

	if(command != RemoteCommand_ChargeCash) {
		LOG_ERROR(LOG_FR, "Unsupported command " << command);
		sendFailureResponse();
		return;
	}

	if(nodeRoot->getNumberField("value", &value) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'value' not found");
		sendFailureResponse();
		return;
	}

	if(cashless->deposite(value) == false) {
		LOG_ERROR(LOG_FR, "Field 'value' not found");
		sendFailureResponse();
		return;
	}

	sendSuccessResponse();
}

void ErpCommandProcessor::sendSuccessResponse() {
	LOG_DEBUG(LOG_HTTP, "sendSuccessResponse");
	data.clear();
	data << "{\"success\":true}";
	gotoStateSendResponse();
}

void ErpCommandProcessor::sendFailureResponse() {
	LOG_DEBUG(LOG_HTTP, "sendFailureResponse");
	data.clear();
	data << "{\"success\":false}";
	gotoStateSendResponse();
}

void ErpCommandProcessor::gotoStateSendResponse() {
	LOG_DEBUG(LOG_HTTP, "gotoStateSendResponse");
	if(server->sendResponse(&resp) == false) {
		LOG_ERROR(LOG_HTTP, "sendResponse failed");
		return;
	}
	state = State_SendResponse;
}

void ErpCommandProcessor::stateSendResponseEvent(Event *event) {
	switch(event->getType()) {
		case Http::Server::Event_ResponseComplete: stateSendResponseEventSendComplete(event); return;
		case Http::Server::Event_ResponseError: procUnwaitedClose(); return;
		default: LOG_ERROR(LOG_HTTP, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void ErpCommandProcessor::stateSendResponseEventSendComplete(Event *event) {
	LOG_DEBUG(LOG_HTTP, "stateSendResponseEventSendComplete");
	state = State_Idle;
}

void ErpCommandProcessor::procUnwaitedClose() {
	LOG_DEBUG(LOG_HTTP, "procUnwaitedClose");
	state = State_Idle;
}
