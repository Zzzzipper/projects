#include "EthernetInterface.h"
#include "TcpProtocol.h"
#include "TcpGateway.h"
#include "Enc28j60.h"

#include "lib/spi/SPI.h"

#include "common/sim900/include/GsmDriver.h"
#include "common/utils/include/StringParser.h"
#include "common/utils/include/Utils.h"
#include "common/logger/include/Logger.h"

#include "extern/lwip/include/netif/ethernet.h"
#include "extern/lwip/include/lwip/etharp.h"
#include "extern/lwip/include/lwip/netif.h"
#include "extern/lwip/include/lwip/tcp.h"
#include "extern/lwip/include/lwip/init.h"
#include "extern/lwip/include/lwip/timeouts.h"

#define SPI_BUFFER_SIZE  1023
#define ETHERNET_MTU 1500   // MTU value

static uint8_t buf[SPI_BUFFER_SIZE+1] __attribute__ ((section (".ccmram")));

EthernetInterface::EthernetInterface(uint8_t *mac) :
	receiver(NULL)
{
	this->mac[0] = mac[0];
	this->mac[1] = mac[1];
	this->mac[2] = mac[2];
	this->mac[3] = mac[3];
	this->mac[4] = mac[4];
	this->mac[5] = mac[5];
#if (HW_VERSION < HW_3_2_0)
	enc28j60 = new ENC28J60(SPI::get(SPI_3));
#elif (HW_VERSION >= HW_3_2_0)
	enc28j60 = new ENC28J60(SPI::get(SPI_2));
#else
	#error "HW_VERSION must be defined in project settings"
#endif
}

EthernetInterface::~EthernetInterface() {
	delete enc28j60;
}

void EthernetInterface::setReceiver(EthernetReceiver *receiver) {
	this->receiver = receiver;
}

void EthernetInterface::init() {
	LOG_DEBUG(LOG_ETH, "init");
	LOG_DEBUG_HEX(LOG_ETH, mac, sizeof(mac));
	enc28j60->init(mac);
}

void EthernetInterface::shutdown() {
	LOG_DEBUG(LOG_ETH, "shutdown");
}

uint8_t *EthernetInterface::getAddr() {
	return mac;
}

uint16_t EthernetInterface::getAddrLen() {
	return sizeof(mac);
}

void EthernetInterface::sendSpi(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ETH, "sendSpi>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
	if(LOG_ETH > LOG_LEVEL_INFO) {
		printEthernetFrame(data, dataLen);
	}

	enc28j60->sendFrame((uint8_t*)data, dataLen);
}

void EthernetInterface::execute() {
	uint16_t plen = enc28j60->recvFrame(buf, SPI_BUFFER_SIZE);
    if(plen == 0) {
    	return;
    }

	LOG_DEBUG(LOG_ETH, "recvSpi<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
	if(LOG_ETH > LOG_LEVEL_INFO) {
		printEthernetFrame(buf, plen);
	}

	if(receiver == NULL) {
		LOG_ERROR(LOG_ETH, "receiver not set");
		return;
	}
	receiver->procRecvFrame(buf, plen);
}
