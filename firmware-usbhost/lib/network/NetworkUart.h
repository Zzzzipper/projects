#ifndef LIB_NETWORK_NETWORKUART_H
#define LIB_NETWORK_NETWORKUART_H

#include "common/uart/include/interface.h"
#include "common/utils/include/Buffer.h"
#include "common/utils/include/Fifo.h"

#include "extern/lwip/include/lwip/err.h"
#include "extern/lwip/include/lwip/ip4_addr.h"

class TimerEngine;
class Timer;
class Network;
struct tcp_pcb;

class NetworkUart : public AbstractUart {
public:
	NetworkUart(TimerEngine *timers, Network *network, const char *ipaddr, uint16_t port);
	virtual void send(uint8_t b) override;
	virtual void sendAsync(uint8_t b) override;
	virtual uint8_t receive() override;
	virtual bool isEmptyReceiveBuffer() override;
	virtual bool isFullTransmitBuffer() override;
	virtual void setReceiveHandler(UartReceiveHandler *handler) override;
	virtual void setTransmitHandler(UartTransmitHandler *handler) override;
	virtual void execute() override;

private:
	enum State {
		State_Idle = 0,
		State_Connecting,
		State_Wait,
		State_Sending,
		State_SendingWait,
	};
	TimerEngine *timers;
	Network *network;
	ip4_addr_t ipaddr;
	uint16_t port;
	Timer *sendTimer;
	Timer *idleTimer;
	State state;
	struct tcp_pcb *ethConn;
	Buffer sendBuf;
	Fifo<uint8_t> *recvBuf;
	UartTransmitHandler *sendHandler;
	UartReceiveHandler *recvHandler;

	void procSendTimer();
	void procIdleTimer();

	void gotoStateConnecting();
	void gotoStateWait();
	void gotoStateSending();
	void stateSendingTimeout();
	void procError();

	static err_t tcp_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
	static err_t tcp_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
	static err_t tcp_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len);
	static void tcp_err_cb(void *arg, err_t err);
};

#endif
