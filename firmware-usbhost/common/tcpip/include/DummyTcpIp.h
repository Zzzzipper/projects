#ifndef COMMON_TCPIP_DUMMYTCPIP_H
#define COMMON_TCPIP_DUMMYTCPIP_H

#include "TcpIp.h"

class DummyTcpIp : public TcpIp {
public:
	void setObserver(EventObserver *observer) { return; }
	bool connect(const char *domainname, uint16_t port, Mode mode) { return false; }
	bool hasRecvData() { return false; }
	bool send(const uint8_t *data, uint32_t len) { return false; }
	bool recv(uint8_t *buf, uint32_t bufSize) { return false; }
	void close() { return; }
};

#endif
