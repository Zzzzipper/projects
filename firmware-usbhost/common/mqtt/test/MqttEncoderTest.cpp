#include "test/include/Test.h"
#include "mqtt/MqttEncoder.h"
#include "utils/include/Buffer.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace Mqtt {

class EncoderTest : public TestSet {
public:
	EncoderTest();
    bool init();
    void cleanup();
	bool testMakeLength();
	bool testParseLength();
	bool testMakeConnectRequest();
	bool testMakeSubscribeRequest();
	bool testMakePublishRequest();
	bool testParsePublishRequest();
	bool testMultiPublishRequest();

private:
};

TEST_SET_REGISTER(Mqtt::EncoderTest);

EncoderTest::EncoderTest() {
	TEST_CASE_REGISTER(EncoderTest, testMakeLength);
	TEST_CASE_REGISTER(EncoderTest, testParseLength);
	TEST_CASE_REGISTER(EncoderTest, testMakeConnectRequest);
	TEST_CASE_REGISTER(EncoderTest, testMakeSubscribeRequest);
	TEST_CASE_REGISTER(EncoderTest, testMakePublishRequest);
	TEST_CASE_REGISTER(EncoderTest, testParsePublishRequest);
	TEST_CASE_REGISTER(EncoderTest, testMultiPublishRequest);
}

bool EncoderTest::init() {
    return true;
}

void EncoderTest::cleanup() {
}

bool EncoderTest::testMakeLength() {
	Buffer sent(32);
	Length::make(100, &sent);
	TEST_HEXDATA_EQUAL("64", sent.getData(), sent.getLen());

	sent.clear();
	Length::make(200, &sent);
	TEST_HEXDATA_EQUAL("C801", sent.getData(), sent.getLen());

	sent.clear();
	Length::make(4000000, &sent);
	TEST_HEXDATA_EQUAL("8092F401", sent.getData(), sent.getLen());

	// примеры из документации MQTT 3.1.1
	sent.clear();
	Length::make(0, &sent);
	TEST_HEXDATA_EQUAL("00", sent.getData(), sent.getLen());

	sent.clear();
	Length::make(127, &sent);
	TEST_HEXDATA_EQUAL("7F", sent.getData(), sent.getLen());

	sent.clear();
	Length::make(128, &sent);
	TEST_HEXDATA_EQUAL("8001", sent.getData(), sent.getLen());

	sent.clear();
	Length::make(16383, &sent);
	TEST_HEXDATA_EQUAL("FF7F", sent.getData(), sent.getLen());

	sent.clear();
	Length::make(16384, &sent);
	TEST_HEXDATA_EQUAL("808001", sent.getData(), sent.getLen());

	sent.clear();
	Length::make(2097151, &sent);
	TEST_HEXDATA_EQUAL("FFFF7F", sent.getData(), sent.getLen());

	sent.clear();
	Length::make(2097152, &sent);
	TEST_HEXDATA_EQUAL("80808001", sent.getData(), sent.getLen());

	sent.clear();
	Length::make(268435455, &sent);
	TEST_HEXDATA_EQUAL("FFFFFF7F", sent.getData(), sent.getLen());
	return true;
}

bool EncoderTest::testParseLength() {
	Buffer recv(32);
	uint32_t length;
	TEST_NUMBER_EQUAL(1, hexToData("64", &recv));
	TEST_NUMBER_EQUAL(1, Length::parse(recv.getData(), recv.getLen(), &length));
	TEST_NUMBER_EQUAL(100, length);

	TEST_NUMBER_EQUAL(2, hexToData("C801", &recv));
	TEST_NUMBER_EQUAL(2, Length::parse(recv.getData(), recv.getLen(), &length));
	TEST_NUMBER_EQUAL(200, length);

	TEST_NUMBER_EQUAL(4, hexToData("8092F401", &recv));
	TEST_NUMBER_EQUAL(4, Length::parse(recv.getData(), recv.getLen(), &length));
	TEST_NUMBER_EQUAL(4000000, length);

	// примеры из документации MQTT 3.1.1
	TEST_NUMBER_EQUAL(1, hexToData("00", &recv));
	TEST_NUMBER_EQUAL(1, Length::parse(recv.getData(), recv.getLen(), &length));
	TEST_NUMBER_EQUAL(0, length);

	TEST_NUMBER_EQUAL(1, hexToData("7F", &recv));
	TEST_NUMBER_EQUAL(1, Length::parse(recv.getData(), recv.getLen(), &length));
	TEST_NUMBER_EQUAL(127, length);

	TEST_NUMBER_EQUAL(2, hexToData("8001", &recv));
	TEST_NUMBER_EQUAL(2, Length::parse(recv.getData(), recv.getLen(), &length));
	TEST_NUMBER_EQUAL(128, length);

	TEST_NUMBER_EQUAL(2, hexToData("FF7F", &recv));
	TEST_NUMBER_EQUAL(2, Length::parse(recv.getData(), recv.getLen(), &length));
	TEST_NUMBER_EQUAL(16383, length);

	TEST_NUMBER_EQUAL(3, hexToData("808001", &recv));
	TEST_NUMBER_EQUAL(3, Length::parse(recv.getData(), recv.getLen(), &length));
	TEST_NUMBER_EQUAL(16384, length);

	TEST_NUMBER_EQUAL(3, hexToData("FFFF7F", &recv));
	TEST_NUMBER_EQUAL(3, Length::parse(recv.getData(), recv.getLen(), &length));
	TEST_NUMBER_EQUAL(2097151, length);

	TEST_NUMBER_EQUAL(4, hexToData("80808001", &recv));
	TEST_NUMBER_EQUAL(4, Length::parse(recv.getData(), recv.getLen(), &length));
	TEST_NUMBER_EQUAL(2097152, length);

	TEST_NUMBER_EQUAL(4, hexToData("FFFFFF7F", &recv));
	TEST_NUMBER_EQUAL(4, Length::parse(recv.getData(), recv.getLen(), &length));
	TEST_NUMBER_EQUAL(268435455, length);
	return true;
}

bool EncoderTest::testMakeConnectRequest() {
	Mqtt::Encoder encoder;
	encoder.setUsername("ephor");
	encoder.setPassword("2kDR8TMCu5dhiN2");

	Buffer sent(512);
	sent.addUint8('M');
	encoder.marshall(PacketType_Connect, &sent);
	TEST_HEXDATA_EQUAL(
		"10"
		"2B00044D51545404C2"
		"006400072F3A6570686F"
		"7200056570686F72000F"
		"326B445238544D437535"
		"6468694E32",
		sent.getData(), sent.getLen());
	return true;
}

bool EncoderTest::testMakeSubscribeRequest() {
	Mqtt::Encoder encoder;
	encoder.setUsername("ephor");
	encoder.setPassword("2kDR8TMCu5dhiN2");

	Buffer sent(512);
	sent.addUint8('M');
	encoder.registerTopic("topic1");
	encoder.registerTopic("topic2");
	encoder.marshall(PacketType_Subscribe, &sent);
	TEST_HEXDATA_EQUAL(
		"82" // header
		"21" // length
		"0001" // packet id
		"000A" // topic length
		"2F616D712F746F706963" // amq/topic
		"00" // topic qos
		"0006" // topic length
		"746F70696331" // topic1
		"00" // topic qos
		"0006" // topic length
		"746F70696332" // topic2
		"00", // topic qos
		sent.getData(), sent.getLen());
	return true;
}

bool EncoderTest::testMakePublishRequest() {
	Mqtt::Encoder encoder;
	encoder.setUsername("ephor");
	encoder.setPassword("2kDR8TMCu5dhiN2");

	const char *topic = "1/store/DATA/kitchen/request_face_recognize/v1/ANY/ANY/cbe08ec0-e5bd-4af1-88c9-578b9d0ce6f1";
	String payload;
	payload << "{\"type\": \"kitchen.request_face_recognize\", \"payload\": { \"deviceId\": \"1\"}}";
	QoS qos = Mqtt::QoS_1;
	encoder.publish(topic, payload.getData(), payload.getLen(), qos);
	Buffer sent(512);
	sent.addUint8('M');
	encoder.marshall(PacketType_Publish, &sent);
	TEST_HEXDATA_EQUAL(
		"32" // header (packet type)
		"A801" // length
		"005B" // topic size
		"312F73746F72652F4441" // topic value
		"54412F6B69746368656E"
		"2F726571756573745F66"
		"6163655F7265636F676E"
		"697A652F76312F414E59"
		"2F414E592F6362653038"
		"6563302D653562642D34"
		"6166312D383863392D35"
		"37386239643063653666"
		"31"
		"0001" // packet id
		"7B2274797065223A2022" // payload value
		"6B69746368656E2E7265"
		"71756573745F66616365"
		"5F7265636F676E697A65"
		"222C20227061796C6F61"
		"64223A207B2022646576"
		"6963654964223A202231"
		"227D7D",
		sent.getData(), sent.getLen());
	return true;
}

/*
x30;xA6;x01;x00;x5B;x31;x2F;x73;x74;x6F;x72;x65;x2F;x44;x41;x54;x41;x2F;x6B;x69;x74;x63;x68;x65;x6E;x2F;x72;x65;x71;x75;x65;x73;x74;x5F;x66;x61;x63;x65;x5F;x72;x65;x63;x6F;x67;x6E;x69;x7A;x65;x2F;x76;x31;x2F;x41;x4E;x59;x2F;x41;x4E;x59;x2F;x63;x62;x65;x30;x38;x65;x63;x30;x2D;x65;x35;x62;x64;x2D;x34;x61;x66;x31;x2D;x38;x38;x63;x39;x2D;x35;x37;x38;x62;x39;x64;x30;x63;x65;x36;x66;x31;x7B;x22;x74;x79;x70;x65;x22;x3A;x20;x22;x6B;x69;x74;x63;x68;x65;x6E;x2E;x72;x65;x71;x75;x65;x73;x74;x5F;x66;x61;x63;x65;x5F;x72;x65;x63;x6F;x67;x6E;x69;x7A;x65;x22;x2C;x20;x22;x70;x61;x79;x6C;x6F;x61;x64;x22;x3A;x20;x7B;x20;x22;x64;x65;x76;x69;x63;x65;x49;x64;x22;x3A;x20;x22;x31;x22;x7D;x7D;
 */
bool EncoderTest::testParsePublishRequest() {
	Buffer recvData(512);
	TEST_NUMBER_EQUAL(169, hexToData(
			"30A601005B312F73746F"
			"72652F444154412F6B69"
			"746368656E2F72657175"
			"6573745F666163655F72"
			"65636F676E697A652F76"
			"312F414E592F414E592F"
			"63626530386563302D65"
			"3562642D346166312D38"
			"3863392D353738623964"
			"3063653666317B227479"
			"7065223A20226B697463"
			"68656E2E726571756573"
			"745F666163655F726563"
			"6F676E697A65222C2022"
			"7061796C6F6164223A20"
			"7B202264657669636549"
			"64223A202231227D7D",
			&recvData));

	Mqtt::Encoder encoder;
	Marshaller *m = encoder.readPacket(recvData);
//	TEST_NUMBER_NOT_EQUAL(0, (int)m);

	TEST_NUMBER_EQUAL(PacketType_Publish, encoder.lastArrivedPacketType());
	PublishMarshaller *publish = (PublishMarshaller*)m;
	TEST_STRING_EQUAL("1/store/DATA/kitchen/request_face_recognize/v1/ANY/ANY/cbe08ec0-e5bd-4af1-88c9-578b9d0ce6f1", publish->getTopic()->getString());
	TEST_HEXDATA_EQUAL("7B2274797065223A20226B69746368656E2E726571756573745F666163655F7265636F676E697A65222C20227061796C6F6164223A207B20226465766963654964223A202231227D7D", publish->getPayload()->getData(), publish->getPayload()->getLen());

	TEST_NUMBER_EQUAL(171, hexToData(
			"32" // header (packet type)
			"A801" // length
			"005B" // topic size
			"312F73746F72652F4441" // topic value
			"54412F6B69746368656E"
			"2F726571756573745F66"
			"6163655F7265636F676E"
			"697A652F76312F414E59"
			"2F414E592F6362653038"
			"6563302D653562642D34"
			"6166312D383863392D35"
			"37386239643063653666"
			"31"
			"0001" // packet id
			"7B2274797065223A2022" // payload value
			"6B69746368656E2E7265"
			"71756573745F66616365"
			"5F7265636F676E697A65"
			"222C20227061796C6F61"
			"64223A207B2022646576"
			"6963654964223A202231"
			"227D7D",
			&recvData));

	Marshaller *m2 = encoder.readPacket(recvData);
//	TEST_NUMBER_NOT_EQUAL(0, (int)m2);

	TEST_NUMBER_EQUAL(PacketType_Publish, encoder.lastArrivedPacketType());
	PublishMarshaller *publish2 = (PublishMarshaller*)m2;
	TEST_STRING_EQUAL("1/store/DATA/kitchen/request_face_recognize/v1/ANY/ANY/cbe08ec0-e5bd-4af1-88c9-578b9d0ce6f1", publish2->getTopic()->getString());
	TEST_HEXDATA_EQUAL("7B2274797065223A20226B69746368656E2E726571756573745F666163655F7265636F676E697A65222C20227061796C6F6164223A207B20226465766963654964223A202231227D7D", publish2->getPayload()->getData(), publish2->getPayload()->getLen());
	return true;
}

bool EncoderTest::testMultiPublishRequest() {
	Buffer recvData(512);
	TEST_NUMBER_EQUAL(169, hexToData(
			"30A601005B312F73746F"
			"72652F444154412F6B69"
			"746368656E2F72657175"
			"6573745F666163655F72"
			"65636F676E697A652F76"
			"312F414E592F414E592F"
			"63626530386563302D65"
			"3562642D346166312D38"
			"3863392D353738623964"
			"3063653666317B227479"
			"7065223A20226B697463"
			"68656E2E726571756573"
			"745F666163655F726563"
			"6F676E697A65222C2022"
			"7061796C6F6164223A20"
			"7B202264657669636549"
			"64223A202231227D7D",
			&recvData));

	Mqtt::Encoder encoder;
	Marshaller *m = encoder.readPacket(recvData);
//	TEST_NUMBER_NOT_EQUAL(0, (int)m);

	TEST_NUMBER_EQUAL(PacketType_Publish, encoder.lastArrivedPacketType());
	PublishMarshaller *publish = (PublishMarshaller*)m;
	TEST_STRING_EQUAL("1/store/DATA/kitchen/request_face_recognize/v1/ANY/ANY/cbe08ec0-e5bd-4af1-88c9-578b9d0ce6f1", publish->getTopic()->getString());
	TEST_HEXDATA_EQUAL("7B2274797065223A20226B69746368656E2E726571756573745F666163655F7265636F676E697A65222C20227061796C6F6164223A207B20226465766963654964223A202231227D7D", publish->getPayload()->getData(), publish->getPayload()->getLen());

	TEST_NUMBER_EQUAL(171, hexToData(
			"32" // header (packet type)
			"A801" // length
			"005B" // topic size
			"312F73746F72652F4441" // topic value
			"54412F6B69746368656E"
			"2F726571756573745F66"
			"6163655F7265636F676E"
			"697A652F76312F414E59"
			"2F414E592F6362653038"
			"6563302D653562642D34"
			"6166312D383863392D35"
			"37386239643063653666"
			"31"
			"0001" // packet id
			"7B2274797065223A2022" // payload value
			"6B69746368656E2E7265"
			"71756573745F66616365"
			"5F7265636F676E697A65"
			"222C20227061796C6F61"
			"64223A207B2022646576"
			"6963654964223A202231"
			"227D7D",
			&recvData));

	Marshaller *m2 = encoder.readPacket(recvData);
//	TEST_NUMBER_NOT_EQUAL(0, (int)m2);

	TEST_NUMBER_EQUAL(PacketType_Publish, encoder.lastArrivedPacketType());
	PublishMarshaller *publish2 = (PublishMarshaller*)m2;
	TEST_STRING_EQUAL("1/store/DATA/kitchen/request_face_recognize/v1/ANY/ANY/cbe08ec0-e5bd-4af1-88c9-578b9d0ce6f1", publish2->getTopic()->getString());
	TEST_HEXDATA_EQUAL("7B2274797065223A20226B69746368656E2E726571756573745F666163655F7265636F676E697A65222C20227061796C6F6164223A207B20226465766963654964223A202231227D7D", publish2->getPayload()->getData(), publish2->getPayload()->getLen());
	return true;
}

}
