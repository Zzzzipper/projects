#ifndef LIB_SIM900_TCPCONNECTION_H
#define LIB_SIM900_TCPCONNECTION_H

#include "sim900/command/GsmCommand.h"
#include "tcpip/include/TcpIp.h"
#include "config/include/StatStorage.h"

#include <stdint.h>

class EventObserver;
class TimerEngine;
class Timer;

namespace Gsm {

class TcpConnection : public TcpIp, public Command::Observer {
public:
	TcpConnection(TimerEngine *timers, CommandProcessor *commandProcessor, uint16_t id, StatStorage *stat);
	virtual ~TcpConnection();
	bool isConnected();
	void accept(uint16_t id);
	virtual void setObserver(EventObserver *observer) override;
	virtual bool connect(const char *domainname, uint16_t port, Mode mode) override;
	virtual bool hasRecvData() override;
	virtual bool send(const uint8_t *data, uint32_t dataLen) override;
	virtual bool recv(uint8_t *buf, uint32_t bufSize) override;
	virtual void close() override;

private:
	enum State {
		State_Idle = 0,
		State_GprsCheck,
		State_GprsWait,
		State_Ssl,
		State_Open,
		State_OpenWait,
		State_Ping,
		State_Ready,
		State_Send,
		State_SendClose,
		State_RecvWait,
		State_Recv,
		State_RecvClose,
		State_Close,
		State_FakeClose,
	};
	TimerEngine *timers;
	CommandProcessor *commandProcessor;
	uint16_t id;
	StringBuilder eventPrefix;
	Timer *timer;
	EventCourier courier;
	StatNode *state;
	StatNode *wrongStateCount;
	StatNode *executeErrorCount;
	StatNode *cipStatusErrorCount;
	StatNode *gprsErrorCount;
	StatNode *cipSslErrorCount;
	StatNode *cipStartErrorCount;
	StatNode *connectErrorCount;
	StatNode *sendErrorCount;
	StatNode *recvErrorCount;
	StatNode *unwaitedCloseCount;
	StatNode *idleTimeoutCount;
	StatNode *rxtxTimeoutCount;
	StatNode *cipPing1ErrorCount;
	StatNode *cipPing2ErrorCount;
	StatNode *otherErrorCount;
	StringBuilder serverDomainName;
	uint16_t serverPort;
	Mode serverMode;
	Command *command;
	uint8_t *data;
	uint16_t dataLen;
	uint16_t dataSize;
	uint16_t procLen;
	uint16_t packetLen;
	uint8_t tryCount;
	bool recvDataFlag;

	void setId(uint16_t id);
	void procTimer();
	bool isEvent(const char *data, const char *expected);
	bool isEventConnectionClosed(const char *data);
	bool isEventRecvData(const char *data);

	void gotoStateIdle();
	void gotoStateGprsCheck();
	void stateGprsCheckResponse(Command::Result result, const char *data);
	void gotoStateGprsWait(uint32_t line);
	void stateGprsWaitEvent(Event *event);
	void stateGprsWaitTimeout();

	void gotoStateSsl();
	void stateSslResponse(Command::Result result);
	void gotoStateOpen();
	void stateOpenResponse(Command::Result result);
	void gotoStateOpenWait();
	void stateOpenWaitEvent(const char *data);
	void stateOpenWaitTimeout();
	void gotoStatePing();
	void statePingResponse(Command::Result result);
	void stateOpenError(uint32_t line);

	void gotoStateReady();
	void stateReadyEvent(const char *data);
	void stateReadyTimeout();

	void gotoStateSend();
	void stateSendResponse(Command::Result result);
	void stateSendEvent(const char *data);
	void stateSendTimeout();
	void gotoStateSendClose();
	void stateSendCloseResponse();

	void gotoStateRecvWait();
	void stateRecvWaitEvent(const char *data);
	void stateRecvWaitTimeout();

	void gotoStateRecv();
	void stateRecvResponse(Command::Result result, const char *data);
	bool parseRecvResponse(const char *data);
	void stateRecvEvent(const char *data);
	void stateRecvTimeout();
	void gotoStateRecvClose();
	void stateRecvCloseResponse();

	void gotoStateClose();
	void stateCloseResponse();

	void gotoStateFakeClose();
	void stateFakeCloseTimeout();

public:
	virtual void proc(Event *event);
	virtual void procResponse(Command::Result result, const char *data);
	void procEvent(const char *data);
};

}
#endif
