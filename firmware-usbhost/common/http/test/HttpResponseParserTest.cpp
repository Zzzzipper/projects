#include "test/include/Test.h"
#include "http/HttpResponseParser.h"
#include "http/include/HttpClient.h"
#include "logger/include/Logger.h"

/*
HTTP/1.1 200 OK
Date: Mon, 13 May 2019 14:28:14 GMT
Server: Apache/2.4.29 (Ubuntu)
Set-Cookie: PHPSESSID=fictlbkntgk1opt8sonvr397q4; expires=Mon, 13-May-2019 16:28:15 GMT; Max-Age=7200; path=/
Expires: Thu, 19 Nov 1981 08:52:00 GMT
Cache-Control: no-store, no-cache, must-revalidate
Pragma: no-cache
Set-Cookie: PHPSESSID=fictlbkntgk1opt8sonvr397q4; expires=Mon, 13-May-2019 16:28:15 GMT; Max-Age=7200; path=/
Vary: Accept-Encoding
Content-Length: 275
Content-Type: text/html; charset=UTF-8

{"success":false,"code":2,"message":"\\u0412 \\u0434\\u043e\\u0441\\u0442\\u0443\\u043f\\u0435 \\u043e\\u0442\\u043a\\u0430\\u0437\\u0430\\u043d\\u043e. \\u041d\\u0435\\u0432\\u0435\\u0440\\u043d\\u044b\\u0439 \\u043b\\u043e\\u0433\\u0438\\u04

3d \\u0438\\u043b\\u0438 \\u043f\\u0430\\u0440\\u
 */

namespace Http {

class ResponseParserTest : public TestSet {
public:
	ResponseParserTest();
	bool testOK();
	bool testBrokenHeader();
	bool testCuttedData();
	bool testWithoutContentLength();
	bool test256Error();
};

TEST_SET_REGISTER(Http::ResponseParserTest);

ResponseParserTest::ResponseParserTest() {
	TEST_CASE_REGISTER(ResponseParserTest, testOK);
	TEST_CASE_REGISTER(ResponseParserTest, testBrokenHeader);
	TEST_CASE_REGISTER(ResponseParserTest, testCuttedData);
	TEST_CASE_REGISTER(ResponseParserTest, testWithoutContentLength);
	TEST_CASE_REGISTER(ResponseParserTest, test256Error);
}

/*
=== Login =========================
HTTP/1.1 200 OK
Server: nginx/1.9.5
Date: Sun, 24 Jan 2016 10:00:04 GMT
Content-Type: text/html
Content-Length: 16
Connection: keep-alive
Keep-Alive: timeout=30
X-Powered-By: PHP/5.5.31
Expires: Thu, 19 Nov 1981 08:52:00 GMT
Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0
Pragma: no-cache
Set-Cookie: PHPSESSID=8a4bb025730a7f81fec32d9358c0e005; expires=Sun, 24-Jan-2016 12:00:04 GMT; Max-Age=7200; path=/
=== Logout ========================
HTTP/1.1 200 OK
Server: nginx/1.9.5
Date: Sun, 24 Jan 2016 09:59:00 GMT
Content-Type: text/html
Content-Length: 35
Connection: keep-alive
Keep-Alive: timeout=30
X-Powered-By: PHP/5.5.31
Expires: Thu, 19 Nov 1981 08:52:00 GMT
Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0
Pragma: no-cache
Set-Cookie: PHPSESSID=8a4bb025730a7f81fec32d9358c0e005; expires=Sun, 24-Jan-2016 11:59:00 GMT; Max-Age=7200; path=/
*/
bool ResponseParserTest::testOK() {
	Http::ResponseParser parser;
	Response resp;
	StringBuilder phpSessionId(64, 64);
	StringBuilder respData(1024, 1024);
	resp.phpSessionId = &phpSessionId;
	resp.data = &respData;

	parser.start(&resp);
	TEST_NUMBER_EQUAL(1024, parser.getBufSize());
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
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_OK, resp.statusCode);

	const char *part3 =
		"Set-Cookie: PHPSESSID=8a4bb025730a7f81fec32d9358c0e005; expires=Sun, 24-Jan-2016 12:00:04 GMT; Max-Age=7200; path=/\r\n"
		"\r\n"
		"{\"success\":true}";
	uint16_t part3Len = strlen(part3);
	memcpy(parser.getBuf(), part3, part3Len);
	parser.parseData(part3Len);
	TEST_NUMBER_EQUAL(1008, parser.getBufSize());
	TEST_NUMBER_EQUAL(1, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_OK, resp.statusCode);
	TEST_NUMBER_EQUAL(16, resp.contentLength);
	TEST_NUMBER_EQUAL(0, resp.rangeFrom);
	TEST_NUMBER_EQUAL(256, resp.rangeTo);
	TEST_NUMBER_EQUAL(1024, resp.rangeLength);
	TEST_SUBSTR_EQUAL("8a4bb025730a7f81fec32d9358c0e005", (const char*)phpSessionId.getData(), phpSessionId.getLen());
	TEST_SUBSTR_EQUAL("{\"success\":true}", (const char*)respData.getData(), respData.getLen());
	return true;
}

bool ResponseParserTest::testBrokenHeader() {
	Http::ResponseParser parser;
	Response resp;
	StringBuilder phpSessionId(64, 64);
	StringBuilder respData(1024, 1024);
	resp.phpSessionId = &phpSessionId;
	resp.data = &respData;

	parser.start(&resp);
	TEST_NUMBER_EQUAL(1024, parser.getBufSize());
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
	TEST_NUMBER_EQUAL(1024, parser.getBufSize());
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
	TEST_NUMBER_EQUAL(1024, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_ParserError, resp.statusCode);

	const char *part3 =
		"Set-Cookie: PHPSESSID=8a4bb025730a7f81fec32d9358c0e005; expires=Sun, 24-Jan-2016 12:00:04 GMT; Max-Age=7200; path=/\r\n"
		"\r\n"
		"{\"success\":true}";
	uint16_t part3Len = strlen(part3);
	memcpy(parser.getBuf(), part3, part3Len);
	parser.parseData(part3Len);
	TEST_NUMBER_EQUAL(1024, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_ParserError, resp.statusCode);
	TEST_NUMBER_EQUAL(0, resp.contentLength);
	TEST_NUMBER_EQUAL(0, resp.rangeFrom);
	TEST_NUMBER_EQUAL(0, resp.rangeTo);
	TEST_NUMBER_EQUAL(0, resp.rangeLength);
	TEST_SUBSTR_EQUAL("", (const char*)phpSessionId.getData(), phpSessionId.getLen());

	return true;
}

bool ResponseParserTest::testCuttedData() {
	Http::ResponseParser parser;
	Response resp;
	StringBuilder phpSessionId(64, 64);
	StringBuilder respData(1024, 1024);
	resp.phpSessionId = &phpSessionId;
	resp.data = &respData;

	parser.start(&resp);
	TEST_NUMBER_EQUAL(1024, parser.getBufSize());
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
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_OK, resp.statusCode);

	const char *part3 =
		"Set-Cookie: PHPSESSID=8a4bb025730a7f81fec32d9358c0e005; expires=Sun, 24-Jan-2016 12:00:04 GMT; Max-Age=7200; path=/\r\n"
		"\r\n"
		"{\"succe";
	uint16_t part3Len = strlen(part3);
	memcpy(parser.getBuf(), part3, part3Len);
	parser.parseData(part3Len);
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_OK, resp.statusCode);
	TEST_NUMBER_EQUAL(16, resp.contentLength);
	TEST_NUMBER_EQUAL(0, resp.rangeFrom);
	TEST_NUMBER_EQUAL(256, resp.rangeTo);
	TEST_NUMBER_EQUAL(1024, resp.rangeLength);
	TEST_SUBSTR_EQUAL("8a4bb025730a7f81fec32d9358c0e005", (const char*)phpSessionId.getData(), phpSessionId.getLen());

	return true;
}

bool ResponseParserTest::testWithoutContentLength() {
	Http::ResponseParser parser;
	Response resp;
	StringBuilder phpSessionId(64, 64);
	StringBuilder respData(1024, 1024);
	resp.phpSessionId = &phpSessionId;
	resp.data = &respData;

	parser.start(&resp);
	TEST_NUMBER_EQUAL(1024, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_Unknown, resp.statusCode);

	const char *part1 =
		"HTTP/1.1 200 OK\r\n"
		"Server: nginx/1.10.3\r\n"
		"Date: Thu, 29 Nov 2018 17:06:54 GMT\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Transfer-Encoding: chunked\r\n"
		"Connection: keep-alive\r\n"
		"\r\n";
	uint16_t part1Len = strlen(part1);
	memcpy(parser.getBuf(), part1, part1Len);
	parser.parseData(part1Len);
	TEST_NUMBER_EQUAL(1024, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_OK, resp.statusCode);

	const char *part2 =
		"2c1\r\n"
		"{\"id\":\"8696960491890171543521995\","
		"\"deviceSN\":\"0390580038004444\","
		"\"deviceRN\":\"5892308424002406\","
		"\"fsNumber\":\"9999078900005419\","
		"\"ofdName\":\"ОФД-Я (тест)\","
		"\"ofdWebsite\":\"www.ofd-ya.ru\","
		"\"ofdinn\":\"7728699517\","
		"\"fnsWebsite\":\"www.nalog.ru\","
		"\"companyINN\":\"690209812752\","
		"\"companyName\":\"ИП Войткевич Алексей Олегович (ЭФОР)\","
		"\"documentNumber\":1866,";
	uint16_t part2Len = strlen(part2);
	memcpy(parser.getBuf(), part2, part2Len);
	parser.parseData(part2Len);
	TEST_NUMBER_EQUAL(693, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_OK, resp.statusCode);

	const char *part3 =
		"\"shiftNumber\":264,"
		"\"documentIndex\":5,"
		"\"processedAt\":\"2018-11-29T20:06:00\","
		"\"content\":{"
		"\"type\":1,"
		"\"positions\":[{\"quantity\":1.000,\"price\":1.23,\"tax\":2,\"text\":\"Тесточино0\"}],"
		"\"checkClose\":{\"payments\":[{\"type\":1,\"amount\":1.23}],\"taxationSystem\":0},"
		"\"automatNumber\":\"159\","
		"\"settlementAddress\":\"ул. Афанасия Никитина, 90\","
		"\"settlementPlace\":\"Разработка3\"},"
		"\"change\":0.0,\"fp\":\"3326875305\""
		"}\r\n"
		"0";
	uint16_t part3Len = strlen(part3);
	memcpy(parser.getBuf(), part3, part3Len);
	parser.parseData(part3Len);
	TEST_NUMBER_EQUAL(319, parser.getBufSize());
	TEST_NUMBER_EQUAL(1, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_OK, resp.statusCode);
	TEST_NUMBER_EQUAL(705, resp.contentLength);
	TEST_NUMBER_EQUAL(0, resp.rangeFrom);
	TEST_NUMBER_EQUAL(0, resp.rangeTo);
	TEST_NUMBER_EQUAL(0, resp.rangeLength);
	TEST_SUBSTR_EQUAL("", (const char*)phpSessionId.getData(), phpSessionId.getLen());
	TEST_SUBSTR_EQUAL(
		"{\"id\":\"8696960491890171543521995\","
		"\"deviceSN\":\"0390580038004444\","
		"\"deviceRN\":\"5892308424002406\","
		"\"fsNumber\":\"9999078900005419\","
		"\"ofdName\":\"ОФД-Я (тест)\","
		"\"ofdWebsite\":\"www.ofd-ya.ru\","
		"\"ofdinn\":\"7728699517\","
		"\"fnsWebsite\":\"www.nalog.ru\","
		"\"companyINN\":\"690209812752\","
		"\"companyName\":\"ИП Войткевич Алексей Олегович (ЭФОР)\","
		"\"documentNumber\":1866,"
		"\"shiftNumber\":264,"
		"\"documentIndex\":5,"
		"\"processedAt\":\"2018-11-29T20:06:00\","
		"\"content\":{"
		"\"type\":1,"
		"\"positions\":[{\"quantity\":1.000,\"price\":1.23,\"tax\":2,\"text\":\"Тесточино0\"}],"
		"\"checkClose\":{\"payments\":[{\"type\":1,\"amount\":1.23}],\"taxationSystem\":0},"
		"\"automatNumber\":\"159\","
		"\"settlementAddress\":\"ул. Афанасия Никитина, 90\","
		"\"settlementPlace\":\"Разработка3\"},"
		"\"change\":0.0,\"fp\":\"3326875305\""
		"}\r\n", (const char*)respData.getData(), respData.getLen());
	return true;
}

bool ResponseParserTest::test256Error() {
	Http::ResponseParser parser;
	Response resp;
	StringBuilder phpSessionId(64, 64);
	StringBuilder respData(256, 256);
	resp.phpSessionId = &phpSessionId;
	resp.data = &respData;

	parser.start(&resp);
	TEST_NUMBER_EQUAL(256, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_Unknown, resp.statusCode);

	const char *part1 =
		"HTTP/1.1 200 OK\r\n"
		"Date: Mon, 13 May 2019 14:28:14 GMT\r\n"
		"Server: Apache/2.4.29 (Ubuntu)\r\n"
		"Set-Cookie: PHPSESSID=fictlbkntgk1opt8sonvr397q4; expires=Mon, 13-May-2019 16:28:15 GMT; Max-Age=7200; path=/\r\n"
		"Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n"
		"Cache-Control: no-s";
	uint16_t part1Len = strlen(part1);
	memcpy(parser.getBuf(), part1, part1Len);
	parser.parseData(part1Len);
	TEST_NUMBER_EQUAL(237, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_OK, resp.statusCode);

	const char *part2 =
		"tore, no-cache, must-revalidate\r\n"
		"Pragma: no-cache\r\n"
		"Set-Cookie: PHPSESSID=fictlbkntgk1opt8sonvr397q4; expires=Mon, 13-May-2019 16:28:15 GMT; Max-Age=7200; path=/\r\n"
		"Vary: Accept-Encoding\r\n"
		"Content-Length: 275\r\n"
		"Content-Type: text/html; charse";
	uint16_t part2Len = strlen(part2);
	memcpy(parser.getBuf(), part2, part2Len);
	parser.parseData(part2Len);
	TEST_NUMBER_EQUAL(225, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_OK, resp.statusCode);

	const char *part3 =
		"t=UTF-8\r\n"
		"\r\n"
		"{\"success\":false,\"code\":2,\"message\":\"\\u0412 \\u0434\\u043e\\u0441\\u0442\\u0443\\u043f\\u0435 \\u043e\\u0442\\u043a\\u0430\\u0437\\u0430\\u043d\\u043e. \\u041d\\u0435\\u0432\\u0435\\u0440\\u043d\\u044b\\u0439 \\u043b\\u043e\\u0433\\u0438\\u04";
	uint16_t part3Len = strlen(part3);
	memcpy(parser.getBuf(), part3, part3Len);
	parser.parseData(part3Len);
	TEST_NUMBER_EQUAL(42, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_OK, resp.statusCode);

	const char *part4 =
		"3d \\u0438\\u043b\\u0438 \\u043f\\u0430\\u0440\\u";
	uint16_t part4Len = strlen(part4);
	memcpy(parser.getBuf(), part4, part4Len);
	parser.parseData(part4Len);
	TEST_NUMBER_EQUAL(0, parser.getBufSize());
	TEST_NUMBER_EQUAL(0, parser.isComplete());
	TEST_NUMBER_EQUAL(Http::Response::Status_OK, resp.statusCode);
//Внимание! Данные прочитаны не все, но места в буфере уже нет!
	return true;
}

}
