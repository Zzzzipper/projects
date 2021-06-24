#ifndef COMMON_TERMINALFA_PACKETLAYER_H
#define COMMON_TERMINALFA_PACKETLAYER_H

#include "TerminalFaProtocol.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"

namespace TerminalFa {

class PacketLayer : public PacketLayerInterface, public UartReceiveHandler {
public:
	PacketLayer(TimerEngine *timers, AbstractUart *uart);
	virtual ~PacketLayer();
	void setObserver(Observer *observer);
	bool sendPacket(Buffer *data);

private:
	enum State {
		State_Idle = 0,
		State_RecvHeader,
		State_RecvData,
		State_SkipData,
	};
	TimerEngine *timers;
	AbstractUart *uart;
	Observer *observer;
	State state;
	Timer *timer;
	Buffer *recvBuf;
	uint16_t recvLength;
	Error recvError;

	void sendData(uint8_t *data, uint16_t dataLen);
	void sendCrc(Header *header, uint8_t *data, uint16_t dataLen);
	void skipData();

	void gotoStateRecvHeader();
	void stateRecvHeaderRecv();
	void gotoStateRecvData();
	void stateRecvDataRecv();
	void gotoStateSkip(Error error);
	void stateSkipRecv();
	void stateSkipTimeout();

	void procRecvError(Error error);

public:
	void handle();
	void procTimer();
};

}

#endif
