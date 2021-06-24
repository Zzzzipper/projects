#ifndef COMMON_HTTP_TESTTCPIP_H
#define COMMON_HTTP_TESTTCPIP_H

#include "tcpip/include/TcpIp.h"
#include "utils/include/StringBuilder.h"
#include "utils/include/Fifo.h"

class TestTcpIp : public TcpIp {
public:
	TestTcpIp(uint16_t recvBufSize, StringBuilder *result, bool humanReadable = false);
	virtual void setObserver(EventObserver *) override;
	virtual bool connect(const char *domainname, uint16_t port, Mode mode) override;
	virtual bool hasRecvData() override;
	virtual bool send(const uint8_t *data, uint32_t len) override;
	virtual bool recv(uint8_t *data, uint32_t size) override;
	virtual void close() override;

	void connectComplete();
	void connectError();
	void sendComplete();
	void setRecvDataFlag(bool dataFlag);
	void incommingData();
	bool addRecvData(const char *data, bool recvDataFlag = true);
	bool addRecvString(const char *data, bool recvDataFlag = true);
	void remoteClose();

private:
	EventObserver *observer;
	StringBuilder *result;
	Fifo<uint8_t> recvQueue;
	uint8_t *recvBuf;
	uint16_t recvBufSize;
	bool recvDataFlag;
	bool humanReadable;

	void sendData();
};

#endif
