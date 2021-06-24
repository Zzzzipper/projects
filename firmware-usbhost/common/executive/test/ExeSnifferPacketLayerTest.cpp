#include "executive/ExeSnifferPacketLayer.h"
#include "uart/include/TestUart.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class ExeSnifferPacketLayerTest : public TestSet {
public:
	ExeSnifferPacketLayerTest();
	bool test();
};

TEST_SET_REGISTER(ExeSnifferPacketLayerTest);

ExeSnifferPacketLayerTest::ExeSnifferPacketLayerTest() {
	TEST_CASE_REGISTER(ExeSnifferPacketLayerTest, test);
}

class TestExeSnifferPacketLayerCustomer : public ExeSnifferPacketLayer::Customer {
public:
	virtual void recvByte(uint8_t req, uint8_t resp) {
		str << "<req=";	str.addHex(req);
		str << ", resp="; str.addHex(resp);
		str << ">";
	}

	virtual void recvData(uint8_t *data, uint16_t dataLen) {
		str << "<data=";
		for(uint16_t i = 0; i < dataLen; i++) {
			str.addHex(data[i]);
		}
		str << ">";
	}

	void clear() { str.clear(); }

	const char *get() { return str.getString(); }

private:
	StringBuilder str;
};

bool ExeSnifferPacketLayerTest::test() {
	TestExeSnifferPacketLayerCustomer customer;
	StatStorage stat;
	ExeSnifferPacketLayer layer(&customer, &stat);

	// byte
	layer.recvRequest(0x31);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("<req=01, resp=00>", customer.get());

	// data block
	customer.clear();
	layer.recvRequest(0x38);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x2A);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x20);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x20);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x20);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x21);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x20);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x21);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x21);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x39);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("<data=2A20202021202121>", customer.get());

	// wrong device
	customer.clear();
	layer.recvRequest(0x51);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());

	// wrong command flag
	customer.clear();
	layer.recvRequest(0x21);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());

	// receive PNAK
	customer.clear();
	layer.recvRequest(0x31);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvResponse(0xFF);
	TEST_STRING_EQUAL("", customer.get());

	// wrong data lenght
	customer.clear();
	layer.recvRequest(0x38);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x2A);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x20);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x20);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x20);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x20);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x21);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x21);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x39);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());

	// wrong finish symbol
	customer.clear();
	layer.recvRequest(0x38);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x2A);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x20);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x20);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x20);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x21);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x20);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x21);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x21);
	layer.recvResponse(0x00);
	TEST_STRING_EQUAL("", customer.get());
	layer.recvRequest(0x31);
	layer.recvResponse(0xFE);
	TEST_STRING_EQUAL("", customer.get());

	return true;
}
