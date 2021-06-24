#include "test/include/Test.h"
#include "config/v3/event/Config3EventIterator.h"
#include "memory/include/RamMemory.h"
#include "timer/include/TestRealTime.h"
#include "logger/include/Logger.h"

class Config3EventListTest : public TestSet {
public:
	Config3EventListTest();
	bool test();
	bool testGetUnsync();
	bool testEventToHumanReadable();
	bool testDamageHeadHeader();
	bool testDamageTailHeader();
	bool testSetSyncValueTooLate();
};

TEST_SET_REGISTER(Config3EventListTest);

Config3EventListTest::Config3EventListTest() {
	TEST_CASE_REGISTER(Config3EventListTest, test);
	TEST_CASE_REGISTER(Config3EventListTest, testGetUnsync);
	TEST_CASE_REGISTER(Config3EventListTest, testEventToHumanReadable);
	TEST_CASE_REGISTER(Config3EventListTest, testDamageHeadHeader);
	TEST_CASE_REGISTER(Config3EventListTest, testDamageTailHeader);
	TEST_CASE_REGISTER(Config3EventListTest, testSetSyncValueTooLate);
}

bool Config3EventListTest::test() {
	RamMemory memory(32000);
	TestRealTime realtime;
	Config3EventList list(&realtime);

	// init
	list.init(5, &memory);
	TEST_NUMBER_EQUAL(5, list.getSize());
	TEST_NUMBER_EQUAL(0, list.getLen());
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getFirst());
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getLast());
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getSync());

	// add first
	list.add(Config3Event::Type_OnlineLast, "0"); // 0,0
	TEST_NUMBER_EQUAL(5, list.getSize());
	TEST_NUMBER_EQUAL(1, list.getLen());
	TEST_NUMBER_EQUAL(0, list.getFirst());
	TEST_NUMBER_EQUAL(0, list.getLast());
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getSync());

	// overflow
	list.add(Config3Event::Type_OnlineLast, "1"); // 0,1
	list.add(Config3Event::Type_OnlineLast, "2"); // 0,2
	list.add(Config3Event::Type_OnlineLast, "3"); // 0,3
	list.add(Config3Event::Type_OnlineLast, "4"); // 0,4
	Fiscal::Sale sale1;
	sale1.setProduct("id1", 0, "name1", 1234, 0, 1);
	sale1.device.set("d1");
	sale1.priceList = 1;
	sale1.fiscalRegister = 11111;
	sale1.fiscalStorage = 22222;
	sale1.fiscalDocument = 33333;
	sale1.fiscalSign = 44444;
	list.add(&sale1);
	TEST_NUMBER_EQUAL(5, list.getSize());
	TEST_NUMBER_EQUAL(1, list.getFirst());
	TEST_NUMBER_EQUAL(0, list.getLast());
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getSync());

	// foreach from first to last
	Config3EventIterator iterator1(&list);
	TEST_NUMBER_EQUAL(true, iterator1.first());
	TEST_NUMBER_EQUAL(Config3Event::Type_OnlineLast, iterator1.getCode());
	TEST_STRING_EQUAL("1", iterator1.getString());

	TEST_NUMBER_EQUAL(true, iterator1.next());
	TEST_NUMBER_EQUAL(Config3Event::Type_OnlineLast, iterator1.getCode());
	TEST_STRING_EQUAL("2", iterator1.getString());

	TEST_NUMBER_EQUAL(true, iterator1.next());
	TEST_NUMBER_EQUAL(Config3Event::Type_OnlineLast, iterator1.getCode());
	TEST_STRING_EQUAL("3", iterator1.getString());

	TEST_NUMBER_EQUAL(true, iterator1.next());
	TEST_NUMBER_EQUAL(Config3Event::Type_OnlineLast, iterator1.getCode());
	TEST_STRING_EQUAL("4", iterator1.getString());

	TEST_NUMBER_EQUAL(true, iterator1.next());
	TEST_NUMBER_EQUAL(Config3Event::Type_Sale, iterator1.getCode());
	Config3EventSale *sale2 = iterator1.getSale();
	TEST_STRING_EQUAL("id1", sale2->selectId.get());
	TEST_STRING_EQUAL("name1", sale2->name.get());
	TEST_STRING_EQUAL("d1", sale2->device.get());
	TEST_NUMBER_EQUAL(1, sale2->priceList);
	TEST_NUMBER_EQUAL(1234, sale2->price);
	TEST_NUMBER_EQUAL(11111, sale2->fiscalRegister);
	TEST_NUMBER_EQUAL(22222, sale2->fiscalStorage);
	TEST_NUMBER_EQUAL(33333, sale2->fiscalDocument);
	TEST_NUMBER_EQUAL(44444, sale2->fiscalSign);

	TEST_NUMBER_EQUAL(false, iterator1.next());

	// findByIndex
	iterator1.findByIndex(0);
	TEST_NUMBER_EQUAL(Config3Event::Type_Sale, iterator1.getCode());
	Config3EventSale *sale3 = iterator1.getSale();
	TEST_STRING_EQUAL("id1", sale3->selectId.get());
	TEST_STRING_EQUAL("name1", sale3->name.get());
	TEST_STRING_EQUAL("d1", sale3->device.get());
	TEST_NUMBER_EQUAL(1, sale3->priceList);
	TEST_NUMBER_EQUAL(1234, sale3->price);
	TEST_NUMBER_EQUAL(11111, sale3->fiscalRegister);
	TEST_NUMBER_EQUAL(22222, sale3->fiscalStorage);
	TEST_NUMBER_EQUAL(33333, sale3->fiscalDocument);
	TEST_NUMBER_EQUAL(44444, sale3->fiscalSign);

	// sync
	list.setSync(3);
	TEST_NUMBER_EQUAL(5, list.getSize());
	TEST_NUMBER_EQUAL(1, list.getFirst());
	TEST_NUMBER_EQUAL(0, list.getLast());
	TEST_NUMBER_EQUAL(3, list.getSync());

	// foreach from sync to last
	Config3EventIterator iterator2(&list);
	TEST_NUMBER_EQUAL(true, iterator2.unsync());
	TEST_NUMBER_EQUAL(Config3Event::Type_OnlineLast, iterator2.getCode());
	TEST_STRING_EQUAL("4", iterator2.getString());

	TEST_NUMBER_EQUAL(true, iterator2.next());
	TEST_NUMBER_EQUAL(Config3Event::Type_Sale, iterator2.getCode());

	TEST_NUMBER_EQUAL(false, iterator2.next());
	list.setSync(iterator2.getIndex());
	TEST_NUMBER_EQUAL(5, list.getSize());
	TEST_NUMBER_EQUAL(1, list.getFirst());
	TEST_NUMBER_EQUAL(0, list.getLast());
	TEST_NUMBER_EQUAL(0, list.getSync());

	return true;
}

bool Config3EventListTest::testGetUnsync() {
	RamMemory memory(32000);
	TestRealTime realtime;
	Config3EventList list(&realtime);

	// init
	list.init(5, &memory);
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getFirst());
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getLast());
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getSync());
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getUnsync());

	// first < last
	list.add(Config3Event::Type_OnlineLast, "0");
	list.add(Config3Event::Type_OnlineLast, "1");
	TEST_NUMBER_EQUAL(0, list.getFirst());
	TEST_NUMBER_EQUAL(1, list.getLast());
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getSync());
	TEST_NUMBER_EQUAL(0, list.getUnsync());

	// sync all
	list.setSync(list.getLast());
	TEST_NUMBER_EQUAL(0, list.getFirst());
	TEST_NUMBER_EQUAL(1, list.getLast());
	TEST_NUMBER_EQUAL(1, list.getSync());
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getUnsync());

	// last < first, unsync > last
	list.add(Config3Event::Type_OnlineLast, "2"); // 0,2
	list.add(Config3Event::Type_OnlineLast, "3"); // 0,3
	list.add(Config3Event::Type_OnlineLast, "4"); // 0,4
	list.add(Config3Event::Type_OnlineLast, "5"); // 1,0
	TEST_NUMBER_EQUAL(1, list.getFirst());
	TEST_NUMBER_EQUAL(0, list.getLast());
	TEST_NUMBER_EQUAL(1, list.getSync());
	TEST_NUMBER_EQUAL(2, list.getUnsync());

	list.add(Config3Event::Type_OnlineLast, "6");
	TEST_NUMBER_EQUAL(2, list.getFirst());
	TEST_NUMBER_EQUAL(1, list.getLast());
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getSync());
	TEST_NUMBER_EQUAL(2, list.getUnsync());

	// sync all
	list.setSync(list.getLast());
	TEST_NUMBER_EQUAL(2, list.getFirst());
	TEST_NUMBER_EQUAL(1, list.getLast());
	TEST_NUMBER_EQUAL(1, list.getSync());
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getUnsync());

	// last < first, unsync < last
	list.add(Config3Event::Type_OnlineLast, "7");
	list.add(Config3Event::Type_OnlineLast, "8");
	TEST_NUMBER_EQUAL(4, list.getFirst());
	TEST_NUMBER_EQUAL(3, list.getLast());
	TEST_NUMBER_EQUAL(1, list.getSync());
	TEST_NUMBER_EQUAL(2, list.getUnsync());

	return true;
}

bool Config3EventListTest::testEventToHumanReadable() {
	DateTime date;
	Config3Event event;
	Fiscal::Sale sale;
	StringBuilder str;

	sale.setProduct("id1", 0, "Тесточино", 1500, 0, 1);
	sale.device.set("CA");
	sale.priceList = 0;
	event.set(&date, &sale, 0);
	TEST_STRING_EQUAL("Продажа", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("\"Тесточино\" за 1500 наличными", str.getString());

	event.set(&date, Config3Event::Type_OnlineStart);
	TEST_STRING_EQUAL("Связь установлена", event.getEventName(&event));

	event.set(&date, Config3Event::Type_CashlessIdNotFound, "123");
	TEST_STRING_EQUAL("Ошибка настройки", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Продукта с номером 123 нет в планограмме", str.getString());

	event.set(&date, Config3Event::Type_PriceListNotFound, "CA0");
	TEST_STRING_EQUAL("Ошибка настройки", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Прайс-листа CA0 нет в планограмме", str.getString());

	event.set(&date, Config3Event::Type_SyncConfigError);
	TEST_STRING_EQUAL("Ошибка настройки", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Планограммы не совпадают", str.getString());

	event.set(&date, Config3Event::Type_PriceNotEqual, "123*456*789");
	TEST_STRING_EQUAL("Ошибка настройки", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Цена не совпадает с планограммой (кнопка 123, планограмма 456, автомат 789)", str.getString());

	event.set(&date, Config3Event::Type_FiscalUnknownError, "123");
	TEST_STRING_EQUAL("Ошибка ФР", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Код ошибки 123", str.getString());

	event.set(&date, Config3Event::Type_FiscalLogicError, "123");
	TEST_STRING_EQUAL("Ошибка ФР", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Ошибка протокола 123", str.getString());

	event.set(&date, Config3Event::Type_FiscalConnectError);
	TEST_STRING_EQUAL("Ошибка ФР", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Нет связи с ФР", str.getString());

	event.set(&date, Config3Event::Type_FiscalPassword);
	TEST_STRING_EQUAL("Ошибка ФР", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Неправильный пароль ФР", str.getString());

	event.set(&date, Config3Event::Type_PrinterNotFound);
	TEST_STRING_EQUAL("Ошибка принтера", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Принтер не найден", str.getString());

	event.set(&date, Config3Event::Type_PrinterNoPaper);
	TEST_STRING_EQUAL("Ошибка принтера", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("В принтере закончилась бумага", str.getString());
	return true;
}

bool Config3EventListTest::testDamageHeadHeader() {
	RamMemory memory(32000);
	TestRealTime realtime;

	// init
	Config3EventList list1(&realtime);
	list1.init(5, &memory);
	TEST_NUMBER_EQUAL(5, list1.getSize());
	TEST_NUMBER_EQUAL(0, list1.getLen());
	list1.add(Config3Event::Type_OnlineLast, "1"); // 0,1
	list1.add(Config3Event::Type_OnlineLast, "2"); // 0,2

	// damage first header
	uint8_t crap[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	memory.setAddress(0);
	memory.write(crap, sizeof(crap));

	Config3EventList list2(&realtime);
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_WrongCrc, list2.load(&memory));

	// repair
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list2.repair(5, &memory));
	TEST_NUMBER_EQUAL(5, list2.getSize());
	TEST_NUMBER_EQUAL(2, list2.getLen());
	return true;
}

bool Config3EventListTest::testDamageTailHeader() {
	RamMemory memory(32000);
	TestRealTime realtime;

	// init
	Config3EventList list1(&realtime);
	list1.init(5, &memory);
	TEST_NUMBER_EQUAL(5, list1.getSize());
	TEST_NUMBER_EQUAL(0, list1.getLen());
	list1.add(Config3Event::Type_OnlineLast, "1"); // 0,1
	list1.add(Config3Event::Type_OnlineLast, "2"); // 0,2

	// damage first header
	uint8_t crap[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	memory.setAddress(545);
	memory.write(crap, sizeof(crap));

	Config3EventList list2(&realtime);
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_WrongCrc, list2.load(&memory));

	// repair
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list2.repair(5, &memory));
	TEST_NUMBER_EQUAL(5, list2.getSize());
	TEST_NUMBER_EQUAL(2, list2.getLen());
	return true;
}

bool Config3EventListTest::testSetSyncValueTooLate() {
	RamMemory memory(32000);
	TestRealTime realtime;
	Config3EventList list(&realtime);
	Config3EventIterator	iterator(&list);

	// init
	list.init(5, &memory);
	TEST_NUMBER_EQUAL(5, list.getSize());
	TEST_NUMBER_EQUAL(0, list.getLen());
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getFirst());
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getLast());
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getSync());
	TEST_NUMBER_EQUAL(false, iterator.unsync());

	// add first
	list.add(Config3Event::Type_OnlineLast, "0");
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getSync());
	TEST_NUMBER_EQUAL(0, list.getUnsync());

	// add second
	list.add(Config3Event::Type_OnlineLast, "0");
	TEST_NUMBER_EQUAL(CONFIG3_EVENT_UNSET, list.getSync());
	TEST_NUMBER_EQUAL(0, list.getUnsync());

	// sync1
	TEST_NUMBER_EQUAL(true, iterator.unsync());
	TEST_NUMBER_EQUAL(0, iterator.getIndex());
	TEST_NUMBER_EQUAL(true, iterator.next());
	TEST_NUMBER_EQUAL(1, iterator.getIndex());
	TEST_NUMBER_EQUAL(false, iterator.next());
	list.setSync(1);

	// sync2
	TEST_NUMBER_EQUAL(false, iterator.unsync());
	list.add(Config3Event::Type_OnlineLast, "0");
	TEST_NUMBER_EQUAL(1, list.getSync());
	list.setSync(1);

	// sync3
	TEST_NUMBER_EQUAL(1, iterator.unsync());
	TEST_NUMBER_EQUAL(2, iterator.getIndex());
	list.setSync(1);
	return true;
}
