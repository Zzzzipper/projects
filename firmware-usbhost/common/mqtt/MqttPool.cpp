#include "MqttPool.h"

namespace Mqtt {

#define MQTT_POOL_SIZE 5

Pool::Pool() :
	pool(MQTT_POOL_SIZE),
	list(MQTT_POOL_SIZE)
{
	for(uint16_t i = 0; i < MQTT_POOL_SIZE; i++) {
		PublishRequest *request = new PublishRequest;
		pool.push(request);
	}
}

Pool::~Pool() {

}

bool Pool::push(const char* topic, const uint8_t *data, uint32_t dataLen, QoS qos) {
	PublishRequest *req = pool.pop();
	if(req == NULL) { return false; }
	req->topic.set(topic);
	req->data.clear();
	req->data.add(data, dataLen);
	req->qos = qos;
	list.push(req);
	return true;
}

PublishRequest *Pool::pop() {
	return list.pop();
}

void Pool::free(PublishRequest *req) {
	pool.push(req);
}

bool Pool::isEmpty() {
	return list.isEmpty();
}

}
