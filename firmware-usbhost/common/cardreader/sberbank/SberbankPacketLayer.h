#ifndef COMMON_SBERBANK_PACKETLAYER_H
#define COMMON_SBERBANK_PACKETLAYER_H

#include "SberbankProtocol.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"
#include "utils/include/Fifo.h"

namespace Sberbank {

class PacketSender {
public:
	PacketSender(TimerEngine *timers, FrameLayerInterface *frameLayer);
	~PacketSender();
	void setObserver(PacketLayerObserver *observer);
	void reset();
	bool sendPacket(Buffer *data);

	void procControl(uint8_t control);
	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Wait,
		State_SendNext,
		State_SendLast,
	};
	TimerEngine *timers;
	FrameLayerInterface *frameLayer;
	PacketLayerObserver *observer;
	State state;
	Timer *timer;
	Fifo<Buffer*> queue;
	Buffer *data;
	Buffer sendBuf;
	uint8_t packetId;
	uint16_t procLen;
	uint16_t sendLen;

	void gotoStateWait();
	void gotoStateSendNext();
	void stateSendNextProcControl(uint8_t control);
	void stateSendNextTimeout();
	void gotoStateSendLast();
	void stateSendLastProcControl(uint8_t control);
	void stateSendLastTimeout();
};

class PacketReceiver {
public:
	PacketReceiver(TimerEngine *timers, FrameLayerInterface *frameLayer);
	~PacketReceiver();
	void setObserver(PacketLayerObserver *observer);
	void reset();

	void procPacket(const uint8_t *data, const uint16_t dataLen);
	void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Wait,
		State_Chain,
	};
	TimerEngine *timers;
	FrameLayerInterface *frameLayer;
	PacketLayerObserver *observer;
	State state;
	Timer *timer;
	Buffer recvBuf;
	uint8_t packetId;

	void stateWaitRecv(const uint8_t *data, const uint16_t dataLen);
	void stateChainRecv(const uint8_t *data, const uint16_t dataLen);
	bool recvPacket(const uint8_t *data, const uint16_t dataLen);
};

class PacketLayer : public PacketLayerInterface, public FrameLayerObserver {
public:
	PacketLayer(TimerEngine *timers, FrameLayerInterface *frameLayer);
	virtual void setObserver(PacketLayerObserver *observer);
	virtual void reset();
	virtual bool sendPacket(Buffer *data);

	virtual void procPacket(const uint8_t *data, const uint16_t len);
	virtual void procControl(uint8_t control);
	virtual void procError(Error error);

private:
	PacketSender sender;
	PacketReceiver receiver;
};

}

#endif
