#ifndef COMMON_TWOCAN_PACKETLAYER_H
#define COMMON_TWOCAN_PACKETLAYER_H

#include "TwocanProtocol.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"

namespace Twocan {

class PacketLayer : public PacketLayerInterface, public UartReceiveHandler {
public:
	PacketLayer(TimerEngine *timers, AbstractUart *uart);
	virtual ~PacketLayer();
	virtual void setObserver(PacketLayerObserver *observer);
	virtual void reset();
	virtual bool sendPacket(Buffer *data);
	virtual bool sendControl(uint8_t byte);

private:
	enum State {
		State_Idle = 0,
		State_STX,
		State_Length,
		State_Data,
		State_CRC,

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
	Buffer crcBuf;

	void sendData(uint8_t *data, uint16_t dataLen);
	void skipData();

	void gotoStateWait();
	void stateStxRecv();
	void gotoStateLength();
	void stateLengthRecv();
	void gotoStateData();
	void stateDataRecv();
	void gotoStateCrc();
	void stateCrcRecv();
	void stateRecvTimeout();

public:
	void handle();
	void procTimer();
};

}

#endif
