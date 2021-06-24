#include "lib/network/EthernetNetwork.h"
#include "TcpProtocol.h"
#include "TcpGateway.h"
#include "Enc28j60.h"

#include "lib/spi/SPI.h"

#include "common/sim900/include/GsmDriver.h"
#include "common/tcpip/include/TcpIpUtils.h"
#include "common/utils/include/StringParser.h"
#include "common/utils/include/Utils.h"
#include "common/logger/include/Logger.h"

#include "extern/lwip/include/netif/ethernet.h"
#include "extern/lwip/include/lwip/etharp.h"
#include "extern/lwip/include/lwip/netif.h"
#include "extern/lwip/include/lwip/tcp.h"
#include "extern/lwip/include/lwip/init.h"
#include "extern/lwip/include/lwip/timeouts.h"

#define ETHERNET_MTU 1500   // MTU value

static EthernetInterface *g_ethernetNetworkDevice = NULL;

EthernetNetwork::EthernetNetwork(
	EthernetInterface *device
) {
	netif1 = new struct netif;
	g_ethernetNetworkDevice = device;
	g_ethernetNetworkDevice->setReceiver(this);
}

EthernetNetwork::~EthernetNetwork() {
	delete netif1;
}

void EthernetNetwork::init(uint32_t addr, uint32_t mask, uint32_t gateway) {
	LOG_DEBUG(LOG_ETH, "init");
	g_ethernetNetworkDevice->init();
	initNetwork(addr, mask, gateway);
}

void EthernetNetwork::shutdown() {
	LOG_DEBUG(LOG_ETH, "shutdown");
}

void EthernetNetwork::procRecvFrame(const uint8_t *data, uint16_t dataLen) {
	LOG_TRACE(LOG_ETH, "procRecvFrame");
	struct pbuf* p = pbuf_alloc(PBUF_RAW, dataLen, PBUF_POOL);
	LOG_DEBUG(LOG_ETH, "pbuf=" << (uint32_t)p);
	if(p == NULL) {
		LOG_ERROR(LOG_ETH, "pbuf_alloc failed");
		return;
	}
    pbuf_take(p, data, dataLen);
	if(netif1->input(p, netif1) != ERR_OK) {
		LOG_ERROR(LOG_ETH, "input failed");
		pbuf_free(p);
	}
}

/*
Network.cpp#103 sendSpi>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Network.cpp#269 printFrame 42
xFF;xFF;xFF;xFF;xFF;xFF;x20;x89;x84;x6A;x96;x00;x08;x06;x00;x01;x08;x00;x06;x04;x00;x01;x20;x89;x84;x6A;x96;x00;xC0;xA8;x01;xC8;x00;x00;x00;x00;x00;x00;xC0;xA8;x01;xD2;
Network.cpp#276 ----- frame start -----
xFF;xFF;xFF;xFF;xFF;xFF;
x20;x89;x84;x6A;x96;x00;
Network.cpp#280 next level 2054
x00;x01;x08;x00;x06;x04;x00;x01;x20;x89;x84;x6A;x96;x00;xC0;xA8;x01;xC8;x00;x00;x00;x00;x00;x00;xC0;xA8;x01;xD2;
Network.cpp#288 ----- frame end -----
etharp.c:1093 etharp_query: queued packet 0x2000aed6 on ARP entry 0
Network.cpp#94 recvSpi<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
Network.cpp#269 printFrame 60
x20;x89;x84;x6A;x96;x00;x14;x1F;xBA;xE7;xF0;x95;x08;x06;x00;x01;x08;x00;x06;x04;x00;x02;x14;x1F;xBA;xE7;xF0;x95;xC0;xA8;x01;xD2;x20;x89;x84;x6A;x96;x00;xC0;xA8;x01;xC8;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;
Network.cpp#276 ----- frame start -----
x20;x89;x84;x6A;x96;x00;
x14;x1F;xBA;xE7;xF0;x95;
Network.cpp#280 next level 2054
x00;x01;x08;x00;x06;x04;x00;x02;x14;x1F;xBA;xE7;xF0;x95;xC0;xA8;x01;xD2;x20;x89;x84;x6A;x96;x00;xC0;xA8;x01;xC8;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;x00;
Network.cpp#288 ----- frame end -----
Network.cpp#199 procRecvFrame
Network.cpp#201 pbuf=536914674
ethernet.c:101 ethernet_input: dest:20:89:84:6a:96:00, src:14:1f:ba:e7:f0:95, type:806
etharp.c:423 etharp_update_arp_entry: 192.168.1.210 - 14:1f:ba:e7:f0:95
etharp.c:293 etharp_find_entry: found matching entry 0
 */
static err_t netif1_linkoutput(struct netif *netif, struct pbuf *p) {
	LOG_DEBUG(LOG_ETH, "netif1_output");
//	lock_interrupts(); todo: защита от прерываний через макросы LWIP
	g_ethernetNetworkDevice->sendSpi((const uint8_t*)p->payload, p->len);
//	unlock_interrupts(); todo: защита от прерываний через макросы LWIP
	return ERR_OK;
}

static err_t netif1_init(struct netif *netif) {
	LOG_DEBUG(LOG_ETH, "netif1_init");
	netif->mtu        = ETHERNET_MTU;
	netif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
	netif->output     = etharp_output; // пакеты вверх
	netif->linkoutput = netif1_linkoutput;  // пакеты вниз
	memcpy(netif->hwaddr, g_ethernetNetworkDevice->getAddr(), g_ethernetNetworkDevice->getAddrLen());
	netif->hwaddr_len = g_ethernetNetworkDevice->getAddrLen();
	return ERR_OK;
}

void EthernetNetwork::initNetwork(uint32_t addr, uint32_t mask, uint32_t gateway) {
	LOG_DEBUG(LOG_ETH, "initNetwork" << LOG_IPADDR(addr) << "," << LOG_IPADDR(mask) << "," << LOG_IPADDR(gateway));
	lwip_init();

	ip4_addr_t ipaddr1;
	ip4_addr_t netmask1;
	ip4_addr_t gateway1;
	ipaddr1.addr = addr;
	netmask1.addr = mask;
	gateway1.addr = gateway;

	if(netif_add(netif1, &ipaddr1, &netmask1, &gateway1, NULL, netif1_init, ethernet_input) == NULL) {
		LOG_ERROR(LOG_ETH, "netif_add failed");
		return;
	}

	netif1->name[0] = 'e';
	netif1->name[1] = '0';
	netif_set_default(netif1);
	netif_set_link_up(netif1);
	netif_set_up(netif1);
}
