#if 1
#ifndef NETWORK_NETWORK_H
#define NETWORK_NETWORK_H

#include "common/config/include/ConfigModem.h"
#include "common/tcpip/include/TcpIp.h"
#include "common/timer/include/TimerEngine.h"

#include <stdint.h>

namespace Gsm { class Driver; }
class EthernetInterface;
class EthernetNetwork;
class FiscalNetwork;
class TcpGateway;
class ENC28J60;
struct netif;
struct tcp_pcb;

class Network {
public:
	Network(ConfigModem *config, Gsm::Driver *gsm, TimerEngine *timerEngine);
	~Network();
	void init();
	void shutdown();
	void restart();
	TcpIp *getTcpConnection1();
	TcpIp *getTcpConnection2();
	TcpIp *getTcpConnection3();
	TcpIp *getTcpConnection4();
	TcpIp *getTcpConnection5();
	void execute();
	void sendPing();
	void sendArpBroadcast();

private:
	ConfigModem *config;
	Gsm::Driver *gsm;
	EthernetInterface *ethernetDevice;
	EthernetNetwork *ethernetNetwork;
	FiscalNetwork *fiscalNetwork;
	TcpIp *conn1;
	TcpIp *conn2;
	TcpIp *conn3;
	TcpIp *conn4;
	TcpIp *conn5;

	void initEthernetDns();
	void executeTimeouts();
};

#endif

#else
#ifndef NETWORK_NETWORK_H
#define NETWORK_NETWORK_H

#include "extern/lwip/include/lwip/err.h"

#include "common/config/include/ConfigModem.h"

#include <stdint.h>

namespace Gsm { class Driver; }
class TcpGateway;
class ENC28J60;
struct netif;
struct tcp_pcb;

class Network {
public:
	Network(ConfigFiscal *config, Gsm::Driver *gsm);
	~Network();
	void init();
	void shutdown();
	TcpIp *getTcpConnection1();
	TcpIp *getTcpConnection2();
	TcpIp *getTcpConnection3();
	TcpIp *getTcpConnection4();
	TcpIp *getTcpConnection5();
	void execute();
	void sendSpi(const uint8_t *data, uint16_t dataLen);
	void sendPing();
	void sendArpBroadcast();

private:
	ConfigFiscal *config;
	Gsm::Driver *gsm;
	TcpGateway *tcpGateway;
	ENC28J60 *enc28j60;

	struct netif *netif1;
	struct netif *netif2;
	struct tcp_pcb *listenPcb;

	void initEthernet();
	void initFiscalNetwork();
	void initFiscalServerPort();
	void executeEnc28j60();
	void executeTimeouts();
	void procRecvFrame(const uint8_t *data, uint16_t dataLen);

	static err_t tcp_accept_cb(void *arg, struct tcp_pcb *newpcb, err_t err);
};

#endif
#endif
