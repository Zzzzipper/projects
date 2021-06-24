#include "MqttPacketLayer.h"

#include "common/logger/include/Logger.h"

namespace Mqtt {

PacketLayer::PacketLayer(
	TcpIp *conn,
	TimerEngine *timerEngine,
	EventEngineInterface *eventengine
) :
	conn(conn),
	timerEngine(timerEngine),
	eventEngine(eventengine),
	state(State_Idle)
{
//	this->conn->setObserver(this);
	this->timer = timerEngine->addTimer<PacketLayer, &PacketLayer::procTimer>(this);
//	this->pingTimer = timerEngine->addTimer<Client, &Client::procPingTimer>(this);
}

bool PacketLayer::connect(const char *addr, uint16_t port) {
	return false;
}

bool PacketLayer::send(const void *data, uint16_t dataLen) {
	return false;
}

void PacketLayer::procTimer() {
	LOG_INFO(LOG_MQTT, "procTimer " << state);
	switch(state) {
//		case State_ReconnectDelay: stateReconnectTimeout(); return;
//		case State_RecvResponse: gotoStateReconnectClose(); return;
//		case State_PubAck: statePubAckTimeout(); return;
		default: LOG_ERROR(LOG_MQTT, "Unwaited timeout " << state);
	}
}

}
