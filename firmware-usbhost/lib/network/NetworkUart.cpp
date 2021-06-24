#include "NetworkUart.h"
#include "TcpProtocol.h"

#include "lib/network/include/Network.h"

#include "common/timer/include/TimerEngine.h"
#include "common/logger/include/Logger.h"

#include "extern/lwip/include/lwip/tcp.h"
#include "extern/lwip/include/lwip/ip4_addr.h"

#define BUFFER_SIZE 512
#define IDLE_TIMEOUT 10000

NetworkUart::NetworkUart(
	TimerEngine *timers,
	Network *network,
	const char *ipaddr,
	uint16_t port
) :
	timers(timers),
	network(network),
	port(port),
	state(State_Idle),
	ethConn(NULL),
	sendBuf(BUFFER_SIZE),
	sendHandler(NULL),
	recvHandler(NULL)
{
	stringToIpAddr(ipaddr, (uint32_t*)(&this->ipaddr));
	recvBuf = new Fifo<uint8_t>(BUFFER_SIZE);
	this->sendTimer = timers->addTimer<NetworkUart, &NetworkUart::procSendTimer>(this);
	this->idleTimer = timers->addTimer<NetworkUart, &NetworkUart::procIdleTimer>(this);
}

void NetworkUart::send(uint8_t b) {
	LOG_TRACE(LOG_ETHTCP, "send " << b);
	sendBuf.addUint8(b);
	switch(state) {
	case State_Idle: gotoStateConnecting(); break;
	case State_Connecting: break;
	case State_Wait: gotoStateSending(); break;
	case State_Sending: break;
	case State_SendingWait: break;
	}
}

void NetworkUart::sendAsync(uint8_t b) {
	send(b);
}

uint8_t NetworkUart::receive() {
	return recvBuf->pop();
}

bool NetworkUart::isEmptyReceiveBuffer() {
	return recvBuf->isEmpty();
}

bool NetworkUart::isFullTransmitBuffer() {
	return (sendBuf.getLen() == sendBuf.getSize());
}

void NetworkUart::setReceiveHandler(UartReceiveHandler *handler) {
	recvHandler = handler;
}

void NetworkUart::setTransmitHandler(UartTransmitHandler *handler) {
	sendHandler = handler;
}

void NetworkUart::execute() {

}

void NetworkUart::procSendTimer() {
	LOG_TRACE(LOG_ETHTCP, "procSendTimer");
	switch(state) {
	case State_Sending: stateSendingTimeout(); return;
	default: LOG_ERROR(LOG_ETHTCP, "Unwaited timer " << state);
	}
}

void NetworkUart::procIdleTimer() {
	LOG_DEBUG(LOG_ETHTCP, "procIdleTimer " << state << " " << (uint32_t)ethConn);
	if(ethConn == NULL) {
		LOG_ERROR(LOG_ETHTCP, "Fatal error: ethConn already NULL");
		state = State_Idle;
		return;
	}
	tcp_abort(ethConn); // если вызывать tcp_close, то падает в конце концов
	ethConn = NULL;
	state = State_Idle;
}

err_t NetworkUart::tcp_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
	NetworkUart *me = (NetworkUart*)arg;
	LOG_DEBUG(LOG_ETHTCP, "tcp_connected " << err << " " << (uint32_t)(me->ethConn));
	if(me->ethConn != tpcb) {
		LOG_ERROR(LOG_ETHTCP, "ethConn != tcpb");
	}
	if(err != ERR_OK) {
		LOG_ERROR(LOG_ETHTCP, "tcp_connected failed");
		me->procError();
		return ERR_OK;
	}
	me->idleTimer->start(IDLE_TIMEOUT);
	if(me->sendBuf.getLen() > 0) {
		me->gotoStateSending();
	} else {
		me->gotoStateWait();
	}
	return ERR_OK;
}

/**
 * ¬ызываетс€ при обнаружени€ вход€щих данных.
 * ¬озвращаемое значение:
 *   ERR_OK - pbuf обработан и из под него можно освободить пам€ть
 *   ERR_*  - pbuf оставить в очереди и обратитьс€ в следующий раз
 */
err_t NetworkUart::tcp_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
	LOG_DEBUG(LOG_ETHTCP, "tcp_recv_cb");
	NetworkUart *me = (NetworkUart*)arg;
	if(me->ethConn != tpcb) {
		LOG_ERROR(LOG_ETHTCP, "ethConn != tcpb");
	}
	if(p == NULL) { // удаленна€ сторона закрыла соединение
		LOG_INFO(LOG_ETHTCP, "Remote side closing connection");
		tcp_close(me->ethConn); //todo: определитьс€ с тем, что правельнее вызывать tcp close или tcp_abort
		me->ethConn = NULL;
#if 0
		me->state = State_Idle;
#else
		me->gotoStateConnecting();
#endif
		return ERR_OK;
	}
	LOG_DEBUG(LOG_ETHTCP, "received " << p->len);
	LOG_TRACE_HEX(LOG_ETHTCP, (const uint8_t*)p->payload, p->len);
//todo: обработка переполнени€ буфера
	for(uint16_t i = 0; i < p->len; i++) {
		me->recvBuf->push(((uint8_t*)p->payload)[i]);
	}
	tcp_recved(tpcb, p->len);
	pbuf_free(p);
	me->idleTimer->start(IDLE_TIMEOUT);
	if(me->recvHandler != NULL) {
		me->recvHandler->handle();
	}
	return ERR_OK;
}

err_t NetworkUart::tcp_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len) {
	LOG_DEBUG(LOG_ETHTCP, "tcp_sent_cb " << len);
	NetworkUart *me = (NetworkUart*)arg;
	if(me->ethConn != tpcb) {
		LOG_ERROR(LOG_ETHTCP, "me->ethConn != tpcb");
	}
	if(me->sendBuf.getLen() > 0) {
		me->gotoStateSending();
	} else {
		me->gotoStateWait();
	}
	return ERR_OK;
}

void NetworkUart::tcp_err_cb(void *arg, err_t err) {
	LOG_DEBUG(LOG_ETHTCP, "tcp_err_cb " << err);
	NetworkUart *me = (NetworkUart*)arg;
	me->ethConn = NULL;
	me->procError();
}

void NetworkUart::gotoStateConnecting() {
	LOG_DEBUG(LOG_ETHTCP, "gotoStateConnecting");
	ethConn = tcp_new();
	if(ethConn == NULL) {
		LOG_ERROR(LOG_ETHTCP, "tcp_new failed");
		procError();
		return;
	}
	tcp_arg(ethConn, this);
	tcp_recv(ethConn, tcp_recv_cb);
	tcp_sent(ethConn, tcp_sent_cb);
	tcp_err(ethConn, tcp_err_cb);

	err_t result = tcp_connect(ethConn, &ipaddr, port, tcp_connected);
	if(result != ERR_OK) {
		LOG_ERROR(LOG_ETHTCP, "tcp_connect failed");
		procError();
		return;
	}

	state = State_Connecting;
}

void NetworkUart::gotoStateWait() {
	LOG_DEBUG(LOG_ETHTCP, "gotoStateWait");
	state = State_Wait;
}

void NetworkUart::gotoStateSending() {
	LOG_DEBUG(LOG_ETHTCP, "gotoStateSending");
	sendTimer->start(1);
	state = State_Sending;
}

void NetworkUart::stateSendingTimeout() {
	LOG_DEBUG(LOG_ETHTCP, "stateSendingTimeout " << sendBuf.getLen());
	LOG_TRACE_HEX(LOG_ETHTCP, sendBuf.getData(), sendBuf.getLen());
	err_t result = tcp_write(ethConn, sendBuf.getData(), sendBuf.getLen(), 1);
	if(result != ERR_OK) {
		procError();
		return;
	}
	sendBuf.clear();
	idleTimer->start(IDLE_TIMEOUT);
	state = State_SendingWait;
	return;
}

void NetworkUart::procError() {
	LOG_DEBUG(LOG_ETHTCP, "procError");
	if(ethConn != NULL) {
		tcp_abort(ethConn); // определитьс€ с тем, что правильнее вызывать tcp_close или tcp_abort
		ethConn = NULL;
	}
	sendBuf.clear();
	recvBuf->clear();
	state = State_Idle;
}
