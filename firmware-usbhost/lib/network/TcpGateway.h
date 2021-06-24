#ifndef _LIB_NETWORK_TCPGATEWAY_H
#define _LIB_NETWORK_TCPGATEWAY_H

#include "common/utils/include/Event.h"
#include "common/utils/include/Buffer.h"

#include "extern/lwip/include/lwip/err.h"

class TcpIp;
struct tcp_pcb;
namespace Sim900 { class Sim900; }

class TcpGateway : public EventObserver {
public:
	TcpGateway(TcpIp *simConn);
	virtual void proc(Event *event);
	err_t acceptConnection(struct tcp_pcb *newPcb);

private:
	enum State {
		State_Idle = 0,
		State_Connecting,
		State_Working,
		State_ClosingSim,
	};
	TcpIp *simConn;
	State state;
	struct tcp_pcb *ethConn;

	class Sender {
	public:
		Sender(TcpIp *simConn);
		void init();
		uint16_t send(const uint8_t *data, uint16_t dataLen);
		void sendComplete();

	private:
		enum State {
			State_Idle = 0,
			State_Sending,
		};
		TcpIp *simConn;
		State state;
		Buffer sendBuf;
	} sender;

	class Receiver {
	public:
		Receiver(TcpIp *simConn);
		void init(struct tcp_pcb *ethConn);
		void incomingData();
		void recvData(uint16_t dataLen);
		void sendComplete(uint16_t dataLen);

	private:
		enum State {
			State_Idle = 0,
			State_Receiving,
			State_Sending,
		};
		TcpIp *simConn;
		State state;
		struct tcp_pcb *ethConn;
		Buffer recvBuf;
		uint16_t sendedLen;

		void procError();
	} receiver;

	static err_t tcp_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
	static err_t tcp_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len);
	static void tcp_err_cb(void *arg, err_t err);

	void stateIdleEvent(Event *event);
	void stateConnectingEvent(Event *event);
	void stateWorkingEvent(Event *event);
	void stateClosingSimEvent(Event *event);
};

#endif
