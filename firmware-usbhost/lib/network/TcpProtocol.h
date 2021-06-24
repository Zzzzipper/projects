#ifndef LIB_NETWORK_TCPPROTOCOL_H
#define LIB_NETWORK_TCPPROTOCOL_H

#include "common/tcpip/include/TcpIpUtils.h"

#include "extern/lwip/include/lwip/ip4_addr.h"

#include "stdint.h"
#include "common.h"

#pragma pack(push,1)
enum EtherType {
	EtherType_Ip4 = 0x800,
	EtherType_ARP = 0x806,
};

struct EthFrame {
	uint8_t dstMac[6];
	uint8_t srcMac[6];
	uint16_t type;
	uint16_t getType() { return ntohs(type); }
};

enum IpProtocol {
	IpProtocol_TCP = 0x06,
	IpProtocol_HeaderSizeMask = 0x0F,
};

enum TcpProtocol {
	TcpProtocol_HeaderSizeMask = 0x0F
};

struct IpDatagram {
	uint8_t version;
	uint8_t service;
	uint16_t size;
	uint16_t id;
	uint16_t flags;
	uint8_t lifeTime;
	uint8_t protocol;
	uint16_t crc;
	uint32_t srcAddr;
	uint32_t dstAddr;
	uint8_t options[4];
	uint8_t data[0];
};

struct TcpHeader {
	uint16_t srcPort;
	uint16_t dstPort;
	uint32_t seqNumber;
	uint32_t ackNumber;
	uint8_t size;
	uint8_t flags;
	uint16_t windowSize;
	uint16_t crc;
	uint16_t priority;
	uint8_t options[4];
	uint8_t data[0];
	uint16_t getSrcPort() { return ntohs(srcPort); }
	uint16_t getDstPort() { return ntohs(dstPort); }
};
#pragma pack(pop)

//extern bool stringToIpAddr(const char *string, ip4_addr_t *ipaddr);
extern bool printEthernetFrame(const uint8_t *data, uint16_t dataLen);
extern bool printIpHeader(const uint8_t *data, uint16_t dataLen);
extern bool printTcpHeader(const uint8_t *data, uint16_t dataLen);

#endif
