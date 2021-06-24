#include "test/include/Test.h"
#include "config/v5/RingList2.h"
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

class RingList2Test : public TestSet {
public:
	RingList2Test();
	bool test();
	bool testGetUnsync();
//	bool testEventToHumanReadable();
//	bool testDamageHeadHeader();
//	bool testDamageTailHeader();
};

TEST_SET_REGISTER(RingList2Test);

RingList2Test::RingList2Test() {
	TEST_CASE_REGISTER(RingList2Test, test);
	TEST_CASE_REGISTER(RingList2Test, testGetUnsync);
//	TEST_CASE_REGISTER(RingList2Test, testEventToHumanReadable);
//	TEST_CASE_REGISTER(RingList2Test, testDamageHeadHeader);
//	TEST_CASE_REGISTER(RingList2Test, testDamageTailHeader);
}

bool RingList2Test::test() {
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
	RingList2 list1;

	// init
	uint32_t dataSize = 5;
	uint32_t listSize = 4;
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.init(dataSize, listSize, &memory));
	TEST_NUMBER_EQUAL(4, list1.getSize());
	TEST_NUMBER_EQUAL(0, list1.getLen());
	TEST_NUMBER_EQUAL(RINGLIST2_UNDEFINED, list1.getFirst());
	TEST_NUMBER_EQUAL(RINGLIST2_UNDEFINED, list1.getLast());

	// add first
	uint8_t entry1[5] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.insert(entry1, sizeof(entry1)));
	TEST_NUMBER_EQUAL(4, list1.getSize());
	TEST_NUMBER_EQUAL(1, list1.getLen());
	TEST_NUMBER_EQUAL(0, list1.getFirst());
	TEST_NUMBER_EQUAL(0, list1.getLast());

	// add
	uint8_t entry2[5] = { 0xBB, 0xBB, 0xBB, 0xBB, 0xBB };
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.insert(entry2, sizeof(entry2)));
	TEST_NUMBER_EQUAL(4, list1.getSize());
	TEST_NUMBER_EQUAL(2, list1.getLen());
	TEST_NUMBER_EQUAL(0, list1.getFirst());
	TEST_NUMBER_EQUAL(1, list1.getLast());

	// add
	uint8_t entry3[5] = { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC };
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.insert(entry3, sizeof(entry3)));
	TEST_NUMBER_EQUAL(4, list1.getSize());
	TEST_NUMBER_EQUAL(3, list1.getLen());
	TEST_NUMBER_EQUAL(0, list1.getFirst());
	TEST_NUMBER_EQUAL(2, list1.getLast());

	// add
	uint8_t entry4[5] = { 0xDD, 0xDD, 0xDD, 0xDD, 0xDD };
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.insert(entry4, sizeof(entry4)));
	TEST_NUMBER_EQUAL(4, list1.getSize());
	TEST_NUMBER_EQUAL(4, list1.getLen());
	TEST_NUMBER_EQUAL(0, list1.getFirst());
	TEST_NUMBER_EQUAL(3, list1.getLast());

	// add and overflow
	uint8_t entry5[5] = { 0xEE, 0xEE, 0xEE, 0xEE, 0xEE };
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.insert(entry5, sizeof(entry5)));
	TEST_NUMBER_EQUAL(4, list1.getSize());
	TEST_NUMBER_EQUAL(4, list1.getLen());
	TEST_NUMBER_EQUAL(1, list1.getFirst());
	TEST_NUMBER_EQUAL(0, list1.getLast());

	// get
	LOG_HEX(memory.getData(), memory.getMaxSize());
	uint8_t entry6[5];
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.get(0, entry6, sizeof(entry6)));
	TEST_HEXDATA_EQUAL("AAAAAAAAAA", entry6, sizeof(entry6));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.get(1, entry6, sizeof(entry6)));
	TEST_HEXDATA_EQUAL("BBBBBBBBBB", entry6, sizeof(entry6));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.get(2, entry6, sizeof(entry6)));
	TEST_HEXDATA_EQUAL("CCCCCCCCCC", entry6, sizeof(entry6));
	TEST_NUMBER_EQUAL(MemoryResult_Ok, list1.get(3, entry6, sizeof(entry6)));
	TEST_HEXDATA_EQUAL("DDDDDDDDDD", entry6, sizeof(entry6));
	TEST_NUMBER_EQUAL(MemoryResult_OutOfIndex, list1.get(4, entry6, sizeof(entry6)));

/*
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
*/
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
/*
	// sync
	list2.remove(3);
	TEST_NUMBER_EQUAL(5, list2.getSize());
	TEST_NUMBER_EQUAL(2, list2.getLen());
	TEST_NUMBER_EQUAL(4, list2.getFirst());
	TEST_NUMBER_EQUAL(0, list2.getLast());
*/
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

bool RingList2Test::testGetUnsync() {
/*	RamMemory memory(32000);
	TestRealTime realtime;
	ConfigEventList list(&realtime);

	// init
	list.init(5, &memory);
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getFirst());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getLast());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getSync());
	TEST_NUMBER_EQUAL(CONFIG_EVENT_UNSET, list.getUnsync());
*/
	return true;
}
