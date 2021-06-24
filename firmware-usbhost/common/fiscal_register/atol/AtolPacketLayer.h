#if 1
#ifndef COMMON_ATOLRECEIVER_H
#define COMMON_ATOLRECEIVER_H

#include "AtolProtocol.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"

namespace Atol {

class PacketParser {
public:
	PacketParser();
	void start();
	void procData(Buffer *data);
	bool isComplete();
	uint8_t getPackeId();
	uint8_t *getData();
	uint16_t getDataLen();

private:
	enum State {
		State_Stx = 0,
		State_Header,
		State_Data,
		State_Crc,
		State_Complete
	};

	State state;
	Buffer data;
	uint8_t packetId;
	uint16_t dataLen;
	Crc crc;
	bool flagESC;

	void gotoStateSTX();
	void stateStxSymbol(uint8_t b);
	void stateHeaderSymbol(uint8_t b);
	void stateDataSymbol(uint8_t b);
	void stateCrcSymbol(uint8_t b);
	void procUnwaitedSTX();
};

class PacketLayer : public PacketLayerInterface, public EventObserver {
public:
	PacketLayer(TimerEngine *timers, TcpIp *conn);
	virtual ~PacketLayer();
	void setObserver(PacketLayerObserver *observer);
	bool connect(const char *domainname, uint16_t port, TcpIp::Mode mode);
	bool sendPacket(const uint8_t *data, const uint16_t dataLen);
	bool disconnect();

private:
	enum State {
		State_Idle = 0,
		State_Connect,
		State_Wait,
		State_Send,
		State_Recv,
		State_Disconnect,
	};
	TimerEngine *timers;
	TcpIp *conn;
	PacketLayerObserver *observer;
	Timer *recvTimer;
	State state;
	PacketParser parser;
	uint8_t sendPacketId;
	Buffer sendBuf;
	Buffer recvBuf;

	void stateConnectEvent(Event *event);
	void stateWaitEvent(Event *event);
	void stateSendEvent(Event *event);
	void gotoStateRecv();
	void stateRecvEvent(Event *event);
	void stateRecvEventRecvData(Event *event);
	void stateDisconnectEvent(Event *event);
	void incSendPacketId();

public:
	virtual void proc(Event *event);
	void procRecvTimer();
};

}

#endif
#else
#ifndef COMMON_ATOLRECEIVER_H
#define COMMON_ATOLRECEIVER_H

#include "AtolProtocol.h"
#include "http/include/TcpIp.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"

namespace Atol {

class PacketLayer : public PacketLayerInterface, public UartReceiveHandler {
public:
	PacketLayer(TimerEngine *timers, TcpIp *conn);
	virtual ~PacketLayer();
	void setObserver(PacketLayerObserver *observer);
	uint8_t sendPacket(const uint8_t *data, const uint16_t dataLen);

private:
	enum State {
		State_STX = 0,
		State_Len0,
		State_Len1,
		State_Id,
		State_Data,
		State_CRC,
	};
	TimerEngine *timers;
	TcpIp *conn;
	PacketLayerObserver *observer;
	Timer *recvTimer;
	State state;
	uint8_t sendPacketId;
	Buffer *recvBuf;
	uint8_t recvPacketId;
	uint16_t recvSize;
	Crc recvCrc;
	bool flagESC;

	void gotoStateSTX();
	void stateSTXHandle();
	void procUnwaitedSTX();
	void stateLen0Handle();
	void stateLen1Handle();
	void stateIdHandle();
	void stateDataHandle();
	void stateCRCHandle();
	void incSendPacketId();

public:
	void handle();
	void procRecvTimer();
};

}

#endif
#endif
