#include "TcpProtocol.h"

#include "common/utils/include/StringParser.h"
#include "common/logger/include/Logger.h"

/*
bool stringToIpAddr(const char *string, ip4_addr_t *ipaddr) {
	StringParser parser(string);
	uint16_t num1 = 0;
	if(parser.getNumber(&num1) == false) {
		return false;
	}
	parser.skipEqual(".");
	uint16_t num2 = 0;
	if(parser.getNumber(&num2) == false) {
		return false;
	}
	parser.skipEqual(".");
	uint16_t num3 = 0;
	if(parser.getNumber(&num3) == false) {
		return false;
	}
	parser.skipEqual(".");
	uint16_t num4 = 0;
	if(parser.getNumber(&num4) == false) {
		return false;
	}
	IP4_ADDR(ipaddr, num1, num2, num3, num4);
	return true;
}
*/
bool printEthernetFrame(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ETH, "printFrame " << dataLen);
	LOG_TRACE_HEX(LOG_ETH, data, dataLen);
	bool result = false;
	if(dataLen < sizeof(EthFrame)) {
		LOG_DEBUG(LOG_ETH, "Data smaller then frame.");
		return result;
	}
	LOG_DEBUG(LOG_ETH, "----- frame start -----");
	EthFrame *frame = (EthFrame*)data;
	LOG_DEBUG_HEX(LOG_ETH, frame->dstMac, sizeof(frame->dstMac));
	LOG_DEBUG_HEX(LOG_ETH, frame->srcMac, sizeof(frame->dstMac));
	LOG_DEBUG(LOG_ETH, "next level " << frame->getType());
	const uint8_t *tail = data + sizeof(EthFrame);
	uint16_t tailLen = dataLen - sizeof(EthFrame);
	if(frame->getType() == EtherType_Ip4) {
		result = printIpHeader(tail, tailLen);
	} else {
		LOG_DEBUG_HEX(LOG_ETH, tail, tailLen);
	}
	LOG_DEBUG(LOG_ETH, "----- frame end -----");
	return result;
}

bool printIpHeader(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ETH, "printIpDatagram " << dataLen);
	bool result = false;
	if(dataLen < offsetof(IpDatagram, options)) {
		LOG_DEBUG(LOG_ETH, "Data smaller then header.");
		return result;
	}
	IpDatagram *header = (IpDatagram*)data;
	LOG_DEBUG(LOG_ETH, "----- ip start -----");
	LOG_DEBUG(LOG_ETH, "src=" << LOG_IPADDR(header->srcAddr));
	LOG_DEBUG(LOG_ETH, "dst=" << LOG_IPADDR(header->dstAddr));
	LOG_DEBUG(LOG_ETH, "next level " << header->protocol);
	uint16_t headerSize = (header->version & IpProtocol_HeaderSizeMask) * 4;
	if(headerSize > dataLen) {
		LOG_DEBUG(LOG_ETH, "Header size too big " << headerSize);
		return result;
	}
	const uint8_t *tail = data + headerSize;
	uint16_t tailLen = dataLen - headerSize;
	if(header->protocol == IpProtocol_TCP) {
		result = printTcpHeader(tail, tailLen);
	} else {
		LOG_DEBUG_HEX(LOG_ETH, tail, tailLen);
	}
	return result;
}

bool printTcpHeader(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ETH, "printTcpHeader " << dataLen);
	bool result = false;
	if(dataLen < offsetof(TcpHeader, options)) {
		LOG_DEBUG(LOG_ETH, "Data smaller then datagram.");
		return result;
	}
	TcpHeader *header = (TcpHeader*)data;
	LOG_DEBUG(LOG_ETH, "----- tcp start -----");
	LOG_DEBUG(LOG_ETH, "srcPort=" << header->getSrcPort() << ", dstPort=" << header->getDstPort());
	uint16_t headerSize = ((header->size >> 4) & TcpProtocol_HeaderSizeMask) * 4;
	LOG_DEBUG(LOG_ETH, "headerSize=" << headerSize);
	if(headerSize > dataLen) {
		LOG_DEBUG(LOG_ETH, "Header size too big " << headerSize);
		return result;
	}
	const uint8_t *tail = data + headerSize;
	uint16_t tailLen = dataLen - headerSize;
	LOG_DEBUG_HEX(LOG_ETH, tail, tailLen);
	return (headerSize == dataLen);
}
