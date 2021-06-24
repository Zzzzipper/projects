#include "NetworkConn.h"
#include "TcpProtocol.h"

#include "lib/network/include/Network.h"

#include "common/timer/include/TimerEngine.h"
#include "common/tcpip/include/TcpIpUtils.h"
#include "common/logger/include/Logger.h"

#include "extern/lwip/include/lwip/tcp.h"
#include "extern/lwip/include/lwip/dns.h"

#define SEND_TIMEOUT 20000
#define SEND_MAX_SIZE TCP_MSS

NetworkConn::NetworkConn(TimerEngine *timers) :
	timers(timers),
	state(State_Idle),
	ethConn(NULL),
	recvBuf(NULL),
	recvDataFlag(false)
{
	this->timer = timers->addTimer<NetworkConn, &NetworkConn::procTimer>(this);
}

NetworkConn::~NetworkConn() {
	this->timers->deleteTimer(timer);
}

void NetworkConn::setObserver(EventObserver *observer) {
	courier.setRecipient(observer);
}

bool NetworkConn::connect(const char *domainname, uint16_t port, Mode mode) {
	LOG_DEBUG(LOG_ETHTCP, "connect");
	if(state != State_Idle) {
		LOG_ERROR(LOG_ETHTCP, "Wrong state " << state);
		return false;
	}

	this->port = port;

	err_t result = dns_gethostbyname(domainname, &ipaddr, dns_found_callback, this);
	if(result == ERR_INPROGRESS) {
		LOG_ERROR(LOG_ETHTCP, "Resolving addr");
		gotoStateResolving();
		return true;
	} else if(result != ERR_OK) {
		LOG_ERROR(LOG_ETHTCP, "Bad addr " << result);
		return false;
	}

	return gotoStateConnect();
}

bool NetworkConn::hasRecvData() {
	return recvDataFlag;
}

bool NetworkConn::send(const uint8_t *data, uint32_t dataLen) {
	LOG_DEBUG(LOG_ETHTCP, "send");
	if(state != State_Wait) {
		LOG_ERROR(LOG_ETHTCP, "Wrong state " << state);
		return false;
	}

	this->sendBuf = data;
	this->sendLen = dataLen;
	this->lastSendLen = dataLen > SEND_MAX_SIZE ? SEND_MAX_SIZE : dataLen;
	this->sendedLen = 0;

	LOG_TRACE_HEX(LOG_ETHTCP, sendBuf + sendedLen, lastSendLen);
	err_t result = tcp_write(ethConn, sendBuf + sendedLen, lastSendLen, 1);
	if(result != ERR_OK) {
		LOG_ERROR(LOG_ETHTCP, "Send failed " << result);
		return false;
	}

	timer->start(SEND_TIMEOUT);
	state = State_Send;
	return true;
}

bool NetworkConn::recv(uint8_t *buf, uint32_t bufSize) {
	LOG_DEBUG(LOG_ETHTCP, "recv " << bufSize);
	if(state != State_Wait) {
		LOG_ERROR(LOG_ETHTCP, "Wrong state " << state);
		return false;
	}

	recvBuf = buf;
	recvBufSize = bufSize;
	return true;
}

void NetworkConn::close() {
	LOG_DEBUG(LOG_ETHTCP, "close" << state);
	gotoStateClose();
}

void NetworkConn::procTimer() {
	LOG_DEBUG(LOG_ETHTCP, "procSendTimer");
	switch(state) {
	case State_Send: stateSendTimeout(); return;
	case State_Close: stateCloseTimeout(); return;
	case State_Disconnect: stateDisconnectTimeout(); return;
	default: LOG_ERROR(LOG_ETHTCP, "Unwaited timeout " << state);
	}
}

void NetworkConn::procRemoteClose() {
	LOG_DEBUG(LOG_ETHTCP, "procRemoteClose");
	state = State_Idle;
	courier.deliver(TcpIp::Event_Close);
}

void NetworkConn::procError() {
	LOG_DEBUG(LOG_ETHTCP, "procError");
	switch(state) {
	case State_Connect: {
		state = State_Idle;
		courier.deliver(TcpIp::Event_ConnectError);
		return;
	}
	default: {
		timer->start(1);
		state = State_Disconnect;
	}
	}
}

void NetworkConn::gotoStateResolving() {
	LOG_DEBUG(LOG_ETHTCP, "gotoStateResolving");
	state = State_Resolving;
}

void NetworkConn::stateResolvingDnsFound(const ip_addr_t *ipaddr) {
	LOG_DEBUG(LOG_ETHTCP, "stateResolvingDnsFound");
	if(ipaddr == NULL) {
		LOG_ERROR(LOG_ETHTCP, "Name not resolved");
		stateConnectError();
		return;
	}

	this->ipaddr.addr = ipaddr->addr;
	LOG_INFO(LOG_ETHTCP, "Resolved " << LOG_IPADDR(ipaddr->addr));
	if(gotoStateConnect() == false) {
		LOG_ERROR(LOG_ETHTCP, "Connect failed");
		stateConnectError();
		return;
	}
}

bool NetworkConn::gotoStateConnect() {
	LOG_DEBUG(LOG_ETHTCP, "gotoStateConnect");
	ethConn = tcp_new();
	if(ethConn == NULL) {
		LOG_ERROR(LOG_ETHTCP, "tcp_new failed");
		return false;
	}

	tcp_arg(ethConn, this);
	tcp_recv(ethConn, tcp_recv_callback);
	tcp_sent(ethConn, tcp_sent_callback);
	tcp_err(ethConn, tcp_err_callback);

	err_t result = tcp_connect(ethConn, &ipaddr, port, tcp_connected_callback);
	if(result != ERR_OK) {
		LOG_ERROR(LOG_ETHTCP, "tcp_connect failed");
		tcp_abort(ethConn);
		ethConn = NULL;
		return false;
	}

	recvDataFlag = false;
	state = State_Connect;
	return true;
}

void NetworkConn::stateConnectTcpConnected() {
	LOG_DEBUG(LOG_ETHTCP, "stateConnectTcpConnected");
	state = State_Wait;
	courier.deliver(TcpIp::Event_ConnectOk);
}

void NetworkConn::stateConnectError() {
	LOG_DEBUG(LOG_ETHTCP, "stateConnectError");
	state = State_Idle;
	courier.deliver(TcpIp::Event_ConnectError);
}

void NetworkConn::stateSendTcpSent(uint16_t dataLen) {
	LOG_DEBUG(LOG_ETHTCP, "stateSendTcpSent");
	sendedLen += dataLen;
	uint16_t tailLen = sendLen - sendedLen;
	if(tailLen > 0) {
		lastSendLen = tailLen > SEND_MAX_SIZE ? SEND_MAX_SIZE : tailLen;
		LOG_TRACE_HEX(LOG_ETHTCP, sendBuf + sendedLen, lastSendLen);
		err_t result = tcp_write(ethConn, sendBuf + sendedLen, lastSendLen, 1);
		if(result != ERR_OK) {
			LOG_ERROR(LOG_ETHTCP, "Send failed " << result);
			gotoStateClose();
			return;
		}

		timer->start(SEND_TIMEOUT);
		state = State_Send;
		return;
	} else {
		timer->stop();
		recvBuf = NULL;
		recvDataFlag = false;
		state = State_Wait;
		courier.deliver(TcpIp::Event_SendDataOk);
		return;
	}
}

void NetworkConn::stateSendTimeout() {
	LOG_DEBUG(LOG_ETHTCP, "stateSendTimeout");
	gotoStateClose();
}

uint16_t NetworkConn::procRecvData(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ETHTCP, "procRecvData " << dataLen << "/" << recvBufSize);
	LOG_TRACE_HEX(LOG_ETHTCP, data, dataLen);
	if(recvBuf == NULL) {
		recvDataFlag = true;
		courier.deliver(TcpIp::Event_IncomingData);
		return 0;
	}

	uint16_t procLen = 0;
	while(procLen < dataLen) {
		uint16_t recvLen = dataLen - procLen;
		if(recvLen > recvBufSize) { recvLen = recvBufSize; }
		for(uint16_t i = 0; i < recvLen; i++) {
			recvBuf[i] = data[procLen + i];
		}

		LOG_DEBUG(LOG_ETHTCP, "read " << recvLen << "/" << procLen << "/" << dataLen);
		LOG_DEBUG_HEX(LOG_ETHTCP, recvBuf, recvLen);
		procLen += recvLen;
		recvBuf = NULL;
		state = State_Wait;
		Event event(TcpIp::Event_RecvDataOk, recvLen);
		courier.deliver(&event);
		if(recvBuf == NULL || recvBufSize == 0) { // я тут теряю данные
			break;
		}
	}

	return procLen;
}

void NetworkConn::gotoStateClose() {
	LOG_DEBUG(LOG_TCPIP, "gotoStateClose");
	if(state == State_Idle) {
		LOG_DEBUG(LOG_TCPIP, "Already closed");
		timer->start(1);
		state = State_Disconnect;
		return;
	} if(state == State_Close) {
		LOG_DEBUG(LOG_TCPIP, "Already closing");
		return;
	} else {
		LOG_DEBUG(LOG_TCPIP, "Start closing");
		timer->start(1);
		state = State_Close;
		return;
	}
}

void NetworkConn::stateCloseTimeout() {
	LOG_DEBUG(LOG_ETHTCP, "stateCloseTimeout");
	tcp_recv(ethConn, NULL);
	tcp_sent(ethConn, NULL);
	err_t result = tcp_close(ethConn);
	if(result == ERR_OK) {
		LOG_DEBUG(LOG_ETHTCP, "tcp_close ERR_OK");
	} else {
		LOG_ERROR(LOG_ETHTCP, "tcp_close " << result);
	}
	state = State_Idle;
	courier.deliver(TcpIp::Event_Close);
}

void NetworkConn::stateDisconnectTimeout() {
	LOG_DEBUG(LOG_ETHTCP, "stateDisconnectTimeout");
	state = State_Idle;
	courier.deliver(TcpIp::Event_Close);
}

void NetworkConn::dns_found_callback(const char *name, const ip_addr_t *ipaddr, void *arg) {
	NetworkConn *me = (NetworkConn*)arg;
	LOG_DEBUG(LOG_ETHTCP, "dns_found_callback");
	me->stateResolvingDnsFound(ipaddr);
}

err_t NetworkConn::tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
	NetworkConn *me = (NetworkConn*)arg;
	LOG_DEBUG(LOG_ETHTCP, "tcp_connected " << err << " " << (uint32_t)(me->ethConn));
	if(me->ethConn != tpcb) {
		LOG_ERROR(LOG_ETHTCP, "ethConn != tcpb");
	}
	me->stateConnectTcpConnected();
	return ERR_OK;
}

/**
 * Вызывается при обнаружения входящих данных.
 * Возвращаемое значение:
 *   ERR_OK   - pbuf обработан и из под него можно освободить память
 *   ERR_BUF  - pbuf не обработан, не освобождать
 *   ERR_ABRT - удаленная сторона закрыла соединение
 *   ERR_*    - pbuf оставить в очереди и обратиться в следующий раз
 */
err_t NetworkConn::tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
	LOG_DEBUG(LOG_ETHTCP, "tcp_recv_callback");
	NetworkConn *me = (NetworkConn*)arg;
	if(me->ethConn != tpcb) {
		LOG_ERROR(LOG_ETHTCP, "ethConn != tcpb");
	}
	if(p == NULL) { // удаленная сторона закрыла соединение
		LOG_INFO(LOG_ETHTCP, "Remote side closing connection");
		tcp_abort(me->ethConn); //todo: определиться с тем, что правильнее вызывать tcp close или tcp_abort
		me->ethConn = NULL;
		me->procRemoteClose();
		return ERR_ABRT;
	}

	uint16_t recvLen = me->procRecvData((const uint8_t*)p->payload, p->len);
	if(recvLen == 0) {
		LOG_INFO(LOG_ETHTCP, "Not ready to recv");
		return ERR_BUF;
	}

	tcp_recved(tpcb, recvLen);
	pbuf_free(p);
	return ERR_OK;
}

err_t NetworkConn::tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
	LOG_DEBUG(LOG_ETHTCP, "tcp_sent_callback " << len);
	NetworkConn *me = (NetworkConn*)arg;
	if(me->ethConn != tpcb) {
		LOG_ERROR(LOG_ETHTCP, "me->ethConn != tpcb");
	}
	me->stateSendTcpSent(len);
	return ERR_OK;
}

void NetworkConn::tcp_err_callback(void *arg, err_t err) {
	LOG_DEBUG(LOG_ETHTCP, "tcp_err_callback " << err);
	NetworkConn *me = (NetworkConn*)arg;
	me->ethConn = NULL;
	me->procError();
}
