#ifndef LIB_SALEMANAGER_EXEMASTER_PACKETLAYER_H_
#define LIB_SALEMANAGER_EXEMASTER_PACKETLAYER_H_

#include "common/utils/include/Event.h"
#include "common/utils/include/Buffer.h"
#include "common/timer/include/TimerEngine.h"
#include "common/uart/include/interface.h"
#include "common/config/include/StatStorage.h"

class ExeMasterPacketLayer : public UartReceiveHandler {
public:
	class Customer {
	public:
		virtual void recvByte(uint8_t byte) = 0;
		virtual void recvTimeout() = 0;
	};

	ExeMasterPacketLayer(AbstractUart *uart, TimerEngine *timers, Customer *customer, StatStorage *stat);
	virtual ~ExeMasterPacketLayer();
	void reset();
	void sendCommand(uint8_t command, uint32_t timeout);
	void sendData(uint16_t baseUnit, uint8_t scalingFactor, uint8_t decimalPoint, bool changeFlag, uint32_t timeout);

	void handle() override;
	bool isInterruptMode() override;
	void procRecvTimer();

	static uint8_t calcParity(uint8_t byte);

private:
	enum State {
		State_Idle,
		State_Delay,
		State_Command,
		State_Data,
	};

	AbstractUart *uart;
	TimerEngine *timers;
	Customer *customer;
	Timer *timerRecv;
	State state;
	StatNode *countCrcError;
	StatNode *countResponse;
	StatNode *countTimeout;
	Buffer data;
	uint16_t dataCount;
	uint16_t tryCount;
	uint32_t timeout;

	void sendParityByte(uint8_t b);

	void gotoStateDelay();
	void stateCommandRecv(uint8_t b);
	void stateCommandRecvTimeout();
	void stateDataRecv(uint8_t b);
	void stateDataRecvTimeout();
	uint8_t decimalPointToDataFormat(uint8_t decimalPoint);
};

#endif
