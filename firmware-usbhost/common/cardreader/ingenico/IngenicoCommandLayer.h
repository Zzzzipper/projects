#ifndef COMMON_INGENIGO_COMMANDLAYER_H
#define COMMON_INGENIGO_COMMANDLAYER_H

#include "IngenicoPacketLayer.h"
#include "IngenicoProtocol.h"
#include "IngenicoDeviceLan.h"
#include "IngenicoDialogDirect.h"

#include "fiscal_storage/include/FiscalStorage.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/Buffer.h"
#include "utils/include/StringBuilder.h"

#include <stdint.h>

namespace Ingenico {

class CommandLayer : public PacketLayerObserver, public EventObserver {
public:
	CommandLayer(Mdb::DeviceContext *context, PacketLayerInterface *packetLayer, TcpIp *conn, TimerEngine *timerEngine, EventEngineInterface *eventEngine, uint32_t maxCredit);
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

	void proc(Event *event);
	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_WaitText,
		State_WaitButton,
		State_Session,
		State_ApprovingStart,
		State_Approving,
		State_ApprovingEnd,
		State_Vending,
		State_Closing,
	};

	enum CommandType : uint8_t {
		CommandType_None			= 0x00,
		CommandType_VendRequest		= 0x01,
		CommandType_PaymentCancel	= 0x02,
		CommandType_QrCode			= 0x03,
		CommandType_Sverka			= 0x04,
		CommandType_SessionComplete	= 0x05,
	};

	Mdb::DeviceContext *context;
	PacketLayerInterface *packetLayer;
	TimerEngine *timerEngine;
	EventEngineInterface *eventEngine;
	uint32_t maxCredit;
	EventDeviceId deviceId;
	Fifo<uint8_t> commandQueue;
	Packet req;
	DeviceLan deviceLan;
	DialogDirect dialogDirect;
	uint32_t productPrice;
	Timer *timer;
	uint16_t approveResult;

	void procStorerc(StringParser *parser);
	void procDl(StringParser *parser);
	void procEndtr(StringParser *parser);
	void procStatus(StringParser *parser);
	void procPing();
	bool procCommand();

	void gotoStateWaitText();
	void stateWaitTextEvent(Event *event);
	void stateWaitTextEventText();

	void gotoStateWaitButton();
	void stateWaitButtonEvent(Event *event);
	void stateWaitButtonEventButton();

	void gotoStateSession();
	void stateSessionTextEvent(Event *event);
	void stateSessionTimeout();

	void gotoStateApprovingStart();
	void stateApprovingStartEvent(Event *event);

	void gotoStateApproving();
	void stateApprovingStorerc(StringParser *parser);

	void gotoStateApprovingEnd();
	void stateApprovingEndStorerc(StringParser *parser);
	void stateApprovingEndEndtr();

	void gotoStateVending();

	void gotoStatePaymentCancel();

	void gotoStateClosing();
	void stateClosingTimeout();
/*
	TimerEngine *timers;
	Timer *timer;
	uint32_t productPrice;
	TlvPacket packet;
	TlvPacketMaker req;
	uint16_t repeatCount;
	StringBuilder qrCodeHeader;
	StringBuilder qrCodeData;

	void gotoStateWait();
	void stateWaitPacket();

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

	void stateClosingTimeout();

	void gotoStateQrCode();
	void stateQrCodeControl(uint8_t control);
	void stateQrCodeTimeout();

	void gotoStateQrCodeWait();
	void stateQrCodeWaitPacket();
	void stateQrCodeWaitTimeout();*/

public:
//	void procTimer();
	virtual void procPacket(const uint8_t *data, const uint16_t len);
	virtual void procError(Error error);
};

}

#endif
