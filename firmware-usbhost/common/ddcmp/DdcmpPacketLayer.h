#ifndef COMMON_DDCMP_PACKETLAYER_H
#define COMMON_DDCMP_PACKETLAYER_H

#include "DdcmpProtocol.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"

namespace Ddcmp {

class PacketLayer : public PacketLayerInterface, public UartReceiveHandler {
public:
	PacketLayer(TimerEngine *timers, AbstractUart *uart);
	virtual ~PacketLayer();
	virtual void setObserver(PacketLayerObserver *observer);
	virtual void reset();
	virtual void sendControl(uint8_t *cmd, uint16_t cmdLen);
	virtual void sendData(uint8_t *cmd, uint16_t cmdLen, uint8_t *data, uint16_t dataLen);

private:
	enum State {
		State_Idle = 0,
		State_Header,
		State_Data,
	};
	TimerEngine *timers;
	AbstractUart *uart;
	PacketLayerObserver *observer;
	State state;
	Timer *timer;
	Buffer sendBuf;
	Buffer recvBuf;
	uint16_t recvDataLen;

	void gotoStateHeader();
	void stateHeaderRecv();
	void gotoStateData();
	void stateDataRecv();
	void stateDataTimeout();

public:
	void handle();
	void procTimer();
};

}

#endif
