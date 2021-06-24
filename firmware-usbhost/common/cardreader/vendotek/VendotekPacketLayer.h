#ifndef COMMON_VENDOTEK_PACKETLAYER_H
#define COMMON_VENDOTEK_PACKETLAYER_H

#include "VendotekProtocol.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"

namespace Vendotek {

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
		State_Start,
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
	uint32_t recvDataLen;
	uint32_t recvDataCnt;
	Buffer crcBuf;

	void sendData(uint8_t *data, uint16_t dataLen);
	void skipData();

	void gotoStateWait();
	void stateStartRecv();
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
