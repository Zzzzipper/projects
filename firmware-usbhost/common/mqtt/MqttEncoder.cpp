#include "MqttEncoder.h"

#include "common/logger/include/Logger.h"

#include <string.h>

namespace Mqtt {

// “екстовое описание ошибки
const char* ErrMalformedLength = "malformed mqtt packet remaining length";
const char* ErrConnRefusedBadProtocolVersion = "connection refused, unacceptable protocol version";
const char* ErrConnRefusedIDRejected = "connection refused, identifier rejected";
const char* ErrConnRefusedServerUnavailable = "connection refused, server unavailable";
const char* ErrConnRefusedBadUsernameOrPassword = "connection refused, bad user name or password";
const char* ErrConnRefusedNotAuthorized = "connection refused, not authorized";
const char* ErrConnUnknown = "connection refused, unknown error";

// ѕараметры протокола по умолчанию
static const char*    ProtocolName = "MQTT";
static const uint8_t  ProtocolLevel = 4; // MQTT v3.1.1

void Length::make(uint32_t length, Buffer *buf) {
	uint32_t l = length;
	do {
		uint8_t b = l % 128;
		l = l / 128;
		if(l > 0) {
			buf->addUint8(b + 128);
		} else {
			buf->addUint8(b);
		}
	} while(l > 0);
}

uint32_t Length::parse(uint8_t *data, uint32_t dataLen, uint32_t *length) {
	uint32_t multiplier = 1;
	uint32_t l = 0;
	uint16_t i = 0;
	uint8_t b = 0;
	do {
		b = data[i];
		l += (b & 0x7F) * multiplier;
		multiplier *= 128;
		if(multiplier > 128*128*128*128) {
			return 0;
		}
		i++;
		if(i > dataLen) {
			return 0;
		}
	} while((b & 128) != 0);
	*length = l;
	return i;
}

/**
 * @brief Marshaller
 */
uint16_t Marshaller::copyBuffer(uint8_t *buf, uint16_t pos, Buffer data) {
    return copyBuffer(buf, pos, data.getData(), data.getLen());
}

uint16_t Marshaller::copyBuffer(uint8_t *buf, uint16_t pos, uint8_t* data, uint16_t len) {
    uint16_t nextPos = pos + len;
    for(uint16_t i = 0; i < len; ++i) {
        buf[i + pos] = data[i];
    }
    return nextPos;
}

uint16_t Marshaller::stringSize(const char *s) {
    return (!s || !strlen(s)) ? 0: strlen(s) + 2;
}

uint16_t Marshaller::swapEndian(uint16_t c) {
    uint16_t d;
    uint8_t* p = (uint8_t*)&d;
    *(p) = (c >> 8);
    *(p + 1) = (c & 0xFF);
    return d;
}

void Marshaller::encodeString(const char* s, Buffer *buffer) {
	uint16_t len = strlen(s);
	uint16_t swapLen = swapEndian(len);
	buffer->add(&swapLen, 2);
	buffer->add(s, len);
}

void Marshaller::decodeString(uint8_t* payload, uint16_t &offset, String *buf) {
    uint16_t len = swapEndian(*((uint16_t*)payload));
    offset += len + 2;
	buf->set((const char*)(payload + 2), len);
}

const char* Marshaller::defaultValue(const char *value, const char *def) {
    return (strcmp(value, "") == 0)? def: value;
}

Marshaller* Marshaller::decode(uint8_t, uint8_t*, uint16_t) {
    return this;
}

/**
 * @brief ConnectMarshaller
 */
void ConnectMarshaller::setWill(const char* topic, const char* message, uint8_t qos) {
    connect.willFlag = true;
    connect.willQOS = qos;
    connect.willTopic = topic;
    connect.willMessage = message;
}

uint16_t ConnectMarshaller::payLoadSize() {
    // Ёто базова€ длина дл€ переменной части, без дополнительных полей
    uint16_t length = 10;

    // TODO This is just formal / cosmetic, but it would be nice to support any protocol names
    length += stringSize(connect.clientId);
    if (connect.willFlag) {
        length += stringSize(connect.willTopic);
        length += stringSize(connect.willMessage);
    }
    if (strlen(connect.username) > 0) {
        length += stringSize(connect.username);
        length += stringSize(connect.password);
    }
    return length;
}

uint8_t ConnectMarshaller::connectFlag() {
    // ”станавливаетс€ willFlag только в том случае, если на самом деле есть набор топиков.
    bool willflag = connect.willFlag && strlen(connect.willTopic) > 0;

    uint8_t willqos = 0;
    bool willretain = false;

    if (willflag) {
        if (connect.willQOS > 0) {
            willqos = connect.willQOS;
        }
        willretain = connect.willRetain;
    }

    bool usernameFlag = false, passwordFlag = false;
    if(strlen(connect.username) > 0) {
        usernameFlag = true;
        if (strlen(connect.password) > 0) {
            passwordFlag = true;
        }
    }

    return bool2int(passwordFlag)<<7
                                   | bool2int(usernameFlag)<<6
                                   | bool2int(willretain)<<5
                                   | willqos<<3
                                   | bool2int(willflag)<<2
                                   | bool2int(connect.cleanSession)<<1;
}

void ConnectMarshaller::marshall(Buffer* packet) {
	if(!packet) {
		return;
	}

	// ‘иксированные заголовки
	packet->clear();
	packet->addUint8(PacketType_Connect);
	Length::make(payLoadSize(), packet);

	// ѕеременные заголовки
	encodeProtocolName(connect.protocolName, packet);
	packet->addUint8(encodeProtocolLevel(connect.protocolLevel));
	packet->addUint8(connectFlag());

	uint16_t swapKeepaliveTimeout = swapEndian(connect.keepalive);
	packet->add(&swapKeepaliveTimeout, 2);

	// TODO: Ќе работает дл€ имени настраиваемого протокола, поскольку позици€ может отличатьс€
	encodeString(connect.clientId, packet);
	if(connect.willFlag && strlen(connect.willTopic) > 0) {
		encodeString(connect.willTopic, packet);
		encodeString(connect.willMessage, packet);
	}

	if(strlen(connect.username) > 0) {
		encodeString(connect.username, packet);
		if(strlen(connect.password) > 0) {
			encodeString(connect.password, packet);
		}
	}

	return;
}

void ConnectMarshaller::encodeProtocolName(const char* name, Buffer *packet) {
	encodeString(defaultValue(name, ProtocolName), packet);
}

uint8_t ConnectMarshaller::encodeProtocolLevel(uint8_t level) {
    return (level == 0)? ProtocolLevel: level;
}

Marshaller* ConnectMarshaller::decode(uint8_t* payload, uint16_t) {
#if 0
    uint16_t offset = 0;

	extractNextString(payload, offset, &(connect.protocolName));
    connect.protocolLevel = uint8_t(payload[offset]);

    offset++;

    uint8_t flag = payload[offset];

    connect.cleanSession = int2bool(int((flag & 2) >> 1));
    connect.willFlag = int2bool(int((flag & 4) >> 2));
    if (connect.willFlag) {
        connect.willQOS = int((flag & 24) >> 3);
        connect.willRetain = int2bool(int((flag & 32) >> 5));
    }

    bool usernameFlag = int2bool(int((flag & 64) >> 6));
    bool passwordFlag = int2bool(int((flag & 128) >> 7));

    offset++;

    connect.keepalive = int(swapEndian(*((uint16_t*)(payload + offset))));
    connect.clientID = extractNextString(payload + offset + 4, offset)
            .getString();

    if (connect.willFlag) {
        connect.willTopic = extractNextString(payload + offset, offset)
                .getString();
        connect.willMessage = extractNextString(payload + offset, offset)
                .getString();
    }

    if (usernameFlag) {
        connect.username = extractNextString(payload + offset, offset)
                .getString();
    }
    if (passwordFlag) {
        connect.password = extractNextString(payload + offset, offset)
                .getString();
    }
#endif
	return this;
}

ConnectMarshaller::ConnectMarshaller()
{
    connect.protocolName = ProtocolName;
    connect.protocolLevel = ProtocolLevel;
}

/**
 * @brief ConnAckMarshaller
 */
Marshaller* ConnAckMarshaller::decode(uint8_t* payload, uint16_t) {
    connack.returnCode = uint8_t(payload[1]);
    return this;
}

void ConnAckMarshaller::marshall(Buffer* packet) {
    if(!packet) {
        return;
    }

    uint16_t fixedLength = 2;
    packet->clear();
    packet->setLen(fixedLength + payLoadSize());

    (*packet)[0] = PacketType_ConnAck;
    (*packet)[1] = uint8_t(payLoadSize());
    // TODO support Session Present flag:
    (*packet)[2] = 0; // reserved
    (*packet)[3] = uint8_t(connack.returnCode);

    return;
}

uint16_t ConnAckMarshaller::payLoadSize() {
    return 2;
}

/**
 * @brief SubscrMarshaller
 */
SubscrMarshaller::SubscrMarshaller() {
    registerTopic("/amq/topic");
}

Marshaller* SubscrMarshaller::decode(uint8_t* payload, uint16_t size) {
    subscr.id = swapEndian(*((uint16_t*)payload));

    uint16_t offset = 2;

	for(uint8_t i = 0; i < MQTT_TOPIC_SUBSRIBERS; ++i) {
        if((offset + 2) <= (size - 3)) {
			decodeString(payload + offset, offset, &(subscr.topics[i].name));
            subscr.topics[i].qos = QoS(*(payload + offset));
            offset++;
        } else {
            break;
        }
    }

    return this;
}

void SubscrMarshaller::marshall(Buffer* packet) {
	if(!packet) {
		return;
	}
	packet->clear();

	// Header
	uint8_t fixedHeaderFlags = 2; // mandatory value
	packet->addUint8(PacketType_Subscribe | fixedHeaderFlags);
	Length::make(payLoadSize(), packet);

	// »дентификатор пакета (он должен быть ненулевым, поэтому мы используем 1,
	// чтобы создать действительный пакет)
	uint16_t id = 1;
	if (subscr.id > id) {
		id = subscr.id;
		subscr.id++;
	}

	uint16_t swapId = swapEndian(id);
	packet->add(&swapId, 2);

	// Topic filters
	for(uint8_t i = 0; i < MQTT_TOPIC_SUBSRIBERS; ++i ) {
		if(subscr.topics[i].name.getLen()) {
			encodeString(subscr.topics[i].name.getString(), packet);
			packet->addUint8(subscr.topics[i].qos);
		}
	}

	return;
}

uint16_t SubscrMarshaller::payLoadSize() {
    uint16_t len = 2;
	for(uint8_t i = 0; i < MQTT_TOPIC_SUBSRIBERS; ++i) {
        if(subscr.topics[i].name.getLen()) {
            len += stringSize(subscr.topics[i].name.getString()) +  1;
        }
    }
    return len;
}

bool SubscrMarshaller::haveTopics() {
    uint16_t count = 0;
	for(uint8_t i = 0; i < MQTT_TOPIC_SUBSRIBERS; ++i) {
        if(subscr.topics[i].name.getLen()) {
            count++;
        }
    }
    return count != 0;
}

uint8_t SubscrMarshaller::registerTopic(const char* topic) {
	if(registered < MQTT_TOPIC_SUBSRIBERS) {
        subscr.topics[registered].name = topic;
        subscr.topics[registered].qos = QoS_0;
        registered++;
    }
    return registered - 1;
}

/**
 * @brief SubAckMarshaller
 */
Marshaller* SubAckMarshaller::decode(uint8_t* payload, uint16_t size) {
    suback.id = swapEndian(*((uint16_t*)payload));
    uint16_t nextPos = 2;
	for(uint8_t i = 0; i < MQTT_TOPIC_SUBSRIBERS; ++i) {
        if((nextPos - 1) <= (size - 1)) {
            suback.returnCodes[i] = *(payload + nextPos);
            nextPos++;
        } else {
            break;
        }
    }
    return this;
}

void SubAckMarshaller::marshall(Buffer* packet) {
    if(!packet) {
        return;
    }
    uint16_t fixedLength = 2;
    packet->clear();
    packet->setLen(fixedLength + payLoadSize());

    // Header
    (*packet)[0] = PacketType_SubAck;
    (*packet)[1] = uint8_t(payLoadSize());

    // Packet ID
    *((uint16_t*)(packet->getData() + 2)) = swapEndian(suback.id);

    // Return codes
    uint16_t nextPos = 4;
	for(uint8_t i = 0; i < MQTT_TOPIC_SUBSRIBERS; ++i) {
        (*packet)[nextPos] = suback.returnCodes[i];
        nextPos++;
    }

    return;
}

uint16_t SubAckMarshaller::payLoadSize() {
	return 2 + MQTT_TOPIC_SUBSRIBERS;
}

/**
 * @brief PingRecMarshaller
 */
Marshaller* PingRecMarshaller::decode(uint8_t*, uint16_t) {
    return this;
}

void PingRecMarshaller::marshall(Buffer* packet) {
    if(!packet) {
        return;
    }

    packet->setLen(2);

    // Header
    (*packet)[0] = PacketType_PingRec;
    (*packet)[1] = uint8_t(0);

    return;
}

uint16_t PingRecMarshaller::payLoadSize() {
    return 2;
}

/**
 * @brief PingRespMarshaller
 */
Marshaller* PingRespMarshaller::decode(uint8_t*, uint16_t) {
    return this;
}

void PingRespMarshaller::marshall(Buffer* packet) {
    if(!packet) {
        return;
    }

    uint16_t fixedLength = 2;
    packet->setLen(fixedLength);
    // Header
    (*packet)[0] = uint8_t(PacketType_PingRec);
    (*packet)[1] = uint8_t(0);

    return;
}

uint16_t PingRespMarshaller::payLoadSize() {
    return 2;
}

void PublishMarshaller::publish(const char* topic, const uint8_t* payload, uint32_t payloadLen, QoS qos) {
    pubpack.qos = qos;
    pubpack.topic = topic;
    pubpack.payload.clear();
    pubpack.payload.add(payload, payloadLen);
}

Marshaller* PublishMarshaller::decode(uint8_t*, uint16_t) {
    return this;
}

Marshaller* PublishMarshaller::decode(uint8_t fixedHeaderFlags, uint8_t* payload, uint16_t size) {
	LOG_DEBUG(LOG_MQTT, "decode");
	pubpack.dup = int2bool(fixedHeaderFlags >> 3);
    pubpack.qos = QoS((fixedHeaderFlags & 6) >> 1);
    pubpack.retain = int2bool(fixedHeaderFlags & 1);

    uint16_t offset = 0;
	decodeString(payload, offset, &(pubpack.topic));

    if(pubpack.qos == QoS_1 || pubpack.qos == QoS_2) {
        pubpack.id = swapEndian(*((uint16_t*)(payload + offset)));
        offset += 2;
    }

	pubpack.payload.clear();
	if(offset < size) {
		pubpack.payload.add(payload + offset, size - offset);
    }

    return this;
}

void PublishMarshaller::marshall(Buffer* packet) {
	if(!packet) {
		return;
	}

	// Header
	packet->clear();
	packet->addUint8(PacketType_Publish | bool2int(pubpack.dup) << 3 | uint8_t(pubpack.qos) << 1 | bool2int(pubpack.retain));
	Length::make(payLoadSize(), packet);

	// Topic
	encodeString(pubpack.topic.getString(), packet);

	// Packet ID
	if(pubpack.qos == QoS_1 || pubpack.qos == QoS_2) {
		// »дентификатор пакета (он должен быть ненулевым, поэтому, если
		// значение равно нулю, мы используем 1 чтобы создать действительный пакет)
		uint16_t id = 1;
		if (pubpack.id > id) {
			id = pubpack.id;
			pubpack.id++;
		}
		uint16_t swapId = swapEndian(id);
		packet->add(&swapId, 2);
	}

	// Published message payload
	packet->add(pubpack.payload.getData(), pubpack.payload.getLen());
	return;
}

uint16_t PublishMarshaller::payLoadSize() {
    uint16_t length = stringSize(pubpack.topic.getString());
    if (pubpack.qos == QoS_1 || pubpack.qos == QoS_2) {
        length += 2;
    }
    length += pubpack.payload.getLen();
    return length;
}

/**
 * @brief PubAckMarshaller
 */
Marshaller* PubAckMarshaller::decode(uint8_t* payload, uint16_t) {
    puback.id = *((uint16_t*)payload);
    return this;
}

void PubAckMarshaller::marshall(Buffer* packet) {
    if(!packet) {
        return;
    }
    uint16_t fixedLength = 2;
    packet->setLen(fixedLength + payLoadSize());

    // Header
    (*packet)[0] = uint8_t(PacketType_PubAck);
    (*packet)[1] = uint8_t(payLoadSize());

    uint16_t id = 1;
    if (puback.id > id) {
        id = puback.id;
        puback.id++;
    }

    // Packet ID
    *((uint16_t*)(packet->getData() + 2)) = swapEndian(id);
    return;
}

uint16_t PubAckMarshaller::payLoadSize() {
    return 2;
}

/**
 * @brief Encoder
 */
Marshaller* Encoder::readPacket(Buffer &reads) {
    LOG_DEBUG(LOG_MQTT, "Encoder::readPacket ");
	if(reads.getLen() < 2) {
		return NULL;
	}
	PacketType packetType = PacketType(reads[0] & 0xF0);
	uint8_t fixedHeaderFlags = reads[0] & 0x0F; // только последние 4 бита
	uint32_t length = 0;
	uint32_t offset = Length::parse(reads.getData() + 1, reads.getLen() - 1, &length);
	if(offset == 0) {
		return NULL;
	}
	return decode(packetType, fixedHeaderFlags, reads.getData() + 1 + offset, length);
}

Marshaller* Encoder::decode(uint8_t packetType, uint8_t fixedHeaderFlags
                            , uint8_t* payload, uint16_t size)
{
    LOG_DEBUG(LOG_MQTT, "Encoder::decode arrived " << uint8_t(packetType));

    // “ип вход€щего пакета дл€ внешних парсеров
    arrived = PacketType(packetType);

    switch (packetType) {
    case PacketType_Connect:
        return conn.decode(payload, size);
    case PacketType_ConnAck:
        return connack.decode(payload, size);
    case PacketType_SubAck:
        return suback.decode(payload, size);
    case PacketType_PingResp:
        return pingresp.decode(payload, size);
    case PacketType_PubAck:
        return puback.decode(payload, size);
    case PacketType_Publish:
        return pubpack.decode(fixedHeaderFlags, payload, size);
        //    case pubackType:
        //        return pubAckPacket.decode(payload)
        //    case subscribeType:
        //        return subscribePacket.decode(payload)
        //    case subackType:
        //        return subAckPacket.decode(payload)
        //    case unsubscribeType:
        //        return unsubscribePacket.decode(payload)
        //    case unsubackType:
        //        return unsubAckPacket.decode(payload)
        //    case pingreqType:
        //        return pingReqPacket.decode(payload)
        //    case pingrespType:
        //        return pingRespPacket.decode(payload)
        //    case disconnectType:
        //        return disconnectPacket.decode(payload)
    default: // Unsupported MQTT packet type
        return nullptr;
    }
}

const char* Encoder::connAckError(uint8_t code) {
    switch (code) {
    case ConnRefusedBadProtocolVersion:
        return ErrConnRefusedBadProtocolVersion;
    case ConnRefusedIDRejected:
        return ErrConnRefusedIDRejected;
    case ConnRefusedServerUnavailable:
        return ErrConnRefusedServerUnavailable;
    case ConnRefusedBadUsernameOrPassword:
        return ErrConnRefusedBadUsernameOrPassword;
    case ConnRefusedNotAuthorized:
        return ErrConnRefusedNotAuthorized;
    }
    return ErrConnUnknown;
}

Encoder::Encoder()
{
}

Encoder::~Encoder() {}


void Encoder::marshall(PacketType type, Buffer *buffer) {
    LOG_DEBUG(LOG_MQTT, "Encoder::Marshall " << uint8_t(type));
    switch(type) {
    case PacketType_Connect: conn.marshall(buffer); break;
    case PacketType_Subscribe: subscr.marshall(buffer); break;
    case PacketType_PingRec: pingrec.marshall(buffer); break;
    case PacketType_Publish: pubpack.marshall(buffer); break;
    default:
        break;
    }
}

bool Encoder::haveTopics() {
    return subscr.haveTopics();
}

uint8_t Encoder::registerTopic(const char* topic) {
    return subscr.registerTopic(topic);
}

void Encoder::publish(const char* topic, const uint8_t* payload, uint32_t len, QoS qos) {
    pubpack.publish(topic, payload, len, qos);
}

}
