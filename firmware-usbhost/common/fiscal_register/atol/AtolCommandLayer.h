#if 1
#ifndef COMMON_ATOL_COMMANDLAYER_H
#define COMMON_ATOL_COMMANDLAYER_H

#include "AtolProtocol.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/LedInterface.h"
#include "utils/include/Buffer.h"
#include "utils/include/StringBuilder.h"

namespace Atol {

class CommandLayer : public TaskLayerObserver {
public:
	CommandLayer(Fiscal::Context *context, const char *ipaddr, uint16_t port, TimerEngine *timers, TaskLayerInterface *taskLayer, EventEngineInterface *eventEngine, LedInterface *leds);
	virtual ~CommandLayer();
	EventDeviceId getDeviceId();
	void reset();
	void sale(Fiscal::Sale *saleData);
	void getLastSale();
	void closeShift();

private:
	enum State {
		State_Idle = 0,
		State_Connect,
		State_KktStatus,
		State_Status,
		State_PrinterStatus,
		State_SaleOpen,
		State_ShiftOpen,
		State_WaitKKT,
		State_CheckOpen,
		State_FN105CheckAddStart,
		State_FN105CheckAddEnd,
		State_CheckClose,
		State_CheckReset,
		State_ModeClose,
		State_ShiftClose1,
		State_ShiftClose2,
		State_ShiftClose3,
		State_ShiftClose4,
		State_ShiftClose5,
		State_LastSale,
		State_DocSize,
		State_DocData,
		State_Disconnect,
	};
	enum Command {
		Command_None = 0,
		Command_Reset,
		Command_Sale,
		Command_GetLastSale,
		Command_CloseShift,
	};

	Fiscal::Context *context;
	const char *ipaddr;
	uint16_t port;
	TimerEngine *timers;
	Timer *timer;
	TaskLayerInterface *taskLayer;
	EventEngineInterface *eventEngine;
	LedInterface *leds;
	EventDeviceId deviceId;
	State state;
	uint64_t fiscalStorage;
	Command command;
	uint16_t password;
	Fiscal::Sale *saleData;
	Buffer request;
	uint8_t tryNumber;
	uint32_t docNumber;
	uint32_t docLen;
	uint32_t docSize;
	Buffer doc;
	EventEnvelope envelope;

	void gotoStateConnect();
	void stateConnectError(TaskLayerObserver::Error error);
	void gotoStateDisconnect(EventInterface *event);
	void stateDisconnectError(TaskLayerObserver::Error error);

	void gotoStateDeviceStatus();
	void stateDeviceStatusResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateStatus();
	void stateStatusResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStatePrinterStatus();
	void stateStatePrinterResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateSaleOpen();
	void stateSaleOpenResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateShiftOpen();
	void stateShiftOpenResponse(const uint8_t *data, const uint16_t dataLen);

	void gotoStateCheckOpen();
	void stateCheckOpenResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateFN105CheckAddStart();
	void stateFN105CheckAddStartResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateFN105CheckAddEnd();
	void stateFN105CheckAddEndResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateCheckClose();
	void stateCheckCloseResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateCheckReset();
	void stateCheckResetResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateModeClose();
	void stateModeCloseResponse(const uint8_t *data, const uint16_t dataLen);

	void gotoStateShiftClose1();
	void stateShiftClose1Response(const uint8_t *data, const uint16_t dataLen);
	void gotoStateShiftClose2();
	void stateShiftClose2Response(const uint8_t *data, const uint16_t dataLen);
	void gotoStateShiftClose3();
	void stateShiftClose3Response(const uint8_t *data, const uint16_t dataLen);
	void gotoStateShiftClose4();
	void stateShiftClose4Timeout();
	void gotoStateShiftClose5();
	void stateShiftClose5Response(const uint8_t *data, const uint16_t dataLen);

	void gotoStateLastSale();
	void stateLastSaleResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateDocSize();
	void stateDocSizeResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateDocData();
	void stateDocDataResponse(const uint8_t *data, const uint16_t dataLen);
	void printSTLV(const uint8_t *data, const uint16_t dataLen);
	uint16_t printTLV(const uint8_t *data, const uint16_t dataLen);

	void procTaskLayerError(TaskLayerObserver::Error errorCode);
	void procResponseError(uint8_t errorCode);
	void procDebugError(const char *file, uint16_t line);

public:
	void procTimer();
	virtual void procRecvData(const uint8_t *data, const uint16_t len);
	virtual void procError(TaskLayerObserver::Error error);
};

}

#endif
#else
#ifndef COMMON_ATOL_COMMANDLAYER_H
#define COMMON_ATOL_COMMANDLAYER_H

#include "AtolProtocol.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "utils/include/Buffer.h"
#include "utils/include/StringBuilder.h"

namespace Atol {

class CommandLayer : public TaskLayerObserver {
public:
	CommandLayer(TimerEngine *timers, TaskLayerInterface *taskLayer, EventEngineInterface *eventEngine);
	virtual ~CommandLayer();
	void sale(Fiscal::Sale *saleData, uint32_t decimalPoint);
	void getLastSale();
	void closeShift();

private:
	enum State {
		State_Idle = 0,
		State_Connect,
		State_Status,
		State_SaleOpen,
		State_ShiftOpen,
		State_WaitKKT,
		State_CheckOpen,
		State_CheckAdd,
		State_FN105CheckAddStart,
		State_FN105CheckAddEnd,
		State_CheckClose,
		State_CheckReset,
		State_ModeClose,
		State_ShiftClose1,
		State_ShiftClose2,
		State_ShiftClose3,
		State_ShiftClose4,
		State_ShiftClose5,
		State_LastSale,
		State_DocSize,
		State_DocData,
		State_Disconnect,
	};
	TimerEngine *timers;
	Timer *timer;
	TaskLayerInterface *taskLayer;
	EventEngineInterface *eventEngine;
	State state;
	uint16_t password;
	Fiscal::Sale *saleData;
	uint32_t decimalPoint;
	Buffer request;
	uint8_t tryNumber;
	uint32_t docNumber;
	uint32_t docLen;
	uint32_t docSize;
	Buffer doc;
	EventEnvelope envelope;

	void gotoStateConnect();
	void gotoStateStatus();
	void stateStatusResponse(const uint8_t *data, const uint16_t dataLen);
	void stateDeviceInfoResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateSaleOpen();
	void stateSaleOpenResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateShiftOpen();
	void stateShiftOpenResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateCheckOpen();
	void stateCheckOpenResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateCheckAdd();
	void stateCheckAddResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateFN105CheckAddStart();
	void stateFN105CheckAddStartResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateFN105CheckAddEnd();
	void stateFN105CheckAddEndResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateCheckClose();
	void stateCheckCloseResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateCheckReset();
	void stateCheckResetResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateModeClose();
	void stateModeCloseResponse(const uint8_t *data, const uint16_t dataLen);

	void gotoStateShiftClose1();
	void stateShiftClose1Response(const uint8_t *data, const uint16_t dataLen);
	void gotoStateShiftClose2();
	void stateShiftClose2Response(const uint8_t *data, const uint16_t dataLen);
	void gotoStateShiftClose3();
	void stateShiftClose3Response(const uint8_t *data, const uint16_t dataLen);
	void gotoStateShiftClose4();
	void stateShiftClose4Timeout();
	void gotoStateShiftClose5();
	void stateShiftClose5Response(const uint8_t *data, const uint16_t dataLen);

	void gotoStateLastSale();
	void stateLastSaleResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateDocSize();
	void stateDocSizeResponse(const uint8_t *data, const uint16_t dataLen);
	void gotoStateDocData();
	void stateDocDataResponse(const uint8_t *data, const uint16_t dataLen);
	void printSTLV(const uint8_t *data, const uint16_t dataLen);
	uint16_t printTLV(const uint8_t *data, const uint16_t dataLen);

	void procError(uint8_t returnCode);
	void procDebug(const char *file, uint16_t line);

public:
	void procTimer();
	virtual void procRecvData(const uint8_t *data, const uint16_t len);
	virtual void procRecvError(TaskLayerObserver::Error error);
};

}

#endif
#endif
