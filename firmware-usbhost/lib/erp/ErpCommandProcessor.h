#ifndef LIB_ERP_COMMANDPROCESSOR_H_
#define LIB_ERP_COMMANDPROCESSOR_H_

#include "ErpCashless.h"

#include "common/sim900/tcp/GsmTcpServer.h"
#include "http/include/HttpServer.h"
#include "utils/include/Json.h"

class ErpCommandProcessor : public Gsm::TcpServerProcessor, public EventObserver {
public:
	ErpCommandProcessor(Mdb::DeviceContext *context, TimerEngine *timerEngine, EventEngineInterface *eventEngine);
	virtual ~ErpCommandProcessor();
	void bind(Http::ServerInterface *server) override;
	void accept() override;
	MdbMasterCashlessInterface *getCashless() { return cashless; }
	void proc(Event *event) override;

private:
	enum State {
		State_Idle = 0,
		State_WaitRequest,
		State_SendResponse,
		State_Close,
	};

	State state;
	Http::ServerInterface *server;
	StringBuilder data;
	Http::Request2 req;
	Http::Response resp;
	JsonParser jsonParser;
	uint16_t command;
	uint32_t value;
	ErpCashless *cashless;

	void stateWaitRequestEvent(Event *event);
	void stateWaitRequestEventRequestIncoming();
	void sendSuccessResponse();
	void sendFailureResponse();
	void gotoStateSendResponse();
	void stateSendResponseEvent(Event *event);
	void stateSendResponseEventSendComplete(Event *event);
	void procUnwaitedClose();
};

#endif
