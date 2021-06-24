#ifndef COMMON_GSM_FIRMWARE_UPDATER_H
#define COMMON_GSM_FIRMWARE_UPDATER_H

#include "sim900/command/GsmCommand.h"
#include "sim900/GsmHardware.h"
#include "dex/include/DexDataParser.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Event.h"
#include "utils/include/Buffer.h"

namespace Gsm {

class FirmwareUpdater : public Dex::DataParser, public UartReceiveHandler {
public:	
	FirmwareUpdater(HardwareInterface *hardware, AbstractUart *uart, TimerEngine *timers);
	virtual ~FirmwareUpdater();

	virtual void setObserver(EventObserver *observer);
	virtual Result start(uint32_t dataSize);
	virtual Result procData(const uint8_t *data, const uint16_t len);
	virtual Result complete();
	virtual void error();

	virtual void handle();

private:
	enum State {
		State_Idle = 0,
		State_PowerButtonPress,
		State_PowerButtonDelay,
		State_Sync,
		State_HeaderRecv,
		State_HeaderSend,
		State_PacketSize,
		State_DataRecv,
		State_DataSend,
		State_End,
		State_Reset,
		State_Error,
	};

	HardwareInterface *hardware;
	AbstractUart *uart;
	TimerEngine *timers;
	EventCourier courier;
	State state;
	Timer *timer;
	uint16_t tries;
	Buffer data;
	uint16_t maxSize;
	uint32_t count;

	void procTimer();
	void gotoStatePowerButtonPress();
	void gotoStatePowerButtonDelay();

	void gotoStateSync();
	void stateSyncTimeout();
	void stateSyncRecv();

	void gotoStateHeaderRecv();
	void stateHeaderRecvData(const uint8_t *data, const uint16_t len);
	void stateHeaderSendRecv();

	void statePacketSizeRecv();

	void stateDataRecvData(const uint8_t *data, const uint16_t len);
	void stateDataSendRecv();
	void stateDataEndRecv();

	void gotoStateEnd();
	void stateEndRecv();

	void gotoStateReset();
	void stateResetRecv();

	void gotoStateError();
	void stateErrorRecv();
	void stateErrorTimeout();
};

}

#endif
