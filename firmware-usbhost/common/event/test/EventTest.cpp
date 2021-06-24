#include "event/include/Event2.h"
#include "event/include/EventEngine.h"
#include "utils/include/NetworkProtocol.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class EventTest : public TestSet {
public:
	EventTest();
	bool testEnvelope();
	bool testUsage();

private:
};

TEST_SET_REGISTER(EventTest);

EventTest::EventTest() {
	TEST_CASE_REGISTER(EventTest, testEnvelope);
	TEST_CASE_REGISTER(EventTest, testUsage);
}

bool EventTest::testEnvelope() {
	EventEnvelope envelope(200);
	envelope.setType(369);
	TEST_NUMBER_EQUAL(true, envelope.addUint16(1, 1234));
	TEST_NUMBER_EQUAL(true, envelope.addUint32(2, 12345678));
	TEST_NUMBER_EQUAL(true, envelope.addString(3, "Test string"));
	uint8_t data1[] = { 0xF0, 0xF1, 0xF2, 0xF3 };
	TEST_NUMBER_EQUAL(true, envelope.addData(4, data1, sizeof(data1)));

	TEST_NUMBER_EQUAL(369, envelope.getType());
	uint16_t param1 = 0;
	TEST_NUMBER_EQUAL(true, envelope.getUint16(1, &param1));
	TEST_NUMBER_EQUAL(1234, param1);
	uint32_t param2 = 0;
	TEST_NUMBER_EQUAL(true, envelope.getUint32(2, &param2));
	TEST_NUMBER_EQUAL(12345678, param2);
	StrParam<20> param3;
	TEST_NUMBER_EQUAL(true, envelope.getString(3, param3.get(), param3.getSize()));
	TEST_STRING_EQUAL("Test string", param3.get());
	uint8_t param4[10];
	uint16_t param4Len = 0;
	TEST_NUMBER_EQUAL(true, envelope.getData(4, param4, sizeof(param4), &param4Len));
	TEST_NUMBER_EQUAL(4, param4Len);
	TEST_HEXDATA_EQUAL("F0F1F2F3", param4, param4Len);
	return true;
}

class EventExample : public EventInterface {
public:
	enum ParamId {
		ParamId_type = 1234,
		ParamId_param1,
		ParamId_param2,
		ParamId_param3,
		ParamId_param4
	};

	EventExample() : EventInterface(ParamId_type) {}
	EventExample(uint8_t param1, uint16_t param2, uint32_t param3, const char *param4) :
		EventInterface(ParamId_type), param1(param1), param2(param2), param3(param3), param4(param4) {}
	uint8_t getParam1() { return param1; }
	uint16_t getParam2() { return param2; }
	uint32_t getParam3() { return param3; }
	const char *getParam4() { return param4.get(); }

	virtual bool open(EventEnvelope *envelope) {
		if(envelope->getType() != ParamId_type) { return false; }
		if(envelope->getUint8(ParamId_param1, &param1) == false) { return false; }
		if(envelope->getUint16(ParamId_param2, &param2) == false) { return false; }
		if(envelope->getUint32(ParamId_param3, &param3) == false) { return false; }
		if(envelope->getString(ParamId_param4, param4.get(), param4.getSize()) == false) { return false; }
		return true;
	}
	virtual bool pack(EventEnvelope *envelope) {
		envelope->clear();
		envelope->setType(ParamId_type);
		if(envelope->addUint8(ParamId_param1, param1) == false) { return false; }
		if(envelope->addUint16(ParamId_param2, param2) == false) { return false; }
		if(envelope->addUint32(ParamId_param3, param3) == false) { return false; }
		if(envelope->addString(ParamId_param4, param4.get()) == false) { return false; }
		return true;
	}

private:
	uint8_t param1;
	uint16_t param2;
	uint32_t param3;
	StrParam<20> param4;
};

bool EventTest::testUsage() {
	EventEnvelope envelope(200);

	// error
	EventExample example1;
	TEST_NUMBER_EQUAL(false, example1.open(&envelope));

	// save
	EventExample example2(255, 1234, 12345678, "Test text");
	TEST_NUMBER_EQUAL(true, example2.pack(&envelope));

	// load
	EventExample example3;
	TEST_NUMBER_EQUAL(true, example3.open(&envelope));
	TEST_NUMBER_EQUAL(255, example3.getParam1());
	TEST_NUMBER_EQUAL(1234, example3.getParam2());
	TEST_NUMBER_EQUAL(12345678, example3.getParam3());
	TEST_STRING_EQUAL("Test text", example3.getParam4());
	return true;
}
