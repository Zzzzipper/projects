#ifndef NETWORK_FISCALNETWORK_H
#define NETWORK_FISCALNETWORK_H

#include "extern/lwip/include/lwip/err.h"

#include "common/config/include/ConfigModem.h"

#include <stdint.h>

namespace Gsm { class Driver; }
class TcpGateway;
struct netif;
struct tcp_pcb;

class FiscalNetwork {
public:
	FiscalNetwork(ConfigFiscal *config, Gsm::Driver *gsm);
	~FiscalNetwork();
	void init();
	void shutdown();

private:
	ConfigFiscal *config;
	TcpGateway *tcpGateway;
	struct netif *netif2;
	struct tcp_pcb *listenPcb;

	void initNetwork();
	void initServer();

	static err_t tcp_accept_cb(void *arg, struct tcp_pcb *newpcb, err_t err);
};

#endif
