#ifndef COMMON_MQTT_ENCODER_H
#define COMMON_MQTT_ENCODER_H

#include "MqttProtocol.h"

namespace Mqtt {

class Length {
public:
	static void make(uint32_t length, Buffer *buf);
	static uint32_t parse(uint8_t *data, uint32_t dataLen, uint32_t *length);
};

/**
 * @brief The Marshaller class
 */
class Marshaller {
public:
    virtual Marshaller* decode(uint8_t* payload, uint16_t size) = 0;
    virtual void marshall(Buffer* buffer) = 0;
    virtual uint16_t payLoadSize() = 0;

    virtual Marshaller* decode(uint8_t fixedHeaderFlags, uint8_t* payload, uint16_t size);

	void encodeString(const char* s, Buffer *buffer);
	void decodeString(uint8_t *payload, uint16_t &offset, String *buf);

    const char* defaultValue(const char* value, const char* def);

    uint16_t swapEndian(uint16_t c);
    uint16_t stringSize(const char* s);
    uint16_t copyBuffer(uint8_t *buf, uint16_t pos, uint8_t* data, uint16_t len);
    uint16_t copyBuffer(uint8_t *buf, uint16_t pos, Buffer data);

    inline int bool2int(bool b) { return b? 1: 0; }
    inline bool int2bool(int i) { return i == 1; }
};

/**
 * @brief The ConnectMarshaller class
 */
class ConnectMarshaller: public Marshaller {
public:
    ConnectMarshaller();

    virtual Marshaller* decode(uint8_t* payload, uint16_t size) final;
    virtual void marshall(Buffer* packet) final;
    virtual uint16_t payLoadSize() final;

    void setWill(const char* topic, const char* message, uint8_t qos);

    inline void setClientId(const char* clientId) { connect.clientId = clientId; }
    inline void setUsername(const char* username) { connect.username = username; }
    inline void setPassword(const char* password) { connect.password = password; }

private:
    uint8_t encodeProtocolLevel(uint8_t level);
    uint8_t connectFlag();

	void encodeProtocolName(const char *name, Buffer *packet);
	void encodeClientID(const char* id, Buffer *packet);

    ConnectPacket connect;
};

/**
 * @brief The ConnAckMarshaller class
 */
class ConnAckMarshaller: public Marshaller {
public:
    virtual Marshaller* decode(uint8_t* payload, uint16_t size) final;
    virtual void marshall(Buffer* buffer) final;
    virtual uint16_t payLoadSize() final;

private:
    ConnAckPacket connack;
};


class SubscrMarshaller: public Marshaller {
public:
    SubscrMarshaller();

    virtual Marshaller* decode(uint8_t* payload, uint16_t size) final;
    virtual void marshall(Buffer* buffer) final;
    virtual uint16_t payLoadSize() final;

    bool haveTopics();
    uint8_t registerTopic(const char* topic);

private:
    SubscribePacket subscr;
    uint8_t registered = 0;
};


/**
 * @brief The SubAckMarshaller class
 */
class SubAckMarshaller: public Marshaller {
public:
    virtual Marshaller* decode(uint8_t* payload, uint16_t size) final;
    virtual void marshall(Buffer* buffer) final;
    virtual uint16_t payLoadSize() final;

private:
    SubAckPacket suback;
};

/**
 * @brief The PingRecMarshaller class
 */
class PingRecMarshaller: public Marshaller {
public:
    virtual Marshaller* decode(uint8_t* payload, uint16_t size) final;
    virtual void marshall(Buffer* buffer) final;
    virtual uint16_t payLoadSize() final;

};

/**
 * @brief The PingRespMarshaller class
 */
class PingRespMarshaller: public Marshaller {
public:
    virtual Marshaller* decode(uint8_t* payload, uint16_t size) final;
    virtual void marshall(Buffer* buffer) final;
    virtual uint16_t payLoadSize() final;

private:
    PublishPacket publish;
};

/**
 * @brief The PublishMarshaller class
 */
class PublishMarshaller: public Marshaller {
public:
    virtual Marshaller* decode(uint8_t* payload, uint16_t size) final;
    virtual Marshaller* decode(uint8_t fixedHeaderFlags, uint8_t* payload, uint16_t size) final;
    virtual void marshall(Buffer* buffer) final;
    virtual uint16_t payLoadSize() final;

	void publish(const char* topic, const uint8_t* payload, uint32_t payloadLen, QoS qos);
	String *getTopic() { return &pubpack.topic; }
	Buffer *getPayload() { return &pubpack.payload; }

private:
    PublishPacket pubpack;
};

/**
 * @brief The PubAckMarshaller class
 */
class PubAckMarshaller: public Marshaller {
public:
    virtual Marshaller* decode(uint8_t* payload, uint16_t size) final;
    virtual void marshall(Buffer* packet) final;
    virtual uint16_t payLoadSize() final;

private:
    PubAckPacket puback;
};

/**
 * @brief The Encoder class
 */
class Encoder {
public:
    Encoder();
    virtual ~Encoder();

	void marshall(PacketType type, Buffer* buffer);
    Marshaller* readPacket(Buffer& reads);

    inline PacketType lastArrivedPacketType() {
        PacketType out = arrived;
        arrived = PacketType_Reserved1Type;
        return out;
    }
    inline void setClientId(const char* clientId) { conn.setClientId(clientId); }
    inline void setUsername(const char* username) { conn.setUsername(username); }
    inline void setPassword(const char* password) { conn.setPassword(password); }

    bool haveTopics();
    uint8_t registerTopic(const char* topic);
	void publish(const char* topic, const uint8_t* payload, uint32_t len, QoS qos);

private:
    const char* connAckError(uint8_t code);
    Marshaller* decode(uint8_t packetType, uint8_t fixedHeaderFlags, uint8_t* payload, uint16_t size);

    PacketType arrived = PacketType_Reserved1Type;

    ConnectMarshaller  conn;
    ConnAckMarshaller  connack;
    SubscrMarshaller   subscr;
    SubAckMarshaller   suback;
    PingRecMarshaller  pingrec;
    PingRespMarshaller pingresp;
    PublishMarshaller  pubpack;
    PubAckMarshaller   puback;

};

}

#endif // MQTTENCODER_H
