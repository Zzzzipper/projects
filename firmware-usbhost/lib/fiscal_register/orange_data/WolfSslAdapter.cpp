#include "WolfSslAdapter.h"

#include "logger/include/Logger.h"

#include "wolfssl/ssl.h"

#define PACKET_SIZE 768

WolfSslAdapter::WolfSslAdapter(TcpIp *conn) :
	conn(conn),
	buf(PACKET_SIZE)
{
	conn->setObserver(this);
}

void WolfSslAdapter::setObserver(EventObserver *observer) {
	courier.setRecipient(observer);
}

bool WolfSslAdapter::connect(const char *domainname, uint16_t port) {
	if(conn->connect(domainname, port, TcpIp::Mode_TcpIp) == false) {
		LOG_ERROR(LOG_FRP, "Start connect failed");
		return false;
	}
	state = State_Connecting;
	return true;
}

int WolfSslAdapter::procSend(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FRP, "procSend " << dataLen);
	switch(state) {
	case State_Wait: return stateWaitProcSend(data, dataLen);
	case State_Sending: return WOLFSSL_CBIO_ERR_WANT_WRITE;
	case State_SendComplete: return stateSendCompleteProcSend();
	default: LOG_ERROR(LOG_FRP, "Unwaited send " << state); return -1;
	}
}

int WolfSslAdapter::procRecv(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FRP, "procRecv " << dataLen);
	switch(state) {
	case State_Wait: return stateWaitProcRecv(data, dataLen);
	case State_Recving: return WOLFSSL_CBIO_ERR_WANT_READ;
	case State_RecvComplete: return stateRecvCompleteProcRecv(data, dataLen);
	default: LOG_ERROR(LOG_FRP, "Unwaited send " << state); return -1;
	}
}

void WolfSslAdapter::close() {
	LOG_DEBUG(LOG_FRP, "close");
	state = State_Closing;
	conn->close();
}

void WolfSslAdapter::proc(Event *event) {
	LOG_DEBUG(LOG_FRP, "proc " << state);
	switch(state) {
	case State_Connecting: stateConnectingEvent(event); break;
	case State_Sending: stateSendingEvent(event); break;
	case State_RecvWait: stateRecvWaitEvent(event); break;
	case State_Recving: stateRecvingEvent(event); break;
	case State_Closing: stateClosingEvent(event); break;
	default: LOG_ERROR(LOG_FRP, "Unwaited data state=" << state << "," << event->getType());
	}
}

void WolfSslAdapter::stateConnectingEvent(Event *event) {
	LOG_DEBUG(LOG_FRP, "stateConnectingEvent");
	switch(event->getType()) {
	case TcpIp::Event_ConnectOk: {
		state = State_Wait;
		courier.deliver(event);
		return;
	}
	case TcpIp::Event_ConnectError: {
		state = State_Idle;
		EventError errorEvent(TcpIp::Event_ConnectError, "wsa", __LINE__, ((EventError*)event)->trace.getString());
		courier.deliver(&errorEvent);
		return;
	}
	case TcpIp::Event_Close: {
		state = State_Idle;
		courier.deliver(event);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

int WolfSslAdapter::stateWaitProcSend(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FRP, "stateWaitProcSend");
	uint16_t sendLen = dataLen;
	if(sendLen > buf.getSize()) {
		sendLen = buf.getSize();
	}
	buf.clear();
	buf.add(data, sendLen);

	LOG_DEBUG_HEX(LOG_FRP, buf.getData(), buf.getLen());
	bool ret = conn->send(buf.getData(), buf.getLen());
	if(ret == false) {
		LOG_ERROR(LOG_FRP, "send failed");
		conn->close();
		state = State_Idle;
		return WOLFSSL_CBIO_ERR_CONN_RST;
	}

	state = State_Sending;
	return WOLFSSL_CBIO_ERR_WANT_WRITE;
}

void WolfSslAdapter::stateSendingEvent(Event *event) {
	LOG_DEBUG(LOG_FRP, "stateSendingEvent");
	switch(event->getType()) {
	case TcpIp::Event_SendDataOk: {
		state = State_SendComplete;
		courier.deliver(TcpIp::Event_SendDataOk);
		return;
	}
	case TcpIp::Event_SendDataError:
	case TcpIp::Event_Close: {
		state = State_Idle;
		courier.deliver(TcpIp::Event_Close);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

int WolfSslAdapter::stateSendCompleteProcSend() {
	LOG_DEBUG(LOG_FRP, "stateSendCompleteProcSend");
	state = State_Wait;
	return buf.getLen();
}

int WolfSslAdapter::stateWaitProcRecv(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FRP, "stateWaitProcRecv");
	(void)data;
	recvLen = dataLen;
	if(recvLen > buf.getSize()) {
		recvLen = buf.getSize();
	}

	if(conn->hasRecvData() == true) {
		if(conn->recv(buf.getData(), recvLen) == false) {
			LOG_ERROR(LOG_FRP, "recv failed");
			conn->close();
			state = State_Idle;
			return WOLFSSL_CBIO_ERR_CONN_RST;
		}
		state = State_Recving;
		return WOLFSSL_CBIO_ERR_WANT_READ;
	} else {
		state = State_RecvWait;
		return WOLFSSL_CBIO_ERR_WANT_READ;
	}
}

void WolfSslAdapter::stateRecvWaitEvent(Event *event) {
	LOG_DEBUG(LOG_FRP, "stateRecvWaitEvent");
	switch(event->getType()) {
	case TcpIp::Event_IncomingData: {
		if(conn->recv(buf.getData(), recvLen) == false) {
			conn->close();
			state = State_Idle;
			courier.deliver(TcpIp::Event_Close);
			return;
		}
		state = State_Recving;
		return;
	}
	case TcpIp::Event_Close: {
		state = State_Idle;
		courier.deliver(TcpIp::Event_Close);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

void WolfSslAdapter::stateRecvingEvent(Event *event) {
	LOG_DEBUG(LOG_FRP, "stateRecvingEvent");
	switch(event->getType()) {
	case TcpIp::Event_RecvDataOk: {
		buf.setLen(event->getUint16());
		LOG_DEBUG_HEX(LOG_FRP, buf.getData(), buf.getLen());
		state = State_RecvComplete;
		courier.deliver(TcpIp::Event_RecvDataOk);
		return;
	}
	case TcpIp::Event_RecvDataError:
	case TcpIp::Event_Close: {
		state = State_Idle;
		courier.deliver(TcpIp::Event_Close);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

int WolfSslAdapter::stateRecvCompleteProcRecv(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FRP, "stateRecvCompleteProcRecv " << recvLen << "," << buf.getLen() << "/" << dataLen);
	for(uint16_t i = 0; i < buf.getLen(); i++) {
		data[i] = buf[i];
	}
	state = State_Wait;
	return buf.getLen();
}

void WolfSslAdapter::stateClosingEvent(Event *event) {
	LOG_DEBUG(LOG_FRP, "stateClosingEvent");
	switch(event->getType()) {
	case TcpIp::Event_Close: {
		state = State_Idle;
		courier.deliver(event);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}
