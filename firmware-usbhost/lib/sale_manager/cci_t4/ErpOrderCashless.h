#ifndef LIB_SALEMANAGER_CCI_ERPORDERCASHLESS_H_
#define LIB_SALEMANAGER_CCI_ERPORDERCASHLESS_H_

#include "lib/sale_manager/cci_t3/ErpOrderMaster.h"

#include "common/mdb/master/cashless/MdbMasterCashless.h"

class ErpOrderCashless : public MdbMasterCashlessInterface, public EventSubscriber {
public:
	ErpOrderCashless(ConfigModem *config, CodeScannerInterface *scanner, TcpIp *tcpConn, TimerEngine *timerEngine, EventEngineInterface *eventEngine);

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
		State_Approving,
		State_Vending,
		State_Closing,
	};

	ErpOrderMaster *erpOrderMaster;
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	State state;
	uint16_t productId;
	uint32_t productPrice;

	void gotoStateSale();

	void stateApprovingEvent(EventEnvelope *envelope);
	void stateApprovingEventOrderRequest(EventEnvelope *envelope);
	void stateApprovingEventOrderCancel();

	void stateClosingEvent(EventEnvelope *envelope);
	void stateClosingEventOrderEnd(EventEnvelope *envelope);
};

#endif
