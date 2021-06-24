#if 1
#ifndef COMMON_HTTP_RESPONSEPARSER_H
#define COMMON_HTTP_RESPONSEPARSER_H

#include "http/include/Http.h"

namespace Http {

class ResponseParser {
public:
	void start(Response *resp);
	void parseData(uint16_t dataLen);
	uint8_t *getBuf();
	uint32_t getBufSize();
	bool isComplete();

private:
	enum State {
		State_RecvHead,
		State_RecvLength,
		State_RecvData,
		State_RecvComplete,
		State_RecvError,
	};
	State state;
	Response *resp;
	uint16_t procData;
	uint16_t unprocData;
	uint32_t recvData;
	bool lengthFound;

	void parseHead();
	void shiftData();
	bool procLine(char *line, uint16_t lineLen);
	bool parseHeader(char *line, uint16_t lineLen);
	void parseHeaderHead(char *line, uint16_t lineLen);
	void parseHeaderContentLength(char *line, uint16_t lineLen);
	void parseHeaderContentRange(char *line, uint16_t lineLen);
	void parseHeaderPhpSessionId(char *line, uint16_t lineLen);
	bool parseHeadEnd();
	bool parseLength(char *line, uint16_t lineLen);
	void gotoStateRecvData();
	void stateRecvData();
};

}

#endif
#else
#ifndef COMMON_HTTP_RESPONSEPARSER_H
#define COMMON_HTTP_RESPONSEPARSER_H

#include "http/include/Http.h"

namespace Http {

class ResponseParser {
public:
	void start(Response *resp);
	void parseData(uint16_t dataLen);
	uint8_t *getBuf();
	uint32_t getBufSize();
	bool isComplete();

private:
	enum State {
		State_RecvHead,
		State_RecvDataTail,
		State_RecvData,
		State_RecvLength,
		State_RecvComplete,
		State_RecvError,
	};
	State state;
	Response *resp;
	uint16_t procData;
	uint32_t recvData;
	bool lengthFound;

	void parseHead();
	void shiftData();
	void parseHeadEnd();
	void parseHeader(char *line, uint16_t lineLen);
	void parseHeaderHead(char *line, uint16_t lineLen);
	void parseHeaderContentLength(char *line, uint16_t lineLen);
	void parseHeaderContentRange(char *line, uint16_t lineLen);
	void parseHeaderPhpSessionId(char *line, uint16_t lineLen);
	void parseTail();
	void parseData();
};

}

#endif
#endif
