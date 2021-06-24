#ifndef LIB_RFID_CASHLESS_H
#define LIB_RFID_CASHLESS_H

#include "RfidReader.h"
#include "RfidInterface.h"

#include "lib/client/ClientContext.h"

#include "common/fiscal_register/include/QrCodeStack.h"
#include "common/mdb/master/cashless/MdbMasterCashless.h"
#include "common/config/include/ConfigModem.h"
#include "common/timer/include/RealTime.h"
#include "common/http/include/HttpClient.h"
#include "common/http/include/HttpTransport.h"
#include "common/utils/include/Json.h"

class GramophoneInterface;
class Melody;

namespace Rfid {

class Cashless : public MdbMasterCashlessInterface, public EventSubscriber, public EventObserver {
public:
	enum Type {
		Type_Lock = 0,
		Type_Unlimited = 1,
		Type_OnlineWallet = 2,
		Type_New = 3,
		Type_Discount = 4,
	};

	Cashless(ConfigModem *config, ClientContext *client, TcpIp *tcpConn, RealTimeInterface *realtime, TimerEngine *timerEngine, GramophoneInterface *gramophone, ScreenInterface *screen, EventEngineInterface *eventEngine);
	virtual ~Cashless();
	EventDeviceId getDeviceId() override;
	void reset() override;
	bool isRefundAble() override;
	void disable() override;
	void enable() override;
	bool revalue(uint32_t credit) override;
	bool sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) override;
	bool saleComplete() override;
	bool saleFailed() override;
	bool closeSession() override;
	void service();

	void proc(EventEnvelope *envelope) override;
	void proc(Event *event) override;
	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Disabled,
		State_Enabled,
		State_Checking,
		State_Session,
		State_Approving,
		State_Vending,
		State_Closing,
		State_Discount,
		State_Service,
		State_Registration,
	};

//+++
	ConfigModem *config;
	ClientContext *client;
	RealTimeInterface *realtime;
	Mdb::DeviceContext *context;
	DecimalPointConverter converter;
	Http::Client *httpClient;
	Http::Transport *httpTransport;
	StringBuilder *reqPath;
	StringBuilder *reqData;
	StringBuilder *respData;
	Http::Request req;
	Http::Response resp;
	JsonParser jsonParser;
//+++
	TimerEngine *timerEngine;
	Timer *timer;
	GramophoneInterface *gramophone;
	ScreenInterface *screen;
	EventEngineInterface *eventEngine;
	Melody *melody;
	EventDeviceId deviceId;
	uint16_t productId;
	uint32_t productPrice;
	StringBuilder *productName;
	uint32_t wareId;
	bool enabling;
	Rfid::Uid uid;
	uint16_t type;
	uint32_t credit;

	void gotoStateDisabled();
	void gotoStateEnabled();
	void stateEnabledEvent(EventEnvelope *envelope);

	void gotoStateChecking();
	void stateCheckingEvent(Event *event);
	void stateCheckingEventRequestComplete();
	void stateCheckingEventRequestError();
	void stateCheckingTimeout();

	void gotoStateSession();
	void stateSessionEvent(EventEnvelope *envelope);
	void stateSessionTimeout();

	void gotoStateApproving();
	void stateApprovingEvent(Event *event);
	void stateApprovingEventRequestComplete();
	void stateApprovingEventRequestError();
	void stateApprovingTimeout();

	void gotoStateClosing();
	void stateClosingTimeout();

	void gotoStateDiscount();
	void stateDiscountEvent(EventEnvelope *envelope);
	void stateDiscountTimeout();

	void gotoStateService(bool showText = true);
	void stateServiceEvent(EventEnvelope *envelope);

	void gotoStateRegistration();
	void stateRegistrationEvent(Event *event);
	void stateRegistrationEventRequestComplete();
	void stateRegistrationEventRequestError();
	void stateRegistrationTimeout();
};

}

#endif
