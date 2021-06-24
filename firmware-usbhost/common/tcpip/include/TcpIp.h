#ifndef LIB_TCPIP_TCPIP_H
#define LIB_TCPIP_TCPIP_H

#include "utils/include/Event.h"
#include "utils/include/StringBuilder.h"

class TcpIp {
public:
	enum Mode {
		Mode_TcpIp = 0,
		Mode_TcpIpOverSsl
	};
	enum EventType {
		Event_ConnectOk		= GlobalId_TcpIp | 0x01,
		Event_ConnectError	= GlobalId_TcpIp | 0x02, //EventError
		Event_IncomingData	= GlobalId_TcpIp | 0x03,
		Event_SendDataOk	= GlobalId_TcpIp | 0x04,
		Event_SendDataError	= GlobalId_TcpIp | 0x05,
		Event_RecvDataOk	= GlobalId_TcpIp | 0x06, //uint16_t: receive data size
		Event_RecvDataError	= GlobalId_TcpIp | 0x07,
		Event_Close			= GlobalId_TcpIp | 0x08,
	};
	virtual ~TcpIp() {}
	virtual void setObserver(EventObserver *observer) = 0;
	virtual bool connect(const char *domainname, uint16_t port, Mode mode) = 0;
	virtual bool hasRecvData() = 0;
	virtual bool send(const uint8_t *data, uint32_t len) = 0;
	virtual bool recv(uint8_t *buf, uint32_t bufSize) = 0;
	virtual void close() = 0;
};

#endif
