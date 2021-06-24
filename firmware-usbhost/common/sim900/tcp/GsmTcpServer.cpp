#include "sim900/tcp/GsmTcpServer.h"
#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"

namespace Gsm {

TcpServer::TcpServer(TimerEngine *timerEngine, TcpConnection *conn) :
	conn(conn),
	proc(NULL)
{
	server = new Http::Server(timerEngine, conn);
}

TcpServer::~TcpServer() {
	delete server;
}

void TcpServer::bind(TcpServerProcessor *serverProcessor) {
	this->proc = serverProcessor;
	if(this->proc != NULL) {
		this->proc->bind(server);
	}
}

void TcpServer::procEvent(const char *data) {
	if(proc == NULL) {
		return;
	}
	StringParser parser(data, strlen(data));
	uint16_t recvId;
	if(parser.getNumber(&recvId) == false) {
		return;
	}
	if(parser.compareAndSkip(", REMOTE IP:") == false) {
		return;
	}
	LOG_INFO(LOG_HTTP, ">>>>>INCOMMING " << recvId);
	if(conn->isConnected() == false) {
		conn->accept(recvId);
		proc->accept();
	}
}

}
