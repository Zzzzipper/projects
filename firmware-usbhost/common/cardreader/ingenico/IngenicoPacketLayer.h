#ifndef COMMON_INGENICO_PACKETLAYER_H
#define COMMON_INGENICO_PACKETLAYER_H

#include "IngenicoProtocol.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"

namespace Ingenico {

class PacketLayer : public PacketLayerInterface, public UartReceiveHandler {
public:
	PacketLayer(TimerEngine *timers, AbstractUart *uart);
	virtual ~PacketLayer();
	virtual void setObserver(PacketLayerObserver *observer);
	virtual void reset();
	virtual bool sendPacket(Buffer *data);

private:
	enum State {
		State_Idle = 0,
		State_SOH,
		State_Length,
		State_Data,

		State_RecvHeader,
		State_RecvData,
		State_SkipData,
	};
	TimerEngine *timers;
	AbstractUart *uart;
	PacketLayerObserver *observer;
	State state;
	Timer *timer;
	Buffer recvBuf;
	uint16_t recvLength;

	void sendData(uint8_t *data, uint16_t dataLen);
	void skipData();

	void gotoStateWait();
	void stateSohRecv();
	void gotoStateLength();
	void stateLengthRecv();
	void gotoStateData();
	void stateDataRecv();
	void gotoStateCrc();
	void stateCrcRecv();
	void stateRecvTimeout();

#if 0
	void gotoStateSkip(Error error);
	void stateSkipRecv();
	void stateSkipTimeout();
	void procRecvError(Error error);
#endif

public:
	void handle();
	void procTimer();
};

}

#endif
