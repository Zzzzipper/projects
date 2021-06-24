#if 1
#include "lib/network/include/Network.h"
#include "lib/network/TcpProtocol.h"
#include "lib/network/NetworkConn.h"
#include "lib/network/EthernetInterface.h"
#include "lib/network/EthernetNetwork.h"
#include "lib/network/FiscalNetwork.h"

#include "common/sim900/include/GsmDriver.h"
#include "common/tcpip/include/DummyTcpIp.h"
#include "common/utils/include/StringParser.h"
#include "common/utils/include/Utils.h"
#include "common/logger/include/Logger.h"

#include "extern/lwip/include/lwip/dns.h"
#include "extern/lwip/include/lwip/timeouts.h"

Network::Network(
	ConfigModem *config,
	Gsm::Driver *gsm,
	TimerEngine *timerEngine
) :
	config(config),
	gsm(gsm),
	ethernetDevice(NULL),
	ethernetNetwork(NULL),
	fiscalNetwork(NULL)
{
#ifdef DEVICE_ETHERNET
	ethernetDevice = new EthernetInterface(config->getAutomat()->getEthMac());
	ethernetNetwork = new EthernetNetwork(ethernetDevice);
	fiscalNetwork = new FiscalNetwork(config->getFiscal(), gsm);
#endif
	if(config->getAutomat()->getInternetDevice() == ConfigAutomat::InternetDevice_Gsm && gsm != NULL) {
		LOG_ERROR(LOG_ETH, "Internet through GSM");
		conn1 = gsm->getTcpConnection1();
		conn2 = gsm->getTcpConnection2();
		conn3 = gsm->getTcpConnection3();
		conn4 = gsm->getTcpConnection4();
		conn5 = gsm->getTcpConnection5();
	} else {
#ifdef DEVICE_ETHERNET
		LOG_ERROR(LOG_ETH, "Internet through ETHERNET");
//		conn1 = gsm->getTcpConnection1(); //for kfc
		conn1 = new NetworkConn(timerEngine);
		conn2 = new NetworkConn(timerEngine);
		conn3 = new NetworkConn(timerEngine);
		conn4 = new NetworkConn(timerEngine);
		conn5 = new NetworkConn(timerEngine);
#else
		conn1 = new DummyTcpIp;
		conn2 = new DummyTcpIp;
		conn3 = new DummyTcpIp;
		conn4 = new DummyTcpIp;
		conn5 = new DummyTcpIp;
#endif
	}
}

Network::~Network() {
	delete conn5;
	delete conn4;
	delete conn3;
	delete conn2;
	delete conn1;
#ifdef DEVICE_ETHERNET
	delete fiscalNetwork;
	delete ethernetNetwork;
	delete ethernetDevice;
#endif
}

void Network::init() {
	LOG_DEBUG(LOG_ETH, "init");
	ConfigAutomat *automat = config->getAutomat();
#ifdef DEVICE_ETHERNET
	fiscalNetwork->init();
	ethernetNetwork->init(automat->getEthAddr(), automat->getEthMask(), automat->getEthGateway());
	if(automat->getInternetDevice() == ConfigAutomat::InternetDevice_Ethernet) {
		initEthernetDns();
	}
#endif
}

void Network::shutdown() {
	LOG_DEBUG(LOG_ETH, "shutdown");
}

void Network::restart() {
	if(config->getAutomat()->getInternetDevice() == ConfigAutomat::InternetDevice_Gsm && gsm != NULL) {
		gsm->restart();
	}
}

TcpIp *Network::getTcpConnection1() {
	return conn1;
}

TcpIp *Network::getTcpConnection2() {
	return conn2;
}

TcpIp *Network::getTcpConnection3() {
	return conn3;
}

TcpIp *Network::getTcpConnection4() {
	return conn4;
}

TcpIp *Network::getTcpConnection5() {
	return conn5;
}

void Network::execute() {
#ifdef DEVICE_ETHERNET
	ethernetDevice->execute();
	executeTimeouts();
#endif
}

void Network::sendPing() {
#ifdef DEVICE_ETHERNET
	uint8_t packet[] = { 0x00 };
	ethernetDevice->sendSpi(packet, sizeof(packet));
#endif
}

void Network::sendArpBroadcast() {
#ifdef DEVICE_ETHERNET
	uint8_t packet[] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x20, 0x89, 0x84, 0x6A, 0x96, 0x00,
		0x08, 0x06,	0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x20, 0x89, 0x84, 0x6A, 0x96, 0x00, 0xC0, 0xA8, 0x01, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xA8, 0x01, 0xD2 };
	ethernetDevice->sendSpi(packet, sizeof(packet));
#endif
}

void Network::initEthernetDns() {
#ifdef DEVICE_ETHERNET
	LOG_DEBUG(LOG_ETH, "initEthernetDns");
	dns_init();
	ip4_addr_t ipaddr1;
	IP4_ADDR(&ipaddr1, 8,8,8,8);
	dns_setserver(0, &ipaddr1);
#endif
}

//todo: перевести на TimerEngine - такая возможность предусмотрена
void Network::executeTimeouts() {
#ifdef DEVICE_ETHERNET
    sys_check_timeouts();
#endif
}
#else
#include "lib/network/include/Network.h"
#include "NetworkUtils.h"
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

static Network *g_network = NULL;
static uint8_t hwaddr1[6]  = { 0x20, 0x89, 0x84, 0x6A, 0x96, 0x00 };
static uint8_t buf[SPI_BUFFER_SIZE+1] __attribute__ ((section (".ccmram")));

Network::Network(
	ConfigFiscal *config,
	Gsm::Driver *gsm
) :
	config(config),
	gsm(gsm),
	tcpGateway(NULL),
	listenPcb(NULL)
{
	LOG_DEBUG(LOG_ETH, "Network");
	if(gsm != NULL) {
		LOG_INFO(LOG_ETH, "Tcp Gateway inited");
		tcpGateway = new TcpGateway(gsm->getTcpConnection4());
	}

#if (HW_VERSION < HW_3_2_0)
	enc28j60 = new ENC28J60(SPI::get(SPI_3));
#elif (HW_VERSION >= HW_3_2_0)
	enc28j60 = new ENC28J60(SPI::get(SPI_2));
#else
	#error "HW_VERSION must be defined in project settings"
#endif

	netif1 = new struct netif;
	netif2 = new struct netif;
	g_network = this;
}

Network::~Network() {
	delete netif2;
	delete netif1;
	delete enc28j60;
}

void Network::init() {
	LOG_DEBUG(LOG_ETH, "init");
	enc28j60->init(hwaddr1);
	initEthernet();
	initFiscalServerPort();
}

void Network::shutdown() {
	LOG_DEBUG(LOG_ETH, "shutdown");
}

TcpIp *Network::getTcpConnection1() {
	return NULL;
}

TcpIp *Network::getTcpConnection2() {
	return NULL;
}

TcpIp *Network::getTcpConnection3() {
	return NULL;
}

TcpIp *Network::getTcpConnection4() {
	return NULL;
}

TcpIp *Network::getTcpConnection5() {
	return NULL;
}

void Network::execute() {
	executeEnc28j60();
	executeTimeouts();
}

void Network::sendSpi(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ETH, "sendSpi>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
	if(LOG_ETH > LOG_LEVEL_INFO) {
		printEthernetFrame(data, dataLen);
	}

	enc28j60->sendFrame((uint8_t*)data, dataLen);
}

void Network::sendPing() {
	uint8_t packet[] = { 0x00 };
	sendSpi(packet, sizeof(packet));
}

void Network::sendArpBroadcast() {
	uint8_t packet[] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x20, 0x89, 0x84, 0x6A, 0x96, 0x00,
		0x08, 0x06,	0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x20, 0x89, 0x84, 0x6A, 0x96, 0x00, 0xC0, 0xA8, 0x01, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xA8, 0x01, 0xD2 };
	sendSpi(packet, sizeof(packet));
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
	g_network->sendSpi((const uint8_t*)p->payload, p->len);
//	unlock_interrupts(); todo: защита от прерываний через макросы LWIP
	return ERR_OK;
}

static err_t netif1_init(struct netif *netif) {
	LOG_DEBUG(LOG_ETH, "netif1_init");
	netif->mtu        = ETHERNET_MTU;
	netif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
	netif->output     = etharp_output; // пакеты вверх
	netif->linkoutput = netif1_linkoutput;  // пакеты вниз
	memcpy(netif->hwaddr, hwaddr1, sizeof(hwaddr1));
	netif->hwaddr_len = sizeof(hwaddr1);
	return ERR_OK;
}

static err_t netif2_output(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr) {
	LOG_ERROR(LOG_ETH, "netif2_linkoutput");
	return ERR_OK;
}

static err_t netif2_linkoutput(struct netif *netif, struct pbuf *p) {
	LOG_ERROR(LOG_ETH, "netif2_linkoutput");
	return ERR_OK;
}

static err_t netif2_init(struct netif *netif) {
	LOG_DEBUG(LOG_ETH, "netif2_init");
	netif->mtu        = ETHERNET_MTU;
	netif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
	netif->output     = netif2_output;
	netif->linkoutput = netif2_linkoutput;
	return ERR_OK;
}

void Network::initEthernet() {
	LOG_DEBUG(LOG_ETH, "initEthernet");
	lwip_init();

	ip4_addr_t ipaddr1;
	ip4_addr_t netmask1;
	ip4_addr_t gateway1;
	IP4_ADDR(&ipaddr1, 192,168,1,200);
	IP4_ADDR(&netmask1, 255,255,255,0);
	IP4_ADDR(&gateway1, 0,0,0,0);
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

void Network::initFiscalNetwork() {
	LOG_INFO(LOG_ETH, "initFiscalNetwork");
	ip4_addr_t ipaddr2;
	ip4_addr_t netmask2;
	ip4_addr_t gateway2;
	if(stringToIpAddr(config->getOfdAddr(), &ipaddr2) == false) {
		LOG_ERROR(LOG_ETH, "Wrong OFD address");
		return;
	}
	IP4_ADDR(&netmask2, 255,255,255,255);
	IP4_ADDR(&gateway2, 0,0,0,0);
	if(netif_add(netif2, &ipaddr2, &netmask2, &gateway2, NULL, netif2_init, ip_input) == NULL) {
		LOG_ERROR(LOG_ETH, "netif_add failed");
		return;
	}
	netif2->name[0] = 'e';
	netif2->name[1] = '1';
	netif_set_link_up(netif2);
	netif_set_up(netif2);
}

void Network::initFiscalServer() {
	LOG_INFO(LOG_ETH, "initFiscalServer");
	static ip4_addr_t ipaddr;
	if(stringToIpAddr(config->getOfdAddr(), &ipaddr) == false) {
		LOG_ERROR(LOG_ETH, "Wrong OFD address");
		return;
	}

	listenPcb = tcp_new();
	if(listenPcb == NULL) {
		LOG_ERROR(LOG_ETH, "tcp_new failed");
		return;
	}

	err_t result = tcp_bind(listenPcb, &ipaddr, config->getOfdPort());
	if(result != ERR_OK) {
		LOG_ERROR(LOG_ETH, "tcp_bind failed " << result);
		return;
	}
	// Обратите внимание, что создается новый контекст и удаляется старый
	listenPcb = tcp_listen(listenPcb);
	if(listenPcb == NULL) {
		LOG_ERROR(LOG_ETH, "tcp_listen failed");
		return;
	}
	tcp_arg(listenPcb, this);
	tcp_accept(listenPcb, tcp_accept_cb);
}

void Network::executeEnc28j60() {
	uint16_t plen = enc28j60->recvFrame(buf, SPI_BUFFER_SIZE);
    if(plen == 0) {
    	return;
    }

	LOG_DEBUG(LOG_ETH, "recvSpi<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
	if(LOG_ETH > LOG_LEVEL_INFO) {
		printEthernetFrame(buf, plen);
	}

	procRecvFrame(buf, plen);
}

//todo: перевести на TimerEngine - такая возможность предусмотрена
void Network::executeTimeouts() {
    sys_check_timeouts();
}

void Network::procRecvFrame(const uint8_t *data, uint16_t dataLen) {
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

/**
 * Вызывается при обнаружении входящего соединения.
 * Обратите внимание, что создается новый контекст для каждого соединения.
 * Новый контект нужно инициализировать калбеками каждый раз.
 */
err_t Network::tcp_accept_cb(void *arg, struct tcp_pcb *newPcb, err_t err) {
	LOG_INFO(LOG_ETH, "tcp_accept_fn2 " << LOG_IPADDR(newPcb->remote_ip.addr) << ":" << newPcb->remote_port	<< "->" << LOG_IPADDR(newPcb->local_ip.addr) << ":" << newPcb->local_port);
	Network *me = (Network*)arg;
	if(me->tcpGateway == NULL) {
		LOG_INFO(LOG_ETH, "Gateway not defined");
		tcp_abort(newPcb);
		return ERR_ABRT;
	}
	return me->tcpGateway->acceptConnection(newPcb);
}
#endif
