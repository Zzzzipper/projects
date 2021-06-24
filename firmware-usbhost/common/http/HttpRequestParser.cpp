#include "HttpRequestParser.h"

#include "utils/include/StringParser.h"
#include "utils/include/Buffer.h"
#include "logger/include/Logger.h"

#include "platform/include/platform.h"

#include <string.h>
#include <strings.h>

namespace Http {

void RequestParser::start(Request2 *req) {
	if(req->data == NULL) {
		LOG_ERROR(LOG_HTTP, "Request not inited");
		this->state = State_RecvError;
		return;
	}
	this->req = req;
	this->req->method = Request::Method_Unknown;
	this->req->contentLength = 0;
	this->req->data->clear();
	this->procData = 0;
	this->state = State_RecvHead;
}

void RequestParser::parseData(uint16_t dataLen) {
	LOG_TRACE(LOG_HTTP, "parseData");
	LOG_TRACE_STR(LOG_HTTP, req->data->getData() + procData, dataLen);
	req->data->setLen(procData + dataLen);
	switch(state) {
	case State_RecvHead: parseHead(); return;
	case State_RecvDataTail: parseTail(); return;
	case State_RecvData: parseData(); return;
	default:;
	}
}

uint8_t *RequestParser::getBuf() {
	return (req->data->getData() + procData);
}

uint32_t RequestParser::getBufSize() {
	return (req->data->getSize() - procData);
}

bool RequestParser::isComplete() {
	return (state == State_RecvComplete || state == State_RecvError);
}

void RequestParser::parseHead() {
	LOG_DEBUG(LOG_HTTP, "parse head");
	bool crFlag = false;
	uint8_t *data = req->data->getData();
	uint16_t dataLen = req->data->getLen();
	procData = 0;
	for(uint16_t i = 0; i < dataLen; i++) {
		uint8_t b = data[i];
		if(b == '\r') {
			crFlag = true;
		} else {
			if(crFlag == true && b == '\n') {
				uint16_t headerLen = i - procData - 1;
				if(headerLen == 0) {
					procData = i + 1;
					shiftData();
					if(req->contentLength > procData) {
						LOG_DEBUG(LOG_HTTP, "parse head complete (next tail)");
						state = State_RecvDataTail;
						return;
					} else {
						LOG_DEBUG(LOG_HTTP, "parse head complete");
						req->data->setLen(procData);
						state = State_RecvComplete;
						return;
					}
				}
				parseHeader((char*)(data + procData), headerLen);
				procData = i + 1;
				crFlag = false;
			} else {
				crFlag = false;
			}
		}
	}
	shiftData();
	LOG_DEBUG(LOG_HTTP, "wait head start");
}

void RequestParser::parseTail() {
	LOG_INFO(LOG_HTTP, "parseTail");
	LOG_DEBUG_STR(LOG_HTTP, req->data->getData(), req->data->getLen());
	procData = 0;
	recvData = req->data->getLen();
	if(req->contentLength > recvData) {
		LOG_DEBUG(LOG_HTTP, "wait tail start");
		state = State_RecvData;
		return;
	} else {
		LOG_DEBUG(LOG_HTTP, "parse tail complete");
		state = State_RecvComplete;
		return;
	}
}

void RequestParser::parseData() {
	LOG_INFO(LOG_HTTP, "parseData");
	LOG_DEBUG_STR(LOG_HTTP, req->data->getData(), req->data->getLen());
	recvData += req->data->getLen();
	if(req->contentLength > recvData) {
		state = State_RecvData;
		LOG_DEBUG(LOG_HTTP, "wait data start");
		return;
	} else {
		LOG_DEBUG(LOG_HTTP, "parse data complete");
		state = State_RecvComplete;
		return;
	}
}

void RequestParser::shiftData() {
	uint8_t *data = req->data->getData();
	uint16_t dataLen = req->data->getLen();
	uint16_t tail = dataLen - procData;
	for(uint16_t k = 0; k < tail; k++) {
		data[k] = data[procData + k];
	}
	procData = tail;
}

void RequestParser::parseHeader(char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_HTTP, "parseHeader");
	if(state == State_RecvError) {
		LOG_DEBUG(LOG_HTTP, "Parsing stopped");
		return;
	}

	LOG_DEBUG_STR(LOG_HTTP, line, lineLen);
	if(req->method == Request::Method_Unknown) {
		parseHeaderHead(line, lineLen);
		return;
	} else {
		parseHeaderHost(line, lineLen);
		parseHeaderContentLength(line, lineLen);
		parseHeaderContentRange(line, lineLen);
		parseHeaderPhpSessionId(line, lineLen);
		return;
	}
}
/*
*header << (req->method == Request::Method_GET ? "GET" : "POST") << " " << req->serverPath << " HTTP/1.1\r\n";
*header << "Host: " << req->serverName << "\r\n";
*header << "Content-Type: application/text/plain; charset=windows-1251\r\n";
*header << "Cache-Control: no-cache\r\n";
if(req->keepAlive == true) { *header << "Connection: keep-alive\r\n"; }
 */
void RequestParser::parseHeaderHead(char *line, uint16_t lineLen) {
	char method[8];
	StringParser parser(line, lineLen);
	uint16_t methodLen = parser.getValue(" ", method, sizeof(method));
	if(strncasecmp("GET", method, methodLen) == 0) {
		req->method = Request::Method_GET;
	} else if(strncasecmp("POST", method, methodLen) == 0) {
		req->method = Request::Method_POST;
	} else if(strnicmp("OPTIONS", method, methodLen) == 0) {
		req->method = Request::Method_OPTIONS;
	} else {
		LOG_ERROR(LOG_HTTP, "Wrong method");
		state = State_RecvError;
		return;
	}

	parser.skipSpace();

	if(req->serverPath == NULL) {
		LOG_DEBUG(LOG_HTTP, "ServerPath buffer is NULL");
		return;
	}
	uint16_t uriLen = parser.getValue(" ", (char*)req->serverPath->getData(), req->serverPath->getSize());
	req->serverPath->setLen(uriLen);
	LOG_DEBUG(LOG_HTTP, "Server-Path = " << req->serverPath->getString());
}

void RequestParser::parseHeaderHost(char *line, uint16_t lineLen) {
	if(req->serverName == NULL) {
		LOG_DEBUG(LOG_HTTP, "ServerName buffer is NULL");
		return;
	}

	StringParser parser(line, lineLen);
	if(parser.compareAndSkip("Host:") == false) {
		return;
	}

	parser.skipSpace();

	uint16_t uriLen = parser.getValue(":\r\n", (char*)req->serverName->getData(), req->serverName->getSize());
	req->serverName->setLen(uriLen);

	if(parser.getNumber(&req->serverPort) == false) { req->serverPort = 80; }
	LOG_DEBUG(LOG_HTTP, "Host = " << req->serverName->getString() << ":" << req->serverPort);
}

void RequestParser::parseHeaderContentLength(char *line, uint16_t lineLen) {
	StringParser parser(line, lineLen);
	if(parser.compareAndSkip("Content-Length:") == false) {
		return;
	}
	parser.skipSpace();
	if(parser.getNumber(&req->contentLength) == false) {
		LOG_ERROR(LOG_HTTP, "Wrong Content-Length format");
		state = State_RecvError;
		return;
	}
	LOG_DEBUG(LOG_HTTP, "Content-Length = " << req->contentLength);
}

void RequestParser::parseHeaderContentRange(char *line, uint16_t lineLen) {
	StringParser parser(line, lineLen);
	if(parser.compareAndSkip("Range:") == false) {
		return;
	}

	parser.skipSpace();
	if(parser.compareAndSkip("bytes=") == false) {
		LOG_ERROR(LOG_HTTP, "Wrong Range unit");
		state = State_RecvError;
		return;
	}

	parser.skipSpace();
	if(parser.getNumber(&req->rangeFrom) == false) {
		LOG_ERROR(LOG_HTTP, "Wrong Content-Range from");
		state = State_RecvError;
		return;
	}
	if(parser.compareAndSkip("-") == false) {
		LOG_ERROR(LOG_HTTP, "Wrong Content-Range syntax");
		state = State_RecvError;
		return;
	}
	if(parser.getNumber(&req->rangeTo) == false) {
		LOG_ERROR(LOG_HTTP, "Wrong Content-Range to");
		state = State_RecvError;
		return;
	}
	LOG_DEBUG(LOG_HTTP, "Range = " << req->rangeFrom << "-" << req->rangeTo);
}

//if(req->phpSessionId != NULL) { *header << "Cookie: PHPSESSID=" << req->phpSessionId << ";\r\n"; }
void RequestParser::parseHeaderPhpSessionId(char *line, uint16_t lineLen) {
	if(req->phpSessionId == NULL) {
		LOG_DEBUG(LOG_HTTP, "PhpSessionId buffer is NULL");
		return;
	}

	StringParser parser(line, lineLen);
	if(parser.compareAndSkip("Cookie:") == false) {
		return;
	}

	char name[32];
	while(parser.hasUnparsed() == true) {
		parser.skipSpace();
		uint16_t nameLen = parser.getValue("=", name, sizeof(name));
		if(nameLen == 0) {
			return;
		}
		parser.skipEqual("=");
		if(strnicmp("PHPSESSID", name, nameLen) == 0) {
			uint16_t valueLen = parser.getValue(";", (char*)req->phpSessionId->getData(), req->phpSessionId->getSize());
			req->phpSessionId->setLen(valueLen);
			return;
		} else {
			parser.skipNotEqual(";");
			parser.skipEqual(";");
		}
	}
}

}
