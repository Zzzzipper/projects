#ifndef COMMON_FISCALREGISTER_RPSYSTEM1FA_H
#define COMMON_FISCALREGISTER_RPSYSTEM1FA_H

#include "fiscal_register/include/FiscalRegister.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/Buffer.h"
#include "utils/include/StringBuilder.h"

#include <stdint.h>

namespace RpSystem1Fa {

class PacketLayer;

class FiscalRegister : public Fiscal::Register {
public:
	FiscalRegister(uint16_t interfaceType, AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine);
	virtual ~FiscalRegister();
	virtual EventDeviceId getDeviceId();
	virtual void sale(Fiscal::Sale *saleData, uint32_t decimalPoint);
	virtual void getLastSale() {}
	virtual void closeShift();

private:
	enum State {
		State_Idle = 0,
		State_Status,
		State_ShiftOpen,
		State_CheckOpen,
		State_CheckAdd,
		State_CheckClose,
		State_CheckCancel,
		State_ShiftClose,
	};

	TimerEngine *timers;
	Timer *timer;
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	State state;
	PacketLayer *packetLayer;
	uint32_t password;
	StringBuilder name;
	uint32_t price;
	Buffer request;

	void gotoStateStatus();
	void stateStatusResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateShiftOpen();
	void stateShiftOpenResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateCheckOpen();
	void stateCheckOpenResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateCheckAdd();
	void stateCheckAddResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateCheckClose();
	void stateCheckCloseResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateCheckCancel();
	void stateCheckCancelResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateShiftClose();
	void stateShiftCloseResponse(const uint8_t *data, const uint16_t dataLen);

	void procError();

public:
	void procTimer();
	void procRecvData(const uint8_t *data, const uint16_t len);
	void procRecvError();
};

}

#endif // RPSYSTEMAFA_H
