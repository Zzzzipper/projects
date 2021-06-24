#ifndef COMMON_ECP_SERVERPACKETLAYER_H
#define COMMON_ECP_SERVERPACKETLAYER_H

#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"
#include "EcpProtocol.h"

namespace Ecp {

class ServerPacketLayer : public ServerPacketLayerInterface, public UartReceiveHandler {
public:
	ServerPacketLayer(TimerEngine *timers, AbstractUart *uart);
	virtual ~ServerPacketLayer();
	virtual void setObserver(Observer *observer);
	virtual void reset();
	virtual void shutdown();
	virtual void disconnect();
	virtual bool sendData(const Buffer *data);

private:
	enum State {
		State_Idle = 0,
		State_Connecting,
		State_STX,
		State_Length,
		State_Data,
		State_CRC,
		State_Answer,
		State_Disconnecting,
	};

	enum Command {
		Command_None = 0,
		Command_Disconnect,
	};

	AbstractUart *uart;
	TimerEngine *timers;
	Observer *observer;
	State state;
	Command command;
	Timer *timer;
	Buffer *recvBuf;
	uint8_t recvLength;
	uint16_t tries;

	void gotoStateConnecting();
	void stateConnectingRecv();
	void gotoStateSTX();
	void stateSTXRecv();
	void gotoStateLenght();
	void stateLengthRecv();
	void stateDataRecv();
	void stateCRCRecv();
	void gotoStateAnswer();
	void stateAnswerTimeout();
	void stateDisconnectingRecv();

public:
	virtual void handle();
	void procTimer();
};

}

#endif
