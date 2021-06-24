#include "test/include/Test.h"
#include "lib/fiscal_register/orange_data/OrangeDataResponseParser.h"
#include "utils/include/Json.h"
#include "logger/include/Logger.h"

namespace OrangeData {

class PollResponseParserTest : public TestSet {
public:
	PollResponseParserTest();
	bool testResponse202();
	bool testResponse200();
};

TEST_SET_REGISTER(OrangeData::PollResponseParserTest);

PollResponseParserTest::PollResponseParserTest() {
	TEST_CASE_REGISTER(PollResponseParserTest, testResponse202);
	TEST_CASE_REGISTER(PollResponseParserTest, testResponse200);
}

bool PollResponseParserTest::testResponse202() {
	StringBuilder buf(2048, 2048);
	PollResponseParser parser(0, &buf);

	const char *data =
"HTTP/1.1 202 Accepted\r\n"
"Server: nginx/1.10.3\r\n"
"Date: Thu, 29 Nov 2018 16:06:12 GMT\r\n"
"Content-Length: 0\r\n"
"Connection: keep-alive\r\n"
"\r\n";

	parser.start();
	buf.set(data);
	parser.parse(buf.getLen());
	TEST_NUMBER_EQUAL(OrangeData::PollResponseParser::Result_Busy, parser.getResult());
	TEST_NUMBER_EQUAL(0, parser.getFiscalRegister());
	TEST_NUMBER_EQUAL(0, parser.getFiscalStorage());
	TEST_NUMBER_EQUAL(0, parser.getFiscalDocument());
	TEST_NUMBER_EQUAL(0, parser.getFiscalSign());
	return true;
}

bool PollResponseParserTest::testResponse200() {
	StringBuilder buf(2048, 2048);
	PollResponseParser parser(0, &buf);

	const char *data =
"HTTP/1.1 200 OK\r\n"
"Server: nginx/1.10.3\r\n"
"Date: Thu, 29 Nov 2018 17:06:54 GMT\r\n"
"Content-Type: application/json; charset=utf-8\r\n"
"Transfer-Encoding: chunked\r\n"
"Connection: keep-alive\r\n"
"\r\n"
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
"}\r\n"
"0";

	parser.start();
	buf.set(data);
	parser.parse(buf.getLen());
	TEST_NUMBER_EQUAL(OrangeData::PollResponseParser::Result_Ok, parser.getResult());
	DateTime *fiscalDatetime = parser.getFiscalDatetime();
	TEST_NUMBER_EQUAL(2018, fiscalDatetime->getYear());
	TEST_NUMBER_EQUAL(11, fiscalDatetime->month);
	TEST_NUMBER_EQUAL(29, fiscalDatetime->day);
	TEST_NUMBER_EQUAL(20, fiscalDatetime->hour);
	TEST_NUMBER_EQUAL(6, fiscalDatetime->minute);
	TEST_NUMBER_EQUAL(0, fiscalDatetime->second);
	TEST_NUMBER_EQUAL(5892308424002406, parser.getFiscalRegister());
	TEST_NUMBER_EQUAL(9999078900005419, parser.getFiscalStorage());
	TEST_NUMBER_EQUAL(1866, parser.getFiscalDocument());
	TEST_NUMBER_EQUAL(3326875305, parser.getFiscalSign());
	return true;
}

}
