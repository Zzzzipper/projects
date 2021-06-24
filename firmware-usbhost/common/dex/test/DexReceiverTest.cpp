#include "dex/DexReceiver.h"
#include "dex/DexCrc.h"
#include "utils/include/StringBuilder.h"
#include "uart/include/TestUart.h"
#include "test/include/Test.h"

class TestReceiverCustomer : public Dex::Receiver::Customer {
public:
	void procControl(DexControl control) override {
		str << "<control:";
		str.addHex(control);
		str << ">";
	}

	void procData(const uint8_t *data, const uint16_t len, bool last) override {
		str << "<data:";
		for(uint16_t i = 0; i < len; i++) {
			str.addHex(data[i]);
		}
		str << ":" << last << ">";
	}

	void procConfirm(const uint8_t number) override {
		str << "<confirm:";
		str.addHex(number);
		str << ">";
	}

	void procWrongCrc() override {
		str << "<wrong-crc>";
	}

	const char *getStr() { return str.getString(); }
	void clearStr() { str.clear(); }

private:
	StringBuilder str;
};

class DexReceiverTest : public TestSet {
public:
	DexReceiverTest();
	bool init();
	void cleanup();
	bool test();

private:
	Dex::Crc *crc;
	Dex::Receiver *receiver;
	TestReceiverCustomer *customer;
	TestUart *uart;
};

TEST_SET_REGISTER(DexReceiverTest);

DexReceiverTest::DexReceiverTest() {
	TEST_CASE_REGISTER(DexReceiverTest, test);
}

bool DexReceiverTest::init() {
	crc = new Dex::Crc;
	uart = new TestUart(1024);
	customer = new TestReceiverCustomer;
	receiver = new Dex::Receiver(customer);
	receiver->setConnection(uart);
	receiver->reset();
	return true;
}

void DexReceiverTest::cleanup() {
	delete receiver;
	delete customer;
	delete uart;
	delete crc;
}

bool DexReceiverTest::test() {
	//recv ENQ
	uart->addRecvData(DexControl_ENQ);
	TEST_STRING_EQUAL("<control:05>", customer->getStr());
	customer->clearStr();

	//recv DLE,0
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData('0');
	TEST_STRING_EQUAL("<confirm:30>", customer->getStr());
	customer->clearStr();

	//recv DLE,SOH,AABBCCDDEEFF,DLE,ETX,CRC0,CRC1
	uint8_t data1[] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
	crc->start();
	crc->add(data1, sizeof(data1));
	crc->addUint8(DexControl_ETB);
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_SOH);
	uart->addRecvData(data1, sizeof(data1));
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_ETB);
	uart->addRecvData(crc->getLowByte());
	uart->addRecvData(crc->getHighByte());
	TEST_STRING_EQUAL("<data:AABBCCDDEEFF:0>", customer->getStr());
	customer->clearStr();

	//recv DLE,SOH,AABBCCDDEEFF,DLE,ETX,CRC0,CRC1
	uint8_t data2[] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
	crc->start();
	crc->add(data2, sizeof(data2));
	crc->addUint8(DexControl_ETX);
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_SOH);
	uart->addRecvData(data2, sizeof(data2));
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData(DexControl_ETX);
	uart->addRecvData(crc->getLowByte());
	uart->addRecvData(crc->getHighByte());
	TEST_STRING_EQUAL("<data:AABBCCDDEEFF:1>", customer->getStr());
	customer->clearStr();

	//send DLE,1
	uart->addRecvData(DexControl_DLE);
	uart->addRecvData('1');
	TEST_STRING_EQUAL("<confirm:31>", customer->getStr());
	customer->clearStr();

	//recv EOT
	uart->addRecvData(DexControl_EOT);
	TEST_STRING_EQUAL("<control:04>", customer->getStr());
	customer->clearStr();
	return true;
}
