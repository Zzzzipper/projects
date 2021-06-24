#include "lib/sale_manager/mdb_slave/MdbSnifferBillValidator.h"

#include "common/mdb/slave/MdbSlaveTester.h"
#include "common/utils/include/Hex.h"
#include "common/timer/include/TestRealTime.h"
#include "common/event/include/TestEventEngine.h"
#include "common/logger/include/Logger.h"
#include "common/test/include/Test.h"

class TestSnifferBillValidatorEventEngine : public TestEventEngine {
public:
	TestSnifferBillValidatorEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case MdbSnifferBillValidator::Event_Enable: procEvent(envelope, MdbSnifferBillValidator::Event_Enable, "Enable"); break;
		case MdbSnifferBillValidator::Event_Disable: procEvent(envelope, MdbSnifferBillValidator::Event_Disable, "Disable"); break;
		case MdbSnifferBillValidator::Event_DepositeBill: procEventDeposite(envelope, MdbSnifferBillValidator::Event_DepositeBill, "DepositeBill"); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}

	void procEventDeposite(EventEnvelope *envelope, uint16_t type, const char *name) {
		Mdb::EventDeposite event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { *result << "<event-pack-error>"; return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.getRoute() << "," << event.getNominal() << ">";
	}
};

class MdbSnifferBillValidatorTest : public TestSet {
public:
	MdbSnifferBillValidatorTest();
	bool init();
	void cleanup();
	bool testTubeStatus();
	bool testEnableDisable();
	bool testDepositeBill();

private:
	StringBuilder *result;
	TestRealTime *realtime;
	TestSnifferBillValidatorEventEngine *eventEngine;
	MdbBillValidatorContext *context;
	MdbSlaveTester *tester;
	MdbSnifferBillValidator *sniffer;

	bool gotoStateSale();
	void recvResponse(const char *hex, bool crc);
};

TEST_SET_REGISTER(MdbSnifferBillValidatorTest);

MdbSnifferBillValidatorTest::MdbSnifferBillValidatorTest() {
	TEST_CASE_REGISTER(MdbSnifferBillValidatorTest, testTubeStatus);
	TEST_CASE_REGISTER(MdbSnifferBillValidatorTest, testEnableDisable);
	TEST_CASE_REGISTER(MdbSnifferBillValidatorTest, testDepositeBill);
}

bool MdbSnifferBillValidatorTest::init() {
	result = new StringBuilder;
	realtime = new TestRealTime;
	eventEngine = new TestSnifferBillValidatorEventEngine(result);
	context = new MdbBillValidatorContext(2, realtime, 25000);
	sniffer = new MdbSnifferBillValidator(context, eventEngine);
	tester = new MdbSlaveTester(sniffer);
	sniffer->reset();
	return true;
}

void MdbSnifferBillValidatorTest::cleanup() {
	delete tester;
	delete sniffer;
	delete context;
	delete eventEngine;
	delete realtime;
	delete result;
}

void MdbSnifferBillValidatorTest::recvResponse(const char *hex, bool crc) {
	uint8_t buf[256];
	uint16_t len =  hexToData(hex, strlen(hex), buf, sizeof(buf));
	sniffer->procResponse(buf, len ,crc);
}

bool MdbSnifferBillValidatorTest::testTubeStatus() {
	TEST_STRING_EQUAL("   ", context->getManufacturer());
	TEST_STRING_EQUAL("            ", context->getModel());
	TEST_STRING_EQUAL("            ", context->getSerialNumber());
	TEST_NUMBER_EQUAL(1643, context->getCurrency());
	TEST_NUMBER_EQUAL(2, context->getDecimalPoint());
	TEST_NUMBER_EQUAL(BV_STACKER_UNKNOWN, context->getBillInStacker());

	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("30"));

	TEST_NUMBER_EQUAL(true, tester->recvCommand("30"));
	recvResponse("00", true);

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));

	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	recvResponse("06", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));

	// Setup
	TEST_NUMBER_EQUAL(true, tester->recvCommand("31"));

	TEST_NUMBER_EQUAL(true, tester->recvCommand("31"));
	recvResponse("x01x00x07x00x64x01x00x00xFFxFFxFFx01x05x0Ax32x00x00x00x00x00x00x00x00x00x00x00x00", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("3700"));
	recvResponse("x4Ax4Fx46x34x33x32x36x20x20x20x20x20x20x20x20x31x31x4Dx44x52x55x5Fx5Fx30x30x30x34x00x25", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));

	// BillType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("3400000000"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	recvResponse("00", false);

	// Stacker
	TEST_NUMBER_EQUAL(true, tester->recvCommand("36"));
	recvResponse("015E", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));

	TEST_STRING_EQUAL("JOF", context->getManufacturer());
	TEST_STRING_EQUAL("11MDRU__0004", context->getModel());
	TEST_STRING_EQUAL("4326        ", context->getSerialNumber());
	TEST_NUMBER_EQUAL(7, context->getCurrency());
	TEST_NUMBER_EQUAL(1, context->getDecimalPoint());
	TEST_NUMBER_EQUAL(350, context->getBillInStacker());
	return true;
}

bool MdbSnifferBillValidatorTest::gotoStateSale() {
	TEST_STRING_EQUAL("   ", context->getManufacturer());
	TEST_STRING_EQUAL("            ", context->getModel());
	TEST_STRING_EQUAL("            ", context->getSerialNumber());
	TEST_NUMBER_EQUAL(1643, context->getCurrency());
	TEST_NUMBER_EQUAL(2, context->getDecimalPoint());
	TEST_NUMBER_EQUAL(BV_STACKER_UNKNOWN, context->getBillInStacker());

	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("30"));
	recvResponse("00", true);

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	recvResponse("06", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));

	// Setup
	TEST_NUMBER_EQUAL(true, tester->recvCommand("31"));
	recvResponse("x01x00x07x00x64x01x00x00xFFxFFxFFx01x05x0Ax32x00x00x00x00x00x00x00x00x00x00x00x00", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("3700"));
	recvResponse("x4Ax4Fx46x34x33x32x36x20x20x20x20x20x20x20x20x31x31x4Dx44x52x55x5Fx5Fx30x30x30x34x00x25", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));

	// BillType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("34000F0000"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	recvResponse("00", false);

	// Stacker
	TEST_NUMBER_EQUAL(true, tester->recvCommand("36"));
	recvResponse("015E", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));
	return true;
}

bool MdbSnifferBillValidatorTest::testEnableDisable() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// BillType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("3400000000"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,Disable>", result->getString());
	result->clear();

	// BillType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("3400000000"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("", result->getString());

	// BillType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("34000F0000"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// BillType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("34000F0000"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool MdbSnifferBillValidatorTest::testDepositeBill() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// DepositeBill
	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	recvResponse("3392", true);
	TEST_STRING_EQUAL("", result->getString());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("3501"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("", result->getString());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	recvResponse("09", true);
	TEST_STRING_EQUAL("", result->getString());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	recvResponse("09", true);
	TEST_STRING_EQUAL("", result->getString());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("33"));
	recvResponse("3382", true);
	TEST_STRING_EQUAL("<event=1,DepositeBill,1,10000>", result->getString());
	result->clear();

	// BillType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("3400000000"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,Disable>", result->getString());
	result->clear();

	// BillType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("34000F0000"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();
	return true;
}
