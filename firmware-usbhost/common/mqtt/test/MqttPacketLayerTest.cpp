#include "test/include/Test.h"
#include "mqtt/MqttPacketLayer.h"
#include "mqtt/MqttProtocol.h"
#include "http/test/TestTcpIp.h"
#include "timer/include/TimerEngine.h"
#include "event/include/TestEventEngine.h"
#include "utils/include/StringParser.h"
#include "utils/include/Buffer.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace Mqtt {

class TestMqttClientEventEngine : public TestEventEngine {
public:
	TestMqttClientEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
/*		case Mqtt::Event_ConnectComplete: procEvent(envelope, Mqtt::Event_ConnectComplete, "ConnectComplete"); break;
		case Mqtt::Event_PublishComplete: procEvent(envelope, Mqtt::Event_PublishComplete, "PublishComplete"); break;
		case Mqtt::Event_PublishError: procEvent(envelope, Mqtt::Event_PublishError, "PublishError"); break;
		case Mqtt::Event_IncommingMessage: procEvent(envelope, Mqtt::Event_IncommingMessage, "IncommingMessage"); break;*/
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}
};

class PacketLayerTest : public TestSet {
public:
	PacketLayerTest();
	bool init();
	void cleanup();
	bool test();

private:
	StringBuilder *result;
	TimerEngine *timerEngine;
	TestMqttClientEventEngine *eventEngine;
	TestTcpIp *tcpIp = nullptr;
	PacketLayer *layer;
};

TEST_SET_REGISTER(Mqtt::PacketLayerTest);

PacketLayerTest::PacketLayerTest() {
	TEST_CASE_REGISTER(PacketLayerTest, test);
}

bool PacketLayerTest::init() {
	result = new StringBuilder;
	tcpIp = new TestTcpIp(1024, result, false);
	timerEngine = new TimerEngine;
	eventEngine = new TestMqttClientEventEngine(result);
	layer = new PacketLayer(tcpIp, timerEngine, eventEngine);
	return true;
}

void PacketLayerTest::cleanup() {
	delete layer;
	delete tcpIp;
	delete timerEngine;
	delete result;
}

bool PacketLayerTest::test() {
	layer->connect("127.0.0.1", 1234);

	// recv packet


	// send data
	Buffer data1(32);
	TEST_NUMBER_NOT_EQUAL(0, hexToData("0013c00c001d440a00d30000000000000001000000dbd5", &data1));
	layer->send(data1.getData(), data1.getLen());
	TEST_STRING_EQUAL("12345", result->getString());

/*    char addr[] = "95.216.78.99";
	uint16_t port = 1883;
	char username[] = "ephor";
	char password[] = "2kDR8TMCu5dhiN2";
	TEST_NUMBER_EQUAL(true, client->connect(addr, port, TcpIp::Mode_TcpIp, username, password));
	TEST_STRING_EQUAL("<connect:95.216.78.99,1883,0>", result->getString());
	result->clear();

	// connected
	Event event(TcpIp::Event_ConnectOk);
	client->proc(&event);

	// send connect request
	TEST_STRING_EQUAL("<send=102B00044D51545404C2006400072F3A6570686F7200056570686F72000F326B445238544D4375356468694E32,len=45>", result->getString());
	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);

	// connect response
	tcpIp->incommingData();
	tcpIp->addRecvData("20020000", false);

	// send subscriptions request
	TEST_STRING_EQUAL("<recv=512><send=820F0001000A2F616D712F746F70696300,len=17>", result->getString());
	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);

	// subscriptions response
	tcpIp->incommingData();
	tcpIp->addRecvData("900400010000", false);
	TEST_STRING_EQUAL("<recv=512><event=1,ConnectComplete>", result->getString());
	result->clear();

	// ping request
	timerEngine->tick(MQTT_PING_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("<send=C000,len=2>", result->getString());
	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);

	// ping response
	tcpIp->incommingData();
	tcpIp->addRecvData("D000", false);
	TEST_STRING_EQUAL("<recv=512>", result->getString());
	result->clear();

	// ping request
	timerEngine->tick(MQTT_PING_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("<send=C000,len=2>", result->getString());
	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);

	// ping response
	tcpIp->incommingData();
	tcpIp->addRecvData("D000", false);
	TEST_STRING_EQUAL("<recv=512>", result->getString());
	result->clear();

	// publish request
	const char publish1[] = "data1";
	TEST_NUMBER_EQUAL(true, client->publish("topic1", (uint8_t*)publish1, strlen(publish1), QoS::QoS_0));
	TEST_STRING_EQUAL("<send=300D0006746F706963316461746131,len=15>", result->getString());
	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);

	// publish response
	tcpIp->incommingData();
	tcpIp->addRecvData("D000", false);
	TEST_STRING_EQUAL("<recv=512>", result->getString());
	result->clear();

	// publish ack
	tcpIp->incommingData();
	tcpIp->addRecvData("40020001", false);
	TEST_STRING_EQUAL("<recv=512>", result->getString());
	result->clear();

	// ping request
	timerEngine->tick(MQTT_PING_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("<send=C000,len=2>", result->getString());
	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);

	// ping response
	tcpIp->incommingData();
	tcpIp->addRecvData("D000", false);
	TEST_STRING_EQUAL("<recv=512>", result->getString());
	result->clear();

	// ping request
	timerEngine->tick(MQTT_PING_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("<send=C000,len=2>", result->getString());
	result->clear();
	event.set(TcpIp::Event_SendDataOk);
	client->proc(&event);

	// ping response
	tcpIp->incommingData();
	tcpIp->addRecvData("D000", false);
	TEST_STRING_EQUAL("<recv=512>", result->getString());
	result->clear();

	// incomming message
	tcpIp->incommingData();
	tcpIp->addRecvData(
			"305B0054312F73746F72"
			"652F444154412F6B6974"
			"6368656E2F61736B5F66"
			"6F725F7468655F50494E"
			"2F76312F414E592F414E"
			"592F6362653038656330"
			"2D653562642D34616631"
			"2D383863392D35373862"
			"39643063653666317465"
			"737431", false);
	TEST_STRING_EQUAL("<recv=512><event=1,IncommingMessage>", result->getString()); // send incomming message (publish) ask?
	result->clear();*/
	return true;
}

}
