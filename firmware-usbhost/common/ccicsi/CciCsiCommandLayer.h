#ifndef COMMON_CCICSI_COMMANDLAYER_H
#define COMMON_CCICSI_COMMANDLAYER_H

#include "CciCsiPacketLayer.h"
#include "CciCsiProtocol.h"

#include "fiscal_storage/include/FiscalStorage.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/Buffer.h"
#include "utils/include/StringBuilder.h"

#include <stdint.h>

namespace CciCsi {

class CommandLayer : public PacketLayerObserver {
public:
	CommandLayer(PacketLayerInterface *packetLayer, TimerEngine *timers, EventEngineInterface *eventEngine);
	virtual ~CommandLayer();
	EventDeviceId getDeviceId();

	void reset();
	bool isInited();
	bool isEnable();
	void setCredit(uint32_t credit);
	void approveVend(uint32_t productPrice);
	void denyVend();
	void cancelVend();

private:
	enum State {
		State_Idle = 0,
		State_Wait,
		State_Credit,
		State_Approving,
		State_Approve,
		State_Vending,
	};

//	Mdb::DeviceContext *context;
	PacketLayerInterface *packetLayer;
	TimerEngine *timers;
	Timer *timer;
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	State state;
	uint32_t credit;
	Buffer response;
	uint16_t productId;
	uint16_t statusCount;

	void stateWaitRequest(const uint8_t *data, uint16_t dataLen);

	void stateCreditRequest(const uint8_t *data, uint16_t dataLen);
	void stateCreditRequestInquiry(const uint8_t *data, uint16_t dataLen);

	void gotoStateApproving();
	void stateApprovingRequest(const uint8_t *data, uint16_t dataLen);
	void stateApprovingRequestStatus();
	void stateApprovingRequestInquiry(const uint8_t *data, uint16_t dataLen);

	void stateApproveRequest(const uint8_t *data, uint16_t dataLen);
	void stateApproveRequsetInquiry(const uint8_t *data, uint16_t dataLen);

	void stateVendingRequest(const uint8_t *data, uint16_t dataLen);
	void stateVendingRequestStatus(const uint8_t *data, uint16_t dataLen);

	void sendIdentification();
	void sendStatusWait();
	void sendStatusCredit();
	void sendVendApproved();
	void sendVendDenied();

public:
	void procTimer();
	virtual void procPacket(const uint8_t *data, const uint16_t len) override;
	virtual void procControl(uint8_t control) override;
	virtual void procError(Error error) override;
};

}

#endif
