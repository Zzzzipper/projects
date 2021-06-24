#ifndef COMMON_CARDREADER_QUICKPAYMASTER_H_
#define COMMON_CARDREADER_QUICKPAYMASTER_H_

#include "common/config/include/ConfigModem.h"
#include "common/fiscal_register/include/QrCodeStack.h"
#include "common/http/include/HttpClient.h"
#include "common/http/include/HttpTransport.h"
#include "common/utils/include/Json.h"

namespace QuickPay {

class CommandLayer : public EventObserver {
public:
	enum EventType {
		Event_QrReceived	 = GlobalId_QuickPay | 0x01,
		Event_QrApproved	 = GlobalId_QuickPay | 0x02,
		Event_QrDenied		 = GlobalId_QuickPay | 0x03,
		Event_QrError		 = GlobalId_QuickPay | 0x04,
	};

	CommandLayer(ConfigModem *config, TcpIp *tcpConn, TimerEngine *timerEngine, EventEngineInterface *eventEngine);
	virtual ~CommandLayer();

	void reset();
	void sale(uint32_t productPrice);
	void saleComplete();
	void saleFailed();

	void proc(Event *event) override;
	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_QrLoad,
		State_Wait,
		State_QrCheck,
		State_QrDelay,
	};

	TimerEngine *timerEngine;
	Timer *timer;
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	State state;
	Http::Client *httpClient;
	Http::Transport *httpTransport;
	StringBuilder *reqPath;
	StringBuilder *reqData;
	StringBuilder *respData;
	Http::Request req;
	Http::Response resp;
	JsonParser jsonParser;
/*
	void gotoStateWait();

	void gotoStateOrderStart();
	void stateOrderStartEvent(Event *event);
	void stateOrderStartEventRequestComplete();
	void stateOrderStartEventRequestError();

	void gotoStateOrderComplete();
	void stateOrderCompleteEvent(Event *event);
	void stateOrderCompleteEventRequestComplete();
	void stateOrderCompleteEventRequestError();*/
};

}

#endif
