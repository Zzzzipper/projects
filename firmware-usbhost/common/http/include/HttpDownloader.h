#ifndef LIB_HTTP_HTTPDOWNLOADER_H
#define LIB_HTTP_HTTPDOWNLOADER_H

#include "HttpClient.h"

#include "utils/include/Buffer.h"
#include "dex/include/DexDataParser.h"

class TimerEngine;

namespace Http {

class Downloader : public EventObserver {
public:
	Downloader(TimerEngine *timers, Http::ClientInterface *connection);
	virtual ~Downloader();
	void download(const char *serverName, uint16_t serverPort, const char *serverPath, Dex::DataParser *receiver);
	void proc(Event *event);

private:
	enum State {
		State_Idle = 0,
		State_Start,
		State_Recv,
	};

	TimerEngine *timers;
	Http::ClientInterface *connection;
	State state;
	StringBuilder serverName;
	uint16_t serverPort;
	StringBuilder serverPath;
	StringBuilder reqData;
	Http::Request req;
	Http::Response resp;
	StringBuilder data;
	uint32_t dataFrom;
	uint32_t dataTo;
	uint32_t dataSize;
	Dex::DataParser *receiver;
	uint16_t tryNumber;

	void gotoStateStart();
	void stateStartEvent(Event *event);
	void stateStartEventComplete();
	void gotoStateRecv();
	void stateRecvEvent(Event *event);
	void stateRecvEventComplete();
	void tryAgain();
	void procError();
};

}

#endif
