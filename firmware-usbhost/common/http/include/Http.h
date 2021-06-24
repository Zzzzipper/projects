#ifndef COMMON_HTTP_H
#define COMMON_HTTP_H

#include "utils/include/Event.h"
#include "utils/include/StringBuilder.h"

namespace Http {

enum ContentType {
	ContentType_Json = 1,
};

class Request {
public:
	enum Method {
		Method_Unknown = 0,
		Method_GET,
		Method_POST,
		Method_OPTIONS,
	};
	Method method;
	const char *serverName;
	uint16_t serverPort;
	const char *serverPath;
	bool keepAlive;
	uint16_t contentType;
	uint32_t contentLength;
	const char *phpSessionId;
	uint32_t rangeFrom;
	uint32_t rangeTo;
	StringBuilder *data;

	Request();
};

class Request2 {
public:
	Request::Method method;
	StringBuilder *serverName;
	uint16_t serverPort;
	StringBuilder *serverPath;
	bool keepAlive;
	uint16_t contentType;
	uint32_t contentLength;
	StringBuilder *phpSessionId;
	uint32_t rangeFrom;
	uint32_t rangeTo;
	StringBuilder *data;

	Request2();
};

enum Status {
	Status_Unknown				= 0,
	Status_OK					= 200,
	Status_Created				= 201,
	Status_PartialContent		= 206,
	Status_BadRequest			= 400,
	Status_Unauthorized			= 401,
	Status_NotFound				= 404,
	Status_ServerError			= 500,
	Status_ServiceUnavailable	= 503,
};

class Response {
public:
	enum Status {
		Status_Unknown				= 0,
		Status_ParserError			= 1,
		Status_OK					= 200,
		Status_Created				= 201,
		Status_Accepted				= 202,
		Status_PartialContent		= 206,
		Status_BadRequest			= 400,
		Status_Unauthorized			= 401,
		Status_Conflict				= 409,
		Status_ServerError			= 500,
		Status_ServiceUnavailable	= 503,
	};
	uint16_t statusCode;
	uint16_t contentType;
	uint32_t contentLength;
	StringBuilder *phpSessionId;
	uint32_t rangeFrom;
	uint32_t rangeTo;
	uint32_t rangeLength;
	StringBuilder *data;

	Response();
};

class ClientInterface {
public:
	enum EventType {
		Event_RequestComplete	= GlobalId_HttpClient | 0x01,
		Event_RequestError		= GlobalId_HttpClient | 0x02,
	};

	virtual ~ClientInterface() {}
	virtual void setObserver(EventObserver *observer) = 0;
	virtual bool sendRequest(const Request *req, Response *resp) = 0;
	virtual bool close() = 0;
};

class ServerInterface {
public:
	enum EventType {
		Event_RequestIncoming	= GlobalId_HttpClient | 0x03,
		Event_ResponseComplete	= GlobalId_HttpClient | 0x04,
		Event_ResponseError		= GlobalId_HttpClient | 0x05,
	};

	virtual ~ServerInterface() {}
	virtual void setObserver(EventObserver *observer) = 0;
	virtual bool accept(Request2 *req) = 0;
	virtual bool sendResponse(const Response *resp) = 0;
	virtual bool close() = 0;
};

extern const char *statusCodeToString(int code);
extern const char *contentTypeToString(int code);

}

#endif
