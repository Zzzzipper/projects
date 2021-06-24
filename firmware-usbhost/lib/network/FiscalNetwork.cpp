#include "lib/network/FiscalNetwork.h"
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

#define ETHERNET_MTU 1500   // MTU value

FiscalNetwork::FiscalNetwork(
	ConfigFiscal *config,
	Gsm::Driver *gsm
) :
	config(config),
	tcpGateway(NULL),
	netif2(NULL),
	listenPcb(NULL)
{
	LOG_DEBUG(LOG_ETH, "FiscalNetwork");
	if(gsm != NULL) {
		LOG_INFO(LOG_ETH, "Tcp Gateway inited");
		tcpGateway = new TcpGateway(gsm->getTcpConnection4());
	}
	netif2 = new struct netif;
}

FiscalNetwork::~FiscalNetwork() {
	delete netif2;
	if(tcpGateway != NULL) { delete tcpGateway; }
}

void FiscalNetwork::init() {
	LOG_DEBUG(LOG_ETH, "init");
	initNetwork();
	initServer();
}

void FiscalNetwork::shutdown() {
	LOG_DEBUG(LOG_ETH, "shutdown");
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

void FiscalNetwork::initNetwork() {
	LOG_INFO(LOG_ETH, "initNetwork");
	ip4_addr_t ipaddr2;
	ip4_addr_t netmask2;
	ip4_addr_t gateway2;
	if(stringToIpAddr(config->getOfdAddr(), (uint32_t*)(&ipaddr2)) == false) {
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

void FiscalNetwork::initServer() {
	LOG_INFO(LOG_ETH, "initServer");
	static ip4_addr_t ipaddr;
	if(stringToIpAddr(config->getOfdAddr(), (uint32_t*)(&ipaddr)) == false) {
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
	// ќбратите внимание, что создаетс€ новый контекст и удал€етс€ старый
	listenPcb = tcp_listen(listenPcb);
	if(listenPcb == NULL) {
		LOG_ERROR(LOG_ETH, "tcp_listen failed");
		return;
	}
	tcp_arg(listenPcb, this);
	tcp_accept(listenPcb, tcp_accept_cb);
}

/**
 * ¬ызываетс€ при обнаружении вход€щего соединени€.
 * ќбратите внимание, что создаетс€ новый контекст дл€ каждого соединени€.
 * Ќовый контект нужно инициализировать калбеками каждый раз.
 */
err_t FiscalNetwork::tcp_accept_cb(void *arg, struct tcp_pcb *newPcb, err_t err) {
	LOG_INFO(LOG_ETH, "tcp_accept_fn2 " << LOG_IPADDR(newPcb->remote_ip.addr) << ":" << newPcb->remote_port	<< "->" << LOG_IPADDR(newPcb->local_ip.addr) << ":" << newPcb->local_port);
	FiscalNetwork *me = (FiscalNetwork*)arg;
	if(me->tcpGateway == NULL) {
		LOG_INFO(LOG_ETH, "Gateway not defined");
		tcp_abort(newPcb);
		return ERR_ABRT;
	}
	return me->tcpGateway->acceptConnection(newPcb);
}
