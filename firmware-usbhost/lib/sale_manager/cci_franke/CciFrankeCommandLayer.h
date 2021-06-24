#ifndef COMMON_CCIFRANKE_COMMANDLAYER_H
#define COMMON_CCIFRANKE_COMMANDLAYER_H

#include "common/ccicsi/CciCsiPacketLayer.h"
#include "common/ccicsi/CciCsiProtocol.h"
#include "common/fiscal_storage/include/FiscalStorage.h"
#include "common/uart/include/interface.h"
#include "common/timer/include/TimerEngine.h"
#include "common/event/include/EventEngine.h"
#include "common/utils/include/Buffer.h"
#include "common/utils/include/StringBuilder.h"

#include "lib/client/ClientContext.h"

#include <stdint.h>

namespace Cci {
namespace Franke {

class CommandLayer : public CciCsi::PacketLayerObserver {
public:
	CommandLayer(CciCsi::PacketLayerInterface *packetLayer, TimerEngine *timers, EventEngineInterface *eventEngine);
	virtual ~CommandLayer();
	EventDeviceId getDeviceId();
	void disableProducts();
	void enableProducts();

#if 1
	void reset();
	bool isInited();
	bool isEnable();
	void setCredit(uint32_t credit);
	void approveVend(uint32_t productPrice);
	void denyVend(bool close);
	void cancelVend();
#else
	void reset();
	bool isInited();
	bool isEnable();
	void setCredit(uint32_t credit);
	void disableProducts();
	void enableProducts();
	void approveVend(uint32_t productPrice);
	void denyVend();
	void cancelVend();
#endif

private:
	enum State {
		State_Idle = 0,
		State_Wait,
		State_Credit,
		State_Approving,
		State_Vending,
	};

//	Mdb::DeviceContext *context;
	CciCsi::PacketLayerInterface *packetLayer;
	TimerEngine *timers;
	Timer *timer;
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	State state;
	uint16_t operation;
	uint32_t credit;
	Buffer response;
	uint16_t productId;
	uint32_t productState;
	uint16_t statusCount;

	void stateWaitRequest(const uint8_t *data, uint16_t dataLen);
	void stateWaitOperationInquiry(const uint8_t *data, uint16_t dataLen);
	void stateWaitOperationB(const uint8_t *data, uint16_t dataLen);

	void stateCreditRequest(const uint8_t *data, uint16_t dataLen);
	void stateCreditOperationInquiry(const uint8_t *data, uint16_t );
	void stateCreditOperationB(const uint8_t *data, uint16_t dataLen);

	void gotoStateApproving();
	void stateApprovingRequest(const uint8_t *data, uint16_t dataLen);
	void stateApprovingRequestStatus();
	void stateApprovingRequestInquiry(const uint8_t *data, uint16_t dataLen);

	void stateVendingRequest(const uint8_t *data, uint16_t dataLen);
	void stateVendingRequestStatus();

	// T,S,X,M,I,C,B,R,D
	void sendIdentification();
	void sendButtons();
	void sendBillingEnable();
	void sendMachineMode();
	void sendTelemetry();
	void sendC();
	void sendStatusNoAction();
	void sendStatusReady();
	void sendInquiryApproved();
	void sendInquiryDenied();
	void sendBApproved();
	void sendBDenied();

public:
	void procTimer();
	virtual void procPacket(const uint8_t *data, const uint16_t len) override;
	virtual void procControl(uint8_t control) override;
	virtual void procError(Error error) override;
};

}
}

#endif
