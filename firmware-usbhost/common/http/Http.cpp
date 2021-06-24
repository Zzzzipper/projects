#include "include/Http.h"

namespace Http {

Request::Request() :
	serverName(NULL),
	serverPort(80),
	serverPath(NULL),
	keepAlive(false),
	contentType(ContentType_Json),
	contentLength(0),
	phpSessionId(NULL),
	rangeFrom(0),
	rangeTo(0),
	data(NULL)
{}

Request2::Request2() :
	serverName(NULL),
	serverPort(80),
	serverPath(NULL),
	keepAlive(false),
	contentType(ContentType_Json),
	contentLength(0),
	phpSessionId(NULL),
	rangeFrom(0),
	rangeTo(0),
	data(NULL)
{}

Response::Response() :
	statusCode(Status_OK),
	contentType(ContentType_Json),
	contentLength(0),
	phpSessionId(NULL),
	rangeFrom(0),
	rangeTo(0),
	rangeLength(0),
	data(NULL)
{}

const char *statusCodeToString(int code) {
	switch(code) {
	case Status_OK: return "OK";
	case Status_Created: return "Created";
	case Status_PartialContent: return "Partial Content";
	case Status_BadRequest: return "Bad Request";
	case Status_Unauthorized: return "Unauthorized";
	case Status_NotFound: return "Not Found";
	case Status_ServerError: return "Internal Server Error";
	case Status_ServiceUnavailable: return "Service Unavailable";
	default: return "Unknown";
	}
}

const char *contentTypeToString(int type) {
	switch(type) {
	case ContentType_Json: return "application/json";
	default: return "application/text/plain";
	}
}


}
