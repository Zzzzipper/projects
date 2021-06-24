#ifndef LIB_NETWORK_NETWORKCONN_H
#define LIB_NETWORK_NETWORKCONN_H

#include "common/tcpip/include/TcpIp.h"
#include "common/uart/include/interface.h"
#include "common/utils/include/Buffer.h"
#include "common/utils/include/Fifo.h"

#include "extern/lwip/include/lwip/err.h"
#include "extern/lwip/include/lwip/ip_addr.h"

class TimerEngine;
class Timer;
class Network;
struct tcp_pcb;

class NetworkConn : public TcpIp {
public:
	NetworkConn(TimerEngine *timers);
	~NetworkConn();
	virtual void setObserver(EventObserver *observer) override;
	virtual bool connect(const char *domainname, uint16_t port, Mode mode) override;
	virtual bool hasRecvData() override;
	virtual bool send(const uint8_t *data, uint32_t dataLen) override;
	virtual bool recv(uint8_t *buf, uint32_t bufSize) override;
	virtual void close() override;

private:
	enum State {
		State_Idle = 0,
		State_Resolving,
		State_Connect,
		State_Wait,
		State_Send,
		State_SendWait,
		State_Close,
		State_Disconnect,
	};
	TimerEngine *timers;
	Timer *timer;
	State state;
	EventCourier courier;
	Event event;
	ip4_addr_t ipaddr;
	uint16_t port;
	struct tcp_pcb *ethConn;
	const uint8_t *sendBuf;
	uint16_t sendLen;
	uint16_t lastSendLen;
	uint16_t sendedLen;
	uint8_t *recvBuf;
	uint16_t recvBufSize;
	bool recvDataFlag;

	void procTimer();

	void gotoStateResolving();
	void stateResolvingDnsFound(const ip_addr_t *ipaddr);

	bool gotoStateConnect();
	void stateConnectTcpConnected();
	void stateConnectError();

	void stateSendTcpSent(uint16_t dataLen);
	void stateSendTimeout();

	void procRemoteClose();

	uint16_t procRecvData(const uint8_t *data, uint16_t dataLen);

	void procError();

	void gotoStateClose();
	void stateCloseTimeout();
	void stateDisconnectTimeout();

	static void dns_found_callback(const char *name, const ip_addr_t *ipaddr, void *arg);
	static err_t tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err);
	static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
	static err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len);
	static void tcp_err_callback(void *arg, err_t err);
};

#endif
