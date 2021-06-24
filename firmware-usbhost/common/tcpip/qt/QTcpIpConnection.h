#ifndef COMMON_TCPIP_QCONNECTION_H
#define COMMON_TCPIP_QCONNECTION_H

#include "common/tcpip/include/TcpIp.h"

#include <QObject>
#include <QTcpSocket>

class TimerEngine;
class Timer;
class Network;

class QTcpIpConnection : public QObject, public TcpIp {
	Q_OBJECT
public:
	QTcpIpConnection(TimerEngine *timers);
	virtual void setObserver(EventObserver *observer);
	virtual bool connect(const char *domainname, uint16_t port, Mode mode);
	virtual bool hasRecvData();
	virtual bool send(const uint8_t *data, uint32_t len);
	virtual bool recv(uint8_t *buf, uint32_t bufSize);
	virtual void close();
	virtual void dump(StringBuilder *) {}

private slots:
	void handleConnected();
	void handleDisconnected();
	void handleBytesWritten(qint64 bytes);
	void handleReadyRead();
	void handleError(QAbstractSocket::SocketError socketError);

private:
	enum State {
		State_Idle = 0,
		State_Connecting,
		State_Wait,
		State_Sending,
		State_SendingWait,
		State_Disconnecting,
	};

	TimerEngine *timers;
	Timer *sendTimer;
	Timer *idleTimer;
	EventCourier courier;
	QTcpSocket *conn;
	QByteArray sendBuf;
	State state;
	uint8_t *recvBuf;
	uint32_t recvBufSize;
	Event event;

	void procIdleTimer();
};

#endif
