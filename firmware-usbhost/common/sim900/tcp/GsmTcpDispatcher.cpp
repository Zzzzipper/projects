#include "GsmTcpDispatcher.h"
#include "logger/include/Logger.h"

namespace Gsm {

TcpDispatcher::TcpDispatcher(TimerEngine *timers, CommandProcessor *commandProcessor, StatStorage *stat) {
	this->conn1 = new TcpConnection(timers, commandProcessor, 1, stat);
	this->conn2 = new TcpConnection(timers, commandProcessor, 2, stat);
	this->conn3 = new TcpConnection(timers, commandProcessor, 3, stat);
	this->conn4 = new TcpConnection(timers, commandProcessor, 4, stat);
	this->conn5 = new TcpConnection(timers, commandProcessor, 5, stat);
#ifdef GSM_STATIC_IP
	this->conn0 = new TcpConnection(timers, commandProcessor, 0, stat);
	this->server = new TcpServer(timers, conn4);
#endif
}

TcpDispatcher::~TcpDispatcher() {
#ifdef GSM_STATIC_IP
	delete this->server;
	delete this->conn0;
#endif
	delete this->conn5;
	delete this->conn4;
	delete this->conn3;
	delete this->conn2;
	delete this->conn1;
}

void TcpDispatcher::bind(TcpServerProcessor *serverProcessor) {
#ifdef GSM_STATIC_IP
	server->bind(serverProcessor);
#endif
}

void TcpDispatcher::procEvent(const char *data) {
	LOG_DEBUG(LOG_SIMD, "procEvent");
	conn1->procEvent(data);
	conn2->procEvent(data);
	conn3->procEvent(data);
	conn4->procEvent(data);
	conn5->procEvent(data);
#ifdef GSM_STATIC_IP
	conn0->procEvent(data);
	server->procEvent(data);
#endif
}

void TcpDispatcher::proc(Event *event) {
	conn1->proc(event);
	conn2->proc(event);
	conn3->proc(event);
	conn4->proc(event);
	conn5->proc(event);
}

}
