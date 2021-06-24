#include "ccicsi/CciCsiCommandLayer.h"
#include "ccicsi/CciCsiProtocol.h"
#include "ccicsi/test/TestCciCsiPacketLayer.h"
#include "utils/include/Hex.h"
#include "timer/include/TimerEngine.h"
#include "timer/include/TestRealTime.h"
#include "event/include/TestEventEngine.h"
#include "mdb/slave/cashless/MdbSlaveCashless3.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

namespace CciCsi {

class TestSlaveCashlessEventEngine : public TestEventEngine {
public:
	TestSlaveCashlessEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case MdbSlaveCashlessInterface::Event_Reset: procEvent(envelope, MdbSlaveCashlessInterface::Event_Reset, "Reset"); break;
		case MdbSlaveCashlessInterface::Event_Enable: procEvent(envelope, MdbSlaveCashlessInterface::Event_Enable, "Enable"); break;
		case MdbSlaveCashlessInterface::Event_Disable: procEvent(envelope, MdbSlaveCashlessInterface::Event_Disable, "Disable"); break;
		case MdbSlaveCashlessInterface::Event_VendRequest: procEventVendRequest(envelope, MdbSlaveCashlessInterface::Event_VendRequest, "VendRequest"); break;
		case MdbSlaveCashlessInterface::Event_VendComplete: procEvent(envelope, MdbSlaveCashlessInterface::Event_VendComplete, "VendComplete"); break;
		case MdbSlaveCashlessInterface::Event_VendCancel: procEvent(envelope, MdbSlaveCashlessInterface::Event_VendCancel, "VendCancel"); break;
		case MdbSlaveCashlessInterface::Event_CashSale: procEventVendRequest(envelope, MdbSlaveCashlessInterface::Event_CashSale, "CashSale"); break;
		case MdbSlaveCashlessInterface::Event_Error: procEventError(envelope, MdbSlaveCashlessInterface::Event_Error, "Error"); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}

private:
	void procEventVendRequest(EventEnvelope *envelope, uint16_t type, const char *name) {
		MdbSlaveCashlessInterface::EventVendRequest event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.getProductId() << "," << event.getPrice() << ">";
	}

	void procEventError(EventEnvelope *envelope, uint16_t type, const char *name) {
		Mdb::EventError event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.code << "," << event.data.getString() << ">";
	}
};

class CommandLayerTest : public TestSet {
public:
	CommandLayerTest();
	bool init();
	void cleanup();
	bool testPayment();
	bool testStateApprovingChangeCashlessId();
	bool testStateApprovingCancelPayment();
	bool testStateApproveChangeCashlessId();

private:
	StringBuilder *result;
	TestRealTime *realtime;
	Mdb::DeviceContext *context;
	TestPacketLayer *packetLayer;
	TimerEngine *timerEngine;
	TestSlaveCashlessEventEngine *eventEngine;
	CommandLayer *commandLayer;
};

TEST_SET_REGISTER(CciCsi::CommandLayerTest);

CommandLayerTest::CommandLayerTest() {
	TEST_CASE_REGISTER(CciCsi::CommandLayerTest, testPayment);
	TEST_CASE_REGISTER(CciCsi::CommandLayerTest, testStateApprovingChangeCashlessId);
	TEST_CASE_REGISTER(CciCsi::CommandLayerTest, testStateApprovingCancelPayment);
	TEST_CASE_REGISTER(CciCsi::CommandLayerTest, testStateApproveChangeCashlessId);
}

bool CommandLayerTest::init() {
	result = new StringBuilder;
	realtime = new TestRealTime;
	context = new Mdb::DeviceContext(2, realtime);
	packetLayer = new TestPacketLayer;
	timerEngine = new TimerEngine();
	eventEngine = new TestSlaveCashlessEventEngine(result);
	commandLayer = new CommandLayer(packetLayer, timerEngine, eventEngine);
	return true;
}

void CommandLayerTest::cleanup() {
	delete commandLayer;
	delete eventEngine;
	delete timerEngine;
	delete packetLayer;
	delete context;
	delete realtime;
	delete result;
}

//todo: timeout ожидания продажи
//todo: test на deny с timeout'ом ожидания отмены
bool CommandLayerTest::testPayment() {
	commandLayer->reset();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// identification
	packetLayer->recvPacket("x58");
	TEST_STRING_EQUAL("06583136303230373034", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// status => NotReady
	packetLayer->recvPacket("x53");
	TEST_STRING_EQUAL("065330903000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// status => Ready
	commandLayer->setCredit(10000);
	packetLayer->recvPacket("x53");
	TEST_STRING_EQUAL("065331903000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// inquiry(id=201) => NotEnoughReady
	packetLayer->recvPacket("x49x32x30x31x31");
	TEST_STRING_EQUAL("064930", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendRequest,201,0>", result->getString());
	result->clear();

	// inquiry(id=201) => NotEnoughReady
	packetLayer->recvPacket("x49x32x30x31x31");
	TEST_STRING_EQUAL("064930", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// inquiry(id=201) => Approve
	commandLayer->approveVend(0);
	packetLayer->recvPacket("x49x32x30x31x31");
	TEST_STRING_EQUAL("064931", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// status => Ready
	packetLayer->recvPacket("x53");
	TEST_STRING_EQUAL("065331903000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendComplete>", result->getString());
	result->clear();
	return true;
}

bool CommandLayerTest::testStateApprovingChangeCashlessId() {
	commandLayer->reset();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// identification
	packetLayer->recvPacket("x58");
	TEST_STRING_EQUAL("06583136303230373034", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// status => NotReady
	packetLayer->recvPacket("x53");
	TEST_STRING_EQUAL("065330903000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// status => Ready
	commandLayer->setCredit(10000);
	packetLayer->recvPacket("x53");
	TEST_STRING_EQUAL("065331903000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// inquiry(id=201) => NotEnoughReady
	packetLayer->recvPacket("x49x32x30x31x31");
	TEST_STRING_EQUAL("064930", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendRequest,201,0>", result->getString());
	result->clear();

	// inquiry(id=201) => NotEnoughReady
	packetLayer->recvPacket("x49x32x30x31x31");
	TEST_STRING_EQUAL("064930", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// inquiry(id=203) => NotEnoughReady
	packetLayer->recvPacket("x49x32x30x33x31");
	TEST_STRING_EQUAL("064930", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendCancel>", result->getString());
	result->clear();

	// status => Ready
	packetLayer->recvPacket("x53");
	TEST_STRING_EQUAL("065330903000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// inquiry(id=207) => NotEnoughReady
	commandLayer->setCredit(10000);
	packetLayer->recvPacket("x49x32x30x37x31");
	TEST_STRING_EQUAL("064930", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendRequest,207,0>", result->getString());
	result->clear();
	return true;
}

bool CommandLayerTest::testStateApprovingCancelPayment() {
	commandLayer->reset();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// identification
	packetLayer->recvPacket("x58");
	TEST_STRING_EQUAL("06583136303230373034", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// status => NotReady
	packetLayer->recvPacket("x53");
	TEST_STRING_EQUAL("065330903000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// status => Ready
	commandLayer->setCredit(10000);
	packetLayer->recvPacket("x53");
	TEST_STRING_EQUAL("065331903000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// inquiry(id=201) => NotEnoughReady
	packetLayer->recvPacket("x49x32x30x31x31");
	TEST_STRING_EQUAL("064930", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendRequest,201,0>", result->getString());
	result->clear();

	// inquiry(id=201) => NotEnoughReady
	packetLayer->recvPacket("x49x32x30x31x31");
	TEST_STRING_EQUAL("064930", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	for(uint16_t i = 0; i < 9; i++) {
		// status => Ready
		packetLayer->recvPacket("x53");
		TEST_STRING_EQUAL("065331903000", packetLayer->getSendData());
		packetLayer->clearSendData();
		TEST_STRING_EQUAL("", result->getString());
	}

	// status => Ready
	packetLayer->recvPacket("x53");
	TEST_STRING_EQUAL("065331903000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendCancel>", result->getString());
	result->clear();

	// status => Ready
	packetLayer->recvPacket("x53");
	TEST_STRING_EQUAL("065330903000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// inquiry(id=207) => NotEnoughReady
	commandLayer->setCredit(10000);
	packetLayer->recvPacket("x49x32x30x37x31");
	TEST_STRING_EQUAL("064930", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendRequest,207,0>", result->getString());
	result->clear();
	return true;
}

bool CommandLayerTest::testStateApproveChangeCashlessId() {
	commandLayer->reset();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// identification
	packetLayer->recvPacket("x58");
	TEST_STRING_EQUAL("06583136303230373034", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// status => NotReady
	packetLayer->recvPacket("x53");
	TEST_STRING_EQUAL("065330903000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// status => Ready
	commandLayer->setCredit(10000);
	packetLayer->recvPacket("x53");
	TEST_STRING_EQUAL("065331903000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// inquiry(id=201) => NotEnoughReady
	packetLayer->recvPacket("x49x32x30x31x31");
	TEST_STRING_EQUAL("064930", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendRequest,201,0>", result->getString());
	result->clear();

	// inquiry(id=201) => NotEnoughReady
	packetLayer->recvPacket("x49x32x30x31x31");
	TEST_STRING_EQUAL("064930", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// inquiry(id=207) => Approve
	commandLayer->approveVend(0);
	packetLayer->recvPacket("x49x32x30x37x31");
	TEST_STRING_EQUAL("064930", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendCancel>", result->getString());
	result->clear();

	// status => Ready
	packetLayer->recvPacket("x53");
	TEST_STRING_EQUAL("065330903000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// inquiry(id=207) => NotEnoughReady
	commandLayer->setCredit(10000);
	packetLayer->recvPacket("x49x32x30x37x31");
	TEST_STRING_EQUAL("064930", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendRequest,207,0>", result->getString());
	result->clear();
	return true;
}

}
