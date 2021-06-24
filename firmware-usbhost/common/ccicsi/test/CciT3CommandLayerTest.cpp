#include "ccicsi/CciT3CommandLayer.h"
#include "ccicsi/test/TestCciCsiPacketLayer.h"
#include "mdb/slave/cashless/MdbSlaveCashless3.h"
#include "utils/include/Hex.h"
#include "timer/include/TimerEngine.h"
#include "timer/include/TestRealTime.h"
#include "event/include/TestEventEngine.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

namespace Cci {
namespace T3 {

class TestCommandLayerEventEngine : public TestEventEngine {
public:
	TestCommandLayerEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case OrderDeviceInterface::Event_VendRequest: procEventVendRequest(envelope, OrderDeviceInterface::Event_VendRequest, "Request"); break;
		case OrderDeviceInterface::Event_VendCompleted: procEvent(envelope, OrderDeviceInterface::Event_VendCompleted, "Completed"); break;
		case OrderDeviceInterface::Event_VendCancelled: procEvent(envelope, OrderDeviceInterface::Event_VendCancelled, "Cancelled"); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}

private:
	void procEventVendRequest(EventEnvelope *envelope, uint16_t type, const char *name) {
		EventUint16Interface event(type);
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.getValue() << ">";
	}
};

class CommandLayerTest : public TestSet {
public:
	CommandLayerTest();
	bool init();
	void cleanup();
	bool testProductState();
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
	Order *order;
};

TEST_SET_REGISTER(Cci::T3::CommandLayerTest);

CommandLayerTest::CommandLayerTest() {
	TEST_CASE_REGISTER(Cci::T3::CommandLayerTest, testProductState);
	TEST_CASE_REGISTER(Cci::T3::CommandLayerTest, testPayment);
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
	order = new Order;
	commandLayer->setOrder(order);
	return true;
}

void CommandLayerTest::cleanup() {
	delete order;
	delete commandLayer;
	delete eventEngine;
	delete timerEngine;
	delete packetLayer;
	delete context;
	delete realtime;
	delete result;
}

bool CommandLayerTest::testProductState() {
//	{"type":"kitchen.request_pour_beverage","payload":{"deviceId":"thermoplan1","beverages":[
//	{"bc":"2","qty":1},{"bc":"6","qty":1},{"bc":"3","qty":1},{"bc":"7","qty":1},{"bc":"5","qty":1},{"bc":"9","qty":1}]}}
	Order order1;
	order1.clear();
	order1.add(2, 1);
	order1.add(6, 1);
	order1.add(3, 1);
	order1.add(7, 1);
	order1.add(5, 1);
	order1.add(9, 1);

	Cci::T3::ProductState state1;
	state1.set(&order1);
	uint32_t flag1 = state1.get();

//	{"type":"kitchen.request_pour_beverage","payload":{"deviceId":"thermoplan1","beverages":[
//	{"bc":"3","qty":10},{"bc":"6","qty":10},{"bc":"7","qty":10},{"bc":"5","qty":10},{"bc":"9","qty":10},{"bc":"2","qty":10}]}}
	Order order2;
	order2.clear();
	order2.add(3, 10);
	order2.add(6, 10);
	order2.add(7, 10);
	order2.add(5, 10);
	order2.add(9, 10);
	order2.add(2, 10);

	Cci::T3::ProductState state2;
	state2.set(&order2);
	uint32_t flag2 = state2.get();

	TEST_NUMBER_EQUAL(flag1, flag2);

	Order order3;
	order3.add(1, 2);
	order3.add(2, 4);
	order3.add(3, 1);

	TEST_NUMBER_EQUAL(1, order3.getFirstCid()); order3.remove(1);
	TEST_NUMBER_EQUAL(1, order3.getFirstCid()); order3.remove(1);
	TEST_NUMBER_EQUAL(2, order3.getFirstCid()); order3.remove(2);
	TEST_NUMBER_EQUAL(2, order3.getFirstCid()); order3.remove(2);
	TEST_NUMBER_EQUAL(2, order3.getFirstCid()); order3.remove(2);
	TEST_NUMBER_EQUAL(2, order3.getFirstCid()); order3.remove(2);
	TEST_NUMBER_EQUAL(3, order3.getFirstCid()); order3.remove(3);
	TEST_NUMBER_EQUAL(OrderCell::CidUndefined, order3.getFirstCid());
	return true;
}

bool CommandLayerTest::testPayment() {
	commandLayer->reset();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// BillingEnable
	packetLayer->recvPacket("5631");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ButtonEnable(enable all)
	packetLayer->recvPacket("543030323030303034");
	TEST_STRING_EQUAL("0654393030303030303030", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ButtonEnable(disable all)
	commandLayer->disable();
	packetLayer->recvPacket("543030323030303034");
	TEST_STRING_EQUAL("0654394646464630303030", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ButtonEnable(enable all)
	commandLayer->enable();
	packetLayer->recvPacket("543030323030303034");
	TEST_STRING_EQUAL("0654393030303030303030", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// VendRequest
	packetLayer->recvPacket("4930303831");
	TEST_STRING_EQUAL("064930", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,Request,8>", result->getString());
	result->clear();

	// BillingEnable
	packetLayer->recvPacket("5631");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ButtonEnable(disable all)
	packetLayer->recvPacket("543030323030303034");
	TEST_STRING_EQUAL("0654394646464630303030", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// BillingEnable
	order->add(8, 1);
	commandLayer->approveVend();
	packetLayer->recvPacket("5631");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ButtonEnable(enable only 8)
	packetLayer->recvPacket("543030323030303034");
	TEST_STRING_EQUAL("0654394646374630303030", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// VendRequest
	packetLayer->recvPacket("4930303831");
	TEST_STRING_EQUAL("064931", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,Completed>", result->getString());
	result->clear();

	// BillingEnable
	packetLayer->recvPacket("5631");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ButtonEnable(enable all)
	packetLayer->recvPacket("543030323030303034");
	TEST_STRING_EQUAL("0654393030303030303030", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());
	return true;
}
#if 0
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
#endif
}
}
