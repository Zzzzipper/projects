#include "lib/erp/ErpProtocol.h"

#include "common/test/include/Test.h"
#include "common/memory/include/RamMemory.h"
#include "common/config/ConfigModem.h"
#include "common/http/include/TestHttp.h"
#include "common/timer/include/TestRealTime.h"
#include "common/utils/include/TestEventObserver.h"
#include "common/logger/include/Logger.h"

class ErpProtocolTest : public TestSet {
public:
	ErpProtocolTest();
	void init();
	void cleanup();
	bool testEventRepeat();
	bool testEventOverflow();

private:
	StringBuilder *result;
	RamMemory *memory;
	TestRealTime *realtime;
	StatStorage *stat;
	ConfigModem *configModem;
	Http::TestConnection *http;
	TestEventObserver *observer;
	ErpProtocol *erp;

	void addSaleEvent(const char *id, const char *name, uint32_t price, ConfigEventList *events);
};

TEST_SET_REGISTER(ErpProtocolTest);

ErpProtocolTest::ErpProtocolTest() {
	TEST_CASE_REGISTER(ErpProtocolTest, testEventRepeat);
	TEST_CASE_REGISTER(ErpProtocolTest, testEventOverflow);
}

void ErpProtocolTest::init() {
	result = new StringBuilder(1024, 1024);
	memory = new RamMemory(32000);
	realtime = new TestRealTime;
	stat = new StatStorage;
	configModem = new ConfigModem(memory, realtime, stat);
	configModem->getBoot()->setHardwareVersion(0x01000002);
	configModem->getBoot()->setFirmwareVersion(0x01020003);
	configModem->getBoot()->setServerDomain("erp.ephor.online");
	configModem->getBoot()->setServerPort(443);
	configModem->getBoot()->setServerPassword("password");
	http = new Http::TestConnection;
	observer = new TestEventObserver(result);
	erp = new ErpProtocol();
	erp->init(configModem, http, realtime);
	erp->setObserver(observer);
}

void ErpProtocolTest::cleanup() {
	delete erp;
	delete observer;
	delete http;
	delete configModem;
	delete stat;
	delete realtime;
	delete memory;
	delete result;
}

void ErpProtocolTest::addSaleEvent(const char *id, const char *name, uint32_t price, ConfigEventList *events) {
	ConfigEventSale sale1;
	sale1.selectId.set(id);
	sale1.wareId = 0;
	sale1.name.set(name);
	sale1.device.set("CA");
	sale1.priceList = 0;
	sale1.price = price;
	sale1.taxSystem = 0;
	sale1.taxRate = 0;
	sale1.taxValue = 0;
	sale1.fiscalRegister = 11111;
	sale1.fiscalStorage = 22222;
	sale1.fiscalDocument = 33333;
	sale1.fiscalSign = 44444;
	events->add(&sale1);
}

bool ErpProtocolTest::testEventRepeat() {
	RamMemory memory(32000);
	TestRealTime realtime;
	StringBuilder buf(4096, 4096);

	ConfigEventList events(&realtime);
	events.init(5, &memory);

	// Two events
	addSaleEvent("1", "name1", 10, &events);
	addSaleEvent("2", "name2", 20, &events);
	TEST_NUMBER_EQUAL(true, erp->sendSyncRequest("root", 0, 0, &events, &buf));
	TEST_STRING_EQUAL("{\"hv\":\"1.0.2\",\"sv\":\"1.2.3\",\"release\":1,\"sq\":0,"
		"\"devices\":["
		"{\"type\":2,\"state\":0,\"params\":[{\"t\":1,\"v\":\"\"},{\"t\":2,\"v\":\"\"},{\"t\":3,\"v\":\"\"},{\"t\":4,\"v\":0},{\"t\":32768,\"v\":0},{\"t\":32770,\"v\":0}],\"events\":[]}],"
		"\"events\":["
		"{\"date\":\"2000-01-02 10:20:30\",\"type\":3,\"select_id\":\"1\",\"name\":\"name1\",\"payment_device\":\"CA\",\"price_list\":0,\"value\":1000,\"tax_system\":0,\"tax_rate\":0,\"tax_value\":0,\"fn\":22222,\"fd\":33333,\"fp\":44444},"
		"{\"date\":\"2000-01-02 10:20:30\",\"type\":3,\"select_id\":\"2\",\"name\":\"name2\",\"payment_device\":\"CA\",\"price_list\":0,\"value\":2000,\"tax_system\":0,\"tax_rate\":0,\"tax_value\":0,\"fn\":22222,\"fd\":33333,\"fp\":44444}"
		"]}", http->getRequest()->data->getString());
	Event event1(Http::Connection::Event_RequestError);
	erp->proc(&event1);

	// Repeat events
	TEST_NUMBER_EQUAL(true, erp->sendSyncRequest("root", 0, 0, &events, &buf));
	TEST_STRING_EQUAL("{\"hv\":\"1.0.2\",\"sv\":\"1.2.3\",\"release\":1,\"sq\":0,"
		"\"devices\":["
		"{\"type\":2,\"state\":0,\"params\":[{\"t\":1,\"v\":\"\"},{\"t\":2,\"v\":\"\"},{\"t\":3,\"v\":\"\"},{\"t\":4,\"v\":0},{\"t\":32768,\"v\":0},{\"t\":32770,\"v\":0}],\"events\":[]}],"
		"\"events\":["
		"{\"date\":\"2000-01-02 10:20:30\",\"type\":3,\"select_id\":\"1\",\"name\":\"name1\",\"payment_device\":\"CA\",\"price_list\":0,\"value\":1000,\"tax_system\":0,\"tax_rate\":0,\"tax_value\":0,\"fn\":22222,\"fd\":33333,\"fp\":44444},"
		"{\"date\":\"2000-01-02 10:20:30\",\"type\":3,\"select_id\":\"2\",\"name\":\"name2\",\"payment_device\":\"CA\",\"price_list\":0,\"value\":2000,\"tax_system\":0,\"tax_rate\":0,\"tax_value\":0,\"fn\":22222,\"fd\":33333,\"fp\":44444}"
		"]}", http->getRequest()->data->getString());
	http->getResponse()->data->set("{\"success\":true,\"data\":[{\"sv\":\"0.0.0\",\"date\":\"2000-01-03 01:01:01\",\"update\":0}]}");
	Event event2(Http::Connection::Event_RequestComplete);
	erp->proc(&event2);

	// No events
	TEST_NUMBER_EQUAL(true, erp->sendSyncRequest("root", 0, 0, &events, &buf));
	TEST_STRING_EQUAL("{\"hv\":\"1.0.2\",\"sv\":\"1.2.3\",\"release\":1,\"sq\":0,"
		"\"devices\":["
		"{\"type\":2,\"state\":0,\"params\":[{\"t\":1,\"v\":\"\"},{\"t\":2,\"v\":\"\"},{\"t\":3,\"v\":\"\"},{\"t\":4,\"v\":0},{\"t\":32768,\"v\":0},{\"t\":32770,\"v\":0}],\"events\":[]}],"
		"\"events\":[]}", http->getRequest()->data->getString());
	http->getResponse()->data->set("{\"success\":true,\"data\":[{\"sv\":\"0.0.0\",\"date\":\"2000-01-03 01:01:01\",\"update\":0}]}");
	Event event3(Http::Connection::Event_RequestComplete);
	erp->proc(&event3);
	return true;
}

bool ErpProtocolTest::testEventOverflow() {
	RamMemory memory(32000);
	TestRealTime realtime;
	StringBuilder buf(4096, 4096);

	ConfigEventList events(&realtime);
	events.init(5, &memory);

	// Two events
	addSaleEvent("1", "name1", 10, &events);
	addSaleEvent("2", "name2", 20, &events);
	TEST_NUMBER_EQUAL(true, erp->sendSyncRequest("root", 0, 0, &events, &buf));
	TEST_STRING_EQUAL("{\"hv\":\"1.0.2\",\"sv\":\"1.2.3\",\"release\":1,\"sq\":0,"
		"\"devices\":["
		"{\"type\":2,\"state\":0,\"params\":[{\"t\":1,\"v\":\"\"},{\"t\":2,\"v\":\"\"},{\"t\":3,\"v\":\"\"},{\"t\":4,\"v\":0},{\"t\":32768,\"v\":0},{\"t\":32770,\"v\":0}],\"events\":[]}],"
		"\"events\":["
		"{\"date\":\"2000-01-02 10:20:30\",\"type\":3,\"select_id\":\"1\",\"name\":\"name1\",\"payment_device\":\"CA\",\"price_list\":0,\"value\":1000,\"tax_system\":0,\"tax_rate\":0,\"tax_value\":0,\"fn\":22222,\"fd\":33333,\"fp\":44444},"
		"{\"date\":\"2000-01-02 10:20:30\",\"type\":3,\"select_id\":\"2\",\"name\":\"name2\",\"payment_device\":\"CA\",\"price_list\":0,\"value\":2000,\"tax_system\":0,\"tax_rate\":0,\"tax_value\":0,\"fn\":22222,\"fd\":33333,\"fp\":44444}"
		"]}", http->getRequest()->data->getString());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, events.getSync());
	TEST_NUMBER_EQUAL(0, events.getUnsync());

	http->getResponse()->data->set("{\"success\":true,\"data\":[{\"sv\":\"0.0.0\",\"date\":\"2000-01-03 01:01:01\",\"update\":0}]}");
	Event event1(Http::Connection::Event_RequestComplete);
	erp->proc(&event1);
	TEST_NUMBER_EQUAL(1, events.getSync());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, events.getUnsync());

	// Repeat events
	addSaleEvent("3", "name3", 30, &events);
	addSaleEvent("4", "name4", 40, &events);
	addSaleEvent("5", "name5", 50, &events);
	addSaleEvent("6", "name6", 60, &events);
	addSaleEvent("7", "name7", 70, &events);
	TEST_NUMBER_EQUAL(true, erp->sendSyncRequest("root", 0, 0, &events, &buf));
	TEST_STRING_EQUAL("{\"hv\":\"1.0.2\",\"sv\":\"1.2.3\",\"release\":1,\"sq\":0,"
		"\"devices\":["
		"{\"type\":2,\"state\":0,\"params\":[{\"t\":1,\"v\":\"\"},{\"t\":2,\"v\":\"\"},{\"t\":3,\"v\":\"\"},{\"t\":4,\"v\":0},{\"t\":32768,\"v\":0},{\"t\":32770,\"v\":0}],\"events\":[]}],"
		"\"events\":["
		"{\"date\":\"2000-01-02 10:20:30\",\"type\":3,\"select_id\":\"3\",\"name\":\"name3\",\"payment_device\":\"CA\",\"price_list\":0,\"value\":3000,\"tax_system\":0,\"tax_rate\":0,\"tax_value\":0,\"fn\":22222,\"fd\":33333,\"fp\":44444},"
		"{\"date\":\"2000-01-02 10:20:30\",\"type\":3,\"select_id\":\"4\",\"name\":\"name4\",\"payment_device\":\"CA\",\"price_list\":0,\"value\":4000,\"tax_system\":0,\"tax_rate\":0,\"tax_value\":0,\"fn\":22222,\"fd\":33333,\"fp\":44444},"
		"{\"date\":\"2000-01-02 10:20:30\",\"type\":3,\"select_id\":\"5\",\"name\":\"name5\",\"payment_device\":\"CA\",\"price_list\":0,\"value\":5000,\"tax_system\":0,\"tax_rate\":0,\"tax_value\":0,\"fn\":22222,\"fd\":33333,\"fp\":44444},"
		"{\"date\":\"2000-01-02 10:20:30\",\"type\":3,\"select_id\":\"6\",\"name\":\"name6\",\"payment_device\":\"CA\",\"price_list\":0,\"value\":6000,\"tax_system\":0,\"tax_rate\":0,\"tax_value\":0,\"fn\":22222,\"fd\":33333,\"fp\":44444},"
		"{\"date\":\"2000-01-02 10:20:30\",\"type\":3,\"select_id\":\"7\",\"name\":\"name7\",\"payment_device\":\"CA\",\"price_list\":0,\"value\":7000,\"tax_system\":0,\"tax_rate\":0,\"tax_value\":0,\"fn\":22222,\"fd\":33333,\"fp\":44444}"
		"]}", http->getRequest()->data->getString());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, events.getSync());
	TEST_NUMBER_EQUAL(2, events.getUnsync());
	http->getResponse()->data->set("{\"success\":true,\"data\":[{\"sv\":\"0.0.0\",\"date\":\"2000-01-03 01:01:01\",\"update\":0}]}");
	Event event2(Http::Connection::Event_RequestComplete);
	erp->proc(&event2);
	TEST_NUMBER_EQUAL(1, events.getSync());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, events.getUnsync());

	// No events
	TEST_NUMBER_EQUAL(true, erp->sendSyncRequest("root", 0, 0, &events, &buf));
	TEST_STRING_EQUAL("{\"hv\":\"1.0.2\",\"sv\":\"1.2.3\",\"release\":1,\"sq\":0,"
		"\"devices\":["
		"{\"type\":2,\"state\":0,\"params\":[{\"t\":1,\"v\":\"\"},{\"t\":2,\"v\":\"\"},{\"t\":3,\"v\":\"\"},{\"t\":4,\"v\":0},{\"t\":32768,\"v\":0},{\"t\":32770,\"v\":0}],\"events\":[]}],"
		"\"events\":[]}", http->getRequest()->data->getString());
	http->getResponse()->data->set("{\"success\":true,\"data\":[{\"sv\":\"0.0.0\",\"date\":\"2000-01-03 01:01:01\",\"update\":0}]}");
	Event event3(Http::Connection::Event_RequestComplete);
	erp->proc(&event3);
	return true;
}
