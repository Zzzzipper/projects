#ifndef SHTRIHMRECEIVER_H
#define SHTRIHMRECEIVER_H

#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"

class ShtrihM;

class ShtrihmReceiver : public UartReceiveHandler {
public:
	ShtrihmReceiver(TimerEngine *timers, AbstractUart *uart, ShtrihM *context);
	virtual ~ShtrihmReceiver();
	bool sendPacket(Buffer *data);

private:
	enum State {
		State_Idle = 0,
		State_ENQ,
		State_Confirm,
		State_STX,
		State_Length,
		State_Data,
		State_CRC,
		State_Answer,
		State_NoConnection,
		State_SkipSTX,
		State_SkipLength,
		State_SkipData,
		State_SkipCRC,
		State_SkipAnswer,
		State_SkipPause,
	};
	TimerEngine *timers;
	AbstractUart *uart;
	ShtrihM *observer;
	State state;
	Timer *timer;
	Buffer *sendBuf;
	Buffer *recvBuf;
	uint8_t recvLength;
	uint8_t tryNumber;

	void gotoStateENQ();
	void stateENQRecv();
	void stateENQTimeout();
	void gotoStateConfirm();
	void stateConfirmRecv();
	void gotoStateSTX();
	void stateSTXRecv();
	void gotoStateLenght();
	void stateLengthRecv();
	void stateDataRecv();
	void stateCRCRecv();
	void gotoStateAnswer();
	void stateAnswerTimeout();
	void gotoStateNoConnection();
	void gotoStateSkipSTX();
	void stateSkipSTXRecv();
	void gotoStateSkipLenght();
	void stateSkipLengthRecv();
	void stateSkipDataRecv();
	void stateSkipCRCRecv();
	void gotoStateSkipAnswer();
	void stateSkipAnswerTimeout();

public:
	void handle();
	void procTimer();
};

#endif // SHTRIHMRECEIVER_H
