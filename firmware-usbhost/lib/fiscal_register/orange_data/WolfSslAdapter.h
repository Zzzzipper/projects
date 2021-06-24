#ifndef LIB_ORANGEDATA_WOLFSSLADAPTER_H
#define LIB_ORANGEDATA_WOLFSSLADAPTER_H

#include "tcpip/include/TcpIp.h"
#include "utils/include/Buffer.h"

struct WOLFSSL;

class WolfSslAdapterInterface {
public:
	virtual void setObserver(EventObserver *observer) = 0;
	virtual bool connect(const char *domainname, uint16_t port) = 0;
	virtual int procSend(uint8_t *data, uint16_t dataLen) = 0;
	virtual int procRecv(uint8_t *data, uint16_t dataLen) = 0;
	virtual void close() = 0;

	static int CbIOSend(WOLFSSL *, char *buf, int sz, void *ctx) {
		WolfSslAdapterInterface *conn = (WolfSslAdapterInterface*)ctx;
		return conn->procSend((uint8_t*)buf, sz);
	}
	static int CbIORecv(WOLFSSL *, char *buf, int sz, void *ctx) {
		WolfSslAdapterInterface *conn = (WolfSslAdapterInterface*)ctx;
		return conn->procRecv((uint8_t*)buf, sz);
	}
	static int CbIOSendDummy(WOLFSSL *, char *buf, int sz, void *ctx) {
		(void)buf;
		(void)ctx;
		static int flag = 0;
		if(flag == 0) { flag = 1; return -2;}
		flag = 0;
		return sz;
	}
	static int CbIORecvDummy(WOLFSSL *, char *buf, int sz, void *ctx) {
		(void)buf;
		(void)sz;
		(void)ctx;
		return 0;
	}

};

class WolfSslAdapter : public WolfSslAdapterInterface, public EventObserver {
public:
	WolfSslAdapter(TcpIp *conn);
	virtual void setObserver(EventObserver *observer) override;
	virtual bool connect(const char *domainname, uint16_t port) override;
	virtual int procSend(uint8_t *data, uint16_t dataLen) override;
	virtual int procRecv(uint8_t *data, uint16_t dataLen) override;
	virtual void close() override;

	virtual void proc(Event *event) override;

private:
	enum State {
		State_Idle = 0,
		State_Connecting,
		State_Wait,
		State_Sending,
		State_SendComplete,
		State_RecvWait,
		State_Recving,
		State_RecvComplete,
		State_Closing,
	};

	TcpIp *conn;
	EventCourier courier;
	State state;
	Buffer buf;
	uint16_t recvLen;

	void stateConnectingEvent(Event *event);

	int stateWaitProcSend(uint8_t *data, uint16_t dataLen);
	void stateSendingEvent(Event *event);
	int stateSendCompleteProcSend();

	int stateWaitProcRecv(uint8_t *data, uint16_t dataLen);
	void stateRecvWaitEvent(Event *event);
	void stateRecvingEvent(Event *event);
	int stateRecvCompleteProcRecv(uint8_t *data, uint16_t dataLen);

	void stateClosingEvent(Event *event);
};

#endif
