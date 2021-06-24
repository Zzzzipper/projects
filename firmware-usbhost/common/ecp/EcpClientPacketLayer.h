#ifndef COMMON_ECP_CLIENTPACKETLAYER_H
#define COMMON_ECP_CLIENTPACKETLAYER_H

#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"
#include "EcpProtocol.h"

namespace Ecp {

class ClientPacketLayer : public ClientPacketLayerInterface, public UartReceiveHandler {
public:
	ClientPacketLayer(TimerEngine *timers, AbstractUart *uart);
	virtual ~ClientPacketLayer();
	virtual void setObserver(Observer *observer);
	virtual bool connect();
	virtual void disconnect();
	virtual bool sendData(const Buffer *data);

private:
	enum State {
		State_Idle = 0,
		State_Connecting,
		State_Ready,
		State_Confirm,
		State_Length,
		State_Data,
		State_CRC,
		State_Answer,
		State_KeepAlive,
		State_Disconnecting,
	};
	AbstractUart *uart;
	TimerEngine *timers;
	Observer *observer;
	State state;
	Timer *timer;
	const Buffer *sendBuf;
	Buffer *recvBuf;
	uint8_t recvLength;
	uint16_t tries;

	void gotoStateConnecting();
	void stateConnectionRecv();
	void gotoStateReady();
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
	void gotoKeepAlive();
	void stateKeepAliveRecv();
	void stateKeepAliveTimeout();
	void gotoStateDisconnecting();
	void stateStateDisconnectingRecv();
	void stateStateDisconnectingTimeout();
	void procError();

public:
	virtual void handle();
	void procTimer();
};

}

#endif
