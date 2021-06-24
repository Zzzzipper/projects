#ifndef ATOLTASKLAYER_H
#define ATOLTASKLAYER_H

#include "AtolProtocol.h"
#include "timer/include/TimerEngine.h"
#include "utils/include/Buffer.h"

namespace Atol {

class TaskLayer : public TaskLayerInterface, public PacketLayerObserver {
public:
	TaskLayer(TimerEngine *timers, PacketLayerInterface *packetLayer);
	virtual void setObserver(TaskLayerObserver *observer);
	virtual bool connect(const char *domainname, uint16_t port, TcpIp::Mode mode);
	virtual bool sendRequest(const uint8_t *data, const uint16_t dataLen);
	virtual bool disconnect();

private:
	enum State {
		State_Idle = 0,
		State_Connect,
		State_InitWait,
		State_Init,
		State_Wait,
		State_TaskAdd,
		State_TaskAsync,
		State_TaskReq,
		State_TackReconnect,
		State_Disconnect,
		State_ReconnectPause,
	};
	TimerEngine *timers;
	PacketLayerInterface *packetLayer;
	TaskLayerObserver *observer;
	const char *ipaddr;
	uint16_t port;
	TcpIp::Mode mode;
	State state;
	Timer *timer;
	const uint8_t *sendData;
	uint16_t sendDataLen;
	Buffer task;
	uint8_t taskId;
	uint8_t tryNumber;

	void gotoStateConnect();
	void stateConnectProcError(PacketLayerObserver::Error error);

	void gotoStateInitWait();
	void stateInitWaitTimeout();

	void gotoStateInit();
	void stateInitResponse(const uint8_t packetId, const uint8_t *data, const uint16_t dataLen);
	void stateInitError(PacketLayerObserver::Error error);

	void gotoStateTaskAdd();
	void stateTaskAddResponse(const uint8_t packetId, const uint8_t *data, const uint16_t dataLen);
	void stateTaskAddResponseStopped(const uint8_t *data, const uint16_t dataLen);
	void stateTaskAddError(PacketLayerObserver::Error error);

	void gotoStateTaskAsync();
	void stateTaskAsyncResponse(const uint8_t packetId, const uint8_t *data, const uint16_t dataLen);
	void stateTaskAsyncTimeout();

	void gotoStateTaskReq();
	void stateTaskReqResponse(const uint8_t packetId, const uint8_t *data, const uint16_t dataLen);

	void gotoStateTaskReconnect();
	void stateTaskReconnectError(PacketLayerObserver::Error error);

	void gotoStateReconnectPause();
	void stateReconnectPauseTimeout();

	void incTaskId();
	void procError(TaskLayerObserver::Error error);

public:
	virtual void procRecvData(uint8_t packetId, const uint8_t *data, const uint16_t len);
	virtual void procError(PacketLayerObserver::Error error);
	void procTimer();
};

}

#endif // ATOLTASKLAYER_H
