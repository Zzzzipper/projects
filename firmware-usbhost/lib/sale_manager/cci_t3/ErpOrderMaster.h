#ifndef LIB_SALEMANAGER_CSI_ORDERCASHLESS_H
#define LIB_SALEMANAGER_CSI_ORDERCASHLESS_H

#include "lib/client/ClientContext.h"

#include "common/code_scanner/CodeScanner.h"
#include "common/config/include/ConfigModem.h"
#include "common/http/include/HttpClient.h"
#include "common/http/include/HttpTransport.h"
#include "common/utils/include/Json.h"

#define CID_UNDEFINED 0xFFFF

class OrderInterface {
public:
	enum EventType {
		Event_OrderApprove	 = GlobalId_Order | 0x01,
		Event_OrderRequest	 = GlobalId_Order | 0x02, // EventVendRequest
		Event_OrderCancel	 = GlobalId_Order | 0x03,
		Event_OrderEnd		 = GlobalId_Order | 0x04,
	};

	virtual void reset() = 0;
	virtual void disable() = 0;
	virtual void enable() = 0;
	virtual void setProductId(uint16_t cid) = 0;
	virtual bool saleComplete() = 0;
	virtual bool saleFailed() = 0;
	virtual bool closeSession() = 0;
};

class ErpOrderMaster : public OrderInterface, public EventObserver, public CodeScanner::Observer {
public:
	ErpOrderMaster(ConfigModem *config, CodeScannerInterface *scanner, TcpIp *tcpConn, TimerEngine *timerEngine, EventEngineInterface *eventEngine);
	virtual ~ErpOrderMaster();

	void reset() override;
	void disable() override;
	void enable() override;
	void setProductId(uint16_t cid) override;
	bool saleComplete() override;
	bool saleFailed() override;
	bool closeSession() override;

	bool procCode(uint8_t *data, uint16_t dataLen) override;
	void proc(Event *event) override;
	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Wait,
		State_OrderStart,
		State_OrderProcessing,
		State_OrderComplete,
	};

	ConfigModem *config;
	CodeScannerInterface *scanner;
	TimerEngine *timerEngine;
//	Timer *timer;
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	State state;
	bool enabled;
	uint16_t searchCid;
	StringBuilder oid;
	uint16_t cid;
	Http::Client *httpClient;
	Http::Transport *httpTransport;
	StringBuilder *reqPath;
	StringBuilder *reqData;
	StringBuilder *respData;
	Http::Request req;
	Http::Response resp;
	JsonParser jsonParser;

	void gotoStateWait();

	void gotoStateOrderStart();
	void stateOrderStartEvent(Event *event);
	void stateOrderStartEventRequestComplete();
	void stateOrderStartEventRequestError();

	void gotoStateOrderComplete();
	void stateOrderCompleteEvent(Event *event);
	void stateOrderCompleteEventRequestComplete();
	void stateOrderCompleteEventRequestError();
};

#endif
