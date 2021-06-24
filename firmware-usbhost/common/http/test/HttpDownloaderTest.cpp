#include "test/include/Test.h"
#include "http/include/HttpDownloader.h"
#include "http/include/TestHttp.h"
#include "dex/test/TestDataParser.h"
#include "timer/include/TimerEngine.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace Http {

class DownloaderTest : public TestSet {
public:
	DownloaderTest();
	bool testDownload404();
	bool testDownload200();
};

TEST_SET_REGISTER(Http::DownloaderTest);

DownloaderTest::DownloaderTest() {
	TEST_CASE_REGISTER(DownloaderTest, testDownload404);
	TEST_CASE_REGISTER(DownloaderTest, testDownload200);
}

bool DownloaderTest::testDownload404() {
	TimerEngine timers;
	TestConnection connection;
	TestDataParser parser;
	Downloader downloader(&timers, &connection);
	TEST_STRING_EQUAL("<setObserver>", connection.getResult());
	connection.clearResult();

	downloader.download("test.ephor.online", 443, "/firmware.bin", &parser);
	TEST_STRING_EQUAL("<request>", connection.getResult());
	TEST_STRING_EQUAL("test.ephor.online", connection.getRequest()->serverName);
	TEST_STRING_EQUAL("/firmware.bin", connection.getRequest()->serverPath);
	TEST_STRING_EQUAL("", connection.getRequest()->data->getString());
	connection.clearResult();

	connection.getResponse()->statusCode = 404;
	connection.getResponse()->data->set("");
	Event event1(Http::Client::Event_RequestComplete);
	downloader.proc(&event1);
	TEST_STRING_EQUAL("<error>", parser.getData());
	return true;
}

bool DownloaderTest::testDownload200() {
	TimerEngine timers;
	TestConnection connection;
	TestDataParser parser;
	Downloader downloader(&timers, &connection);
	TEST_STRING_EQUAL("<setObserver>", connection.getResult());
	connection.clearResult();

	downloader.download("test.ephor.online", 443, "/firmware.bin", &parser);
	TEST_STRING_EQUAL("<request>", connection.getResult());
	TEST_STRING_EQUAL("test.ephor.online", connection.getRequest()->serverName);
	TEST_STRING_EQUAL("/firmware.bin", connection.getRequest()->serverPath);
	TEST_STRING_EQUAL("", connection.getRequest()->data->getString());
	connection.clearResult();
#if 0
	connection.getResponse()->statusCode = 200;
	connection.getResponse()->data->set("");
	Event event1(Http::Client::Event_RequestComplete);
	downloader.proc(&event1);
	TEST_STRING_EQUAL("<error>", parser.getData());
	return true;
#else
	//todo: написать полноценный тест
	return true;
#endif
}

}
