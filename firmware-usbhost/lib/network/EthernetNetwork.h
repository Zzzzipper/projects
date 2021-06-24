#ifndef NETWORK_ETHERNETNETWORK_H
#define NETWORK_ETHERNETNETWORK_H

#include "EthernetInterface.h"

#include <stdint.h>

struct netif;
struct tcp_pcb;

class EthernetNetwork : public EthernetReceiver {
public:
	EthernetNetwork(EthernetInterface *interface);
	~EthernetNetwork();
	void init(uint32_t addr, uint32_t mask, uint32_t gateway);
	void shutdown();

	void procRecvFrame(const uint8_t *data, uint16_t dataLen) override;

private:
	struct netif *netif1;
	struct tcp_pcb *listenPcb;

	void initNetwork(uint32_t addr, uint32_t mask, uint32_t gateway);
};

#endif
