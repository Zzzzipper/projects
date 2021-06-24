#include "WolfSslConnection.h"

#include "memory/include/MallocTest.h"
#include "logger/include/Logger.h"

#include "wolfssl/ssl.h"
#include "wolfssl/internal.h"

//RFC5246 TLS error codes
//https://tools.ietf.org/html/rfc5246
WolfSslConnection::WolfSslConnection(WolfSslAdapterInterface *conn) :
	conn(conn),
	ctx(NULL),
	ssl(NULL),
	session(NULL)
{
	conn->setObserver(this);
}

WolfSslConnection::~WolfSslConnection() {
	cleanup();
}

bool WolfSslConnection::init(ConfigCert *publicKey, ConfigCert *privateKey) {
	if(ctx != NULL) {
		LOG_ERROR(LOG_FRP, "CTX already inited");
		return false;
	}

#ifdef DEBUG_MEMORY
	wolfSSL_SetAllocators(MallocMallocEx, MallocFreeEx, MallocReallocEx);
#endif
#ifdef DEBUG
	wolfSSL_SetLoggingCb(CbLog);
	wolfSSL_Debugging_ON();
#endif

	if(wolfSSL_Init() != SSL_SUCCESS) {
		LOG_ERROR(LOG_FRP, "SSL init failed");
		return false;
	}

	int ret;
	ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
	if(ctx == NULL) {
		LOG_ERROR(LOG_FRP, "Error in setting client ctx");
		return false;
	}

	wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);
	wolfSSL_SetIORecv(ctx, conn->CbIORecv);
	wolfSSL_SetIOSend(ctx, conn->CbIOSend);

	StringBuilder buf(CERT_SIZE, CERT_SIZE);
	MemoryResult result = publicKey->load(&buf);
	if(result != MemoryResult_Ok) {
		return false;
	}

	ret = wolfSSL_CTX_use_certificate_buffer(ctx, buf.getData(), buf.getLen(), WOLFSSL_FILETYPE_PEM);
	if(ret != SSL_SUCCESS) {
		LOG_ERROR(LOG_FRP, "SSL public key failed " << ret);
		cleanup();
		return false;
	}

	result = privateKey->load(&buf);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_FRP, "SSL private key failed " << ret);
		return false;
	}

	ret = wolfSSL_CTX_use_PrivateKey_buffer(ctx, buf.getData(), buf.getLen(), WOLFSSL_FILETYPE_PEM);
	if(ret != SSL_SUCCESS) {
		LOG_ERROR(LOG_FRP, "SSL private key failed " << ret);
		cleanup();
		return false;
	}

	session = NULL;
	return true;
}

void WolfSslConnection::cleanup() {
	if(ssl != NULL) {
		wolfSSL_free(ssl);
		ssl = NULL;
	}
	if(ctx == NULL) {
		wolfSSL_CTX_free(ctx);
		wolfSSL_Cleanup();
		ctx = NULL;
	}
}

void WolfSslConnection::setObserver(EventObserver *observer) {
	courier.setRecipient(observer);
}

bool WolfSslConnection::connect(const char *domainname, uint16_t port, Mode mode) {
	LOG_DEBUG(LOG_FRP, "connect");
	if(mode != TcpIp::Mode_TcpIpOverSsl) {
		LOG_ERROR(LOG_FRP, "Wrong option");
		return false;
	}

	if(conn->connect(domainname, port) == false) {
		LOG_ERROR(LOG_FRP, "Start connect failed");
		return false;
	}

	ssl = wolfSSL_new(ctx);
	if(ssl == NULL) {
		LOG_ERROR(LOG_FRP, "SSL init failed");
		return false;
	}

	if(session != NULL) {
		if(wolfSSL_set_session(ssl, session) != SSL_SUCCESS) {
			LOG_INFO(LOG_FRP, "wolfSSL_set_session failed");
			session = NULL;
		}
	}

	wolfSSL_SetIOReadCtx(ssl, conn);
	wolfSSL_SetIOWriteCtx(ssl, conn);
	state = State_Connect;
	return true;
}

bool WolfSslConnection::hasRecvData() {
	return false;
}

bool WolfSslConnection::send(const uint8_t *data, uint32_t len) {
	if(state != State_Wait) {
		return false;
	}
	gotoStateSend(data, len);
	return true;
}

bool WolfSslConnection::recv(uint8_t *buf, uint32_t bufSize) {
	if(state != State_Wait) {
		return false;
	}
	gotoStateRecv(buf, bufSize);
	return true;
}

void WolfSslConnection::close() {
	gotoStateDisconnect();
}

void WolfSslConnection::proc(Event *event) {
	LOG_DEBUG(LOG_FRP, "proc " << state);
	switch(state) {
	case State_Connect: stateConnectEvent(event); break;
	case State_Send: stateSendEvent(event); break;
	case State_Recv: stateRecvEvent(event); break;
	case State_Disconnect: stateDisconnectEvent(event); break;
	default: LOG_ERROR(LOG_FRP, "Unwaited data state=" << state << "," << event->getType());
	}
}

void WolfSslConnection::CbLog(const int logLevel, const char *const logMessage) {
	switch(logLevel) {
	case ERROR_LOG: (*Logger::get()) << "E " << logMessage << Logger::endl; return;
	case INFO_LOG:  (*Logger::get()) << "I " << logMessage << Logger::endl; return;
	case ENTER_LOG: (*Logger::get()) << "> " << logMessage << Logger::endl; return;
	case LEAVE_LOG: (*Logger::get()) << "< " << logMessage << Logger::endl; return;
	default: (*Logger::get()) << "T" << logMessage << Logger::endl; return;
	}
}

void WolfSslConnection::stateConnectEvent(Event *event) {
	LOG_DEBUG(LOG_FRP, "stateConnectEvent");
	switch(event->getType()) {
	case TcpIp::Event_ConnectOk:
	case TcpIp::Event_SendDataOk:
	case TcpIp::Event_RecvDataOk: {
		int ret = wolfSSL_connect(ssl);
		if(ret != SSL_SUCCESS) {
			int error = wolfSSL_get_error(ssl, ret);
			if(error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
				return;
			} else {
				LOG_ERROR(LOG_FRP, "wolfSSL_connect failed " << error);
				gotoStateDisconnect();
				return;
			}
		}
		LOG_INFO(LOG_FRP, "CONNECTED");
		state = State_Wait;
		courier.deliver(TcpIp::Event_ConnectOk);
		return;
	}
	case TcpIp::Event_ConnectError: {
		LOG_INFO(LOG_FRP, "CONNECT FAILED");
		wolfSSL_free(ssl);
		ssl = NULL;
#ifdef DEBUG_MEMORY
		MallocPrintInfo();
#endif
		state = State_Idle;
		EventError errorEvent(TcpIp::Event_ConnectError, "wsc", __LINE__, ((EventError*)event)->trace.getString());
		courier.deliver(&errorEvent);
		return;
	}
	case TcpIp::Event_Close: {
		LOG_INFO(LOG_FRP, "CONNECT FAILED");
		wolfSSL_free(ssl);
		ssl = NULL;
#ifdef DEBUG_MEMORY
		MallocPrintInfo();
#endif
		state = State_Idle;
		courier.deliver(event);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

void WolfSslConnection::gotoStateSend(const uint8_t *data, uint32_t len) {
	LOG_DEBUG(LOG_FRP, "gotoStateSend");
	sendData = data;
	sendDataLen = len;
	state = State_Send;
	int ret = wolfSSL_write(ssl, sendData, sendDataLen);
	if(ret <= 0) {
		int error = wolfSSL_get_error(ssl, ret);
		if(error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
			return;
		} else {
			LOG_ERROR(LOG_FRP, "wolfSSL_write failed " << error);
			gotoStateDisconnect();
			return;
		}
	}
	LOG_INFO(LOG_FRP, "SENDED");
	state = State_Wait;
	courier.deliver(TcpIp::Event_SendDataOk);
}

void WolfSslConnection::stateSendEvent(Event *event) {
	LOG_DEBUG(LOG_FRP, "stateSendEvent");
	switch(event->getType()) {
	case TcpIp::Event_SendDataOk: {
		int ret = wolfSSL_write(ssl, sendData, sendDataLen);
		if(ret <= 0) {
			int error = wolfSSL_get_error(ssl, ret);
			if(error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
				return;
			} else {
				LOG_ERROR(LOG_FRP, "wolfSSL_write failed " << error);
				gotoStateDisconnect();
				return;
			}
		}
		LOG_INFO(LOG_FRP, "SENDED");
		state = State_Wait;
		courier.deliver(TcpIp::Event_SendDataOk);
		return;
	}
	case TcpIp::Event_SendDataError:
	case TcpIp::Event_Close: {
		gotoStateDisconnect();
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

void WolfSslConnection::gotoStateRecv(uint8_t *buf, uint32_t bufSize) {
	LOG_DEBUG(LOG_FRP, "gotoStateRecv");
	recvBuf = buf;
	recvBufSize = bufSize;
	state = State_Recv;
	int ret = wolfSSL_read(ssl, recvBuf, recvBufSize);
	if(ret <= 0) {
		int error = wolfSSL_get_error(ssl, ret);
		if(error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
			return;
		} else {
			LOG_ERROR(LOG_FRP, "wolfSSL_read failed " << error);
			gotoStateDisconnect();
			return;
		}
	}
	LOG_INFO(LOG_FRP, "RECVED " << ret);
	state = State_Wait;
	Event event(TcpIp::Event_RecvDataOk, (uint16_t)ret);
	courier.deliver(&event);
}

void WolfSslConnection::stateRecvEvent(Event *event) {
	LOG_DEBUG(LOG_FRP, "stateRecvEvent");
	switch(event->getType()) {
	case TcpIp::Event_RecvDataOk: {
		int ret = wolfSSL_read(ssl, recvBuf, recvBufSize);
		if(ret <= 0) {
			int error = wolfSSL_get_error(ssl, ret);
			if(error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
				return;
			} else {
				LOG_ERROR(LOG_FRP, "wolfSSL_read failed " << error);
				gotoStateDisconnect();
				return;
			}
		}
		LOG_INFO(LOG_FRP, "RECVED " << ret);
		state = State_Wait;
		Event event2(TcpIp::Event_RecvDataOk, (uint16_t)ret);
		courier.deliver(&event2);
		return;
	}
	case TcpIp::Event_RecvDataError:
	case TcpIp::Event_Close: {
		gotoStateDisconnect();
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

void WolfSslConnection::gotoStateDisconnect() {
	LOG_DEBUG(LOG_FRP, "gotoStateDisconnect");
	conn->close();
	state = State_Disconnect;
}

void WolfSslConnection::stateDisconnectEvent(Event *event) {
	LOG_DEBUG(LOG_FRP, "stateDisconnectEvent");
	switch(event->getType()) {
	case TcpIp::Event_Close: {
		LOG_INFO(LOG_FRP, "DISCONNECTED");
		session = wolfSSL_get_session(ssl);
		wolfSSL_free(ssl);
		ssl = NULL;
#ifdef DEBUG_MEMORY
		MallocPrintInfo();
#endif
		state = State_Idle;
		courier.deliver(TcpIp::Event_Close);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}
