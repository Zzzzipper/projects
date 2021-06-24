#include "test/include/Test.h"
#include "config/include/StatStorage.h"
#include "logger/include/Logger.h"

class StatStorageTest : public TestSet {
public:
	StatStorageTest();
	bool test();
};

TEST_SET_REGISTER(StatStorageTest);

StatStorageTest::StatStorageTest() {
	TEST_CASE_REGISTER(StatStorageTest, test);
}

bool StatStorageTest::test() {
#if 0
	uint8_t data[40];
	memset(data, 0, sizeof(data));
	StatStorage storage1(data, sizeof(data));
	StatNode *node1  = storage1.add(1);
	StatNode *node2  = storage1.add(2);
	StatNode *node3  = storage1.add(3);
	StatNode *node4  = storage1.add(4);
	StatNode *node5  = storage1.add(5);
	StatNode *node6  = storage1.add(6);

	node1->inc();
	TEST_NUMBER_EQUAL(1, node1->get());
	TEST_NUMBER_EQUAL(0, node2->get());
	TEST_NUMBER_EQUAL(0, node3->get());
	TEST_NUMBER_EQUAL(0, node4->get());
	node5->inc();
	node6->inc();
	TEST_NUMBER_EQUAL(2, node5->get());
	TEST_NUMBER_EQUAL(2, node6->get());
#endif
/*
	StatStorage storage2(data, sizeof(data));
	StatNode *node21 = storage2.add(1);
	StatNode *node25 = storage2.add(5);
	StatNode *node26 = storage2.add(6);
	TEST_NUMBER_EQUAL(1, node21->get());
	TEST_NUMBER_EQUAL(2, node25->get());
	TEST_NUMBER_EQUAL(2, node26->get());*/
	return true;
}
