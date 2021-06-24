#include "test/include/Test.h"
#include "lib/fiscal_register/orange_data/OrangeDataResponseParser.h"
#include "utils/include/Json.h"
#include "logger/include/Logger.h"

namespace OrangeData {

class CheckResponseParserTest : public TestSet {
public:
	CheckResponseParserTest();
	bool testResponse201();
	bool testResponse400();
};

TEST_SET_REGISTER(OrangeData::CheckResponseParserTest);

CheckResponseParserTest::CheckResponseParserTest() {
	TEST_CASE_REGISTER(CheckResponseParserTest, testResponse201);
	TEST_CASE_REGISTER(CheckResponseParserTest, testResponse400);
}

bool CheckResponseParserTest::testResponse201() {
	StringBuilder buf(2048, 2048);
	CheckResponseParser parser(0, &buf);

	const char *data =
"HTTP/1.1 201 Created\r\n"
"Server: nginx/1.12.0\r\n"
"Date: Thu, 16 Aug 2018 14:37:42 GMT\r\n"
"Content-Length: 0\r\n"
"Connection: keep-alive\r\n"
"\r\n";

	parser.start();
	buf.set(data);
	parser.parse(buf.getLen());
	TEST_NUMBER_EQUAL(OrangeData::CheckResponseParser::Result_Ok, parser.getResult());
	return true;
}

bool CheckResponseParserTest::testResponse400() {
	StringBuilder buf(2048, 2048);
	CheckResponseParser parser(0, &buf);

	const char *data =
"HTTP/1.1 400 Bad Request\r\n"
"Server: nginx/1.12.0\r\n"
"Date: Thu, 16 Aug 2018 14:40:07 GMT\r\n"
"Content-Type: application/json; charset=utf-8\r\n"
"Transfer-Encoding: chunked\r\n"
"Connection: keep-alive\r\n"
"\r\n"
"aa\r\n"
"{\"errors\":["
"\"Длина значения в поле 'SettlementAddress' должна быть от 1 до 243, получено 0\","
"\"Длина значения в поле 'SettlementPlace' должна быть от 1 до 243, получено 0\""
"]}"
"0";
	parser.start();
	buf.set(data);
	parser.parse(buf.getLen());
	TEST_NUMBER_EQUAL(OrangeData::CheckResponseParser::Result_Error, parser.getResult());
	Fiscal::EventError *error = parser.getError();
	TEST_NUMBER_EQUAL(ConfigEvent::Type_FiscalUnknownError, error->code);
	TEST_STRING_EQUAL("400*Длина значения в поле 'SettlementAddress' долж", error->data.getString());
	return true;
}

}
