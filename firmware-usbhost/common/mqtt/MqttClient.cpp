#if 1
#include "include/MqttClient.h"
#include "MqttProtocol.h"

#include "common/logger/include/Logger.h"

#include <stdio.h>

namespace Mqtt {

Client::Client(
	TcpIp *conn,
	TimerEngine *timerEngine,
	EventEngineInterface *eventengine
) :
	conn(conn),
	timerEngine(timerEngine),
	eventEngine(eventengine),
	deviceId(eventEngine),
	publishRequest(NULL),
	incommingTopic(MQTT_TOPIC_SIZE, MQTT_TOPIC_SIZE),
	incommingData(MQTT_PACKET_SIZE)
{
	this->conn->setObserver(this);
	this->waitingTimer = timerEngine->addTimer<Client, &Client::procWaitTimer>(this);
	this->pingTimer = timerEngine->addTimer<Client, &Client::procPingTimer>(this);
}

bool Client::connect(
	const char *addr,
	uint16_t port,
	TcpIp::Mode mode,
	const char *clientId,
	const char *userName,
	const char *passWord
) {
	LOG_INFO(LOG_MQTT, "connect");
	if(state != State_Idle) {
		LOG_ERROR(LOG_MQTT, "Wrong state " << state);
		return false;
	}

	this->addr = addr;
	this->port = port;
	this->mode = mode;
	this->userName = userName;
	this->passWord = passWord;
	encoder.setClientId(clientId);
	encoder.setUsername(this->userName);
	encoder.setPassword(this->passWord);
	return gotoStateConnect();
}

bool Client::subscribe(const char* topic) {
	LOG_INFO(LOG_MQTT, "subscribe to " << topic);
	encoder.registerTopic(topic);
	return true;
}

bool Client::publish(const char* topic, uint8_t *data, uint32_t dataLen, QoS qos) {
	LOG_INFO(LOG_MQTT, "publish");
	LOG_DEBUG(LOG_MQTT, "topic=" << topic << ", qos=" << qos);
	LOG_DEBUG_HEX(LOG_MQTT, data, dataLen);
	if(pool.push(topic, data, dataLen, qos) == false) {
		LOG_ERROR(LOG_MQTT, "Pool overflow");
		return false;
	}

	if(state == State_Wait) {
		gotoStatePublish();
		return true;
	}

	return true;
}

void Client::disconnect() {
	LOG_INFO(LOG_MQTT, "disconnect");
	waitingTimer->stop();
	pingTimer->stop();
	conn->close();
}

void Client::proc(Event *event) {
	LOG_DEBUG(LOG_MQTT, "Event: state=" << state << ", event=" << event->getType());
	switch(state) {
		case State_Connect: stateConnectEvent(event); break;
		case State_SendConnect: stateSendConnectEvent(event); break;
		case State_Subscribe: stateSubscribeEvent(event); break;
		case State_Wait: stateWaitEvent(event); break;
		case State_Ping: statePingEvent(event); break;
		case State_RecvResponse: stateRecvResponseEvent(event); break;
		case State_Publish: statePublishEvent(event); break;
		case State_PubAck: statePubAckEvent(event); break;
		case State_Close: stateCloseEvent(event); break;
		case State_ReconnectClose: stateReconnectCloseEvent(event); break;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void Client::procWaitTimer() {
	LOG_INFO(LOG_MQTT, "Timeout of wait: state=" << state);
	switch(state) {
		case State_ReconnectDelay: stateReconnectTimeout(); return;
		case State_RecvResponse: gotoStateReconnectClose(); return;
		case State_PubAck: statePubAckTimeout(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited timeout " << state);
	}
}

void Client::procPingTimer() {
	LOG_INFO(LOG_MQTT, "Timeout of ping: state=" << state);
	switch(state) {
		case State_Wait: gotoStatePing(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited timeout " << state);
	}
}

bool Client::gotoStateConnect() {
	LOG_DEBUG(LOG_MQTT, "gotoStateConnect");
	if(this->conn->connect(this->addr, this->port, this->mode) == false) {
		LOG_ERROR(LOG_MQTT, "Connect failed");
		gotoStateReconnectClose();
		return false;
	}

	state = State_Connect;
	return true;
}

void Client::stateConnectEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "stateConnectEvent");
	switch(event->getType()) {
		case TcpIp::Event_ConnectOk: gotoStateSendConnect(); return;
		case TcpIp::Event_ConnectError: gotoStateReconnectClose(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
	}
}

void Client::gotoStateSendConnect() {
	LOG_INFO(LOG_MQTT, "gotoStateSendConnect");
	encoder.marshall(PacketType_Connect, &sendBuf);
	if(this->conn->send(sendBuf.getData(), sendBuf.getLen()) == false) {
		LOG_ERROR(LOG_MQTT, "Send failed");
		gotoStateReconnectClose();
		return;
	}

	waitingTimer->stop();
	pingTimer->stop();
	state = State_SendConnect;
}

void Client::stateSendConnectEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "stateSendConnecEvent");
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: gotoStateRecvResponse(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: gotoStateReconnectClose(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
	}
}

void Client::gotoStateSubscribe() {
	LOG_INFO(LOG_MQTT, "gotoStateSubscribe");
	if(encoder.haveTopics() == false) {
		LOG_INFO(LOG_MQTT, "..haven't default topics");
		gotoStateWait();
		return;
	}

	encoder.marshall(PacketType_Subscribe, &sendBuf);
	if(this->conn->send(sendBuf.getData(), sendBuf.getLen()) == false) {
		LOG_ERROR(LOG_MQTT, "Send failed");
		gotoStateReconnectClose();
		return;
	}

	state = State_Subscribe;
}

void Client::stateSubscribeEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "stateSubScribeEvent");
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: gotoStateRecvResponse(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: gotoStateReconnectClose(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
	}
}

void Client::gotoStateWait() {
	LOG_INFO(LOG_MQTT, "gotoStateWait");
	if(publishRequest != NULL || pool.isEmpty() == false) {
		gotoStatePublish();
		return;
	}

	pingTimer->start(MQTT_PING_TIMEOUT);
	state = State_Wait;
}

void Client::stateWaitEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "stateWaitEvent");
	switch(event->getType()) {
		case TcpIp::Event_IncomingData: gotoStateRecvResponse(); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: gotoStateReconnectClose(); return;
		default: {
			LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType());
			return;
		}
	}
}

void Client::gotoStatePing() {
	LOG_INFO(LOG_MQTT, "gotoStatePing");
	encoder.marshall(PacketType_PingRec, &sendBuf);
	if(this->conn->send(sendBuf.getData(), sendBuf.getLen()) == false) {
		LOG_ERROR(LOG_MQTT, "Send failed");
		gotoStateReconnectClose();
		return;
	}

	state = State_Ping;
}

void Client::statePingEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "statePingEvent");
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: gotoStateRecvResponse(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: gotoStateReconnectClose(); return;
		default: {
			LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType());
			return;
		}
	}
}

void Client::gotoStateRecvResponse() {
	LOG_INFO(LOG_MQTT, "gotoStateRecvResponse");
	recvBuf.clear();
	if(conn->recv(recvBuf.getData(), recvBuf.getSize()) == false) {
		LOG_ERROR(LOG_MQTT, "Recv failed");
		gotoStateReconnectClose();
		return;
	}

	waitingTimer->start(MQTT_SOCKET_TIMEOUT);
	state = State_RecvResponse;
}

void Client::stateRecvResponseEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "stateRecvResponseEvent");
	switch(event->getType()) {
		case TcpIp::Event_RecvDataOk: stateRecvResponseEventRecvData(event->getUint16()); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: gotoStateReconnectClose(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
	}
}

void Client::stateRecvResponseEventRecvData(uint32_t len) {
	LOG_INFO(LOG_MQTT, "stateRecvResponseEventRecvData");
	recvBuf.setLen(len);
	LOG_DEBUG_HEX(LOG_MQTT, recvBuf.getData(), recvBuf.getLen());
	Marshaller* marshaller = encoder.readPacket(recvBuf);
	if(marshaller == nullptr) {
		LOG_ERROR(LOG_MQTT, "Nothing parse, break.");
		return;
	}

	waitingTimer->stop(); // remember kill timer

	switch(encoder.lastArrivedPacketType()) {
		case PacketType_ConnAck: gotoStateSubscribe(); return;
		case PacketType_SubAck: stateRecvResponsePacketSubAck(); return;
		case PacketType_PingResp: stateRecvResponsePacketPingResp(); return;
		case PacketType_Publish: stateRecvResponsePacketPublish(marshaller); return;
		default: LOG_ERROR(LOG_MQTT, "Unsupported packet type " << encoder.lastArrivedPacketType());
	}
}

void Client::stateRecvResponsePacketSubAck() {
	LOG_INFO(LOG_MQTT, "stateRecvResponsePacketSubAck");
	gotoStateWait();
	EventInterface event(deviceId, Event_ConnectComplete);
	eventEngine->transmit(&event);
#if defined (MQTT_TEST)
	((EventEngine*)eventEngine)->execute();
#endif
}

void Client::stateRecvResponsePacketPingResp() {
	LOG_INFO(LOG_MQTT, "stateRecvResponsePacketPingResp");
	gotoStateWait();
}

void Client::stateRecvResponsePacketPublish(Marshaller *marshaller) {
	LOG_INFO(LOG_MQTT, "stateRecvResponsePacketPublish");
	recvBuf.clear();
	if(conn->recv(recvBuf.getData(), recvBuf.getSize()) == false) {
		LOG_ERROR(LOG_MQTT, "Recv failed");
		gotoStateReconnectClose();
		return;
	}

	procResponsePublish(marshaller);
}

void Client::procResponsePublish(Marshaller *marshaller) {
	LOG_INFO(LOG_MQTT, "procResponsePublish");
	PublishMarshaller *publish = (PublishMarshaller*)marshaller;
	incommingTopic.set(publish->getTopic()->getString());
	incommingData.clear();
	incommingData.add(publish->getPayload()->getData(), publish->getPayload()->getLen());
	gotoStateWait();
	EventInterface event(deviceId, Mqtt::Event_IncommingMessage);
	eventEngine->transmit(&event);
}

void Client::gotoStatePublish() {
	LOG_INFO(LOG_MQTT, "gotoStatePublish");
	if(publishRequest == NULL) {
		publishRequest = pool.pop();
		if(publishRequest == NULL) {
			gotoStateWait();
			return;
		}
	}

	pingTimer->stop();
	encoder.publish(publishRequest->topic.getString(), publishRequest->data.getData(), publishRequest->data.getLen(), publishRequest->qos);
	encoder.marshall(PacketType_Publish, &sendBuf);
	if(this->conn->send(sendBuf.getData(), sendBuf.getLen()) == false) {
		LOG_ERROR(LOG_MQTT, "Send failed");
		gotoStateReconnectClose();
		return;
	}

	state = State_Publish;
}

void Client::statePublishEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "statePingEvent");
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: statePublishEventSendDataOk(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: gotoStateReconnectClose(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
	}
}

void Client::statePublishEventSendDataOk() {
	LOG_INFO(LOG_MQTT, "statePublishEventSendDataOk");
	pool.free(publishRequest);
	publishRequest = NULL;
	gotoStatePubAck();
}

void Client::gotoStatePubAck() {
	LOG_INFO(LOG_MQTT, "gotoStatePubAck");
	recvBuf.clear();
	if(conn->recv(recvBuf.getData(), recvBuf.getSize()) == false) {
		LOG_ERROR(LOG_MQTT, "Recv failed");
		gotoStateReconnectClose();
		return;
	}

	waitingTimer->start(MQTT_SOCKET_TIMEOUT);
	state = State_PubAck;
}

void Client::statePubAckEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "statePubAckEvent");
	switch(event->getType()) {
		case TcpIp::Event_RecvDataOk: statePubAckEventRecvData(event->getUint16()); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: gotoStateReconnectClose(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
	}
}

void Client::statePubAckEventRecvData(uint32_t len) {
	LOG_INFO(LOG_MQTT, "statePubAckEventRecvData");
	recvBuf.setLen(len);
	LOG_DEBUG_HEX(LOG_MQTT, recvBuf.getData(), recvBuf.getLen());
	Marshaller* marshaller = encoder.readPacket(recvBuf);
	if(marshaller == nullptr) {
		LOG_ERROR(LOG_MQTT, "Nothing parse, break.");
		return;
	}

	switch(encoder.lastArrivedPacketType()) {
		case PacketType_PubAck: statePubAckPacketPubAck(); return;
		case PacketType_Publish: statePubAckPacketPublish(marshaller); return;
		default: LOG_ERROR(LOG_MQTT, "Unsupported packet type " << encoder.lastArrivedPacketType());
	}
}

void Client::statePubAckPacketPubAck() {
	LOG_INFO(LOG_MQTT, "statePubAckPacketPubAck");
	waitingTimer->stop();
	gotoStateWait();
	EventInterface event(deviceId, Mqtt::Event_PublishComplete);
	eventEngine->transmit(&event);
}

void Client::statePubAckPacketPublish(Marshaller *marshaller) {
	LOG_INFO(LOG_MQTT, "statePubAckPacketPublish");
	recvBuf.clear();
	if(conn->recv(recvBuf.getData(), recvBuf.getSize()) == false) {
		LOG_ERROR(LOG_MQTT, "Recv failed");
		gotoStateReconnectClose();
		return;
	}

	procResponsePublish(marshaller);
}

void Client::statePubAckTimeout() {
	LOG_INFO(LOG_MQTT, "statePubAckTimeout");
	gotoStateReconnectClose();
	EventInterface event(deviceId, Mqtt::Event_PublishError);
	eventEngine->transmit(&event);
}

void Client::gotoStateClose() {
	LOG_INFO(LOG_MQTT, "gotoStateClose");
	state = State_Idle;
}

void Client::stateCloseEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "stateCloseEvent");
	switch(event->getType()) {
		case TcpIp::Event_Close: stateCloseEventClose(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
	}
}

void Client::stateCloseEventClose() {
	LOG_DEBUG(LOG_MQTT, "stateCloseEventClose");
	state = State_Idle;
}

void Client::gotoStateReconnectClose() {
	LOG_DEBUG(LOG_MQTT, "gotoStateReconnectClose");
	conn->close();
	state = State_ReconnectClose;
}

void Client::stateReconnectCloseEvent(Event *event) {
	LOG_DEBUG(LOG_MQTT, "stateReconnectCloseEvent");
	switch(event->getType()) {
		case TcpIp::Event_Close: gotoStateReconnectDelay(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
	}
}

void Client::gotoStateReconnectDelay() {
	LOG_DEBUG(LOG_MQTT, "gotoStateReconnectDelay");
	waitingTimer->start(MQTT_TRY_CONNECT_DELAY);
	state = State_ReconnectDelay;
}

void Client::stateReconnectTimeout() {
	LOG_DEBUG(LOG_MQTT, "stateReconnectTimeout");
	gotoStateConnect();
}

}
#else
#include "include/MqttClient.h"
#include "MqttProtocol.h"

#include "common/logger/include/Logger.h"

#include <stdio.h>

namespace Mqtt {

Client::Client(
	TcpIp *conn,
	TimerEngine *timerEngine,
	EventEngineInterface *eventengine
) :
	conn(conn),
	timerEngine(timerEngine),
	eventEngine(eventengine),
	deviceId(eventEngine),
	publishRequest(NULL),
	incommingTopic(MQTT_TOPIC_SIZE, MQTT_TOPIC_SIZE),
	incommingData(MQTT_PACKET_SIZE)
{
	this->conn->setObserver(this);
	this->waitingTimer = timerEngine->addTimer<Client, &Client::procWaitTimer>(this);
	this->pingTimer = timerEngine->addTimer<Client, &Client::procPingTimer>(this);
}

bool Client::connect(
	const char *addr,
	uint16_t port,
	TcpIp::Mode mode,
	const char *userName,
	const char *passWord
) {
	LOG_INFO(LOG_MQTT, "connect");
	if(state != State_Idle) {
		LOG_ERROR(LOG_MQTT, "Wrong state " << state);
		return false;
	}

	this->addr = addr;
	this->port = port;
	this->mode = mode;
	this->userName = userName;
	this->passWord = passWord;
	encoder.setUsername(this->userName);
	encoder.setPassword(this->passWord);	
	return gotoStateConnect();
}

bool Client::subscribe(const char* topic) {
	LOG_INFO(LOG_MQTT, "subscribe to " << topic);
	encoder.registerTopic(topic);
	return true;
}

bool Client::publish(const char* topic, uint8_t *data, uint32_t dataLen, QoS qos) {
	LOG_INFO(LOG_MQTT, "publish");
	LOG_DEBUG(LOG_MQTT, "topic=" << topic << ", qos=" << qos);
	LOG_DEBUG_HEX(LOG_MQTT, data, dataLen);
	if(pool.pop(topic, data, dataLen, qos) == false) {
		LOG_ERROR(LOG_MQTT, "Pool is empty");
		return false;
	}

	if(state == State_Wait) {
		gotoStatePublish();
		return true;
	}

	return true;
}

void Client::disconnect() {
	LOG_INFO(LOG_MQTT, "disconnect");
	waitingTimer->stop();
	pingTimer->stop();
	conn->close();
}

void Client::proc(Event *event) {
	LOG_DEBUG(LOG_MQTT, "Event: state=" << state << ", event=" << event->getType());
	switch(state) {
		case State_Connect: stateConnectEvent(event); break;
		case State_SendConnect: stateSendConnectEvent(event); break;
		case State_Subscribe: stateSubscribeEvent(event); break;
		case State_Wait: stateWaitEvent(event); break;
		case State_Ping: statePingEvent(event); break;
		case State_RecvResponse: stateRecvResponseEvent(event); break;
		case State_Publish: statePublishEvent(event); break;
		case State_PubAck: statePubAckEvent(event); break;
		case State_Close: stateCloseEvent(event); break;
		case State_ReconnectClose: stateReconnectCloseEvent(event); break;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType());
	}
}

void Client::procWaitTimer() {
	LOG_INFO(LOG_MQTT, "Timeout of wait: state=" << state);
	switch(state) {
		case State_ReconnectDelay: stateReconnectTimeout(); return;
		case State_RecvResponse: gotoStateReconnectClose(); return;
		case State_PubAck: statePubAckTimeout(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited timeout " << state);
	}
}

void Client::procPingTimer() {
	LOG_INFO(LOG_MQTT, "Timeout of ping: state=" << state);
	switch(state) {
		case State_Wait: gotoStatePing(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited timeout " << state);
	}
}

bool Client::gotoStateConnect() {
	LOG_DEBUG(LOG_MQTT, "gotoStateConnect");
	if(this->conn->connect(this->addr, this->port, this->mode) == false) {
		LOG_ERROR(LOG_MQTT, "Connect failed");
		gotoStateReconnectClose();
		return false;
	}

	state = State_Connect;
	return true;
}

void Client::stateConnectEvent(Event *event) {
    LOG_INFO(LOG_MQTT, "stateConnectEvent");
    switch(event->getType()) {
		case TcpIp::Event_ConnectOk: gotoStateSendConnect(); return;
		case TcpIp::Event_ConnectError: gotoStateReconnectClose(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
    }
}

void Client::gotoStateSendConnect() {
	LOG_INFO(LOG_MQTT, "gotoStateSendConnect");
	encoder.marshall(PacketType_Connect, &sendBuf);
	if(this->conn->send(sendBuf.getData(), sendBuf.getLen()) == false) {
		LOG_ERROR(LOG_MQTT, "Send failed");
		gotoStateReconnectClose();
		return;
	}

	waitingTimer->stop();
	pingTimer->stop();
	state = State_SendConnect;
}

void Client::stateSendConnectEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "stateSendConnecEvent");
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: gotoStateRecvResponse(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: gotoStateReconnectClose(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
	}
}

void Client::gotoStateSubscribe() {
	LOG_INFO(LOG_MQTT, "gotoStateSubscribe");
	if(encoder.haveTopics() == false) {
		LOG_INFO(LOG_MQTT, "..haven't default topics");
		gotoStateWait();
		return;
	}

	encoder.marshall(PacketType_Subscribe, &sendBuf);
	if(this->conn->send(sendBuf.getData(), sendBuf.getLen()) == false) {
		LOG_ERROR(LOG_MQTT, "Send failed");
		gotoStateReconnectClose();
		return;
	}

	state = State_Subscribe;
}

void Client::stateSubscribeEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "stateSubScribeEvent");
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: gotoStateRecvResponse(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: gotoStateReconnectClose(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
	}
}

void Client::gotoStateWait() {
	LOG_INFO(LOG_MQTT, "gotoStateWait");
	if(publishRequest != NULL || pool.isEmpty() == false) {
		gotoStatePublish();
		return;
	}

	pingTimer->start(MQTT_PING_TIMEOUT);
	state = State_Wait;
}

void Client::stateWaitEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "stateWaitEvent");
	switch(event->getType()) {
		case TcpIp::Event_IncomingData: gotoStateRecvResponse(); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: gotoStateReconnectClose(); return;
		default: {
			LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType());
			return;
		}
	}
}

void Client::gotoStatePing() {
	LOG_INFO(LOG_MQTT, "gotoStatePing");
	encoder.marshall(PacketType_PingRec, &sendBuf);
	if(this->conn->send(sendBuf.getData(), sendBuf.getLen()) == false) {
		LOG_ERROR(LOG_MQTT, "Send failed");
		gotoStateReconnectClose();
		return;
	}

	state = State_Ping;
}

void Client::statePingEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "statePingEvent");
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: gotoStateRecvResponse(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: gotoStateReconnectClose(); return;
		default: {
			LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType());
			return;
		}
	}
}

void Client::gotoStateRecvResponse() {
	LOG_INFO(LOG_MQTT, "gotoStateRecvResponse");
	recvBuf.clear();
	if(conn->recv(recvBuf.getData(), recvBuf.getSize()) == false) {
		LOG_ERROR(LOG_MQTT, "Recv failed");
		gotoStateReconnectClose();
		return;
	}

	waitingTimer->start(MQTT_SOCKET_TIMEOUT);
	state = State_RecvResponse;
}

void Client::stateRecvResponseEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "stateRecvResponseEvent");
	switch(event->getType()) {
		case TcpIp::Event_RecvDataOk: stateRecvResponseEventRecvData(event->getUint16()); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: gotoStateReconnectClose(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
	}
}

void Client::stateRecvResponseEventRecvData(uint32_t len) {
	LOG_INFO(LOG_MQTT, "stateRecvResponseEventRecvData");
	recvBuf.setLen(len);
	LOG_DEBUG_HEX(LOG_MQTT, recvBuf.getData(), recvBuf.getLen());
	Marshaller* marshaller = encoder.readPacket(recvBuf);
	if(marshaller == nullptr) {
		LOG_ERROR(LOG_MQTT, "Nothing parse, break.");
		return;
	}

	waitingTimer->stop(); // remember kill timer

	switch(encoder.lastArrivedPacketType()) {
		case PacketType_ConnAck: gotoStateSubscribe(); return;
		case PacketType_SubAck: stateRecvResponsePacketSubAck(); return;
		case PacketType_PingResp: gotoStateWait(); return;
		case PacketType_Publish: stateRecvResponsePacketPublish(marshaller); return;
		default: LOG_ERROR(LOG_MQTT, "Unsupported packet type " << encoder.lastArrivedPacketType());
	}
}

void Client::stateRecvResponsePacketSubAck() {
	gotoStateWait();
	EventInterface event(deviceId, Event_ConnectComplete);
	eventEngine->transmit(&event);
#if defined (MQTT_TEST)
	((EventEngine*)eventEngine)->execute();
#endif
}

void Client::stateRecvResponsePacketPublish(Marshaller *marshaller) {
	LOG_INFO(LOG_MQTT, "stateRecvResponsePacketPublish");
	recvBuf.clear();
	if(conn->recv(recvBuf.getData(), recvBuf.getSize()) == false) {
		LOG_ERROR(LOG_MQTT, "Recv failed");
		gotoStateReconnectClose();
		return;
	}

	procResponsePublish(marshaller);
}

void Client::procResponsePublish(Marshaller *marshaller) {
	LOG_INFO(LOG_MQTT, "procResponsePublish");
	PublishMarshaller *publish = (PublishMarshaller*)marshaller;
	incommingTopic.set(publish->getTopic()->getString());
	incommingData.clear();
	incommingData.add(publish->getPayload()->getData(), publish->getPayload()->getLen());
	gotoStateWait();
	EventInterface event(deviceId, Mqtt::Event_IncommingMessage);
	eventEngine->transmit(&event);
}

void Client::gotoStatePublish() {
	LOG_INFO(LOG_MQTT, "gotoStatePublish");
	if(publishRequest == NULL) {
		publishRequest = pool.push();
		if(publishRequest == NULL) {
			gotoStateWait();
			return;
		}
	}

	encoder.publish(publishRequest->topic, publishRequest->data, publishRequest->dataLen, publishRequest->qos);
	encoder.marshall(PacketType_Publish, &sendBuf);
	if(this->conn->send(sendBuf.getData(), sendBuf.getLen()) == false) {
		LOG_ERROR(LOG_MQTT, "Send failed");
		gotoStateReconnectClose();
		return;
	}

	state = State_Publish;
}

void Client::statePublishEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "statePingEvent");
	switch(event->getType()) {
		case TcpIp::Event_SendDataOk: statePublishEventSendDataOk(); return;
		case TcpIp::Event_SendDataError:
		case TcpIp::Event_Close: gotoStateReconnectClose(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
	}
}

void Client::statePublishEventSendDataOk() {
	LOG_INFO(LOG_MQTT, "statePublishEventSendDataOk");
	publishRequest = NULL;
	gotoStatePubAck();
}

void Client::gotoStatePubAck() {
	LOG_INFO(LOG_MQTT, "gotoStatePubAck");
	recvBuf.clear();
	if(conn->recv(recvBuf.getData(), recvBuf.getSize()) == false) {
		LOG_ERROR(LOG_MQTT, "Recv failed");
		gotoStateReconnectClose();
		return;
	}

	waitingTimer->start(MQTT_SOCKET_TIMEOUT);
	state = State_PubAck;
}

void Client::statePubAckEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "statePubAckEvent");
	switch(event->getType()) {
		case TcpIp::Event_RecvDataOk: statePubAckEventRecvData(event->getUint16()); return;
		case TcpIp::Event_RecvDataError:
		case TcpIp::Event_Close: gotoStateReconnectClose(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
	}
}

void Client::statePubAckEventRecvData(uint32_t len) {
	LOG_INFO(LOG_MQTT, "statePubAckEventRecvData");
	recvBuf.setLen(len);
	LOG_DEBUG_HEX(LOG_MQTT, recvBuf.getData(), recvBuf.getLen());
	Marshaller* marshaller = encoder.readPacket(recvBuf);
	if(marshaller == nullptr) {
		LOG_ERROR(LOG_MQTT, "Nothing parse, break.");
		return;
	}

	switch(encoder.lastArrivedPacketType()) {
		case PacketType_PubAck: statePubAckPacketPubAck(); return;
		case PacketType_Publish: statePubAckPacketPublish(marshaller); return;
		default: LOG_ERROR(LOG_MQTT, "Unsupported packet type " << encoder.lastArrivedPacketType());
	}
}

void Client::statePubAckPacketPubAck() {
	LOG_INFO(LOG_MQTT, "statePubAckPacketPubAck");
	waitingTimer->stop();
	gotoStateWait();
	EventInterface event(deviceId, Mqtt::Event_PublishComplete);
	eventEngine->transmit(&event);
}

void Client::statePubAckPacketPublish(Marshaller *marshaller) {
	LOG_INFO(LOG_MQTT, "statePubAckPacketPublish");
	recvBuf.clear();
	if(conn->recv(recvBuf.getData(), recvBuf.getSize()) == false) {
		LOG_ERROR(LOG_MQTT, "Recv failed");
		gotoStateReconnectClose();
		return;
	}

	procResponsePublish(marshaller);
}

void Client::statePubAckTimeout() {
	LOG_INFO(LOG_MQTT, "statePubAckTimeout");
	gotoStateReconnectClose();
	EventInterface event(deviceId, Mqtt::Event_PublishError);
	eventEngine->transmit(&event);
}

void Client::gotoStateClose() {
	LOG_INFO(LOG_MQTT, "gotoStateClose");
	state = State_Idle;
}

void Client::stateCloseEvent(Event *event) {
	LOG_INFO(LOG_MQTT, "stateCloseEvent");
	switch(event->getType()) {
		case TcpIp::Event_Close: stateCloseEventClose(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
	}
}

void Client::stateCloseEventClose() {
	LOG_DEBUG(LOG_MQTT, "stateCloseEventClose");
	state = State_Idle;
}

void Client::gotoStateReconnectClose() {
	LOG_DEBUG(LOG_MQTT, "gotoStateReconnectClose");
	conn->close();
	state = State_ReconnectClose;
}

void Client::stateReconnectCloseEvent(Event *event) {
	LOG_DEBUG(LOG_MQTT, "stateReconnectCloseEvent");
	switch(event->getType()) {
		case TcpIp::Event_Close: gotoStateReconnectDelay(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited event: state=" << state << ", event=" << event->getType()); return;
	}
}

void Client::gotoStateReconnectDelay() {
	LOG_DEBUG(LOG_MQTT, "gotoStateReconnectDelay");
	waitingTimer->start(MQTT_TRY_CONNECT_DELAY);
	state = State_ReconnectDelay;
}

void Client::stateReconnectTimeout() {
	LOG_DEBUG(LOG_MQTT, "stateReconnectTimeout");
	gotoStateConnect();
}

}
#endif
