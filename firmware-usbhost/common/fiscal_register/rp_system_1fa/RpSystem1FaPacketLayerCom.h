#ifndef COMMON_FISCALREGISTER_RPSYSTEM1FAPACKETLAYERCOM_H
#define COMMON_FISCALREGISTER_RPSYSTEM1FAPACKETLAYERCOM_H

#include "RpSystem1FaPacketLayer.h"

#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"

namespace RpSystem1Fa {

class FiscalRegister;

class PacketLayerCom : public PacketLayer, public UartReceiveHandler {
public:
	PacketLayerCom(TimerEngine *timers, AbstractUart *uart);
	virtual ~PacketLayerCom();
	virtual bool sendPacket(const Buffer *data);

private:
	enum State {
		State_Idle = 0,
		State_Confirm,
		State_STX,
		State_Length,
		State_Data,
		State_CRC,
		State_Answer,
		State_NoConnection,
	};
	TimerEngine *timers;
	AbstractUart *uart;
	FiscalRegister *observer;
	State state;
	Timer *timer;
	const Buffer *sendBuf;
	Buffer *recvBuf;
	uint8_t recvLength;

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

public:
	virtual void handle();
	void procTimer();
};

}

#endif
