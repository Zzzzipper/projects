#ifndef COMMON_MQTT_PACKETLAYER_H
#define COMMON_MQTT_PACKETLAYER_H

#include "common/tcpip/include/TcpIp.h"
#include "common/timer/include/TimerEngine.h"
#include "common/event/include/EventEngine.h"

namespace Mqtt {

class PacketLayer {
public:
	PacketLayer(TcpIp *conn, TimerEngine *timerEngine, EventEngineInterface *eventengine);
	bool connect(const char *addr, uint16_t port);
	bool send(const void *data, uint16_t dataLen);
	void procTimer();

private:
	enum State {
		State_Idle = 0,
	};

	TcpIp *conn;
	TimerEngine *timerEngine;
	Timer *timer;
	EventEngineInterface *eventEngine;
	State state;
};

}

#endif
