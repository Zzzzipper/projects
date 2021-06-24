#include "lib/sale_manager/cci_franke/CciFrankeCommandLayer.h"

#include "common/ccicsi/test/TestCciCsiPacketLayer.h"
#include "common/mdb/slave/cashless/MdbSlaveCashless3.h"
#include "common/utils/include/Hex.h"
#include "common/timer/include/TimerEngine.h"
#include "common/timer/include/TestRealTime.h"
#include "common/event/include/TestEventEngine.h"
#include "common/test/include/Test.h"
#include "common/logger/include/Logger.h"

namespace Cci {
namespace Franke {

class TestCommandLayerEventEngine : public TestEventEngine {
public:
	TestCommandLayerEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case MdbSlaveCashlessInterface::Event_Enable: procEvent(envelope, MdbSlaveCashlessInterface::Event_Enable, "Enable"); break;
		case MdbSlaveCashlessInterface::Event_Disable: procEvent(envelope, MdbSlaveCashlessInterface::Event_Disable, "Disable"); break;
		case MdbSlaveCashlessInterface::Event_VendRequest: procEventVendRequest(envelope, MdbSlaveCashlessInterface::Event_VendRequest, "VendRequest"); break;
		case MdbSlaveCashlessInterface::Event_VendComplete: procEvent(envelope, MdbSlaveCashlessInterface::Event_VendComplete, "VendComplete"); break;
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
};

class CommandLayerTest : public TestSet {
public:
	CommandLayerTest();
	bool init();
	void cleanup();
	bool testPayment();
#if 0
	bool testStateApprovingChangeCashlessId();
	bool testStateApprovingCancelPayment();
	bool testStateApproveChangeCashlessId();
#endif

private:
	StringBuilder *result;
	TestRealTime *realtime;
	Mdb::DeviceContext *context;
	CciCsi::TestPacketLayer *packetLayer;
	TimerEngine *timerEngine;
	TestCommandLayerEventEngine *eventEngine;
	CommandLayer *commandLayer;
};

TEST_SET_REGISTER(Cci::Franke::CommandLayerTest);

CommandLayerTest::CommandLayerTest() {
	TEST_CASE_REGISTER(Cci::Franke::CommandLayerTest, testPayment);
#if 0
	TEST_CASE_REGISTER(CciCsi::CommandLayerTest, testStateApprovingChangeCashlessId);
	TEST_CASE_REGISTER(CciCsi::CommandLayerTest, testStateApprovingCancelPayment);
	TEST_CASE_REGISTER(CciCsi::CommandLayerTest, testStateApproveChangeCashlessId);
#endif
}

bool CommandLayerTest::init() {
	result = new StringBuilder;
	realtime = new TestRealTime;
	context = new Mdb::DeviceContext(2, realtime);
	packetLayer = new CciCsi::TestPacketLayer;
	timerEngine = new TimerEngine();
	eventEngine = new TestCommandLayerEventEngine(result);
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

/*
x53;
D 18:47:53 CciFrankeCommandLayer.cpp#108 stateWaitRequest
D 18:47:54 CciFrankeCommandLayer.cpp#95 procPacket
x58;
D 18:47:54 CciFrankeCommandLayer.cpp#108 stateWaitRequest
D 18:47:54 CciFrankeCommandLayer.cpp#95 procPacket
x56;x31;
D 18:47:54 CciFrankeCommandLayer.cpp#108 stateWaitRequest
D 18:47:54 CciFrankeCommandLayer.cpp#292 sendBillingEnable
D 18:47:55 CciFrankeCommandLayer.cpp#95 procPacket
x54;x34;x46;x32;x44;x32;x43;x39;x45;
D 18:47:55 CciFrankeCommandLayer.cpp#108 stateWaitRequest
D 18:47:55 CciFrankeCommandLayer.cpp#278 sendButtons 0
D 18:47:55 CciFrankeCommandLayer.cpp#95 procPacket
x4D;x31;x30;x80;
D 18:47:55 CciFrankeCommandLayer.cpp#108 stateWaitRequest
D 18:47:55 CciFrankeCommandLayer.cpp#311 sendMachineMode
D 18:47:56 CciFrankeCommandLayer.cpp#95 procPacket
x53;
D 18:47:56 CciFrankeCommandLayer.cpp#108 stateWaitRequest
D 18:47:56 CciFrankeCommandLayer.cpp#95 procPacket
x44;x33;x30;x31;x32;x4D;x41;x35;x2A;x43;x50;x33;x30;x2A;x30;x0D;x0A;
D 18:47:56 CciFrankeCommandLayer.cpp#108 stateWaitRequest
D 18:47:56 CciFrankeCommandLayer.cpp#341 sendTelemetry
D 18:47:57 CciFrankeCommandLayer.cpp#95 procPacket
x44;x32;x30;x32;x35;x45;x44;x30;x31;x38;x2A;x32;x30;x31;x39;x31;x32;x32;x32;x2A;x31;x38;x35;x30;x33;x30;x2A;x31;x0D;x0A;
D 18:47:57 CciFrankeCommandLayer.cpp#108 stateWaitRequest
D 18:47:57 CciFrankeCommandLayer.cpp#341 sendTelemetry
D 18:47:57 CciFrankeCommandLayer.cpp#95 procPacket
x44;x33;x30;x30;x30;
D 18:47:57 CciFrankeCommandLayer.cpp#108 stateWaitRequest
D 18:47:57 CciFrankeCommandLayer.cpp#341 sendTelemetry
D 18:47:58 CciFrankeCommandLayer.cpp#95 procPacket
x44;x32;x30;x32;x35;x45;x44;x30;x31;x38;x2A;x32;x30;x31;x39;x31;x32;x32;x32;x2A;x31;x38;x35;x30;x33;x30;x2A;x30;x0D;x0A;
D 18:47:58 CciFrankeCommandLayer.cpp#108 stateWaitRequest
D 18:47:58 CciFrankeCommandLayer.cpp#341 sendTelemetry
D 18:47:58 CciFrankeCommandLayer.cpp#95 procPacket
x53;
D 18:47:58 CciFrankeCommandLayer.cpp#108 stateWaitRequest
D 18:47:59 CciFrankeCommandLayer.cpp#95 procPacket
x53;
D 18:47:59 CciFrankeCommandLayer.cpp#108 stateWaitRequest
D 18:47:59 CciFrankeCommandLayer.cpp#95 procPacket
x53;
D 18:47:59 CciFrankeCommandLayer.cpp#108 stateWaitRequest
D 18:48:00 CciFrankeCommandLayer.cpp#95 procPacket
x53;

 */
bool CommandLayerTest::testPayment() {
	commandLayer->reset();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// Status
	packetLayer->recvPacket("53");
	TEST_STRING_EQUAL("06533180A880", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// Identification
	packetLayer->recvPacket("x58");
	TEST_STRING_EQUAL("06583236303230373034", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// BillingEnable
	packetLayer->recvPacket("5631");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ButtonEnable(enable all)
	packetLayer->recvPacket("543446324431414235");
	TEST_STRING_EQUAL("0654393030303030303030", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// Machine mode
	packetLayer->recvPacket("4D313080");
	TEST_STRING_EQUAL("064D8080", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// Status
	packetLayer->recvPacket("53");
	TEST_STRING_EQUAL("06533180A880", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());
	return true;
}

}
}
