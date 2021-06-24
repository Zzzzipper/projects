#ifndef COMMON_HTTP_REQUESTPARSER_H
#define COMMON_HTTP_REQUESTPARSER_H

#include "http/include/Http.h"

namespace Http {

class RequestParser {
public:
	void start(Request2 *req);
	void parseData(uint16_t dataLen);
	uint8_t *getBuf();
	uint32_t getBufSize();
	bool isComplete();

private:
	enum State {
		State_RecvHead,
		State_RecvDataTail,
		State_RecvData,
		State_RecvComplete,
		State_RecvError,
	};
	State state;
	Request2 *req;
	uint16_t procData;
	uint32_t recvData;

	void parseHead();
	void parseTail();
	void parseData();
	void shiftData();
	void parseHeader(char *line, uint16_t lineLen);
	void parseHeaderHead(char *line, uint16_t lineLen);
	void parseHeaderHost(char *line, uint16_t lineLen);
	void parseHeaderContentLength(char *line, uint16_t lineLen);
	void parseHeaderContentRange(char *line, uint16_t lineLen);
	void parseHeaderPhpSessionId(char *line, uint16_t lineLen);
};

}

#endif
