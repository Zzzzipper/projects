#ifndef COMMON_INPAS_COMMANDLAYER_H
#define COMMON_INPAS_COMMANDLAYER_H

#include "InpasPacketLayer.h"
#include "InpasProtocol.h"
#include "InpasDeviceLan.h"

#include "fiscal_storage/include/FiscalStorage.h"
#include "fiscal_register/include/QrCodeStack.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/Buffer.h"
#include "utils/include/StringBuilder.h"

#include <stdint.h>

namespace Inpas {

class CommandLayer : public PacketLayerObserver {
public:
	CommandLayer(Mdb::DeviceContext *context, PacketLayerInterface *packetLayer, TcpIp *conn, TimerEngine *timers, EventEngineInterface *eventEngine);
	virtual ~CommandLayer();
	EventDeviceId getDeviceId();
	void reset();
	void disable();
	void enable();
	bool sale(uint16_t productId, uint32_t productPrice);
	bool saleComplete();
	bool saleFailed();
	bool closeSession();
	bool printQrCode(const char *header, const char *footer, const char *text);
	bool verification();

private:
	enum State {
		State_Idle = 0,
		State_Init,
		State_InitDelay,
		State_Wait,
		State_Session,
		State_Request,
		State_Approving,
		State_Vending,
		State_PaymentCancel,
		State_PaymentCancelWait,
		State_Closing,
		State_QrCode,
		State_QrCodeWait,
		State_Verification,
	};

	enum Command: uint8_t {
		Command_None = 0,
		Command_VendRequest,
		Command_SessionComplete,
		Command_QrCode,
		Command_Verification,
	};

	Mdb::DeviceContext *context;
	PacketLayerInterface *packetLayer;
	DeviceLan deviceLan;
	TimerEngine *timers;
	Timer *timer;
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	Fifo<uint8_t> commandQueue;
	uint32_t productPrice;
	TlvPacket packet;
	TlvPacketMaker req;
	uint16_t repeatCount;
	StringBuilder qrCodeHeader;
	StringBuilder qrCodeData;
	StringBuilder terminalId;

	bool procCommand();

	void gotoStateInit();
	void stateInitControl(uint8_t control);
	void stateInitPacket();
	void stateInitTimeout();

	void gotoStateInitDelay();
	void stateInitDelayTimeout();

	void gotoStateWait();
	void stateWaitPacket();
	void stateWaitTimeout();

	void gotoStateSession();
	void stateSessionControl(uint8_t control);
	void stateSessionTimeout();

	void gotoStateRequest();
	void stateRequestControl(uint8_t control);
	void stateRequestTimeout();

	void gotoStateApproving();
	void stateApprovingPacket();
	void stateApprovingControl(uint8_t control);
	void stateApprovingTimeout();

	void gotoStatePaymentCancel();
	void statePaymentCancelSend();
	void statePaymentCancelControl(uint8_t control);
	void statePaymentCancelTimeout();

	void gotoStatePaymentCancelWait();
	void statePaymentCancelWaitPacket();
	void statePaymentCancelWaitControl(uint8_t control);
	void statePaymentCancelWaitTimeout();

	void gotoStateClosing();
	void stateClosingTimeout();

	void gotoStateQrCode();
	void stateQrCodeControl(uint8_t control);
	void stateQrCodeTimeout();

	void gotoStateQrCodeWait();
	void stateQrCodeWaitPacket();
	void stateQrCodeWaitTimeout();

	void gotoStateVerification();
	void stateVerificationControl(uint8_t control);
	void stateVerificationPacket();
	void stateVerificationTimeout();

public:
	void procTimer();
	virtual void procPacket(const uint8_t *data, const uint16_t len);
	virtual void procControl(uint8_t control);
	virtual void procError(Error error);
};

}

#endif
