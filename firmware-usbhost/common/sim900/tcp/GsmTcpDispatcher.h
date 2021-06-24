#ifndef LIB_SIM900_TCPDISPATCHER_H
#define LIB_SIM900_TCPDISPATCHER_H

#include "sim900/tcp/GsmTcpServer.h"
//+++
#include "config.h"
//+++

namespace Gsm {

class TcpDispatcher {
public:
	TcpDispatcher(TimerEngine *timers, CommandProcessor *commandProcessor, StatStorage *stat);
	~TcpDispatcher();
	void bind(TcpServerProcessor *serverProcessor);
	TcpIp *getConnection1() { return conn1; }
	TcpIp *getConnection2() { return conn2; }
	TcpIp *getConnection3() { return conn3; }
	TcpIp *getConnection4() { return conn4; }
	TcpIp *getConnection5() { return conn5; }
	void procEvent(const char *data);
	void proc(Event *event);

private:
	TcpConnection *conn1;
	TcpConnection *conn2;
	TcpConnection *conn3;
	TcpConnection *conn4;
	TcpConnection *conn5;
#ifdef GSM_STATIC_IP
	TcpConnection *conn0;
	TcpServer *server;
#endif
};

}

#endif
