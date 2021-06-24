#include "test/include/Test.h"
#include "http/HttpRequestParser.h"
#include "logger/include/Logger.h"

namespace Http {

class RequestParserTest : public TestSet {
public:
	RequestParserTest();
	bool testOK();
	bool testBrokenHeader();
	bool testCuttedData();
};

TEST_SET_REGISTER(Http::RequestParserTest);

RequestParserTest::RequestParserTest() {
	TEST_CASE_REGISTER(RequestParserTest, testOK);
#if 0
	TEST_CASE_REGISTER(RequestParserTest, testBrokenHeader);
	TEST_CASE_REGISTER(RequestParserTest, testCuttedData);
#endif
}

/*
=== CreateAutomatModel =========================
POST /api/1.0/automat/Automat.php?action=CreateAutomatModel&_dc=1539090316532 HTTP/1.1
Host: devs.ephor.online
Connection: keep-alive
Content-Length: 24
Origin: https://devs.ephor.online
X-Requested-With: XMLHttpRequest
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Safari/537.36
Content-Type: application/json
Accept: *//*
Referer: https://devs.ephor.online/client/index.html
Accept-Encoding: gzip, deflate, br
Accept-Language: ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7
Cookie: _ym_uid=1500301628417188656; PHPSESSID=au3fdendice73jro126183e38l

{"name":"1234","type":0}
*/
bool RequestParserTest::testOK() {
	Http::RequestParser parser;
	Request2 req;
	StringBuilder serverName(64, 64);
	StringBuilder serverPath(256, 256);
	StringBuilder phpSessionId(64, 64);
	StringBuilder reqData(1024, 1024);
	req.serverName = &serverName;
	req.serverPath = &serverPath;
	req.phpSessionId = &phpSessionId;
	req.data = &reqData;

	parser.start(&req);
	TEST_NUMBER_EQUAL(1024, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
//	TEST_NUMBER_EQUAL(Http::Response::Status_Unknown, req.statusCode);

	const char *part1 =
		"POST /api/1.0/automat/Automat.php?action=CreateAutomatModel&_dc=1539090316532 HTTP/1.1\r\n"
		"Host: devs.ephor.online\r\n"
		"Connection: keep-alive\r\n"
		"Content-Le";
	uint16_t part1Len = strlen(part1);
	memcpy(parser.getBuf(), part1, part1Len);
	parser.parseData(part1Len);
	TEST_NUMBER_EQUAL(1014, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
//	TEST_NUMBER_EQUAL(Http::Response::Status_OK, req.statusCode);

	const char *part2 =
		"ngth: 24\r\n"
		"Origin: https://devs.ephor.online\r\n"
		"X-Requested-With: XMLHttpRequest\r\n"
		"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Safari/537.36\r\n"
		"Content-Type: application/json\r\n"
		"Accept: */*\r\n"
		"Referer: https://devs.ephor.online/client/index.html\r\n"
		"Accept-Encoding: gzip, deflate, br\r\n"
		"Accept-Language: ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7\r\n";
	uint16_t part2Len = strlen(part2);
	memcpy(parser.getBuf(), part2, part2Len);
	parser.parseData(part2Len);
	TEST_NUMBER_EQUAL(1024, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
//	TEST_NUMBER_EQUAL(Http::Response::Status_OK, req.statusCode);

	const char *part3 =
		"Cookie: _ym_uid=1500301628417188656; PHPSESSID=au3fdendice73jro126183e38l\r\n"
		"\r\n"
		"{\"name\":\"1234\",\"type\":0}";
	uint16_t part3Len = strlen(part3);
	memcpy(parser.getBuf(), part3, part3Len);
	parser.parseData(part3Len);
	TEST_NUMBER_EQUAL(1000, parser.getBufSize());
	TEST_NUMBER_EQUAL(1, parser.isComplete());

	TEST_NUMBER_EQUAL(Http::Request::Method_POST, req.method);
	TEST_SUBSTR_EQUAL("devs.ephor.online", serverName.getString(), serverName.getLen());
	TEST_NUMBER_EQUAL(80, req.serverPort);
	TEST_SUBSTR_EQUAL("/api/1.0/automat/Automat.php?action=CreateAutomatModel&_dc=1539090316532", serverPath.getString(), serverPath.getLen());
	TEST_NUMBER_EQUAL(24, req.contentLength);
	TEST_NUMBER_EQUAL(0, req.rangeFrom);
	TEST_NUMBER_EQUAL(0, req.rangeTo);
	TEST_SUBSTR_EQUAL("au3fdendice73jro126183e38l", phpSessionId.getString(), phpSessionId.getLen());
	TEST_SUBSTR_EQUAL("{\"name\":\"1234\",\"type\":0}", reqData.getString(), reqData.getLen());
	return true;
}
#if 0
bool RequestParserTest::testBrokenHeader() {
	Http::RequestParser parser;
	Response resp;
	StringBuilder phpSessionId(64, 64);
	StringBuilder respData(1024, 1024);
	resp.phpSessionId = &phpSessionId;
	resp.data = &respData;

	parser.start(&resp);
	TEST_NUMBER_EQUAL(1024, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.haveData());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_Unknown, resp.statusCode);

	const char *part1 =
		"0123456789ABCDE\r\n"
		"Server: nginx/1.9.5\r\n"
		"Date: Sun, 24 Jan 2016 10:00:04 GMT\r\n"
		"Content-Ty";
	uint16_t part1Len = strlen(part1);
	memcpy(parser.getBuf(), part1, part1Len);
	parser.parseData(part1Len);
	TEST_NUMBER_EQUAL(1014, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.haveData());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_ParserError, resp.statusCode);

	const char *part2 =
		"pe: text/html\r\n"
		"Content-Length: 16\r\n"
		"Content-Range: bytes 0-256/1024\r\n"
		"Connection: keep-alive\r\n"
		"Keep-Alive: timeout=30\r\n"
		"X-Powered-By: PHP/5.5.31\r\n"
		"Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n"
		"Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"
		"Pragma: no-cache\r\n";
	uint16_t part2Len = strlen(part2);
	memcpy(parser.getBuf(), part2, part2Len);
	parser.parseData(part2Len);
	TEST_NUMBER_EQUAL(1014, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.haveData());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_ParserError, resp.statusCode);

	const char *part3 =
		"Set-Cookie: PHPSESSID=8a4bb025730a7f81fec32d9358c0e005; expires=Sun, 24-Jan-2016 12:00:04 GMT; Max-Age=7200; path=/\r\n"
		"\r\n"
		"{\"success\":true}";
	uint16_t part3Len = strlen(part3);
	memcpy(parser.getBuf(), part3, part3Len);
	parser.parseData(part3Len);
	TEST_NUMBER_EQUAL(1014, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.haveData());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_ParserError, resp.statusCode);
	TEST_NUMBER_EQUAL(0, resp.contentLength);
	TEST_NUMBER_EQUAL(0, resp.rangeFrom);
	TEST_NUMBER_EQUAL(0, resp.rangeTo);
	TEST_NUMBER_EQUAL(0, resp.rangeLength);
	TEST_SUBSTR_EQUAL("", (const char*)phpSessionId.getData(), phpSessionId.getLen());

	return true;
}

bool RequestParserTest::testCuttedData() {
	Http::RequestParser parser;
	Response resp;
	StringBuilder phpSessionId(64, 64);
	StringBuilder respData(1024, 1024);
	resp.phpSessionId = &phpSessionId;
	resp.data = &respData;

	parser.start(&resp);
	TEST_NUMBER_EQUAL(1024, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.haveData());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_Unknown, resp.statusCode);

	const char *part1 =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.9.5\r\n"
		"Date: Sun, 24 Jan 2016 10:00:04 GMT\r\n"
		"Content-Ty";
	uint16_t part1Len = strlen(part1);
	memcpy(parser.getBuf(), part1, part1Len);
	parser.parseData(part1Len);
	TEST_NUMBER_EQUAL(1014, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.haveData());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_OK, resp.statusCode);

	const char *part2 =
		"pe: text/html\r\n"
		"Content-Length: 16\r\n"
		"Content-Range: bytes 0-256/1024\r\n"
		"Connection: keep-alive\r\n"
		"Keep-Alive: timeout=30\r\n"
		"X-Powered-By: PHP/5.5.31\r\n"
		"Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n"
		"Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"
		"Pragma: no-cache\r\n";
	uint16_t part2Len = strlen(part2);
	memcpy(parser.getBuf(), part2, part2Len);
	parser.parseData(part2Len);
	TEST_NUMBER_EQUAL(1024, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.haveData());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_OK, resp.statusCode);

	const char *part3 =
		"Set-Cookie: PHPSESSID=8a4bb025730a7f81fec32d9358c0e005; expires=Sun, 24-Jan-2016 12:00:04 GMT; Max-Age=7200; path=/\r\n"
		"\r\n"
		"{\"succe";
	uint16_t part3Len = strlen(part3);
	memcpy(parser.getBuf(), part3, part3Len);
	parser.parseData(part3Len);
	TEST_NUMBER_EQUAL(0, parser.haveData());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_OK, resp.statusCode);
	TEST_NUMBER_EQUAL(16, resp.contentLength);
	TEST_NUMBER_EQUAL(0, resp.rangeFrom);
	TEST_NUMBER_EQUAL(256, resp.rangeTo);
	TEST_NUMBER_EQUAL(1024, resp.rangeLength);
	TEST_SUBSTR_EQUAL("8a4bb025730a7f81fec32d9358c0e005", (const char*)phpSessionId.getData(), phpSessionId.getLen());

	return true;
}
#endif
}
