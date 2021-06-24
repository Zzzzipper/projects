#ifndef LIB_SIM900_SERVER_H
#define LIB_SIM900_SERVER_H

#include "sim900/include/GsmTcpConnection.h"
#include "http/include/HttpServer.h"
#include "timer/include/TimerEngine.h"

namespace Gsm {

class TcpServerProcessor {
public:
	virtual ~TcpServerProcessor() {}
	virtual void bind(Http::ServerInterface *conn) = 0;
	virtual void accept() = 0;
};

class TcpServer {
public:
	TcpServer(TimerEngine *timerEngine, TcpConnection *conn);
	~TcpServer();
	void bind(TcpServerProcessor *serverProcessor);
	void procEvent(const char *data);

private:
	TcpConnection *conn;
	Http::Server *server;
	TcpServerProcessor *proc;
};

}

#endif
