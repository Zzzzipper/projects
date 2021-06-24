#ifndef COMMON_VENDOTEK_COMMANDLAYER_H
#define COMMON_VENDOTEK_COMMANDLAYER_H

#include "VendotekPacketLayer.h"
#include "VendotekDeviceLan.h"
#include "VendotekProtocol.h"

#include "fiscal_storage/include/FiscalStorage.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/Buffer.h"
#include "utils/include/StringBuilder.h"

#include <stdint.h>

namespace Vendotek {

class CommandLayer : public PacketLayerObserver {
public:
	CommandLayer(Mdb::DeviceContext *context, PacketLayerInterface *packetLayer, TcpIp *conn, TimerEngine *timerEngine, EventEngineInterface *eventEngine, RealTimeInterface *realtime, uint32_t credit);
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
		State_Enabled,
		State_Session,
		State_Approving,
		State_Vending,
		State_Fin,
		State_Aborting,
		State_Closing,
		State_QrCode,
	};

	enum CommandType {
		CommandType_None			= 0x00,
		CommandType_VendRequest		= 0x01,
		CommandType_QrCode			= 0x03,
	};

	Mdb::DeviceContext *context;
	PacketLayerInterface *packetLayer;
	DeviceLan deviceLan;
	TimerEngine *timerEngine;
	Timer *timer;
	Timer *cancelTimer;
	EventEngineInterface *eventEngine;
	RealTimeInterface *realtime;
	EventDeviceId deviceId;
	bool enabled;
	CommandType command;
	uint32_t maxCredit;
	uint16_t errorCount;
	Tlv::PacketMaker req;
	Tlv::Packet resp;
	uint32_t operationId;
	StringBuilder messageName;
	uint32_t productPrice;
	bool vendResult;
	StringBuilder qrCodeData;

	void gotoStateInit();
	void stateInitTimeout();
	void stateInitTimeoutCancel();
	void stateInitPacket();
	void stateInitPacketIdl();

	void gotoStateEnabled();
	void gotoStateEnabled(uint32_t timeout);
	void stateEnabledTimeout();
	void stateEnabledTimeoutCancel();
	void stateEnabledPacket();
	void stateEnabledPacketIdl();
	void stateEnabledPacketDis();
	void stateEnabledPacketSta();

	void gotoStateSession();
	void stateSessionTimeout();
	void stateSessionTimeoutCancel();
	void stateSessionPacket();

	void gotoStateApproving();
	void stateApprovingPacket();
	void stateApprovingTimeoutCancel();

	void gotoStateVending();
	void stateVendingPacket();
	void stateVendingTimeoutCancel();

	void gotoStateFin(bool result);
	void stateFinPacket();
	void stateFinTimeoutCancel();

	void gotoStateAborting();
	void stateAbortingPacket();
	void stateAbortingTimeoutCancel();

	void stateClosingTimeoutCancel();

	void gotoStateQrCode();
	void stateQrCodePacket();
	void stateQrCodeTimeoutCancel();
/*
	DeviceLan deviceLan;
*/

public:
	void procTimer();
	void procCancelTimer();
	virtual void procPacket(const uint8_t *data, const uint16_t len);
	virtual void procError(Error error);
};

}

#endif
