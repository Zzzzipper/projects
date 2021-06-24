#include "mdb/slave/MdbSlaveReceiver.h"
#include "uart/include/TestUart.h"
#include "utils/include/StringBuilder.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class MdbSlaveReceiverTest : public TestSet {
public:
	MdbSlaveReceiverTest();
	bool test();
};

TEST_SET_REGISTER(MdbSlaveReceiverTest);

MdbSlaveReceiverTest::MdbSlaveReceiverTest() {
	TEST_CASE_REGISTER(MdbSlaveReceiverTest, test);
}

class MdbSlaveReceiverCustomerTest : public MdbSlaveReceiver::Customer {
public:
	virtual void recvAddress(const uint8_t address) {
		str.addStr("addr<");
		str.addHex(address);
		str.addStr(">");
	}

	virtual void recvSubcommand(const uint8_t subcommand) {
		str.addStr("sub<");
		str.addHex(subcommand);
		str.addStr(">");
	}

	virtual void recvRequest(const uint8_t *data, uint16_t len) {
		str.addStr("data<");
		for(uint16_t i = 0; i < len; i++) {
			str.addHex(data[i]);
		}
		str.addStr(">");
	}

	virtual void recvConfirm(uint8_t control) {
		str.addStr("confirm<");
		str.addHex(control);
		str.addStr(">");
	}

	void clear() {
		str.clear();
	}

	const char *get() {
		return str.getString();
	}

private:
	StringBuilder str;
};

bool MdbSlaveReceiverTest::test() {
	TestUart uart(256);
	MdbSlaveReceiverCustomerTest customer;
	MdbSlaveReceiver receiver(&customer);
	receiver.setUart(&uart);

	receiver.recvAddress();
	uart.addRecvData("0155");
	TEST_STRING_EQUAL("addr<55>", customer.get());

	customer.clear();
	receiver.recvSubcommand();
	uart.addRecvData("000100020003005B0007");
	TEST_STRING_EQUAL("sub<01>", customer.get());

	customer.clear();
	receiver.recvRequest(4);
	uart.execute();
	TEST_STRING_EQUAL("data<55010203>", customer.get());

	customer.clear();
	receiver.recvConfirm();
	uart.execute();
	TEST_STRING_EQUAL("confirm<07>", customer.get());

	customer.clear();
	receiver.recvAddress();
	uart.addRecvData("01770077");
	TEST_STRING_EQUAL("addr<77>", customer.get());

	customer.clear();
	receiver.recvConfirm();
	uart.addRecvData("0008");
	TEST_STRING_EQUAL("confirm<08>", customer.get());

	return true;
}
