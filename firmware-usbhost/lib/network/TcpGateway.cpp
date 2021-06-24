#include "TcpGateway.h"
#include "TcpProtocol.h"

#include "common/tcpip/include/TcpIp.h"
#include "common/tcpip/include/TcpIpUtils.h"
#include "common/utils/include/StringBuilder.h"
#include "common/logger/include/Logger.h"

#include "extern/lwip/include/lwip/tcp.h"

#define BUFFER_SIZE  1023

TcpGateway::Sender::Sender(TcpIp *simConn) : simConn(simConn), state(State_Idle), sendBuf(BUFFER_SIZE) {
	LOG_DEBUG(LOG_ETH, "Sender");
}

void TcpGateway::Sender::init() {
	LOG_DEBUG(LOG_ETH, "init");
	state = State_Idle;
}

uint16_t TcpGateway::Sender::send(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ETH, "send");
	if(state != State_Idle) {
		LOG_ERROR(LOG_ETH, "Wrong state " << state);
		return 0;
	}

	sendBuf.clear();
	if(dataLen > sendBuf.getSize()) { dataLen = sendBuf.getSize(); }
	sendBuf.add(data, dataLen);

	if(simConn->send(sendBuf.getData(), sendBuf.getLen()) == false) {
		LOG_ERROR(LOG_ETH, "send failed");
		return 0;
	}

	state = State_Sending;
	return sendBuf.getLen();
}

void TcpGateway::Sender::sendComplete() {
	LOG_DEBUG(LOG_ETH, "sendComplete");
	state = State_Idle;
}

TcpGateway::Receiver::Receiver(TcpIp *simConn) : simConn(simConn), state(State_Idle), recvBuf(BUFFER_SIZE) {
	LOG_DEBUG(LOG_ETH, "Receiver");
}

void TcpGateway::Receiver::init(struct tcp_pcb *ethConn) {
	LOG_DEBUG(LOG_ETH, "init");
	this->ethConn = ethConn;
	this->state = State_Idle;
}

void TcpGateway::Receiver::incomingData() {
	LOG_DEBUG(LOG_ETH, "incomingData");
	if(state != State_Idle) {
		LOG_ERROR(LOG_ETH, "Wrong state " << state);
		return;
	}
	if(simConn->hasRecvData() == false) {
		LOG_ERROR(LOG_ETH, "No data");
		state = State_Idle;
		return;
	}
	if(simConn->recv(recvBuf.getData(), recvBuf.getSize()) == false) {
		LOG_ERROR(LOG_ETH, "recv");
		procError();
		return;
	}
	state = State_Receiving;
}

void TcpGateway::Receiver::recvData(uint16_t dataLen) {
	LOG_DEBUG(LOG_ETH, "recvData " << dataLen);
	if(state != State_Receiving) {
		LOG_ERROR(LOG_ETH, "Wrong state " << state);
		return;
	}
	LOG_TRACE_HEX(LOG_ETH, recvBuf.getData(), dataLen);
	recvBuf.setLen(dataLen);

	uint16_t sendSize = tcp_sndbuf(ethConn);
	LOG_ERROR(LOG_ETH, "tcp_sndbuf=" << sendSize);

	err_t result = tcp_write(ethConn, recvBuf.getData(), recvBuf.getLen(), 1); // по логике должно быть 0, но почему-то данные не отправляются в этом режиме
	if(result != ERR_OK) {
		LOG_ERROR(LOG_ETH, "tcp_write failed " << result);
		procError();
		return;
	}
	sendedLen = 0;
	state = State_Sending;
}

void TcpGateway::Receiver::sendComplete(uint16_t dataLen) {
	LOG_DEBUG(LOG_ETH, "sendComplete " << dataLen << ":" << sendedLen);
	if(state != State_Sending) {
		LOG_ERROR(LOG_ETH, "Wrong state " << state);
		return;
	}

	sendedLen += dataLen;
	if(sendedLen > recvBuf.getLen()) {
		LOG_ERROR(LOG_ETH, "Wrong len " << sendedLen << ">" << recvBuf.getLen());
		procError();
		return;
	}
	if(sendedLen < recvBuf.getLen()) {
		LOG_INFO(LOG_ETH, "Wait sending " << sendedLen << ":" << recvBuf.getLen());
		return;
	}

	if(simConn->hasRecvData() == false) {
		LOG_INFO(LOG_ETH, "No data");
		state = State_Idle;
		return;
	}
	if(simConn->recv(recvBuf.getData(), recvBuf.getSize()) == false) {
		LOG_ERROR(LOG_ETH, "recv");
		procError();
		return;
	}
	state = State_Receiving;
}

void TcpGateway::Receiver::procError() {
	LOG_DEBUG(LOG_ETH, "procError");
	state = State_Idle;
}

TcpGateway::TcpGateway(TcpIp *simConn) : simConn(simConn), state(State_Idle), ethConn(NULL), sender(simConn), receiver(simConn) {
	LOG_DEBUG(LOG_ETH, "TcpGateway");
	this->simConn->setObserver(this);
}

void TcpGateway::proc(Event *event) {
	LOG_DEBUG(LOG_ETH, "proc");
	switch(state) {
	case State_Idle: stateIdleEvent(event); return;
	case State_Connecting: stateConnectingEvent(event); return;
	case State_Working: stateWorkingEvent(event); return;
	case State_ClosingSim: stateClosingSimEvent(event); return;
	}
}

err_t TcpGateway::acceptConnection(struct tcp_pcb *newPcb) {
	LOG_DEBUG(LOG_ETH, "acceptConnection");
	if(state != State_Idle) {
		LOG_ERROR(LOG_ETH, "Wrong state " << state);
		tcp_abort(newPcb);
		return ERR_ABRT;
	}

	StringBuilder remoteAddr(IPADDR_STRING_SIZE, IPADDR_STRING_SIZE);
	remoteAddr << LOG_IPADDR(newPcb->local_ip.addr);
	if(simConn->connect(remoteAddr.getString(), newPcb->local_port, TcpIp::Mode_TcpIp) == false) {
		LOG_ERROR(LOG_ETH, "connect failed");
		tcp_abort(newPcb);
		return ERR_ABRT;
	}

	LOG_INFO(LOG_ETH, "connect start");
	tcp_arg(newPcb, this);
	tcp_recv(newPcb, tcp_recv_cb);
	tcp_sent(newPcb, tcp_sent_cb);
	tcp_err(newPcb, tcp_err_cb);
	tcp_accepted(newPcb);
	ethConn = newPcb;
	state = State_Connecting;
	return ERR_OK;
}

/**
 * Вызывается при обнаружения входящих данных.
 * Возвращаемое значение:
 *   ERR_OK - pbuf обработан и из под него можно освободить память
 *   ERR_*  - pbuf оставить в очереди и обратиться в следующий раз
 */
err_t TcpGateway::tcp_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
	LOG_DEBUG(LOG_ETH, "tcp_recv_cb");
	TcpGateway *me = (TcpGateway*)arg;
	if(p == NULL) { // удаленная сторона закрыла соединение
		LOG_INFO(LOG_ETH, "Remote side closing connection");
		tcp_close(me->ethConn); //todo: определиться с тем, что правельнее вызывать tcp close или tcp_abort
		me->simConn->close();
		me->state = State_ClosingSim;
		return ERR_OK;
	}
	LOG_DEBUG(LOG_ETH, "received " << p->len);
	LOG_TRACE_HEX(LOG_ETH, (const uint8_t*)p->payload, p->len);
	if(me->state != State_Working) {
		LOG_ERROR(LOG_ETH, "Wrong state " << me->state);
		return ERR_INPROGRESS;
	}

	uint16_t len = me->sender.send((const uint8_t*)p->payload, p->len);
	if(len == 0) {
		LOG_ERROR(LOG_ETH, "Not now " << me->state);
		return ERR_INPROGRESS;
	}

	tcp_recved(me->ethConn, len);
	pbuf_free(p);
	return ERR_OK;
}

err_t TcpGateway::tcp_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len) {
	LOG_DEBUG(LOG_ETH, "tcp_sent_cb");
	TcpGateway *me = (TcpGateway*)arg;
	me->receiver.sendComplete(len);
	return ERR_OK;
}

void TcpGateway::tcp_err_cb(void *arg, err_t err) {
	LOG_DEBUG(LOG_ETH, "tcp_err_cb " << err);
	TcpGateway *me = (TcpGateway*)arg;
	me->simConn->close();
	me->state = State_ClosingSim;
}

void TcpGateway::stateIdleEvent(Event *event) {
	LOG_DEBUG(LOG_ETH, "stateIdleEvent");
	switch(event->getType()) {
	default: {
		LOG_INFO(LOG_ETH, "Unwaited event " << state << "," << event->getType());
		return;
	}
	}
}

void TcpGateway::stateConnectingEvent(Event *event) {
	LOG_DEBUG(LOG_ETH, "stateConnectingEvent");
	switch(event->getType()) {
	case TcpIp::Event_ConnectOk: {
		LOG_INFO(LOG_ETH, "Event_ConnectOk");
		sender.init();
		receiver.init(ethConn);
		state = State_Working;
		return;
	}
	case TcpIp::Event_ConnectError: {
		LOG_INFO(LOG_ETH, "Event_ConnectError");
		tcp_close(ethConn);
		state = State_Idle;
		return;
	}
	default: {
		LOG_INFO(LOG_ETH, "Unwaited event " << state << "," << event->getType());
		return;
	}
	}
}

void TcpGateway::stateWorkingEvent(Event *event) {
	LOG_DEBUG(LOG_ETH, "stateWorkingEvent");
	switch(event->getType()) {
	case TcpIp::Event_IncomingData: {
		LOG_INFO(LOG_ETH, "Event_IncomingData");
		receiver.incomingData();
		return;
	}
	case TcpIp::Event_RecvDataOk: {
		LOG_INFO(LOG_ETH, "Event_RecvDataOk " << event->getUint16());
		receiver.recvData(event->getUint16());
		return;
	}
	case TcpIp::Event_SendDataOk: {
		LOG_INFO(LOG_ETH, "Event_SendDataOk");
		sender.sendComplete();
		return;
	}
	case TcpIp::Event_Close: {
		LOG_INFO(LOG_ETH, "Event_Close");
		tcp_close(ethConn);
		state = State_Idle;
		return;
	}
	default: {
		LOG_INFO(LOG_ETH, "Unwaited event " << state << "," << event->getType());
		return;
	}
	}
}

void TcpGateway::stateClosingSimEvent(Event *event) {
	LOG_DEBUG(LOG_ETH, "stateClosingEvent");
	switch(event->getType()) {
	case TcpIp::Event_Close: {
		LOG_INFO(LOG_ETH, "Event_Close");
		state = State_Idle;
		return;
	}
	default: {
		LOG_INFO(LOG_ETH, "Unwaited event " << state << "," << event->getType());
		return;
	}
	}
}
