#ifndef COMMON_CCICSI_T3_COMMANDLAYER_H
#define COMMON_CCICSI_T3_COMMANDLAYER_H

#include "CciCsiPacketLayer.h"
#include "CciCsiProtocol.h"

#include "ccicsi/Order.h"
#include "fiscal_storage/include/FiscalStorage.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/Buffer.h"
#include "utils/include/StringBuilder.h"

#include <stdint.h>

namespace Cci {
namespace T3 {

class ProductState {
public:
	ProductState();
	void disable();
	void enable();
	void disable(uint16_t cid);
	void enable(uint16_t cid);
	void set(Order *order);
	uint32_t get();

public:
	uint32_t value;
};

class CommandLayer : public CciCsi::PacketLayerObserver {
public:
	CommandLayer(CciCsi::PacketLayerInterface *packetLayer, TimerEngine *timers, EventEngineInterface *eventEngine);
	virtual ~CommandLayer();
	EventDeviceId getDeviceId();

	void setOrder(Order *order);
	void reset();
	bool isInited();
	bool isEnable();
	void disable();
	void enable();
	void approveVend();
	void denyVend();

private:
	enum State {
		State_Idle = 0,
		State_Wait,
		State_Approving,
		State_Approve,
		State_Vending,
	};

	CciCsi::PacketLayerInterface *packetLayer;
	TimerEngine *timers;
	Timer *timer;
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	State state;
	Order *order;
	ProductState productState;
	Buffer response;
	uint8_t progressCount;

	void gotoStateWait();
	void stateWaitRequest(const uint8_t *data, uint16_t dataLen);
	void stateWaitRequestBillingEnable(const uint8_t *data, uint16_t dataLen);
	void stateWaitRequestInquiry(const uint8_t *data, uint16_t dataLen);

	void gotoStateApproving();
	void stateApprovingRequest(const uint8_t *data, uint16_t dataLen);
	void stateApprovingRequestButtons();
	void stateApprovingTimeout();

	void stateApproveRequest(const uint8_t *data, uint16_t dataLen);
	void stateApproveRequestInquiry(const uint8_t *data, uint16_t dataLen);
	void stateApproveTimeout();

	void stateVendingRequest(const uint8_t *data, uint16_t dataLen);
	void stateVendingRequestStatus(const uint8_t *data, uint16_t dataLen);

	void procBillingEnable();
	void sendButtons();
	void sendVendApproved();
	void sendVendDenied();

public:
	void procTimer();
	virtual void procPacket(const uint8_t *data, const uint16_t len) override;
	virtual void procControl(uint8_t control) override;
	virtual void procError(Error error) override;
};

}
}

#endif
