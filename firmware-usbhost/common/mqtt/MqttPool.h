#ifndef COMMON_MQTT_POOL_H
#define COMMON_MQTT_POOL_H

#include "MqttProtocol.h"

#include "utils/include/Fifo.h"

namespace Mqtt {

class PublishRequest {
public:
	String topic;
	Buffer data;
	QoS qos;

	PublishRequest() : topic(MQTT_TOPIC_SIZE, MQTT_TOPIC_SIZE), data(MQTT_PACKET_SIZE) {}
};

class Pool {
public:
	Pool();
	~Pool();
	bool push(const char* topic, const uint8_t *data, uint32_t dataLen, QoS qos);
	PublishRequest *pop();
	void free(PublishRequest *req);
	bool isEmpty();

private:
	Fifo<PublishRequest*> pool;
	Fifo<PublishRequest*> list;
};

}

#endif
