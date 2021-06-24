#ifndef COMMON_CARDREADER_QUICKPAYCASHLESS_H_
#define COMMON_CARDREADER_QUICKPAYCASHLESS_H_

#include "cardreader/quick_pay/QuickPayMaster.h"
#include "mdb/master/cashless/MdbMasterCashless.h"

namespace QuickPay {

class Cashless : public MdbMasterCashlessInterface, public EventSubscriber {
public:
	Cashless(ConfigModem *config, TcpIp *tcpConn, ScreenInterface *screen, TimerEngine *timerEngine, EventEngineInterface *eventEngine);

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

	void proc(EventEnvelope *envelope) override;

private:
	enum State {
		State_Idle = 0,
		State_Sale,
		State_Loading,
		State_Approving,
		State_Vending,
		State_Refunding,
	};

	CommandLayer *commandLayer;
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	State state;
	uint16_t productId;
	uint32_t productPrice;

	void gotoStateSale();

	void stateLoadingEvent(EventEnvelope *envelope);
	void stateLoadingEventQrReceived(EventEnvelope *envelope);
	void stateLoadingEventQrError();

	void stateApprovingEvent(EventEnvelope *envelope);
	void stateApprovingEventQrApproved(EventEnvelope *envelope);
	void stateApprovingEventQrDenied();
};

}

#endif
