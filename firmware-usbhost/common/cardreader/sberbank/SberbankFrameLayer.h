#ifndef COMMON_SBERBANK_FRAMELAYER_H
#define COMMON_SBERBANK_FRAMELAYER_H

#include "SberbankProtocol.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"

namespace Sberbank {

class FrameLayer : public FrameLayerInterface, public UartReceiveHandler {
public:
	FrameLayer(TimerEngine *timers, AbstractUart *uart);
	virtual ~FrameLayer();
	virtual void setObserver(FrameLayerObserver *observer);
	virtual void reset();
	virtual bool sendPacket(Buffer *data);
	virtual bool sendControl(uint8_t byte);

private:
	enum State {
		State_Idle = 0,
		State_STX,
		State_MARK,
		State_ETX,
	};
	TimerEngine *timers;
	AbstractUart *uart;
	FrameLayerObserver *observer;
	State state;
	Timer *timer;
	Buffer sendBuf;
	Buffer recvBuf;
	uint16_t recvLength;
	Buffer crcBuf;

	void sendData(uint8_t *data, uint16_t dataLen);

	void gotoStateStx();
	void stateStxRecv();
	void gotoStateMark();
	void stateMarkRecv();
	void gotoStateEtx();
	void stateEtxRecv();
	void stateRecvTimeout();

public:
	void handle();
	void procTimer();
};

}

#endif
