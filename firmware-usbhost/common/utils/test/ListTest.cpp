#include "utils/include/List.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class ListTest : public TestSet {
public:
	ListTest();
	bool testMemory();
	bool testSort();
	bool testRemove();
};

TEST_SET_REGISTER(ListTest);

ListTest::ListTest() {
	TEST_CASE_REGISTER(ListTest, testMemory);
	TEST_CASE_REGISTER(ListTest, testSort);
	TEST_CASE_REGISTER(ListTest, testRemove);
}

bool ListTest::testMemory() {
	const int size = 64;
	const char actualValue = 0x55;
	char *actual = new char[2];
	actual[0] = actualValue;

	List<char> list1(3);
	List<char> *list2 = new List<char>(2);
	for(int t = 0; t < size; t++) {
		list1.add(new char[4]);
		list2->add(new char[4]);
	}

	TEST_NUMBER_EQUAL(size, list1.getSize());
	TEST_NUMBER_EQUAL(size, list2->getSize());
	list1.add(actual);

	// Удаляем все, кроме последнего элемента
	const int counts = list1.getSize() - 1;
	for (int i = 0; i < counts; i++) {
		char *c1 = list1.get(0);
		list1.remove(c1);
		delete []c1;
		TEST_NUMBER_EQUAL(size - i, list1.getSize());
	}

	// Последний элемент стал первым, проверяем.
	TEST_NUMBER_EQUAL(1, list1.getSize());
	TEST_NUMBER_EQUAL(actualValue, list1.get(0)[0]);

	list2->clearAndFree();
	TEST_NUMBER_EQUAL(0, list2->getSize());

	list2->add(new char[128]);
	TEST_NUMBER_EQUAL(1, list2->getSize());

	delete list2;
	return true;
}

class Comparator : public List<uint16_t>::Comparator {
public:
	int compare(uint16_t *f, uint16_t *s) {
		if(*f > *s) {
			return 1;
		}
		if(*f < *s) {
			return -1;
		}
		return 0;
	}
};

bool ListTest::testSort() {
	List<uint16_t> list;

	list.add(new uint16_t(5));
	list.add(new uint16_t(2));
	list.add(new uint16_t(3));
	list.add(new uint16_t(7));
	TEST_NUMBER_EQUAL(4, list.getSize());

	Comparator c;
	list.sort(&c);
	TEST_NUMBER_EQUAL(2, *list.get(0));
	TEST_NUMBER_EQUAL(3, *list.get(1));
	TEST_NUMBER_EQUAL(5, *list.get(2));
	TEST_NUMBER_EQUAL(7, *list.get(3));
	return true;
}

bool ListTest::testRemove() {
	List<uint16_t> list;

	uint16_t *entry1 = new uint16_t(5);
	uint16_t *entry2 = new uint16_t(2);
	uint16_t *entry3 = new uint16_t(3);

	list.add(entry1);
	list.add(entry2);
	list.add(entry3);
	TEST_NUMBER_EQUAL(3, list.getSize());

	list.remove(entry3);
	list.remove(entry2);
	list.remove(entry1);
	TEST_NUMBER_EQUAL(0, list.getSize());
	return true;
}
