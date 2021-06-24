#include "mdb/slave/bill_validator/MdbSlaveBillValidator.h"
#include "mdb/slave/MdbSlaveTester.h"
#include "event/include/TestEventEngine.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

class TestSlaveBillValidatorEventEngine : public TestEventEngine {
public:
	TestSlaveBillValidatorEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case MdbSlaveBillValidator::Event_Enable: procEvent(envelope, MdbSlaveBillValidator::Event_Enable, "Enable"); break;
		case MdbSlaveBillValidator::Event_Disable: procEvent(envelope, MdbSlaveBillValidator::Event_Disable, "Disable"); break;
		case MdbSlaveBillValidator::Event_Error: procEventError(envelope, MdbSlaveBillValidator::Event_Error, "Error"); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}

private:
	void procEventError(EventEnvelope *envelope, uint16_t type, const char *name) {
		Mdb::EventError event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.code << "," << event.data.getString() << ">";
	}
};

class MdbSlaveBillValidatorTest : public TestSet {
public:
	MdbSlaveBillValidatorTest();
	bool init();
	void cleanup();
	bool testInit();
	bool testDeposite();
	bool gotoStateDisabled();
	bool testSelfReset();
	bool testStateResetUnsupportedPacket();
	bool testStateResetUnwaitedPacket();

private:
	StringBuilder *result;
	TestSlaveBillValidatorEventEngine *eventEngine;
	MdbBillValidatorContext *context;
	MdbSlaveBillValidator *slave;
	MdbSlaveTester *tester;
};

TEST_SET_REGISTER(MdbSlaveBillValidatorTest);

MdbSlaveBillValidatorTest::MdbSlaveBillValidatorTest() {
	TEST_CASE_REGISTER(MdbSlaveBillValidatorTest, testInit);
	TEST_CASE_REGISTER(MdbSlaveBillValidatorTest, testDeposite);
	TEST_CASE_REGISTER(MdbSlaveBillValidatorTest, testSelfReset);
	TEST_CASE_REGISTER(MdbSlaveBillValidatorTest, testStateResetUnsupportedPacket);
	TEST_CASE_REGISTER(MdbSlaveBillValidatorTest, testStateResetUnwaitedPacket);
}

bool MdbSlaveBillValidatorTest::init() {
	result = new StringBuilder;
	eventEngine = new TestSlaveBillValidatorEventEngine(result);

	context = new MdbBillValidatorContext(2, NULL, 25000);
	context->setManufacturer((uint8_t*)MDB_MANUFACTURER_CODE, MDB_MANUFACTURER_SIZE);
	context->setModel((uint8_t*)"0123456789AB", MDB_MODEL_SIZE);
	context->setSerialNumber((uint8_t*)"0123456789AB", MDB_SERIAL_NUMBER_SIZE);
	context->setCurrency(RUSSIAN_CURRENCY_RUB);
	uint8_t billData[] = { 0x01, 0x05, 0x0A, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	context->init(1, 1, 100, billData, sizeof(billData));

	slave = new MdbSlaveBillValidator(context, eventEngine);
	tester = new MdbSlaveTester(slave);
	slave->reset();
	return true;
}

void MdbSlaveBillValidatorTest::cleanup() {
	delete tester;
	delete slave;
	delete context;
	delete eventEngine;
	delete result;
}

bool MdbSlaveBillValidatorTest::testInit() {
	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("30"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	TEST_HEXDATA_EQUAL("06", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	TEST_HEXDATA_EQUAL("06", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup
	TEST_NUMBER_EQUAL(true, tester->recvCommand("31"));
	TEST_HEXDATA_EQUAL("0116430064010000FFFFFF01050A32000000000000000000000000", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("31"));
	TEST_HEXDATA_EQUAL("0116430064010000FFFFFF01050A32000000000000000000000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("3700"));
	TEST_HEXDATA_EQUAL("4546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("3700"));
	TEST_HEXDATA_EQUAL("4546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Stacker
	TEST_NUMBER_EQUAL(true, tester->recvCommand("36"));
	TEST_HEXDATA_EQUAL("0000", tester->getSendData(), tester->getSendDataLen());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("36"));
	TEST_HEXDATA_EQUAL("0000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// BillType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("3400000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// BillType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("34000F0000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// BillType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("34000F0000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("", result->getString());

	return true;
}

bool MdbSlaveBillValidatorTest::gotoStateDisabled() {
	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("30"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	TEST_HEXDATA_EQUAL("06", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Setup
	TEST_NUMBER_EQUAL(true, tester->recvCommand("31"));
	TEST_HEXDATA_EQUAL("0116430064010000FFFFFF01050A32000000000000000000000000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("3700"));
	TEST_HEXDATA_EQUAL("4546523031323334353637383941423031323334353637383941420100", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Stacker
	TEST_NUMBER_EQUAL(true, tester->recvCommand("36"));
	TEST_HEXDATA_EQUAL("0000", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// BillType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("3400000000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	return true;
}

bool MdbSlaveBillValidatorTest::testDeposite() {
	TEST_NUMBER_EQUAL(true, gotoStateDisabled());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Deposite bill
	slave->deposite(0x92);
	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	TEST_HEXDATA_EQUAL("92", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// 	Escrow
	TEST_NUMBER_EQUAL(true, tester->recvCommand("3501"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	return true;
}

bool MdbSlaveBillValidatorTest::testSelfReset() {
	TEST_NUMBER_EQUAL(true, gotoStateDisabled());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	// Poll after reset
	slave->reset();
	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	TEST_HEXDATA_EQUAL("06", tester->getSendData(), tester->getSendDataLen());
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(0x00));

	// BillType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("34000F0000"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	TEST_HEXDATA_EQUAL("00", tester->getSendConfirm(), tester->getSendConfirmLen());

	return true;
}

bool MdbSlaveBillValidatorTest::testStateResetUnsupportedPacket() {
	// Unsupported packet
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0FAA"));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendDataLen());
	TEST_STRING_EQUAL("<event=1,Error,1283,sbv1*48*07AA*>", result->getString());
	return true;
}

bool MdbSlaveBillValidatorTest::testStateResetUnwaitedPacket() {
	// BillType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("34000F0000"));
	TEST_HEXDATA_EQUAL("", tester->getSendData(), tester->getSendDataLen());
	TEST_STRING_EQUAL("<event=1,Error,1283,sbv1*48*0400*34000F0000>", result->getString());
	return true;
}
