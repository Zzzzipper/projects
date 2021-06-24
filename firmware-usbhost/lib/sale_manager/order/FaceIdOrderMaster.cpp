#include <string.h>
#include <stdio.h>
#include "common/logger/include/Logger.h"
#include "FaceIdOrderMaster.h"

#define FACEID_CHECK_TIMEOUT 30000

static const char *master_check_key		 = "1/store/DATA/kitchen/request_face_recognize/v1/ANY/ANY/cbe08ec0-e5bd-4af1-88c9-578b9d0ce6f1";
static const char *device_pincode_key 	 = "1/store/DATA/kitchen/ask_for_the_PIN/v1/ANY/ANY/cbe08ec0-e5bd-4af1-88c9-578b9d0ce6f1";
static const char *master_pincode_key	 = "1/store/DATA/kitchen/request_verify_PIN/v1/ANY/ANY/cbe08ec0-e5bd-4af1-88c9-578b9d0ce6f1";
static const char *device_beverage_key	 = "1/store/DATA/kitchen/request_pour_beverage/v1/ANY/ANY/cbe08ec0-e5bd-4af1-88c9-578b9d0ce6f1";
static const char *master_distribute_key = "1/store/DATA/kitchen/beverage_issued/v1/ANY/ANY/cbe08ec0-e5bd-4af1-88c9-578b9d0ce6f1";
static const char *device_error_key		 = "1/store/DATA/kitchen/error_type_device/v1/ANY/ANY/cbe08ec0-e5bd-4af1-88c9-578b9d0ce6f1";

/*
1.store.DATA.kitchen.ask_for_the_PIN.v1.ANY.ANY.cbe08ec0-e5bd-4af1-88c9-578b9d0ce6f1
{"type":"kitchen.ask_for_the_PIN", "payload": { "deviceId":"ephor1", "errorVerify":"true"}}
{"type":"kitchen.ask_for_the_PIN", "payload": { "deviceId":"ephor1", "errorVerify":"false"}}
1/store/DATA/kitchen/error_type_device/v1/ANY/ANY/cbe08ec0-e5bd-4af1-88c9-578b9d0ce6f1
{"type": "kitchen.error_type_device", "payload": { "deviceId": "deviceId1" }}
 */
FaceIdOrderMaster::FaceIdOrderMaster(
	TcpIp *conn,
	TimerEngine *timerEngine,
	EventEngineInterface *eventengine,
	const char* faceIdDevice,
	const char* addr,
	uint16_t port,
	const char* username,
	const char* password
) :
	eventEngine(eventengine),
	timerEngine(timerEngine),
	deviceId(eventEngine),
	faceIdDevice(faceIdDevice),
	addr(addr),
	port(port),
	userName(username),
	passWord(password),
	state(State_Idle),
	payload(MQTT_PACKET_SIZE, MQTT_PACKET_SIZE)
{
	client = new Mqtt::Client(conn, timerEngine, eventEngine);
	timer = timerEngine->addTimer<FaceIdOrderMaster, &FaceIdOrderMaster::procTimer>(this);

	// Регистрация топиков для приема
	client->subscribe(device_beverage_key);
	client->subscribe(device_pincode_key);
	client->subscribe(device_error_key);

	// Подписка на глобальные извещения от модуля
	eventEngine->subscribe(this, GlobalId_MqttClient);
}

FaceIdOrderMaster::~FaceIdOrderMaster() {
	delete client;
}

void FaceIdOrderMaster::setOrder(Order *order) {
	this->order = order;
}

void FaceIdOrderMaster::reset() {
	LOG_INFO(LOG_SM, "shutdown");
	state = State_Idle;
}

void FaceIdOrderMaster::connect() {
	LOG_INFO(LOG_SM, "connect");
	client->connect(addr, port, TcpIp::Mode_TcpIp, faceIdDevice, userName, passWord);
	state = State_Init;
}

void FaceIdOrderMaster::check() {
	LOG_INFO(LOG_SM, "check");
	gotoStateCheck();
}

void FaceIdOrderMaster::checkPinCode(const char *pincode) {
	LOG_INFO(LOG_SM, "checkPinCode");
	gotoStateCheckPinCode(pincode);
}

void FaceIdOrderMaster::distribute(uint16_t cid) {
	LOG_INFO(LOG_SM, "distribute");
	payload.clear();
	payload << "{\"type\": \"kitchen.beverage_issued\", \"payload\": {";
	payload << "\"deviceId\": \"" << faceIdDevice << "\",";
	payload << "\"beverageCode\": \"" << cid << "\",";
	payload << "\"error\": \"false\"";
	payload << "}}";
	client->publish(master_distribute_key, payload.getData(), payload.getLen(), Mqtt::QoS_1);
	EventInterface request(deviceId, OrderMasterInterface::Event_Distributed);
	eventEngine->transmit(&request);
}

void FaceIdOrderMaster::complete() {
	LOG_INFO(LOG_SM, "complete");
	state = State_Wait;
}

void FaceIdOrderMaster::proc(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "proc");
	switch(state) {
		case State_Init: stateInitEvent(envelope); return;
		case State_Wait: stateWaitEvent(envelope); return;
		case State_Check: stateCheckEvent(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited envelope " << envelope->getType() << "," << state);
	}
}

void FaceIdOrderMaster::procTimer() {
	LOG_DEBUG(LOG_SM, "procTimer");
	switch(state) {
	case State_Check: stateCheckTimeout(); return;
	default: LOG_ERROR(LOG_SM, "Unwaited timeout " << state);
	}
}

void FaceIdOrderMaster::stateInitEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateInitEvent");
	switch (envelope->getType()) {
		case Mqtt::Event_ConnectComplete: stateInitEventConnectComplete(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited envelope " << envelope->getType() << "," << state);
	}
}

void FaceIdOrderMaster::stateInitEventConnectComplete() {
	LOG_INFO(LOG_SM, "stateInitEventConnectComplete");
	gotoStateWait();
	EventInterface request(deviceId, OrderMasterInterface::Event_Connected);
	eventEngine->transmit(&request);
}

void FaceIdOrderMaster::gotoStateWait() {
	LOG_INFO(LOG_SM, "gotoStateWait");
	state = State_Wait;
}

void FaceIdOrderMaster::stateWaitEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateWaitEvent");
	switch (envelope->getType()) {
		case Mqtt::Event_IncommingMessage: stateCheckEventIncommingMessage(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited envelope " << envelope->getType() << "," << state);
	}
}

void FaceIdOrderMaster::gotoStateCheck() {
	LOG_INFO(LOG_SM, "gotoStateCheck");
	payload.clear();
	payload << "{\"type\": \"kitchen.request_face_recognize\", \"payload\": { \"deviceId\": \"" << faceIdDevice << "\"}}";
	client->publish(master_check_key, payload.getData(), payload.getLen(), Mqtt::QoS_1);
	timer->start(FACEID_CHECK_TIMEOUT);
	state = State_Check;
}

void FaceIdOrderMaster::stateCheckEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCheckEvent");
	switch (envelope->getType()) {
		case Mqtt::Event_IncommingMessage: stateCheckEventIncommingMessage(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited envelope " << envelope->getType() << "," << state);
	}
}

void FaceIdOrderMaster::stateCheckTimeout() {
	LOG_INFO(LOG_SM, "stateCheckTimeout");
    gotoStateWait();
    EventInterface request(deviceId, OrderMasterInterface::Event_Denied);
    eventEngine->transmit(&request);
}

void FaceIdOrderMaster::stateCheckEventIncommingMessage() {
	LOG_INFO(LOG_SM, "stateCheckEventIncommingMessage");
    if(order == NULL) {
        LOG_ERROR(LOG_SM, "order is NULL");
        return;
    }

    LOG_INFO(LOG_SM, "Topic=" << client->getIncommingTopic()->getString());
    LOG_INFO(LOG_SM, "DataLen=" << client->getIncommingData()->getLen());
    JsonParser parser(100);
    if(parser.parse((const char*)client->getIncommingData()->getData(), client->getIncommingData()->getLen()) == false) {
        LOG_ERROR(LOG_SM, "Data parse failed");
    	return;
    }

	JsonNode *root = parser.getRoot();
	if(root == NULL) {
        LOG_ERROR(LOG_SM, "Data parse failed");
    	return;
	}

	JsonNode *payload = root->getField("payload", JsonNode::Type_Object);
	if(payload == NULL) {
        LOG_ERROR(LOG_SM, "Data parse failed");
    	return;
	}

	JsonNode *deviceNode = payload->getField("deviceId", JsonNode::Type_String);
	if(deviceNode == NULL) {
		LOG_ERROR(LOG_SM, "Data parse failed");
		return;
	}

	LOG_INFO(LOG_SM, "DeviceId=");
	LOG_INFO_STR(LOG_SM, deviceNode->getValue(), deviceNode->getValueLen());
	if(strncmp(faceIdDevice, deviceNode->getValue(), deviceNode->getValueLen()) != 0) {
		LOG_ERROR(LOG_SM, "FaceIdDevice not equal");
		return;
	}

	JsonNode *typeNode = root->getField("type", JsonNode::Type_String);
	if(deviceNode == NULL) {
		LOG_ERROR(LOG_SM, "Data parse failed");
		return;
	}

	LOG_INFO(LOG_SM, "Type=");
	LOG_INFO_STR(LOG_SM, typeNode->getValue(), typeNode->getValueLen());
	if(strncmp("kitchen.request_pour_beverage", typeNode->getValue(), typeNode->getValueLen()) == 0) {
		procBeverageMessage(payload);
		return;
	} else if(strncmp("kitchen.ask_for_the_PIN", typeNode->getValue(), typeNode->getValueLen()) == 0) {
		procPinCodeMessage(payload);
		return;
	} else if(strncmp("kitchen.error_type_device", typeNode->getValue(), typeNode->getValueLen()) == 0) {
		procErrorMessage();
		return;
	} else {
		LOG_ERROR(LOG_SM, "Unsupported type " << typeNode->getValue());
		return;
	}
}

void FaceIdOrderMaster::procBeverageMessage(JsonNode *payload) {
	LOG_INFO(LOG_SM, "procBeverageMessage");
	JsonNode *beverages = payload->getField("beverages", JsonNode::Type_Array);
	if(beverages == NULL) {
		LOG_ERROR(LOG_SM, "Data parse failed");
		return;
	}

    order->clear();
	for(uint16_t i = 0;; i++) {
		JsonNode *beverage = beverages->getChildByIndex(i);
		if(beverage == NULL) {
			break;
		}

		uint32_t beverageCode = 0;
		JsonNode *beverageCodeNode = beverage->getNumberField("bc", &beverageCode);
		if(beverageCodeNode == NULL) {
			LOG_ERROR(LOG_SM, "Data parse failed");
			break;
		}

		uint32_t quantity = 0;
		JsonNode *quantityNode = beverage->getNumberField("qty", &quantity);
		if(quantityNode == NULL) {
			LOG_ERROR(LOG_SM, "Data parse failed");
			break;
		}

		LOG_INFO(LOG_SM, "Beverage=" << beverageCode << "/" << quantity);
		order->add(beverageCode, quantity);
	}

    LOG_INFO(LOG_SM, "approved " << order->getQuantity());
    timer->stop();
    state = State_Vending;
    EventInterface request(deviceId, OrderMasterInterface::Event_Approved);
    eventEngine->transmit(&request);
}

/*
errorVerify="false" - first pincode request
errorVerify="true" - next pincode request
3 pincode => gotoStateWait
 */
void FaceIdOrderMaster::procPinCodeMessage(JsonNode *payload) {
	LOG_INFO(LOG_SM, "procPinCodeMessage");
#if 0
	JsonNode *errorNode = payload->getField("errorVerify", JsonNode::Type_TRUE);
	if(errorNode == NULL) {
		LOG_ERROR(LOG_SM, "Data parse failed");
		return;
	}

	if(strncmp("false", errorNode->getValue(), errorNode->getValueLen()) == 0) {
		LOG_ERROR(LOG_SM, "Not enough pincode");
	    EventInterface request(deviceId, OrderMasterInterface::Event_Denied);
	    eventEngine->transmit(&request);
		return;
	}
#endif
    timer->stop();
    gotoStateWait();
    EventInterface request(deviceId, OrderMasterInterface::Event_PinCode);
    eventEngine->transmit(&request);
}

void FaceIdOrderMaster::procErrorMessage() {
	LOG_INFO(LOG_SM, "procErrorMessage");
	timer->stop();
    gotoStateWait();
    EventInterface request(deviceId, OrderMasterInterface::Event_Denied);
    eventEngine->transmit(&request);
}

void FaceIdOrderMaster::gotoStateCheckPinCode(const char *pincode) {
	LOG_INFO(LOG_SM, "gotoStateCheckPinCode");
	payload.clear();
	payload << "{\"type\": \"kitchen.request_verify_PIN\", \"payload\": {";
	payload << "\"deviceId\": \"" << faceIdDevice << "\",";
	payload << "\"PIN\": \"" << pincode << "\"";
	payload << "}}";
	client->publish(master_pincode_key, payload.getData(), payload.getLen(), Mqtt::QoS_1);
	timer->start(FACEID_CHECK_TIMEOUT);
	state = State_Check;
}

void FaceIdOrderMaster::stateVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateVendingEvent");
	switch (envelope->getType()) {
		default: LOG_ERROR(LOG_SM, "Unwaited envelope " << envelope->getType() << "," << state);
	}
}
