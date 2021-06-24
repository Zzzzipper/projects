#include "lib/sale_manager/mdb_slave/MdbSnifferCoinChanger.h"

#include "common/mdb/slave/MdbSlaveTester.h"
#include "common/utils/include/Hex.h"
#include "common/timer/include/TestRealTime.h"
#include "common/event/include/TestEventEngine.h"
#include "common/logger/include/Logger.h"
#include "common/test/include/Test.h"

class TestSnifferCoinChangerEventEngine : public TestEventEngine {
public:
	TestSnifferCoinChangerEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case MdbSnifferCoinChanger::Event_Enable: procEvent(envelope, MdbSnifferCoinChanger::Event_Enable, "Enable"); break;
		case MdbSnifferCoinChanger::Event_Disable: procEvent(envelope, MdbSnifferCoinChanger::Event_Disable, "Disable"); break;
		case MdbSnifferCoinChanger::Event_TubeStatus: procEvent(envelope, MdbSnifferCoinChanger::Event_TubeStatus, "TubeStatus"); break;
		case MdbSnifferCoinChanger::Event_DepositeCoin: procEvent(envelope, MdbSnifferCoinChanger::Event_DepositeCoin, "DepositeCoin"); break;
		case MdbSnifferCoinChanger::Event_DispenseCoin: procEventDispense(envelope, MdbSnifferCoinChanger::Event_DispenseCoin, "DispenseCoin"); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}

private:
	void procEventDispense(EventEnvelope *envelope, uint16_t type, const char *name) {
		EventUint32Interface event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { *result << "<event-pack-error>"; return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.getValue() << ">";
	}
};

class MdbSnifferCoinChangerTest : public TestSet {
public:
	MdbSnifferCoinChangerTest();
	bool init();
	void cleanup();
	bool testTubeStatus();
	bool testEnableDisable();
	bool testCoin();
	bool testDispsense();

private:
	StringBuilder *result;
	TestRealTime *realtime;
	TestSnifferCoinChangerEventEngine *eventEngine;
	MdbCoinChangerContext *context;
	MdbSlaveTester *tester;
	MdbSnifferCoinChanger *sniffer;

	bool gotoStateSale();
	void recvResponse(const char *hex, bool crc);
};

TEST_SET_REGISTER(MdbSnifferCoinChangerTest);

MdbSnifferCoinChangerTest::MdbSnifferCoinChangerTest() {
	TEST_CASE_REGISTER(MdbSnifferCoinChangerTest, testTubeStatus);
	TEST_CASE_REGISTER(MdbSnifferCoinChangerTest, testEnableDisable);
	TEST_CASE_REGISTER(MdbSnifferCoinChangerTest, testCoin);
	TEST_CASE_REGISTER(MdbSnifferCoinChangerTest, testDispsense);
}

bool MdbSnifferCoinChangerTest::init() {
	result = new StringBuilder;
	realtime = new TestRealTime;
	eventEngine = new TestSnifferCoinChangerEventEngine(result);
	context = new MdbCoinChangerContext(2, realtime);
	sniffer = new MdbSnifferCoinChanger(context, eventEngine);
	tester = new MdbSlaveTester(sniffer);
	sniffer->reset();
	return true;
}

void MdbSnifferCoinChangerTest::cleanup() {
	delete tester;
	delete sniffer;
	delete context;
	delete eventEngine;
	delete realtime;
	delete result;
}

void MdbSnifferCoinChangerTest::recvResponse(const char *hex, bool crc) {
	uint8_t buf[256];
	uint16_t len =  hexToData(hex, strlen(hex), buf, sizeof(buf));
	sniffer->procResponse(buf, len ,crc);
}

bool MdbSnifferCoinChangerTest::testTubeStatus() {
	TEST_STRING_EQUAL("   ", context->getManufacturer());
	TEST_STRING_EQUAL("            ", context->getModel());
	TEST_STRING_EQUAL("            ", context->getSerialNumber());
	TEST_NUMBER_EQUAL(1643, context->getCurrency());
	TEST_NUMBER_EQUAL(2, context->getDecimalPoint());
	TEST_NUMBER_EQUAL(0, context->getInTubeValue());

	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("08"));

	TEST_NUMBER_EQUAL(true, tester->recvCommand("08"));
	recvResponse("00", true);

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));

	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	recvResponse("0B", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));

	// Setup
	TEST_NUMBER_EQUAL(true, tester->recvCommand("09"));

	TEST_NUMBER_EQUAL(true, tester->recvCommand("09"));
	recvResponse("x03x00x07x0Ax01x00x5Cx00x00x01x02x05x0Ax0AxFFx00x00x00x00x00x00x00x00", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F00"));
	recvResponse(
		"x4Ax4Fx46x30x35x33x36x37x39x20x20x20x20x20x20x4A"
		"x32x30x30x30x4Dx44x42x20x50x33x32x90x10x00x00x00x01", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));

	// ExpansionFeatureEnable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F010000000F"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("", result->getString());

	// TubeStatus
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0A"));
	recvResponse("x00x40x00x00x41x37x57x00x00x00x00x00x00x00x00x00x00x00", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));
	TEST_STRING_EQUAL("", result->getString());

	// CoinType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0C00000000"));
	recvResponse("00", false);

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	recvResponse("00", false);

	TEST_STRING_EQUAL("JOF", context->getManufacturer());
	TEST_STRING_EQUAL("J2000MDB P32", context->getModel());
	TEST_STRING_EQUAL("053679      ", context->getSerialNumber());
	TEST_NUMBER_EQUAL(7, context->getCurrency());
	TEST_NUMBER_EQUAL(1, context->getDecimalPoint());
	TEST_NUMBER_EQUAL(61000, context->getInTubeValue());
	return true;
}

bool MdbSnifferCoinChangerTest::gotoStateSale() {
	TEST_STRING_EQUAL("   ", context->getManufacturer());
	TEST_STRING_EQUAL("            ", context->getModel());
	TEST_STRING_EQUAL("            ", context->getSerialNumber());
	TEST_NUMBER_EQUAL(1643, context->getCurrency());
	TEST_NUMBER_EQUAL(2, context->getDecimalPoint());
	TEST_NUMBER_EQUAL(0, context->getInTubeValue());

	// Reset
	TEST_NUMBER_EQUAL(true, tester->recvCommand("08"));
	recvResponse("00", true);

	// Poll (wait JustReset)
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	recvResponse("0B", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));

	// Setup
	TEST_NUMBER_EQUAL(true, tester->recvCommand("09"));
	recvResponse("x03x00x07x0Ax01x00x5Cx00x00x01x02x05x0Ax0AxFFx00x00x00x00x00x00x00x00", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));

	// ExpansionIdentification
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F00"));
	recvResponse(
		"x4Ax4Fx46x30x35x33x36x37x39x20x20x20x20x20x20x4A"
		"x32x30x30x30x4Dx44x42x20x50x33x32x90x10x00x00x00x01", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));

	// ExpansionFeatureEnable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F010000000F"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("", result->getString());

	// TubeStatus
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0A"));
	recvResponse("x00x40x00x00x41x37x57x00x00x00x00x00x00x00x00x00x00x00", true);
	TEST_NUMBER_EQUAL(true, tester->recvConfirm(Mdb::Control_ACK));
	TEST_STRING_EQUAL("", result->getString());

	// CoinType
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0CFFFF0000"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Poll
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	recvResponse("00", false);
	return true;
}

bool MdbSnifferCoinChangerTest::testEnableDisable() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Disable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0C00000000"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,Disable>", result->getString());
	result->clear();

	// Disable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0C00000000"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("", result->getString());

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0CFFFF0000"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0CFFFF0000"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool MdbSnifferCoinChangerTest::testCoin() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Coin
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	recvResponse("4657", true);
	TEST_STRING_EQUAL("<event=1,DepositeCoin>", result->getString());
	result->clear();

	// Disable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0C00000000"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,Disable>", result->getString());
	result->clear();

	// Enable
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0CFFFF0000"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,Enable>", result->getString());
	result->clear();

	// Dispense
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F02C8"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,DispenseCoin,20000>", result->getString());
	result->clear();

	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F04"));
	recvResponse("02", true);
	TEST_STRING_EQUAL("", result->getString());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F04"));
	recvResponse("00", true);
	TEST_STRING_EQUAL("", result->getString());

	TEST_NUMBER_EQUAL(true, tester->recvCommand("0F03"));
	recvResponse("02", true);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

bool MdbSnifferCoinChangerTest::testDispsense() {
	TEST_NUMBER_EQUAL(true, gotoStateSale());

	// Dispense
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0D45"));
	recvResponse("00", false);
	TEST_STRING_EQUAL("<event=1,DispenseCoin,4000>", result->getString());
	result->clear();

	// Dispense wait
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	recvResponse("02", true);
	TEST_STRING_EQUAL("", result->getString());

	// Dispense complete
	TEST_NUMBER_EQUAL(true, tester->recvCommand("0B"));
	recvResponse("00", true);
	TEST_STRING_EQUAL("", result->getString());
	return true;
}
