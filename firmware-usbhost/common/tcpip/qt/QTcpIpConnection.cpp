#include "QTcpIpConnection.h"

#include "common/timer/include/TimerEngine.h"
#include "common/logger/include/Logger.h"

QTcpIpConnection::QTcpIpConnection(TimerEngine *timers) :
	timers(timers),
	state(State_Idle),
	recvBuf(NULL)
{
	this->idleTimer = timers->addTimer<QTcpIpConnection, &QTcpIpConnection::procIdleTimer>(this);
	conn = new QTcpSocket(this);
	QObject::connect(conn, SIGNAL(connected()), this, SLOT(handleConnected()));
	QObject::connect(conn, SIGNAL(disconnected()), this, SLOT(handleDisconnected()));
	QObject::connect(conn, SIGNAL(bytesWritten(qint64)), this, SLOT(handleBytesWritten(qint64)));
	QObject::connect(conn, SIGNAL(readyRead()), this, SLOT(handleReadyRead()));
	QObject::connect(conn, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleError(QAbstractSocket::SocketError)));
}

void QTcpIpConnection::setObserver(EventObserver *observer) {
	courier.setRecipient(observer);
}

bool QTcpIpConnection::connect(const char *domainname, uint16_t port, Mode mode) {
	(void)mode;
	LOG_DEBUG(LOG_TCPIP, "gotoStateConnecting " << domainname << ":" << port);
//	conn->disconnect();
	recvBuf = NULL;
	conn->connectToHost(domainname, port);
	state = State_Connecting;
	return true;
}

bool QTcpIpConnection::hasRecvData() {
	return (conn->bytesAvailable() > 0);
}

bool QTcpIpConnection::send(const uint8_t *data, uint32_t len) {
	LOG_DEBUG(LOG_TCPIP, "send");
	sendBuf.clear();
	sendBuf.setRawData((char*)data, len);
	if(conn->write(sendBuf) == -1) {
		return false;
	}
	LOG_TRACE_STR(LOG_TCPIP, (char*)data, len);
	state = State_SendingWait;
	return true;
}

bool QTcpIpConnection::recv(uint8_t *buf, uint32_t bufSize) {
	LOG_DEBUG(LOG_TCPIP, "recv");
	if(conn->bytesAvailable() > 0) {
		qint64 recvLen = conn->read((char*)buf, bufSize);
		if(recvLen == -1) {
			LOG("error=" << conn->errorString().toStdString().c_str());
			event.set(TcpIp::Event_RecvDataError);
			idleTimer->start(1);
			return true;
		}
		LOG_TRACE_STR(LOG_TCPIP, (char*)buf, recvLen);
		event.setUint16(TcpIp::Event_RecvDataOk, recvLen);
		idleTimer->start(1);
		return true;
	} else {
		recvBuf = buf;
		recvBufSize = bufSize;
		return true;
	}
}

void QTcpIpConnection::close() {
	if(conn->isOpen() == true) {
		recvBuf = NULL;
		conn->close();
		state = State_Disconnecting;
		return;
	} else {
		idleTimer->start(1);
		event.set(TcpIp::Event_Close);
		state = State_Disconnecting;
		return;
	}
}

void QTcpIpConnection::handleConnected() {
	LOG_DEBUG(LOG_TCPIP, "handleConnected");
	state = State_Wait;
	courier.deliver(TcpIp::Event_ConnectOk);
}

void QTcpIpConnection::handleBytesWritten(qint64 bytes) {
	LOG_DEBUG(LOG_TCPIP, "handleBytesWritten " << (uint32_t)bytes);
	state = State_Wait;
	courier.deliver(TcpIp::Event_SendDataOk);
}

void QTcpIpConnection::handleReadyRead() {
	LOG_DEBUG(LOG_TCPIP, "handleReadyRead");
	if(recvBuf == NULL) {
		courier.deliver(TcpIp::Event_IncomingData);
		return;
	} else {
		qint64 recvLen = conn->read((char*)recvBuf, recvBufSize);
		if(recvLen == -1) {
			recvBuf = NULL;
			courier.deliver(TcpIp::Event_RecvDataError);
			return;
		}
		recvBuf = NULL;
		Event event(TcpIp::Event_RecvDataOk, (uint16_t)recvLen);
		courier.deliver(&event);
		return;
	}
}

void QTcpIpConnection::handleDisconnected() {
	LOG_DEBUG(LOG_TCPIP, "handleDisconnected");
	idleTimer->start(1);
	event.set(TcpIp::Event_Close);
}

void QTcpIpConnection::handleError(QAbstractSocket::SocketError socketError) {
	LOG_DEBUG(LOG_TCPIP, "handleError " << socketError);
	conn->close();
	state = State_Idle;
	courier.deliver(TcpIp::Event_Close);
}

void QTcpIpConnection::procIdleTimer() {
	state = State_Idle;
	courier.deliver(&event);
}
