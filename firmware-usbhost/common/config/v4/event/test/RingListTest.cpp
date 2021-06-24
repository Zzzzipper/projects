#include "test/include/Test.h"
#include "config/v4/event/RingList.h"
#include "memory/include/RamMemory.h"
#include "timer/include/TestRealTime.h"
#include "logger/include/Logger.h"

#pragma pack(push,1)
struct TestData {
	uint32_t id;
	uint32_t value;
	uint8_t busy;
	uint8_t crc[1];
};
#pragma pack(pop)

class TestEntry {
public:
	void setId(uint32_t id) {
		data.id = id;
	}

	uint32_t getId() {
		return data.id;
	}

	void setBusy(uint8_t busy) {
		data.busy = busy;
	}

	uint8_t getBusy() {
		return data.busy;
	}

	void setValue(uint32_t value) {
		data.value = value;
	}

	uint32_t getValue() {
		return data.value;
	}

	void bind(Memory *memory) {
		this->memory = memory;
		this->address = memory->getAddress();
	}

	MemoryResult init(Memory *memory) {
		this->memory = memory;
		this->address = memory->getAddress();
		memset(&data, 0, sizeof(data));
		return save();
	}

	MemoryResult load(Memory *memory) {
		this->memory = memory;
		this->address = memory->getAddress();
		MemoryCrc crc(memory);
		return crc.readDataWithCrc(&data, sizeof(data));
	}

	MemoryResult save() {
		if(memory == NULL) {
			LOG_ERROR(LOG_CFG, "Memory not inited");
			return MemoryResult_WrongData;
		}
		memory->setAddress(address);
		MemoryCrc crc(memory);
		return crc.writeDataWithCrc(&data, sizeof(data));
	}

	uint32_t getDataSize() {
		return sizeof(data);
	}

private:
	Memory *memory;
	uint32_t address;
	TestData data;
};

class RingListTest : public TestSet {
public:
	RingListTest();
	bool test();
	bool testGetUnsync();
//	bool testEventToHumanReadable();
//	bool testDamageHeadHeader();
//	bool testDamageTailHeader();
};

TEST_SET_REGISTER(RingListTest);

RingListTest::RingListTest() {
	TEST_CASE_REGISTER(RingListTest, test);
	TEST_CASE_REGISTER(RingListTest, testGetUnsync);
//	TEST_CASE_REGISTER(RingListTest, testEventToHumanReadable);
//	TEST_CASE_REGISTER(RingListTest, testDamageHeadHeader);
//	TEST_CASE_REGISTER(RingListTest, testDamageTailHeader);
}

bool RingListTest::test() {
#if 0
	RamMemory memory(32000);
	TestRealTime realtime;
	ConfigEventList list(&realtime);

	// init
	list.init(5, &memory);
	TEST_NUMBER_EQUAL(5, list.getSize());
	TEST_NUMBER_EQUAL(0, list.getLen());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getFirst());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getLast());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getSync());

	// add first
	list.add(ConfigEvent::Type_OnlineLast, (uint32_t)0); // 0,0
	TEST_NUMBER_EQUAL(5, list.getSize());
	TEST_NUMBER_EQUAL(1, list.getLen());
	TEST_NUMBER_EQUAL(0, list.getFirst());
	TEST_NUMBER_EQUAL(0, list.getLast());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getSync());

	// overflow
	list.add(ConfigEvent::Type_OnlineLast, 1); // 0,1
	list.add(ConfigEvent::Type_OnlineLast, 2); // 0,2
	list.add(ConfigEvent::Type_OnlineLast, 3); // 0,3
	list.add(ConfigEvent::Type_OnlineLast, 4); // 0,4
	ConfigEventSale sale1;
	sale1.selectId.set("id1");
	sale1.name.set("name1");
	sale1.device.set("d1");
	sale1.priceList = 1;
	sale1.price = 1234;
	sale1.fiscalRegister = 11111;
	sale1.fiscalStorage = 22222;
	sale1.fiscalDocument = 33333;
	sale1.fiscalSign = 44444;
	list.add(&sale1);
	TEST_NUMBER_EQUAL(5, list.getSize());
	TEST_NUMBER_EQUAL(1, list.getFirst());
	TEST_NUMBER_EQUAL(0, list.getLast());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getSync());

	// foreach from first to last
	ConfigEventIterator iterator1(&list);
	TEST_NUMBER_EQUAL(true, iterator1.first());
	TEST_NUMBER_EQUAL(ConfigEvent::Type_OnlineLast, iterator1.getCode());
	TEST_NUMBER_EQUAL(1, iterator1.getNumber());

	TEST_NUMBER_EQUAL(true, iterator1.next());
	TEST_NUMBER_EQUAL(ConfigEvent::Type_OnlineLast, iterator1.getCode());
	TEST_NUMBER_EQUAL(2, iterator1.getNumber());

	TEST_NUMBER_EQUAL(true, iterator1.next());
	TEST_NUMBER_EQUAL(ConfigEvent::Type_OnlineLast, iterator1.getCode());
	TEST_NUMBER_EQUAL(3, iterator1.getNumber());

	TEST_NUMBER_EQUAL(true, iterator1.next());
	TEST_NUMBER_EQUAL(ConfigEvent::Type_OnlineLast, iterator1.getCode());
	TEST_NUMBER_EQUAL(4, iterator1.getNumber());

	TEST_NUMBER_EQUAL(true, iterator1.next());
	TEST_NUMBER_EQUAL(ConfigEvent::Type_Sale, iterator1.getCode());
	ConfigEventSale *sale2 = iterator1.getSale();
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
	TEST_NUMBER_EQUAL(ConfigEvent::Type_Sale, iterator1.getCode());
	ConfigEventSale *sale3 = iterator1.getSale();
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
	ConfigEventIterator iterator2(&list);
	TEST_NUMBER_EQUAL(true, iterator2.unsync());
	TEST_NUMBER_EQUAL(ConfigEvent::Type_OnlineLast, iterator2.getCode());
	TEST_NUMBER_EQUAL(4, iterator2.getNumber());

	TEST_NUMBER_EQUAL(true, iterator2.next());
	TEST_NUMBER_EQUAL(ConfigEvent::Type_Sale, iterator2.getCode());

	TEST_NUMBER_EQUAL(false, iterator2.next());
	list.setSync(iterator2.getIndex());
	TEST_NUMBER_EQUAL(5, list.getSize());
	TEST_NUMBER_EQUAL(1, list.getFirst());
	TEST_NUMBER_EQUAL(0, list.getLast());
	TEST_NUMBER_EQUAL(0, list.getSync());
	return true;
#else
	RamMemory memory(256);
	RingList<TestEntry> list1;

	// init
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.init(5, &memory));
	TEST_NUMBER_EQUAL(5, list1.getSize());
	TEST_NUMBER_EQUAL(0, list1.getLen());
	TEST_NUMBER_EQUAL(RINGLIST_UNDEFINED, list1.getFirst());
	TEST_NUMBER_EQUAL(RINGLIST_UNDEFINED, list1.getLast());

	// add first
	TestEntry entry1;
	entry1.setValue(123);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.insert(&entry1));
	TEST_NUMBER_EQUAL(5, list1.getSize());
	TEST_NUMBER_EQUAL(1, list1.getLen());
	TEST_NUMBER_EQUAL(0, list1.getFirst());
	TEST_NUMBER_EQUAL(0, list1.getLast());

	// add
	entry1.setValue(1);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.insert(&entry1));
	TEST_NUMBER_EQUAL(5, list1.getSize());
	TEST_NUMBER_EQUAL(2, list1.getLen());
	TEST_NUMBER_EQUAL(0, list1.getFirst());
	TEST_NUMBER_EQUAL(1, list1.getLast());

	// add
	entry1.setValue(2);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.insert(&entry1));
	TEST_NUMBER_EQUAL(5, list1.getSize());
	TEST_NUMBER_EQUAL(3, list1.getLen());
	TEST_NUMBER_EQUAL(0, list1.getFirst());
	TEST_NUMBER_EQUAL(2, list1.getLast());

	// add
	entry1.setValue(3);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.insert(&entry1));
	TEST_NUMBER_EQUAL(5, list1.getSize());
	TEST_NUMBER_EQUAL(4, list1.getLen());
	TEST_NUMBER_EQUAL(0, list1.getFirst());
	TEST_NUMBER_EQUAL(3, list1.getLast());

	// add
	entry1.setValue(4);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.insert(&entry1));
	TEST_NUMBER_EQUAL(5, list1.getSize());
	TEST_NUMBER_EQUAL(5, list1.getLen());
	TEST_NUMBER_EQUAL(0, list1.getFirst());
	TEST_NUMBER_EQUAL(4, list1.getLast());

	// add and overflow
	entry1.setValue(5);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.insert(&entry1));
	TEST_NUMBER_EQUAL(5, list1.getSize());
	TEST_NUMBER_EQUAL(5, list1.getLen());
	TEST_NUMBER_EQUAL(1, list1.getFirst());
	TEST_NUMBER_EQUAL(0, list1.getLast());

	// get
	TestEntry entry2;
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.get(0, &entry2));
	TEST_NUMBER_EQUAL(5, entry2.getValue());
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.get(1, &entry2));
	TEST_NUMBER_EQUAL(1, entry2.getValue());
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.get(2, &entry2));
	TEST_NUMBER_EQUAL(2, entry2.getValue());
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.get(3, &entry2));
	TEST_NUMBER_EQUAL(3, entry2.getValue());
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.get(4, &entry2));
	TEST_NUMBER_EQUAL(4, entry2.getValue());

	// list2
	RingList<TestEntry> list2;

	// init
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list2.loadAndCheck(&memory));
	TEST_NUMBER_EQUAL(5, list2.getSize());
	TEST_NUMBER_EQUAL(5, list2.getLen());
	TEST_NUMBER_EQUAL(1, list2.getFirst());
	TEST_NUMBER_EQUAL(0, list2.getLast());

	// get
	TestEntry entry3;
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list2.get(0, &entry3));
	TEST_NUMBER_EQUAL(5, entry3.getValue());
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list2.get(1, &entry3));
	TEST_NUMBER_EQUAL(1, entry3.getValue());
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list2.get(2, &entry3));
	TEST_NUMBER_EQUAL(2, entry3.getValue());
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list2.get(3, &entry3));
	TEST_NUMBER_EQUAL(3, entry3.getValue());
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list2.get(4, &entry3));
	TEST_NUMBER_EQUAL(4, entry3.getValue());

/*
	// foreach from first to last
	ConfigEventIterator iterator1(&list);
	TEST_NUMBER_EQUAL(true, iterator1.first());
	TEST_NUMBER_EQUAL(ConfigEvent::Type_OnlineLast, iterator1.getCode());
	TEST_NUMBER_EQUAL(1, iterator1.getNumber());

	TEST_NUMBER_EQUAL(true, iterator1.next());
	TEST_NUMBER_EQUAL(ConfigEvent::Type_OnlineLast, iterator1.getCode());
	TEST_NUMBER_EQUAL(2, iterator1.getNumber());

	TEST_NUMBER_EQUAL(true, iterator1.next());
	TEST_NUMBER_EQUAL(ConfigEvent::Type_OnlineLast, iterator1.getCode());
	TEST_NUMBER_EQUAL(3, iterator1.getNumber());

	TEST_NUMBER_EQUAL(true, iterator1.next());
	TEST_NUMBER_EQUAL(ConfigEvent::Type_OnlineLast, iterator1.getCode());
	TEST_NUMBER_EQUAL(4, iterator1.getNumber());

	TEST_NUMBER_EQUAL(true, iterator1.next());
	TEST_NUMBER_EQUAL(ConfigEvent::Type_Sale, iterator1.getCode());
	ConfigEventSale *sale2 = iterator1.getSale();
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
	TEST_NUMBER_EQUAL(ConfigEvent::Type_Sale, iterator1.getCode());
	ConfigEventSale *sale3 = iterator1.getSale();
	TEST_STRING_EQUAL("id1", sale3->selectId.get());
	TEST_STRING_EQUAL("name1", sale3->name.get());
	TEST_STRING_EQUAL("d1", sale3->device.get());
	TEST_NUMBER_EQUAL(1, sale3->priceList);
	TEST_NUMBER_EQUAL(1234, sale3->price);
	TEST_NUMBER_EQUAL(11111, sale3->fiscalRegister);
	TEST_NUMBER_EQUAL(22222, sale3->fiscalStorage);
	TEST_NUMBER_EQUAL(33333, sale3->fiscalDocument);
	TEST_NUMBER_EQUAL(44444, sale3->fiscalSign);
*/

	// sync
	list2.remove(3);
	TEST_NUMBER_EQUAL(5, list2.getSize());
	TEST_NUMBER_EQUAL(2, list2.getLen());
	TEST_NUMBER_EQUAL(4, list2.getFirst());
	TEST_NUMBER_EQUAL(0, list2.getLast());

/*
	// foreach from sync to last
	ConfigEventIterator iterator2(&list);
	TEST_NUMBER_EQUAL(true, iterator2.unsync());
	TEST_NUMBER_EQUAL(ConfigEvent::Type_OnlineLast, iterator2.getCode());
	TEST_NUMBER_EQUAL(4, iterator2.getNumber());

	TEST_NUMBER_EQUAL(true, iterator2.next());
	TEST_NUMBER_EQUAL(ConfigEvent::Type_Sale, iterator2.getCode());

	TEST_NUMBER_EQUAL(false, iterator2.next());
	list.setSync(iterator2.getIndex());
	TEST_NUMBER_EQUAL(5, list.getSize());
	TEST_NUMBER_EQUAL(1, list.getFirst());
	TEST_NUMBER_EQUAL(0, list.getLast());
	TEST_NUMBER_EQUAL(0, list.getSync());*/
	return true;
#endif
}

bool RingListTest::testGetUnsync() {
/*	RamMemory memory(32000);
	TestRealTime realtime;
	ConfigEventList list(&realtime);

	// init
	list.init(5, &memory);
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getFirst());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getLast());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getSync());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getUnsync());

	// first < last
	list.add(ConfigEvent::Type_OnlineLast, (uint32_t)0);
	list.add(ConfigEvent::Type_OnlineLast, 1);
	TEST_NUMBER_EQUAL(0, list.getFirst());
	TEST_NUMBER_EQUAL(1, list.getLast());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getSync());
	TEST_NUMBER_EQUAL(0, list.getUnsync());

	// sync all
	list.setSync(list.getLast());
	TEST_NUMBER_EQUAL(0, list.getFirst());
	TEST_NUMBER_EQUAL(1, list.getLast());
	TEST_NUMBER_EQUAL(1, list.getSync());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getUnsync());

	// last < first, unsync > last
	list.add(ConfigEvent::Type_OnlineLast, 2); // 0,2
	list.add(ConfigEvent::Type_OnlineLast, 3); // 0,3
	list.add(ConfigEvent::Type_OnlineLast, 4); // 0,4
	list.add(ConfigEvent::Type_OnlineLast, 5); // 1,0
	TEST_NUMBER_EQUAL(1, list.getFirst());
	TEST_NUMBER_EQUAL(0, list.getLast());
	TEST_NUMBER_EQUAL(1, list.getSync());
	TEST_NUMBER_EQUAL(2, list.getUnsync());

	list.add(ConfigEvent::Type_OnlineLast, 6);
	TEST_NUMBER_EQUAL(2, list.getFirst());
	TEST_NUMBER_EQUAL(1, list.getLast());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getSync());
	TEST_NUMBER_EQUAL(2, list.getUnsync());

	// sync all
	list.setSync(list.getLast());
	TEST_NUMBER_EQUAL(2, list.getFirst());
	TEST_NUMBER_EQUAL(1, list.getLast());
	TEST_NUMBER_EQUAL(1, list.getSync());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getUnsync());

	// last < first, unsync < last
	list.add(ConfigEvent::Type_OnlineLast, 7);
	list.add(ConfigEvent::Type_OnlineLast, 8);
	TEST_NUMBER_EQUAL(4, list.getFirst());
	TEST_NUMBER_EQUAL(3, list.getLast());
	TEST_NUMBER_EQUAL(1, list.getSync());
	TEST_NUMBER_EQUAL(2, list.getUnsync());
*/
	return true;
}
/*
bool RingListTest::testEventToHumanReadable() {
	DateTime date;
	ConfigEvent event;
	ConfigEventSale sale;
	StringBuilder str;

	sale.name.set("Тесточино");
	sale.device.set("CA");
	sale.priceList = 0;
	sale.price = 1500;
	event.set(&date, &sale);
	TEST_STRING_EQUAL("Продажа", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("\"Тесточино\" за 1500 наличными", str.getString());

	event.set(&date, ConfigEvent::Type_OnlineStart);
	TEST_STRING_EQUAL("Связь установлена", event.getEventName(&event));

	event.set(&date, ConfigEvent::Type_CashlessIdNotFound, "123");
	TEST_STRING_EQUAL("Ошибка настройки", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Продукта с номером 123 нет в планограмме", str.getString());

	event.set(&date, ConfigEvent::Type_PriceListNotFound, "CA0");
	TEST_STRING_EQUAL("Ошибка настройки", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Прайс-листа CA0 нет в планограмме", str.getString());

	event.set(&date, ConfigEvent::Type_SyncConfigError);
	TEST_STRING_EQUAL("Ошибка настройки", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Планограммы не совпадают", str.getString());

	event.set(&date, ConfigEvent::Type_PriceNotEqual, "123*456*789");
	TEST_STRING_EQUAL("Ошибка настройки", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Цена не совпадает с планограммой (кнопка 123, планограмма 456, автомат 789)", str.getString());

	event.set(&date, ConfigEvent::Type_FiscalUnknownError, "123");
	TEST_STRING_EQUAL("Ошибка ФР", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Код ошибки 123", str.getString());

	event.set(&date, ConfigEvent::Type_FiscalLogicError, "123");
	TEST_STRING_EQUAL("Ошибка ФР", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Ошибка протокола 123", str.getString());

	event.set(&date, ConfigEvent::Type_FiscalConnectError);
	TEST_STRING_EQUAL("Ошибка ФР", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Нет связи с ФР", str.getString());

	event.set(&date, ConfigEvent::Type_FiscalPassword);
	TEST_STRING_EQUAL("Ошибка ФР", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Неправильный пароль ФР", str.getString());

	event.set(&date, ConfigEvent::Type_PrinterNotFound);
	TEST_STRING_EQUAL("Ошибка принтера", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("Принтер не найден", str.getString());

	event.set(&date, ConfigEvent::Type_PrinterNoPaper);
	TEST_STRING_EQUAL("Ошибка принтера", event.getEventName(&event));
	event.getEventDescription(&event, &str);
	TEST_STRING_EQUAL("В принтере закончилась бумага", str.getString());
	return true;
}

bool RingListTest::testDamageHeadHeader() {
	RamMemory memory(32000);
	TestRealTime realtime;

	// init
	ConfigEventList list1(&realtime);
	list1.init(5, &memory);
	TEST_NUMBER_EQUAL(5, list1.getSize());
	TEST_NUMBER_EQUAL(0, list1.getLen());
	list1.add(ConfigEvent::Type_OnlineLast, 1); // 0,1
	list1.add(ConfigEvent::Type_OnlineLast, 2); // 0,2

	// damage first header
	uint8_t crap[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	memory.setAddress(0);
	memory.write(crap, sizeof(crap));

	ConfigEventList list2(&realtime);
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_WrongCrc, list2.load(&memory));

	// repair
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list2.repair(5, &memory));
	TEST_NUMBER_EQUAL(5, list2.getSize());
	TEST_NUMBER_EQUAL(2, list2.getLen());
	return true;
}

bool RingListTest::testDamageTailHeader() {
	RamMemory memory(32000);
	TestRealTime realtime;

	// init
	ConfigEventList list1(&realtime);
	list1.init(5, &memory);
	TEST_NUMBER_EQUAL(5, list1.getSize());
	TEST_NUMBER_EQUAL(0, list1.getLen());
	list1.add(ConfigEvent::Type_OnlineLast, 1); // 0,1
	list1.add(ConfigEvent::Type_OnlineLast, 2); // 0,2

	// damage first header
	uint8_t crap[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	memory.setAddress(545);
	memory.write(crap, sizeof(crap));

	ConfigEventList list2(&realtime);
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_WrongCrc, list2.load(&memory));

	// repair
	memory.setAddress(0);
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list2.repair(5, &memory));
	TEST_NUMBER_EQUAL(5, list2.getSize());
	TEST_NUMBER_EQUAL(2, list2.getLen());
	return true;
}
*/
