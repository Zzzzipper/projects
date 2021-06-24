#if 1
#ifndef COMMON_MQTT_CLIENT_H
#define COMMON_MQTT_CLIENT_H

#include "common/tcpip/include/TcpIp.h"
#include "common/timer/include/TimerEngine.h"
#include "common/event/include/EventEngine.h"
#include "mqtt/MqttEncoder.h"
#include "mqtt/MqttPool.h"

namespace Mqtt {

enum EventType {
	Event_ConnectComplete	 = GlobalId_MqttClient | 0x01,
	Event_PublishComplete	 = GlobalId_MqttClient | 0x02,
	Event_PublishError		 = GlobalId_MqttClient | 0x03,
	Event_IncommingMessage	 = GlobalId_MqttClient | 0x04,
};

/**
 * @brief The Client class
 */
class Client : public EventObserver {
public:
	Client(TcpIp *conn, TimerEngine *timerEngine, EventEngineInterface *eventengine);

	// Подключение сокетом. Фатк подключения не означает, что канал живой.
	// Этот факт подтверждается событием после после получения аккредитации
	// от сервера
	bool connect(const char *addr, uint16_t port, TcpIp::Mode mode,
//			const char *clientId, const char *username = nullptr, const char *password = nullptr);
			const char *clientId, const char *username, const char *password);
	// Подписка на топик для получения из него информации
	bool subscribe(const char* topic);
	// Публикация в топик
	bool publish(const char* topic, uint8_t *data, uint32_t dataLen, QoS qos);
	// Правильное отключение с подтверждением отписки (не сделано)
	void disconnect();
	String *getIncommingTopic() { return &incommingTopic; }
	Buffer *getIncommingData() { return &incommingData; }

	void proc(Event *) override;
	void procWaitTimer();
	void procPingTimer();

private:
	enum State {
		State_Idle = 0,
		State_Connect,
		State_SendConnect,
		State_Subscribe,
		State_Wait,
		State_Ping,
		State_RecvResponse,
		State_Publish,
		State_PubAck,
		State_Close,
		State_ReconnectClose,
		State_ReconnectDelay,
	};

	TcpIp *conn = nullptr;
	TimerEngine *timerEngine = nullptr;
	Timer *waitingTimer = nullptr;
	Timer *pingTimer = nullptr;
	EventEngineInterface *eventEngine = nullptr;
	EventDeviceId deviceId;
	Encoder encoder;
	Pool pool;
	State state = State_Idle;
	PublishRequest *publishRequest;
	Buffer sendBuf = MQTT_PACKET_SIZE;
	Buffer recvBuf = MQTT_PACKET_SIZE;
	String incommingTopic;
	Buffer incommingData;
	EventCourier courier;
	const char *addr;
	uint16_t port;
	TcpIp::Mode mode;
	const char *userName;
	const char *passWord;

	bool gotoStateConnect();
	void stateConnectEvent(Event *event);

	void gotoStateSendConnect();
	void stateSendConnectEvent(Event *event);

	void gotoStateSubscribe();
	void stateSubscribeEvent(Event *event);

	void gotoStateWait();
	void stateWaitEvent(Event *event);

	void gotoStatePing();
	void statePingEvent(Event *event);

	void gotoStateRecvResponse();
	void stateRecvResponseEvent(Event *event);
	void stateRecvResponseEventRecvData(uint32_t len);
	void stateRecvResponsePacketSubAck();
	void stateRecvResponsePacketPingResp();
	void stateRecvResponsePacketPublish(Marshaller *marshaller);
	void procResponsePublish(Marshaller *marshaller);

	void gotoStatePublish();
	void statePublishEvent(Event*);
	void statePublishEventSendDataOk();

	void gotoStatePubAck();
	void statePubAckEvent(Event *event);
	void statePubAckEventRecvData(uint32_t len);
	void statePubAckPacketPubAck();
	void statePubAckPacketPublish(Marshaller *marshaller);
	void statePubAckTimeout();

	void gotoStateClose();
	void stateCloseEvent(Event *event);
	void stateCloseEventClose();

	void gotoStateReconnectClose();
	void stateReconnectCloseEvent(Event *event);

	void gotoStateReconnectDelay();
	void stateReconnectTimeout();
};

}

#endif
#else
#ifndef COMMON_MQTT_CLIENT_H
#define COMMON_MQTT_CLIENT_H

#include "common/http/include/TcpIp.h"
#include "common/timer/include/TimerEngine.h"
#include "common/event/include/EventEngine.h"
#include "mqtt/MqttEncoder.h"
#include "mqtt/MqttPool.h"

namespace Mqtt {

enum EventType {
	Event_ConnectComplete	 = GlobalId_MqttClient | 0x01,
	Event_PublishComplete	 = GlobalId_MqttClient | 0x02,
	Event_PublishError		 = GlobalId_MqttClient | 0x03,
	Event_IncommingMessage	 = GlobalId_MqttClient | 0x04,
};

/**
 * @brief The Client class
 */
class Client : public EventObserver {
public:
    Client(TcpIp *conn, TimerEngine *timerEngine, EventEngineInterface *eventengine);

    // Подключение сокетом. Фатк подключения не означает, что канал живой.
    // Этот факт подтверждается событием после после получения аккредитации
    // от сервера
	bool connect(const char *addr, uint16_t port, TcpIp::Mode mode,
				 const char *username = nullptr, const char *password = nullptr);
    // Подписка на топик для получения из него информации
    bool subscribe(const char* topic);
    // Публикация в топик
    bool publish(const char* topic, uint8_t *data, uint32_t dataLen, QoS qos);
	// Правильное отключение с подтверждением отписки (не сделано)
    void disconnect();
	String *getIncommingTopic() { return &incommingTopic; }
	Buffer *getIncommingData() { return &incommingData; }

	void proc(Event *) override;
	void procWaitTimer();
	void procPingTimer();

private:
    enum State {
        State_Idle = 0,
        State_Connect,
        State_SendConnect,
		State_Subscribe,
		State_Wait,
		State_Ping,
        State_RecvResponse,
		State_Publish,
		State_PubAck,
        State_Close,
		State_ReconnectClose,
		State_ReconnectDelay,
    };

    TcpIp *conn = nullptr;
    TimerEngine *timerEngine = nullptr;
    Timer *waitingTimer = nullptr;
    Timer *pingTimer = nullptr;
    EventEngineInterface *eventEngine = nullptr;
    EventDeviceId deviceId;
    Encoder encoder;
	Pool pool;
    State state = State_Idle;
	PublishRequest *publishRequest;
	Buffer sendBuf = MQTT_PACKET_SIZE;
	Buffer recvBuf = MQTT_PACKET_SIZE;
	String incommingTopic;
	Buffer incommingData;
    EventCourier courier;
    const char *addr;
    uint16_t port;
    TcpIp::Mode mode;
    const char *userName;
    const char *passWord;

	bool gotoStateConnect();
	void stateConnectEvent(Event *event);

	void gotoStateSendConnect();
	void stateSendConnectEvent(Event *event);

	void gotoStateSubscribe();
	void stateSubscribeEvent(Event *event);

	void gotoStateWait();
	void stateWaitEvent(Event *event);

	void gotoStatePing();
	void statePingEvent(Event *event);

	void gotoStateRecvResponse();
	void stateRecvResponseEvent(Event *event);
	void stateRecvResponseEventRecvData(uint32_t len);
	void stateRecvResponsePacketSubAck();
	void stateRecvResponsePacketPublish(Marshaller *marshaller);
	void procResponsePublish(Marshaller *marshaller);

	void gotoStatePublish();
	void statePublishEvent(Event*);
	void statePublishEventSendDataOk();

	void gotoStatePubAck();
	void statePubAckEvent(Event *event);
	void statePubAckEventRecvData(uint32_t len);
	void statePubAckPacketPubAck();
	void statePubAckPacketPublish(Marshaller *marshaller);
	void statePubAckTimeout();

    void gotoStateClose();
	void stateCloseEvent(Event *event);
	void stateCloseEventClose();

	void gotoStateReconnectClose();
	void stateReconnectCloseEvent(Event *event);

	void gotoStateReconnectDelay();
	void stateReconnectTimeout();
};

}

#endif
#endif
