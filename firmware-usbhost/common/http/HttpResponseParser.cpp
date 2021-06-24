#include "HttpResponseParser.h"

#include "utils/include/StringParser.h"
#include "utils/include/Buffer.h"
#include "utils/include/Hex.h"
#include "logger/include/Logger.h"

namespace Http {

void ResponseParser::start(Response *resp) {
	this->resp = resp;
	this->resp->statusCode = Response::Status_Unknown;
	this->resp->contentLength = 0;
	this->resp->data->clear();
	this->procData = 0;
	this->recvData = 0;
	this->lengthFound = false;
	this->unprocData = 0;
	this->state = State_RecvHead;
}

void ResponseParser::parseData(uint16_t dataLen) {
	LOG_TRACE(LOG_HTTP, "parseData");
	LOG_TRACE_STR(LOG_HTTP, resp->data->getData() + unprocData, dataLen);
	resp->data->setLen(unprocData + dataLen);
	if(state == State_RecvHead || state == State_RecvLength) {
		parseHead();
	}
	if(state == State_RecvData) {
		stateRecvData();
	}
	if(state == State_RecvError) {
		unprocData = 0;
	} else {
		unprocData = resp->data->getLen();
	}
}

uint8_t *ResponseParser::getBuf() {
	return (resp->data->getData() + unprocData);
}

uint32_t ResponseParser::getBufSize() {
	return (resp->data->getSize() - unprocData);
}

bool ResponseParser::isComplete() {
	return state == State_RecvComplete;
}

void ResponseParser::parseHead() {
	LOG_DEBUG(LOG_HTTP, "parse head");
	bool crFlag = false;
	uint8_t *data = resp->data->getData();
	uint16_t dataLen = resp->data->getLen();
	procData = 0;
	for(uint16_t i = 0; i < dataLen; i++) {
		uint8_t b = data[i];
		if(b == '\r') {
			crFlag = true;
		} else {
			if(crFlag == true && b == '\n') {
				uint8_t *lineStart = data + procData;
				uint16_t lineLen = i - procData - 1;
				procData = i + 1;
				if(procLine((char*)lineStart, lineLen) == false) { break; }
				crFlag = false;
			} else {
				crFlag = false;
			}
		}
	}
	shiftData();
}

void ResponseParser::shiftData() {
	LOG_DEBUG(LOG_HTTP, "shiftData");
	uint8_t *data = resp->data->getData();
	uint16_t dataLen = resp->data->getLen();
	uint16_t tail = dataLen - procData;
	for(uint16_t k = 0; k < tail; k++) {
		data[k] = data[procData + k];
	}
	procData = 0;
	resp->data->setLen(tail);
	LOG_DEBUG_STR(LOG_HTTP, (char*)data, procData);
}

bool ResponseParser::procLine(char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_HTTP, "procLine");
	LOG_DEBUG_STR(LOG_HTTP, line, lineLen);
	switch(state) {
	case State_RecvHead: return parseHeader(line, lineLen);
	case State_RecvLength: return parseLength(line, lineLen);
	case State_RecvError: return false;
	default: return false;
	}
}

bool ResponseParser::parseHeader(char *line, uint16_t lineLen) {
	LOG_DEBUG(LOG_HTTP, "parseHeader");
	if(lineLen == 0) {
		return parseHeadEnd();
	}

	if(resp->statusCode == Response::Status_Unknown) {
		parseHeaderHead(line, lineLen);
		return true;
	} else {
		parseHeaderContentLength(line, lineLen);
		parseHeaderContentRange(line, lineLen);
		parseHeaderPhpSessionId(line, lineLen);
		return true;
	}
}

void ResponseParser::parseHeaderHead(char *line, uint16_t lineLen) {
	StringParser parser(line, lineLen);
	parser.skipWord();
	parser.skipSpace();
	if(parser.getNumber(&resp->statusCode) == false) {
		LOG_ERROR(LOG_HTTP, "Wrong Head format");
		resp->statusCode = Response::Status_ParserError;
		state = State_RecvError;
		return;
	}
}

void ResponseParser::parseHeaderContentLength(char *line, uint16_t lineLen) {
	StringParser parser(line, lineLen);
	if(parser.compareAndSkip("Content-Length:") == false) {
		return;
	}
	parser.skipSpace();
	if(parser.getNumber(&resp->contentLength) == false) {
		LOG_ERROR(LOG_HTTP, "Wrong Content-Length format");
		resp->statusCode = Response::Status_ParserError;
		state = State_RecvError;
		return;
	}
	lengthFound = true;
	LOG_DEBUG(LOG_HTTP, "Content-Length = " << resp->contentLength);
}

void ResponseParser::parseHeaderContentRange(char *line, uint16_t lineLen) {
	StringParser parser(line, lineLen);
	if(parser.compareAndSkip("Content-Range:") == false) {
		return;
	}

	parser.skipSpace();
	if(parser.compareAndSkip("bytes") == false) {
		LOG_ERROR(LOG_HTTP, "Wrong Content-Range unit");
		resp->statusCode = Response::Status_ParserError;
		state = State_RecvError;
		return;
	}

	parser.skipSpace();
	if(parser.getNumber(&resp->rangeFrom) == false) {
		LOG_ERROR(LOG_HTTP, "Wrong Content-Range from");
		resp->statusCode = Response::Status_ParserError;
		state = State_RecvError;
		return;
	}
	if(parser.compareAndSkip("-") == false) {
		LOG_ERROR(LOG_HTTP, "Wrong Content-Range syntax");
		resp->statusCode = Response::Status_ParserError;
		state = State_RecvError;
		return;
	}
	if(parser.getNumber(&resp->rangeTo) == false) {
		LOG_ERROR(LOG_HTTP, "Wrong Content-Range to");
		resp->statusCode = Response::Status_ParserError;
		state = State_RecvError;
		return;
	}
	if(parser.compareAndSkip("/") == false) {
		LOG_ERROR(LOG_HTTP, "Wrong Content-Range syntax");
		resp->statusCode = Response::Status_ParserError;
		state = State_RecvError;
		return;
	}
	if(parser.getNumber(&resp->rangeLength) == false) {
		LOG_ERROR(LOG_HTTP, "Wrong Content-Range size");
		resp->statusCode = Response::Status_ParserError;
		state = State_RecvError;
		return;
	}
	LOG_DEBUG(LOG_HTTP, "Content-Range = " << resp->rangeFrom << "-" << resp->rangeTo << "/" << resp->rangeLength);
}

void ResponseParser::parseHeaderPhpSessionId(char *line, uint16_t lineLen) {
	if(resp->phpSessionId == NULL) {
		return;
	}

	StringParser parser(line, lineLen);
	if(parser.compareAndSkip("Set-Cookie:") == false) {
		return;
	}

	parser.skipSpace();

	if(parser.compareAndSkip("PHPSESSID=") == false) {
		return;
	}

	uint16_t len = parser.getValue(";\r\n", (char*)resp->phpSessionId->getData(), resp->phpSessionId->getSize());
	resp->phpSessionId->setLen(len);
	LOG_DEBUG(LOG_HTTP, "PHPSESSID = " << resp->phpSessionId->getLen());
}

bool ResponseParser::parseHeadEnd() {
	LOG_INFO(LOG_HTTP, "parseHeadEnd");
	if(lengthFound == false) {
		LOG_DEBUG(LOG_HTTP, "parse head complete (next length)");
		state = State_RecvLength;
		return true;
	} else {
		gotoStateRecvData();
		return false;
	}
}

bool ResponseParser::parseLength(char *line, uint16_t lineLen) {
	resp->contentLength = hexToNumber(line, lineLen);
	if(resp->contentLength == 0) {
		LOG_ERROR(LOG_HTTP, "Wrong content-length to");
		resp->statusCode = Response::Status_ParserError;
		state = State_RecvError;
		return false;
	}
	LOG_INFO(LOG_HTTP, "parseLength " << resp->contentLength);
	state = State_RecvData;
	return false;
}

void ResponseParser::gotoStateRecvData() {
	LOG_INFO(LOG_HTTP, "gotoStateRecvData");
	recvData = 0;
	state = State_RecvData;
}

void ResponseParser::stateRecvData() {
	LOG_INFO(LOG_HTTP, "stateRecvData");
	recvData += resp->data->getLen() - procData;
	procData = recvData;
	if(resp->contentLength > recvData) {
		state = State_RecvData;
		LOG_DEBUG(LOG_HTTP, "wait data " << recvData << "/" << resp->contentLength);
		return;
	} else {
		LOG_DEBUG(LOG_HTTP, "parse data complete");
		resp->data->setLen(resp->contentLength);
		state = State_RecvComplete;
		return;
	}
}

}
