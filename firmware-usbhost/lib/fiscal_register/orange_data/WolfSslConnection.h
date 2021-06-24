#ifndef LIB_ORANGEDATA_WOLFSSLCONNECTION_H
#define LIB_ORANGEDATA_WOLFSSLCONNECTION_H

#include "WolfSslAdapter.h"

#include "config/include/ConfigModem.h"

struct WOLFSSL_HEAP_HINT;
struct WOLFSSL_CTX;
struct WOLFSSL;
struct WOLFSSL_SESSION;

class WolfSslConnection : public TcpIp, public EventObserver {
public:
	WolfSslConnection(WolfSslAdapterInterface *conn);
	virtual ~WolfSslConnection();
	bool init(ConfigCert *publicKey, ConfigCert *privateKey);
	void cleanup();
	virtual void setObserver(EventObserver *observer) override;
	virtual bool connect(const char *domainname, uint16_t port, Mode mode) override;
	virtual bool hasRecvData() override;
	virtual bool send(const uint8_t *data, uint32_t len) override;
	virtual bool recv(uint8_t *buf, uint32_t bufSize) override;
	virtual void close() override;

	virtual void proc(Event *event) override;

private:
	enum State {
		State_Idle = 0,
		State_Connect,
		State_Wait,
		State_Send,
		State_Recv,
		State_Disconnect,
	};

	WolfSslAdapterInterface *conn;
	WOLFSSL_HEAP_HINT *hint;
	WOLFSSL_CTX *ctx;
	WOLFSSL *ssl;
	WOLFSSL_SESSION *session;
	State state;
	EventCourier courier;
	const uint8_t *sendData;
	uint16_t sendDataLen;
	uint8_t *recvBuf;
	uint16_t recvBufSize;

	static void CbLog(const int logLevel, const char *const logMessage);

	void stateConnectEvent(Event *event);

	void gotoStateSend(const uint8_t *data, uint32_t dataLen);
	void stateSendEvent(Event *event);

	void gotoStateRecv(uint8_t *buf, uint32_t bufSize);
	void stateRecvEvent(Event *event);

	void gotoStateDisconnect();
	void stateDisconnectEvent(Event *event);
};

#endif
