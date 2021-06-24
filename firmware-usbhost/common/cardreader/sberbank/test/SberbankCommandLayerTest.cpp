#include "cardreader/sberbank/SberbankCommandLayer.h"
#include "cardreader/sberbank/SberbankProtocol.h"
#include "mdb/master/cashless/MdbMasterCashless.h"
#include "utils/include/Hex.h"
#include "timer/include/TimerEngine.h"
#include "timer/include/TestRealTime.h"
#include "mdb/master/cashless/test/TestCashlessEventEngine.h"
#include "http/test/TestTcpIp.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class TestSberbankPacketLayer : public Sberbank::PacketLayerInterface {
public:
	TestSberbankPacketLayer() : recvBuf(256) {}
	virtual ~TestSberbankPacketLayer() {}
	void clearSendData() { sendBuf.clear(); }
	const char *getSendData() { return sendBuf.getString(); }
	void recvPacket(const char *hex) {
		uint16_t len = hexToData(hex, strlen(hex), recvBuf.getData(), recvBuf.getSize());
		recvBuf.setLen(len);
		observer->procPacket(recvBuf.getData(), recvBuf.getLen());
	}
	void recvError(Sberbank::PacketLayerObserver::Error error) { observer->procError(error); }

	virtual void setObserver(Sberbank::PacketLayerObserver *observer) { this->observer = observer; }
	virtual void reset() {}
	virtual bool sendPacket(Buffer *data) {
		for(uint16_t i = 0; i < data->getLen(); i++) {
			sendBuf.addHex((*data)[i]);
		}
		return true;
	}

private:
	Sberbank::PacketLayerObserver *observer;
	StringBuilder sendBuf;
	Buffer recvBuf;
};

class SberbankCommandLayerTest : public TestSet {
public:
	SberbankCommandLayerTest();
	bool init();
	void cleanup();
	bool testPayment();
	bool testPaymentByPoints();
	bool testPaymentDenied();
	bool testPaymentCancelByUser();
	bool testPaymentCancelByTerminal();
	bool testPaymentWrongPin();
	bool testPaymentTimeout();
	bool testStateSessionTimeout();
	bool testSverkaItogov();
	bool testVendingFailed();
	bool testVendingTimeout();
	bool testQrCode();

private:
	StringBuilder *result;
	TestRealTime *realtime;
	Mdb::DeviceContext *context;
	TestSberbankPacketLayer *packetLayer;
	TestTcpIp *tcpIp;
	TimerEngine *timerEngine;
	TestCashlessEventEngine *eventEngine;
	Sberbank::CommandLayer *commandLayer;
};

TEST_SET_REGISTER(SberbankCommandLayerTest);

SberbankCommandLayerTest::SberbankCommandLayerTest() {
	TEST_CASE_REGISTER(SberbankCommandLayerTest, testPayment);
	TEST_CASE_REGISTER(SberbankCommandLayerTest, testPaymentByPoints);
	TEST_CASE_REGISTER(SberbankCommandLayerTest, testPaymentDenied);
	TEST_CASE_REGISTER(SberbankCommandLayerTest, testPaymentCancelByUser);
	TEST_CASE_REGISTER(SberbankCommandLayerTest, testPaymentCancelByTerminal);
	TEST_CASE_REGISTER(SberbankCommandLayerTest, testPaymentWrongPin);
	TEST_CASE_REGISTER(SberbankCommandLayerTest, testPaymentTimeout);
	TEST_CASE_REGISTER(SberbankCommandLayerTest, testStateSessionTimeout);
	TEST_CASE_REGISTER(SberbankCommandLayerTest, testSverkaItogov);
	TEST_CASE_REGISTER(SberbankCommandLayerTest, testVendingFailed);
	TEST_CASE_REGISTER(SberbankCommandLayerTest, testVendingTimeout);
	TEST_CASE_REGISTER(SberbankCommandLayerTest, testQrCode);
}

bool SberbankCommandLayerTest::init() {
	result = new StringBuilder;
	realtime = new TestRealTime;
	uint32_t masterDecimalPoint = 1;
	context = new Mdb::DeviceContext(masterDecimalPoint, realtime);
	packetLayer = new TestSberbankPacketLayer;
	tcpIp = new TestTcpIp(512, result);
	timerEngine = new TimerEngine();
	eventEngine = new TestCashlessEventEngine(result);
	commandLayer = new Sberbank::CommandLayer(context, packetLayer, tcpIp, timerEngine, eventEngine, 2500);
	return true;
}

void SberbankCommandLayerTest::cleanup() {
	delete commandLayer;
	delete eventEngine;
	delete timerEngine;
	delete tcpIp;
	delete packetLayer;
	delete context;
	delete realtime;
	delete result;
}

bool SberbankCommandLayerTest::testPayment() {
	commandLayer->reset();
	commandLayer->enable();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on cardreader
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

#if 0
	// recvResponse
	packetLayer->recvPacket("0004001d440a80401f0000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// session start
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();

	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00CD00000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("<event=1,SessionBegin,2500>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
#else
	// recvResponse
	packetLayer->recvPacket("0004001d440a80401f0000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00CD00000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("<event=1,SessionBegin,2500>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
#endif
	// press button on automat
	commandLayer->sale(25, 900);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL(
		"6D4500F66E070028230000000001000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// payment complete
	packetLayer->recvPacket(
		"00bc00f66e078000003237303333350038333239"
		"3531313537353436003030303100343237363633"
		"2a2a2a2a2a2a323931350000000030322f323000"
		"8e848e819085488e3a0000000000000000000000"
		"00000000000000000000000085f03301460a0200"
		"0132303032333134310056697361000000000000"
		"0000000000003633313030303030303034360000"
		"00008171e79003bfc422a73796f0ee4e0c5ba05d"
		"4d56d2634db744cab644740e69c9c4ddaa6b5f82"
		"c54f4a2097b251e70627a1357bc803");
	TEST_STRING_EQUAL("<event=1,VendApproved,1,900,4,0>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// vend complete
	TEST_NUMBER_EQUAL(true, commandLayer->saleComplete());
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
}

bool SberbankCommandLayerTest::testPaymentByPoints() {
	commandLayer->reset();
	commandLayer->enable();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on cardreader
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a80401f0000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00CD00000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("<event=1,SessionBegin,2500>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on automat
	commandLayer->sale(25, 900);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL(
		"6D4500F66E070028230000000001000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// payment by points complete
	packetLayer->recvPacket(
		"00bc00f66e078000003237303333350038333239"
		"3531313537353436003030303100343237363633"
		"2a2a2a2a2a2a323931350000000030322f323000"
		"8e848e819085488e3a0000000000000000000000"
		"00000000000000000000000085f03301460a0200"
		"0132303032333134310056697361000000000000"
		"000000000000363331303030303030303436581B"
		"00008171e79003bfc422a73796f0ee4e0c5ba05d"
		"4d56d2634db744cab644740e69c9c4ddaa6b5f82"
		"c54f4a2097b251e70627a1357bc803");

	TEST_STRING_EQUAL("<event=1,VendApproved,1,200,4,700>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// vend complete
	TEST_NUMBER_EQUAL(true, commandLayer->saleComplete());
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
}

#if 0
bool SberbankCommandLayerTest::testPaymentByPoints() {
	commandLayer->reset();
	commandLayer->enable();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on cardreader
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a80401f0000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00CD00000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("<event=1,SessionBegin,25000>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on automat
	commandLayer->sale(25, 9000);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL(
		"6D4500F66E070028230000000001000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// payment by points complete
	packetLayer->recvPacket(
		"00bc00f66e078000003237303333350038333239"
		"3531313537353436003030303100343237363633"
		"2a2a2a2a2a2a323931350000000030322f323000"
		"8e848e819085488e3a0000000000000000000000"
		"00000000000000000000000085f03301460a0200"
		"0132303032333134310056697361000000000000"
		"000000000000363331303030303030303436581B"
		"00008171e79003bfc422a73796f0ee4e0c5ba05d"
		"4d56d2634db744cab644740e69c9c4ddaa6b5f82"
		"c54f4a2097b251e70627a1357bc803");

	TEST_STRING_EQUAL("<event=1,VendApproved,2000>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// vend complete
	TEST_NUMBER_EQUAL(true, commandLayer->saleComplete());
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
}
#endif

bool SberbankCommandLayerTest::testPaymentDenied() {
	commandLayer->reset();
	commandLayer->enable();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on cardreader
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a80401f0000");
#if 0 //todo: pause or not?
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// session start
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
#endif
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00CD00000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("<event=1,SessionBegin,2500>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on automat
	commandLayer->sale(25, 900);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL(
		"6D4500F66E070028230000000001000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// payment complete
	packetLayer->recvPacket(
		"00BC00F66E0780D0070000000000000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"000000000000000000000000D5F03301543C0300"
		"0032303032333134310000000000000000000000"
		"0000000000003633313030303030303034360000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"000000000000000000000000000000");
	TEST_STRING_EQUAL("<event=1,VendDenied>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// session close
	TEST_NUMBER_EQUAL(true, commandLayer->closeSession());
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
}

bool SberbankCommandLayerTest::testPaymentCancelByUser() {
	commandLayer->reset();
	commandLayer->enable();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on cardreader
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a80401f0000");
#if 0 //todo: pause or not?
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// session start
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
#endif
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00CD00000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("<event=1,SessionBegin,2500>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// cancel
	commandLayer->closeSession();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
}

/*
 * Отмена продажи через терминал Сбербанка не работает сейчас
 * Команда A008 отсутствует в документации
 */
bool SberbankCommandLayerTest::testPaymentCancelByTerminal() {
	commandLayer->reset();
	commandLayer->enable();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on cardreader
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a80401f0000");
#if 0 //todo: pause or not?
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// session start
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
#endif
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00CD00000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("<event=1,SessionBegin,2500>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on automat
	commandLayer->sale(25, 900);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL(
		"6D4500F66E070028230000000001000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// payment cancel by terminal
	packetLayer->recvPacket(
		"A0080002000000032B000300000000");
	TEST_STRING_EQUAL("<event=1,VendDenied>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// session close
	TEST_NUMBER_EQUAL(true, commandLayer->closeSession());
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
}

bool SberbankCommandLayerTest::testPaymentWrongPin() {
	commandLayer->reset();
	commandLayer->enable();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on cardreader
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a80401f0000");
#if 0 //todo: pause or not?
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// session start
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
#endif
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00CD00000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("<event=1,SessionBegin,2500>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on automat
	commandLayer->sale(25, 900);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL(
		"6D4500F66E070028230000000001000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// payment complete
	packetLayer->recvPacket(
		"00BC00F66E078067113030303030300038333339"
		"3537353531303137003030303700343237363633"
		"2A2A2A2A2A2A363633390000000030332F313900"
		"8F88482048858285908548000000000000000000"
		"000000000000000000000000D5F03301A6480300"
		"0132303032333134310056697361000000000000"
		"0000000000003633313030303030303034360000"
		"00009BB3D73D07FD7673B01CCE6625D47F8D183C"
		"D557D2634DB744CAB644192393E3617A3C672674"
		"68405649E40A9F6E0616540241A203");
	TEST_STRING_EQUAL("<event=1,VendDenied>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// session close
	TEST_NUMBER_EQUAL(true, commandLayer->closeSession());
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
}

bool SberbankCommandLayerTest::testPaymentTimeout() {
	commandLayer->reset();
	commandLayer->enable();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on cardreader
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a80401f0000");
#if 0 //todo: pause or not?
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// session start
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
#endif
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00CD00000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("<event=1,SessionBegin,2500>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on automat
	commandLayer->sale(25, 900);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL(
		"6D4500F66E070028230000000001000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// payment timeout
	timerEngine->tick(MDB_CL_APPROVING_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,VendDenied>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// session close
	TEST_NUMBER_EQUAL(true, commandLayer->closeSession());
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
}

bool SberbankCommandLayerTest::testStateSessionTimeout() {
	commandLayer->reset();
	commandLayer->enable();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on cardreader
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse/session start
	packetLayer->recvPacket("0004001d440a80401f0000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00CD00000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("<event=1,SessionBegin,2500>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// session timeout
	timerEngine->tick(MDB_CL_SESSION_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
}

bool SberbankCommandLayerTest::testSverkaItogov() {
#if 0
	commandLayer->reset();
	commandLayer->enable();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	for(uint16_t i = 0; i < SBERBANK_SVERKA_FIRST; i++) {
		// poll timeout
		timerEngine->tick(SBERBANK_POLL_DELAY);
		timerEngine->execute();
		TEST_STRING_EQUAL("", result->getString());
		TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
		packetLayer->clearSendData();

		// recvResponse
		packetLayer->recvPacket("0004001d440a8009200000");
		TEST_STRING_EQUAL("", result->getString());
		TEST_STRING_EQUAL("", packetLayer->getSendData());
	}

	// uknown command
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL(
		"C00C001D440A008100000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// sverka itogov
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL(
		"6D4500D6E20C0000000000000007000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket(
		"00BC00D6E20C8000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"000000000000000000000000D6F0330192A30200"
		"0032303032333134310000000000000000000000"
		"0000000000003633313030303030303034360000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"000000000000000000000000000000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	for(uint16_t i = 0; i < SBERBANK_SVERKA_NEXT; i++) {
		// poll timeout
		timerEngine->tick(SBERBANK_POLL_DELAY);
		timerEngine->execute();
		TEST_STRING_EQUAL("", result->getString());
		TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
		packetLayer->clearSendData();

		// recvResponse
		packetLayer->recvPacket("0004001d440a8009200000");
		TEST_STRING_EQUAL("", result->getString());
		TEST_STRING_EQUAL("", packetLayer->getSendData());
	}

	// uknown command
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL(
		"C00C001D440A008100000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// sverka itogov
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL(
		"6D4500D6E20C0000000000000007000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket(
		"00BC00D6E20C8000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"000000000000000000000000D6F0330192A30200"
		"0032303032333134310000000000000000000000"
		"0000000000003633313030303030303034360000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"000000000000000000000000000000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
#else
	commandLayer->reset();
	commandLayer->enable();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_SVERKA_FIRST_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A008100000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse/sverka itogov
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL(
		"6D4500D6E20C0000000000000007000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket(
		"00BC00D6E20C8000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"000000000000000000000000D6F0330192A30200"
		"0032303032333134310000000000000000000000"
		"0000000000003633313030303030303034360000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"000000000000000000000000000000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_SVERKA_NEXT_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A008100000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse/sverka itogov
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL(
		"6D4500D6E20C0000000000000007000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket(
		"00BC00D6E20C8000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"000000000000000000000000D6F0330192A30200"
		"0032303032333134310000000000000000000000"
		"0000000000003633313030303030303034360000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"000000000000000000000000000000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
#endif
}

/*
Терминал присылает данный запрос после запроса на одобрение платежа и до ответа с результатом платежа.
Причем платеж одобряют.
CMD(xA0;)
LEN(x08;x00;=8)
OID(x5D;x00;x00;x00;)
CODE(x03;=WRITE)
DEV(x2B;=?)
RESERVED(x00;)
PLEN(x03;x00;=3)
PARAM1(x00;x00;x00;)
 */
bool SberbankCommandLayerTest::testVendingFailed() {
	commandLayer->reset();
	commandLayer->enable();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on cardreader
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse/session start
	packetLayer->recvPacket("0004001d440a80401f0000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00CD00000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("<event=1,SessionBegin,2500>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on automat
	commandLayer->sale(25, 900);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL(
		"6D4500F66E070028230000000001000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// payment complete
	packetLayer->recvPacket(
		"00bc00f66e078000003237303333350038333239"
		"3531313537353436003030303100343237363633"
		"2a2a2a2a2a2a323931350000000030322f323000"
		"8e848e819085488e3a0000000000000000000000"
		"00000000000000000000000085f03301460a0200"
		"0132303032333134310056697361000000000000"
		"0000000000003633313030303030303034360000"
		"00008171e79003bfc422a73796f0ee4e0c5ba05d"
		"4d56d2634db744cab644740e69c9c4ddaa6b5f82"
		"c54f4a2097b251e70627a1357bc803");
	TEST_STRING_EQUAL("<event=1,VendApproved,1,900,4,0>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	packetLayer->clearSendData();

	// vend complete
	TEST_NUMBER_EQUAL(true, commandLayer->saleFailed());
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL(
		"6D4500FF6E07002823000000000D000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket(
		"00bc00f66e078000003237303333350038333239"
		"3531313537353436003030303100343237363633"
		"2a2a2a2a2a2a323931350000000030322f323000"
		"8e848e819085488e3a0000000000000000000000"
		"00000000000000000000000085f03301460a0200"
		"0132303032333134310056697361000000000000"
		"0000000000003633313030303030303034360000"
		"00008171e79003bfc422a73796f0ee4e0c5ba05d"
		"4d56d2634db744cab644740e69c9c4ddaa6b5f82"
		"c54f4a2097b251e70627a1357bc803");
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
}

bool SberbankCommandLayerTest::testVendingTimeout() {
	commandLayer->reset();
	commandLayer->enable();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on cardreader
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse/session start
	packetLayer->recvPacket("0004001d440a80401f0000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00CD00000000000000FFFFFFFF", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("<event=1,SessionBegin,2500>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// press button on automat
	commandLayer->sale(25, 900);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL(
		"6D4500F66E070028230000000001000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// payment complete
	packetLayer->recvPacket(
		"00bc00f66e078000003237303333350038333239"
		"3531313537353436003030303100343237363633"
		"2a2a2a2a2a2a323931350000000030322f323000"
		"8e848e819085488e3a0000000000000000000000"
		"00000000000000000000000085f03301460a0200"
		"0132303032333134310056697361000000000000"
		"0000000000003633313030303030303034360000"
		"00008171e79003bfc422a73796f0ee4e0c5ba05d"
		"4d56d2634db744cab644740e69c9c4ddaa6b5f82"
		"c54f4a2097b251e70627a1357bc803");
	TEST_STRING_EQUAL("<event=1,VendApproved,1,900,4,0>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// tick
	timerEngine->tick(MDB_CL_VENDING_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL(
		"6D4500FF6E07002823000000000D000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket(
		"00bc00f66e078000003237303333350038333239"
		"3531313537353436003030303100343237363633"
		"2a2a2a2a2a2a323931350000000030322f323000"
		"8e848e819085488e3a0000000000000000000000"
		"00000000000000000000000085f03301460a0200"
		"0132303032333134310056697361000000000000"
		"0000000000003633313030303030303034360000"
		"00008171e79003bfc422a73796f0ee4e0c5ba05d"
		"4d56d2634db744cab644740e69c9c4ddaa6b5f82"
		"c54f4a2097b251e70627a1357bc803");
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("C00C001D440A00D30000000000000001000000", packetLayer->getSendData());
	packetLayer->clearSendData();

	// recvResponse
	packetLayer->recvPacket("0004001d440a8009200000");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
}

bool SberbankCommandLayerTest::testQrCode() {
	commandLayer->reset();
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// qrcode request
	commandLayer->printQrCode("header1", "footer1", "text1");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// poll timeout
	timerEngine->tick(SBERBANK_POLL_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL(
		"6D4D00F76E070000000000000039000000000000"
		"0000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000"
		"00000000000000000000000000000008DF550574"
		"65787431", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ack
	packetLayer->recvPacket("00");
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
}

/*
02>23>41>42>50>41>44>41>41>64>52>41>6F>41>30>77>41>41>41>41>41>41>41>41>41>42>41>41>41>41>32>39>55>3D>03
18.12.06 17:29:48 <04

<02<23<41<41<73<41<42<41<41<64<52<41<71<41<43<53<41<41<41<4A<63<39<03
18.12.06 17:29:48 >04>

// запрос на неизвестно что
STX(02)MARK(23)BASE64(414250414441416452416F4167514141414141414141442F2F2F2F2F5774593D)ETX(03)
>>DECODEBASE64(00 13 c0 0c 00 1d 44 0a 00 81 00 00 00 00 00 00 00 ff ff ff ff 5a d6)
>>>NUM(00)
>>>LEN(13)
>>>DATA(c0 0c 00 1d 44 0a 00 81 00 00 00 00 00 00 00 ff ff ff ff)
>>>CRC(5a d6))
>>>>RESULT(c0)
>>>>LEN(0c 00)
>>>>NUM(1d 44 0a 00)
>>>>DATA(81 00 00 00 00 00 00 00 ff ff ff ff)
18.12.06 17:29:49 <04

<02<23<41<41<73<41<42<41<41<64<52<41<71<41<43<53<41<41<41<4A<63<39<03
18.12.06 17:29:49 >04>

// запрос на сверку
STX(02)MARK(23)BASE64(41457874525144573467774141414141414141414277414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141415161303D)ETX(03)
>>DECODEBASE64(00 4c 6d 45 00 d6 e2 0c 00 00 00 00 00 00 00 07 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 41 ad)
>>>NUM(00)
>>>LEN(4с)
>>>DATA(6d 45 00 d6 e2 0c 00 00 00 00 00 00 00 07 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00)
>>>CRC(41 ad))
>>>>RESULT(6d)
>>>>LEN(45 00)
>>>>NUM(d6 e2 0c 00)
>>>>DATA(00 00 00 00 00 00 07 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00)
18.12.06 17:29:53 <04<

// запрос от МТУ
STX(02)MARK(23)BASE64(41412B67434141444141414141514D41417741414141434836503D3D)ETX(03)
<<DECODEBASE64(00 0f a0 08 00 03 00 00 00 01 03 00 03 00 00 00 00 87 e8)
<<<NUM(00)
<<<LEN(0f)
<<<DATA(a0 08 00 03 00 00 00 01 03 00 03 00 00 00 00)
<<<CRC(87 e8)
<<<<RESULT(a0)
<<<<LEN(08 00)
<<<<NUM(03 00 00 00) // OPEN
<<<<DATA(01 03 00 03 00 00 00 00) //PRINTER
18.12.06 17:30:03 >04

// Ответ на команду
STX(02)MARK(23)BASE64(674C5141764144573467794141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414E62774D7747536F774941414449774D44497A4D5451784141414141414141414141414141414141414141414141324D7A45774D4441774D4441774E44594141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414141414144332B513D3D)ETX(03)
<<DECODEBASE64(80 b4 00 bc 00 d6 e2 0c 80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 d6 f0 33 01 92 a3 02 00 00 32 30 30 32 33 31 34 31 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 36 33 31 30 30 30 30 30 30 30 34 36 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 f7 f9)
<<<NUM(80)
<<<LEN(b4)
<<<DATA(00 bc 00 d6 e2 0c 80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 d6 f0 33 01 92 a3 02 00 00 32 30 30 32 33 31 34 31 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 36 33 31 30 30 30 30 30 30 30 34 36 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00)
<<<CRC(f7 f9)
<<<<RESULT(00)
<<<<LEN(bc 00)
<<<<NUM(d6 e2 0c 80)
<<<<DATA(00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 d6 f0 33 01 92 a3 02 00 00 32 30 30 32 33 31 34 31 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 36 33 31 30 30 30 30 30 30 30 34 36 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00)
18.12.06 17:30:03 >06

<STX(02)MARK(23)BASE64(41513841414141414141414141414141414141414141415572673D3D)ETX(03)
<<DECODEBASE64(01 0f 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 14 ae)
<<<NUM(01)
<<<LEN(0f)
<<<DATA(00 00 00 00 00 00 00 00 00 00 00 00 00 00 00)
<<<CRC(14 ae)
<<<<DATA(00 00 00 00 00 00 00 00 00 00 00 00 00 00 00)

// Продолжение обычного опроса
>STX(02)MARK(23)BASE64(414250414441416452416F4130774141414141414141414241414141043239553D)ETX(03)
18.12.06 17:30:03 <15
 */
