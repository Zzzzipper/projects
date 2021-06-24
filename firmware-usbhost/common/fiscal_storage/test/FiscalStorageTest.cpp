#include "fiscal_storage/include/FiscalStorage.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class FiscalStorageTest : public TestSet {
public:
	FiscalStorageTest();
	bool test();
};

TEST_SET_REGISTER(FiscalStorageTest);

FiscalStorageTest::FiscalStorageTest() {
	TEST_CASE_REGISTER(FiscalStorageTest, test);
}

bool FiscalStorageTest::test() {
	Buffer buf(256);

	addTlvHeader(FiscalStorage::Tag_Product, 10, &buf);
	TEST_HEXDATA_EQUAL("23040A00", buf.getData(), buf.getLen());
	buf.clear();

	addTlvUint32(FiscalStorage::Tag_ProductPrice, 2500, &buf);
	TEST_HEXDATA_EQUAL("37040200C409", buf.getData(), buf.getLen());
	buf.clear();

	addTlvFUint32(FiscalStorage::Tag_ProductNumber, 0, 1, &buf);
	TEST_HEXDATA_EQUAL("FF0302000001", buf.getData(), buf.getLen());
	buf.clear();

	addTlvString(FiscalStorage::Tag_ProductName, "01234567890", &buf);
	TEST_HEXDATA_EQUAL("06040B003031323334353637383930", buf.getData(), buf.getLen());
	buf.clear();
	return true;
}
