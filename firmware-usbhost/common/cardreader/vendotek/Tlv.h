#ifndef COMMON_VENDOTEK_TLV_H
#define COMMON_VENDOTEK_TLV_H

#include "utils/include/CodePage.h"
#include "utils/include/NetworkProtocol.h"
#include "timer/include/DateTime.h"

namespace Tlv {
/*
TLV-формат стандартный ISO/IEC 8825-1
*/

#pragma pack(push,1)
struct Header {
	uint8_t type;
	uint8_t len;
	uint8_t value[0];

	bool parse(uint32_t dataLen, uint32_t *headerLen, uint32_t *valueLen);
	uint8_t *getValue();
	uint32_t getValueLen();
	uint32_t getValueOffset();
};
#pragma pack(pop)

class Packet {
public:
	Packet(uint16_t nodeMaxNumber);
	~Packet();
	bool parse(const uint8_t *data, uint32_t dataLen);
	bool getNumber(uint8_t id, uint16_t *num);
	bool getNumber(uint8_t id, uint32_t *num);
	bool getNumber(uint8_t id, uint64_t *num);
	bool getString(uint8_t id, StringBuilder *str);
	uint32_t getData(uint8_t id, void *buf, uint32_t bufSize);
	bool getDateTime(uint8_t id, DateTime *date);
	Header *find(uint8_t id);
	void print();

private:
	Header **nodes;
	int nodeNum;
	int nodeMax;
};

class PacketMaker {
public:
	PacketMaker(uint16_t bufSize);
	Buffer *getBuf();
	const uint8_t *getData();
	uint16_t getDataLen();
	bool addNumber(uint8_t id, uint32_t num);
	bool addNumber(uint8_t id, uint16_t maxSize, uint32_t num);
	bool addString(uint8_t id, const char *str);
	bool addString(uint8_t id, const char *str, uint16_t strLen);
	bool addData(uint8_t id, const void *data, uint16_t dataLen);
	bool addDateTime(uint8_t id, DateTime *datetime);
	void clear();

private:
	Buffer buf;

	void addLength(uint32_t len);
};

}

#endif
