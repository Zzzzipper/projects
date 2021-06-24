#ifndef COMMON_SBERBANK_COMMANDLAYER_H
#define COMMON_SBERBANK_COMMANDLAYER_H

#include "SberbankPacketLayer.h"
#include "SberbankProtocol.h"
#include "SberbankDeviceLan.h"
#include "SberbankPrinter.h"

#include "fiscal_storage/include/FiscalStorage.h"
#include "fiscal_register/include/QrCodeStack.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/Buffer.h"
#include "utils/include/StringBuilder.h"
#include "utils/include/Fifo.h"

#include <stdint.h>

namespace Sberbank {

class CommandLayer : public PacketLayerObserver {
public:
	CommandLayer(Mdb::DeviceContext *context, PacketLayerInterface *packetLayer, TcpIp *conn, TimerEngine *timers, EventEngineInterface *eventEngine, uint32_t maxCredit);
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

	void procPacket(const uint8_t *data, const uint16_t len) override;
	void procError(Error error) override;
	void procPollTimer();
	void procTimer();
	void procSverkaTimer();

private:
	enum State {
		State_Idle = 0,
		State_Disabled,
		State_Enabled,
		State_Sverka1,
		State_Sverka2,
		State_SessionBegin,
		State_Session,
		State_Payment,
		State_PaymentAbort,
		State_ConnOpen,
		State_ConnRead,
		State_ConnWrite,
		State_Vending,
		State_PaymentCancel,
		State_Closing,
		State_QrCode,
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
	DeviceLan deviceLan;
	Printer printer;
	TimerEngine *timers;
	Timer *pollTimer;
	Timer *timer;
	Timer *sverkaTimer;
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	Fifo<uint8_t> commandQueue;
	bool enabled;
	uint32_t maxCredit;
	uint32_t productPrice;
	Buffer sendBuf;
	Buffer recvBuf;
	StringBuilder qrCodeData;

	void commandMasterCall(Packet* packet);
	void commandMasterCallUnsupported(MasterCallRequest *req);
	bool procCommand();

	void gotoStateDisabled();

	void gotoStateEnabled();
	void stateEnabledPollTimeout();
	void stateEnabledRecv(const uint8_t *data, uint16_t dataLen);

	void gotoStateSverka1();
	void stateSverka1Recv(const uint8_t *data, uint16_t dataLen);
	void gotoStateSverka2();
	void stateSverka2Recv(const uint8_t *data, uint16_t dataLen);

	void gotoStateSessionBegin();
	void stateSessionBeginRecv(const uint8_t *data, uint16_t dataLen);

	void gotoStateSession();
	void stateSessionTimeout();

	void gotoStatePaymentAbort();
	void statePaymentAbortTimeout();
	void statePaymentAbortRecv(const uint8_t *data, uint16_t dataLen);

	void gotoStatePayment();
	void statePaymentTimeout();
	void statePaymentRecv(const uint8_t *data, uint16_t dataLen);

	void gotoStateVending();
	void stateVendingTimeout();

	void gotoStatePaymentCancel();
	void statePaymentCancelTimeout();
	void statePaymentCancelRecv(const uint8_t *data, uint16_t dataLen);

	void gotoStateClosing();
	void stateClosingTimeout();

	void gotoStateQrCode();
	void stateQrCodeTimeout();
	void stateQrCodeRecv(const uint8_t *data, uint16_t dataLen);
};

}

#endif
