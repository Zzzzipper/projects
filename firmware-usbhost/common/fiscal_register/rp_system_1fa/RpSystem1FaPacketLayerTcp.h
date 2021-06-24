#ifndef COMMON_FISCALREGISTER_RPSYSTEM1FAPACKETLAYERTCP_H
#define COMMON_FISCALREGISTER_RPSYSTEM1FAPACKETLAYERTCP_H

#include "RpSystem1FaPacketLayer.h"

#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"

namespace RpSystem1Fa {

class FiscalRegister;

class PacketLayerTcp : public PacketLayer, public UartReceiveHandler {
public:
	PacketLayerTcp(TimerEngine *timers, AbstractUart *uart);
	virtual ~PacketLayerTcp();
	virtual bool sendPacket(const Buffer *data);

private:
	enum State {
		State_Idle = 0,
		State_Length,
		State_Data,
		State_Answer,
		State_NoConnection,
	};
	TimerEngine *timers;
	AbstractUart *uart;
	State state;
	Timer *timer;
	const Buffer *sendBuf;
	Buffer *recvBuf;
	uint8_t recvLength;

	void gotoStateLenght();
	void stateLengthRecv();
	void stateDataRecv();
	void gotoStateAnswer();
	void stateAnswerTimeout();
	void gotoStateNoConnection();

public:
	virtual void handle();
	void procTimer();
};

}

#endif
