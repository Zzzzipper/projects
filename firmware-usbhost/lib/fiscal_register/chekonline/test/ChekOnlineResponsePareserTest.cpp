#include "test/include/Test.h"
#include "lib/fiscal_register/chekonline/ChekOnlineResponseParser.h"
#include "utils/include/Json.h"
#include "logger/include/Logger.h"

namespace ChekOnline {

class ResponseParserTest : public TestSet {
public:
	ResponseParserTest();
	bool testOK();
	bool testSyntaxError();
	bool testWrongPassword();
};

TEST_SET_REGISTER(ChekOnline::ResponseParserTest);

ResponseParserTest::ResponseParserTest() {
	TEST_CASE_REGISTER(ResponseParserTest, testOK);
	TEST_CASE_REGISTER(ResponseParserTest, testSyntaxError);
	TEST_CASE_REGISTER(ResponseParserTest, testWrongPassword);
}

bool ResponseParserTest::testOK() {
	StringBuilder buf(2048, 2048);
	EventDeviceId deviceId(1);
	ResponseParser parser(deviceId, &buf);

	const char *data =
"HTTP/1.1 200 OK\r\n"
"Server: nginx/1.12.0\r\n"
"Date: Sun, 19 Aug 2018 13:43:42 GMT\r\n"
"Content-Type: application/json; charset=utf-8\r\n"
"Content-Length: 594\r\n"
"Connection: keep-alive\r\n"
"X-Device-Address: 192.168.142.20:4051\r\n"
"X-Device-Name: 10000000000000000051\r\n"
"\r\n"
"{\"ClientId\":\"690209812752\",\"Date\":{\"Date\":{\"Day\":19,\"Month\":8,\"Year\":18},\"Time\":{\"Hour\":16,\"Minute\":44,\"Second\":43}}"
",\"Device\":{\"Name\":\"10000000000000000051\",\"Address\":\"192.168.142.20:4051\"}"
",\"DeviceRegistrationNumber\":\"2505480089018141\""
",\"DeviceSerialNumber\":\"10000000000000000051\""
",\"DocNumber\":2"
",\"DocumentType\":0"
",\"FNSerialNumber\":\"9999999999999051\""
",\"FiscalDocNumber\":4"
",\"FiscalSign\":259844891"
",\"GrandTotal\":123"
",\"Path\":\"/fr/api/v2/Complex\""
",\"QR\":\"t=20180819T1644\\u0026s=1.23\\u0026fn=9999999999999051\\u0026i=4\\u0026fp=259844891\\u0026n=1\""
",\"RequestId\":\"8683450318545401534697027\""
",\"Response\":{\"Error\":0}}";

	parser.start();
	buf.set(data);
	parser.parse(buf.getLen());
	TEST_NUMBER_EQUAL(ChekOnline::ResponseParser::Result_Ok, parser.getResult());
	TEST_NUMBER_EQUAL(2505480089018141, parser.getFiscalRegister());
	TEST_NUMBER_EQUAL(9999999999999051, parser.getFiscalStorage());
	TEST_NUMBER_EQUAL(4, parser.getFiscalDocument());
	TEST_NUMBER_EQUAL(259844891, parser.getFiscalSign());
	return true;
}

bool ResponseParserTest::testSyntaxError() {
	StringBuilder buf(2048, 2048);
	EventDeviceId deviceId(1);
	ResponseParser parser(deviceId, &buf);

	const char *data =
"HTTP/1.1 500 Internal Server Error\r\n"
"Server: nginx/1.12.0\r\n"
"Date: Mon, 20 Aug 2018 09:22:59 GMT\r\n"
"Content-Type: application/json; charset=utf-8\r\n"
"Content-Length: 99\r\n"
"Connection: keep-alive\r\n"
"X-Fce-Error: 2\r\n"
"\r\n"
"{\"FCEError\":2,\"ErrorDescription\":\"invalid character '\\\"' after object key:value pair\",\"Fatal\":true}";

	parser.start();
	buf.set(data);
	parser.parse(buf.getLen());
	TEST_NUMBER_EQUAL(ChekOnline::ResponseParser::Result_Error, parser.getResult());
	Fiscal::EventError *error = parser.getError();
	TEST_NUMBER_EQUAL(ConfigEvent::Type_FiscalUnknownError, error->code);
	TEST_STRING_EQUAL("500*2", error->data.getString());
	TEST_NUMBER_EQUAL(0, parser.getFiscalRegister());
	TEST_NUMBER_EQUAL(0, parser.getFiscalStorage());
	TEST_NUMBER_EQUAL(0, parser.getFiscalDocument());
	TEST_NUMBER_EQUAL(0, parser.getFiscalSign());
	return true;
}

bool ResponseParserTest::testWrongPassword() {
	StringBuilder buf(2048, 2048);
	EventDeviceId deviceId(1);
	ResponseParser parser(deviceId, &buf);

	const char *data =
"HTTP/1.1 200 OK\r\n"
"Server: nginx/1.12.0\r\n"
"Date: Sun, 19 Aug 2018 17:40:52 GMT\r\n"
"Content-Type: application/json; charset=utf-8\r\n"
"Content-Length: 420\r\n"
"Connection: keep-alive\r\n"
"X-Device-Address: 192.168.142.20:4051\r\n"
"X-Device-Name: 10000000000000000051\r\n"
"\r\n"
"{\"ClientId\":\"690209812752\""
",\"Date\":{\"Date\":{\"Day\":0,\"Month\":0,\"Year\":0},\"Time\":{\"Hour\":0,\"Minute\":0,\"Second\":0}}"
",\"Device\":{\"Name\":\"10000000000000000051\",\"Address\":\"192.168.142.20:4051\"}"
",\"DeviceRegistrationNumber\":\"2505480089018141\""
",\"DeviceSerialNumber\":\"10000000000000000051\""
",\"DocumentType\":0"
",\"FNSerialNumber\":\"9999999999999051\""
",\"Path\":\"/fr/api/v2/Complex\""
",\"RequestId\":\"8683450318545401534711257\""
",\"Response\":{\"Error\":79}}";

	parser.start();
	buf.set(data);
	parser.parse(buf.getLen());
	TEST_NUMBER_EQUAL(ChekOnline::ResponseParser::Result_Error, parser.getResult());
	Fiscal::EventError *error = parser.getError();
	TEST_NUMBER_EQUAL(ConfigEvent::Type_FiscalUnknownError, error->code);
	TEST_STRING_EQUAL("200*79", error->data.getString());
	TEST_NUMBER_EQUAL(0, parser.getFiscalRegister());
	TEST_NUMBER_EQUAL(0, parser.getFiscalStorage());
	TEST_NUMBER_EQUAL(0, parser.getFiscalDocument());
	TEST_NUMBER_EQUAL(0, parser.getFiscalSign());
	return true;
}

}
