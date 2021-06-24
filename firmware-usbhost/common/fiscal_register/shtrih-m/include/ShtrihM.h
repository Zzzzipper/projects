#ifndef SHTRIHM_H
#define SHTRIHM_H

#include "fiscal_register/include/FiscalRegister.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/Buffer.h"
#include "utils/include/StringBuilder.h"

#include <stdint.h>

class ShtrihmReceiver;

class ShtrihM : public Fiscal::Register {
public:
	ShtrihM(AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine);
	virtual ~ShtrihM();
	virtual EventDeviceId getDeviceId();
	virtual void sale(Fiscal::Sale *saleData, uint32_t decimalPoint);
	virtual void getLastSale() {}
	virtual void closeShift();

private:
	enum State {
		State_Idle = 0,
		State_ShortStatus,
		State_PrintContinue,
		State_ShiftOpen,
		State_WaitKKT,
		State_CheckOpen,
		State_CheckAdd,
		State_CheckClose,
		State_CheckReset,
		State_ShiftClose,
		State_ShiftClose2,
	};

	TimerEngine *timers;
	Timer *timer;
	EventEngineInterface *eventEngine;
	ShtrihmReceiver *receiver;
	EventDeviceId deviceId;
	State state;
	uint8_t password;
	StringBuilder name;
	uint8_t paymentType;
	uint32_t credit;
	uint32_t price;
	Buffer packet;
	uint8_t tryNumber;

	void gotoStateShortStatus();
	void stateShortStatusResponse(const uint8_t *data, const uint16_t len);
	void gotoStatePrintContinue();
	void statePrintContinueResponse(const uint8_t *data, const uint16_t len);
	void gotoStateShiftOpen();
	void stateShiftOpenResponse(const uint8_t *data, const uint16_t len);
	void gotoStateShiftClose();
	void stateShiftCloseResponse(const uint8_t *data, const uint16_t len);
	void gotoStateWaitKKT();
	void gotoStateCheckOpen();
	void stateCheckOpenResponse(const uint8_t *data, const uint16_t len);
	void gotoStateCheckAdd();
	void stateCheckAddResponse(const uint8_t *data, const uint16_t len);
	void gotoStateCheckClose();
	void stateCheckCloseResponse(const uint8_t *data, const uint16_t len);
	void gotoStateCheckReset();
	void stateCheckResetResponse(const uint8_t *data, const uint16_t len);
	void gotoStateShiftClose2();
	void stateShiftClose2Response(const uint8_t *data, const uint16_t len);
	void procError();
	void printShortStatusResponse(const uint8_t *data, const uint16_t len);

public:
	void procRecvData(const uint8_t *data, const uint16_t len);
	void procRecvError();
	void procTimer();
};

#endif // SHTRIHM_H
