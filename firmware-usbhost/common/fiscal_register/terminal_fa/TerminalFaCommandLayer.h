#ifndef COMMON_TERMINALFA_COMMANDLAYER_H
#define COMMON_TERMINALFA_COMMANDLAYER_H

#include "TerminalFaPacketLayer.h"
#include "fiscal_storage/include/FiscalStorage.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/Buffer.h"
#include "utils/include/StringBuilder.h"

#include <stdint.h>

namespace TerminalFa {

class CommandLayer : public PacketLayerInterface::Observer {
public:
	CommandLayer(PacketLayerInterface *packetLayer, TimerEngine *timers, EventEngineInterface *eventEngine);
	virtual ~CommandLayer();
	uint16_t getDeviceId();
	void sale(Fiscal::Sale *saleData, uint32_t decimalPoint);
	void getLastSale();
	void closeShift();

private:
	enum State {
		State_Idle = 0,
		State_Status,
		State_FSState,
		State_ShiftOpenStart,
		State_ShiftOpenFinish,
		State_ShiftCloseStart,
		State_ShiftCloseFinish,
		State_DocumentReset,
		State_CheckOpen,
		State_CheckAdd,
		State_CheckPayment,
		State_CheckClose,
	};

	PacketLayerInterface *packetLayer;
	TimerEngine *timers;
	Timer *timer;
	EventEngineInterface *eventEngine;
	State state;
	uint16_t deviceId;
	Fiscal::Sale *saleData;
	uint32_t decimalPoint;
	Buffer request;

	void gotoStateStatus();
	void stateStatusResponse(uint8_t *data, uint16_t dataLen);
	void gotoStateFSState();
	void stateFSStateResponse(uint8_t *data, uint16_t dataLen);

	void gotoStateShiftOpenStart();
	void stateShiftOpenStartResponse(uint8_t *data, uint16_t dataLen);
	void gotoStateShiftOpenFinish();
	void stateShiftOpenFinishResponse(uint8_t *data, uint16_t dataLen);
	void gotoStateShiftCloseStart();
	void stateShiftCloseStartResponse(uint8_t *data, uint16_t dataLen);
	void gotoStateShiftCloseFinish();
	void stateShiftCloseFinishResponse(uint8_t *data, uint16_t dataLen);

	void gotoStateDocumentReset();
	void stateDocumentResetResponse(uint8_t *data, uint16_t dataLen);

	void gotoStateCheckOpen();
	void stateCheckOpenResponse(uint8_t *data, uint16_t dataLen);
	void fillProductTlv(Fiscal::Sale *saleData, Buffer *buf);
	void gotoStateCheckAdd();
	void stateCheckAddResponse(uint8_t *data, uint16_t dataLen);
	void gotoStateCheckPayment();
	void stateCheckPaymentResponse(uint8_t *data, uint16_t dataLen);
	void gotoStateCheckClose();
	void stateCheckCloseResponse(uint8_t *data, uint16_t dataLen);

	void procProtocolError(uint8_t returnCode);
	void procError(ConfigEvent::Code errorCode);
	void procDebug(const char *file, uint16_t line);

public:
	void procTimer();
	virtual void procRecvData(uint8_t *data, uint16_t dataLen);
	virtual void procRecvError(PacketLayerInterface::Error error);
};

}

#endif
