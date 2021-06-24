#include "cardreader/inpas/InpasCommandLayer.h"
#include "cardreader/inpas/InpasProtocol.h"
#include "mdb/master/cashless/MdbMasterCashless.h"
#include "utils/include/Hex.h"
#include "timer/include/TimerEngine.h"
#include "timer/include/TestRealTime.h"
#include "mdb/master/cashless/test/TestCashlessEventEngine.h"
#include "http/test/TestTcpIp.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class TestInpasPacketLayer : public Inpas::PacketLayerInterface {
public:
	TestInpasPacketLayer() : recvBuf(256) {}
	virtual ~TestInpasPacketLayer() {}
	void clearSendData() { sendBuf.clear(); }
	const char *getSendData() { return sendBuf.getString(); }
	void recvPacket(const char *hex) {
		uint16_t len = hexToData(hex, strlen(hex), recvBuf.getData(), recvBuf.getSize());
		recvBuf.setLen(len);
		observer->procPacket(recvBuf.getData(), recvBuf.getLen());
	}
	void recvControl(uint8_t control) { observer->procControl(control); }
	void recvError(Inpas::PacketLayerObserver::Error error) { observer->procError(error); }

	virtual void setObserver(Inpas::PacketLayerObserver *observer) { this->observer = observer; }
	virtual void reset() {}
	virtual bool sendPacket(Buffer *data) {
		for(uint16_t i = 0; i < data->getLen(); i++) {
			sendBuf.addHex((*data)[i]);
		}
		return true;
	}
	virtual bool sendControl(uint8_t control) {
		sendBuf.addHex(control);
		return true;
	}

private:
	Inpas::PacketLayerObserver *observer;
	StringBuilder sendBuf;
	Buffer recvBuf;
};

class InpasCommandLayerTest : public TestSet {
public:
	InpasCommandLayerTest();
	bool init();
	void cleanup();
	bool testInit();
	bool testPayment();
	bool testPaymentThroughModem();
	bool testPressButtonBeforeCloseSession();
	bool testStateSessionTimeout();
	bool testStateSessionCancelByTerminal();
	bool testStateRequestTimeout();
	bool testStateApprovingTimeout();
	bool testStateApprovingCancelByTerminal();
	bool testStateApprovingCancelByUser();
	bool testPaymentCancel();
	bool testPaymentCancelFailed();
	bool testQrCode();
	bool testVerification();

private:
	StringBuilder *result;
	TestRealTime *realtime;
	Mdb::DeviceContext *context;
	TestInpasPacketLayer *packetLayer;
	TestTcpIp *tcpIp;
	TimerEngine *timerEngine;
	TestCashlessEventEngine *eventEngine;
	Inpas::CommandLayer *commandLayer;

	bool gotoStateWait();
};

TEST_SET_REGISTER(InpasCommandLayerTest);

InpasCommandLayerTest::InpasCommandLayerTest() {
	TEST_CASE_REGISTER(InpasCommandLayerTest, testInit);
	TEST_CASE_REGISTER(InpasCommandLayerTest, testPayment);
	TEST_CASE_REGISTER(InpasCommandLayerTest, testPaymentThroughModem);
	TEST_CASE_REGISTER(InpasCommandLayerTest, testPressButtonBeforeCloseSession);
	TEST_CASE_REGISTER(InpasCommandLayerTest, testStateSessionTimeout);
	TEST_CASE_REGISTER(InpasCommandLayerTest, testStateSessionCancelByTerminal);
	TEST_CASE_REGISTER(InpasCommandLayerTest, testStateRequestTimeout);
	TEST_CASE_REGISTER(InpasCommandLayerTest, testStateApprovingTimeout);
	TEST_CASE_REGISTER(InpasCommandLayerTest, testStateApprovingCancelByTerminal);
//	TEST_CASE_REGISTER(InpasCommandLayerTest, testStateApprovingCancelByUser); //todo: тест не дописан
	TEST_CASE_REGISTER(InpasCommandLayerTest, testPaymentCancel);
	TEST_CASE_REGISTER(InpasCommandLayerTest, testPaymentCancelFailed);
	TEST_CASE_REGISTER(InpasCommandLayerTest, testQrCode);
	TEST_CASE_REGISTER(InpasCommandLayerTest, testVerification);
}

bool InpasCommandLayerTest::init() {
	result = new StringBuilder;
	realtime = new TestRealTime;
	uint32_t masterDecimalPoint = 1;
	context = new Mdb::DeviceContext(masterDecimalPoint, realtime);
	packetLayer = new TestInpasPacketLayer;
	tcpIp = new TestTcpIp(512, result);
	timerEngine = new TimerEngine();
	eventEngine = new TestCashlessEventEngine(result);
	commandLayer = new Inpas::CommandLayer(context, packetLayer, tcpIp, timerEngine, eventEngine);
	return true;
}

void InpasCommandLayerTest::cleanup() {
	delete commandLayer;
	delete eventEngine;
	delete timerEngine;
	delete tcpIp;
	delete packetLayer;
	delete context;
	delete realtime;
	delete result;
}

bool InpasCommandLayerTest::testInit() {
	commandLayer->reset();

	// init timeout
	timerEngine->tick(INPAS_INIT_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("19020032361B0000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());
	TEST_NUMBER_EQUAL(Mdb::DeviceContext::Status_NotFound, context->getStatus());

	// init timeout
	timerEngine->tick(INPAS_RECV_TIMEOUT);
	timerEngine->execute();
	timerEngine->tick(INPAS_INIT_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("19020032361B0000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// init timeout
	timerEngine->tick(INPAS_RECV_TIMEOUT);
	timerEngine->execute();
	timerEngine->tick(INPAS_INIT_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("19020032361B0000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// init ack
	packetLayer->recvControl(Inpas::Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_NUMBER_EQUAL(Mdb::DeviceContext::Status_Init, context->getStatus());

	// wait response
	packetLayer->recvPacket("x19x02x00x32x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// init response
	packetLayer->recvPacket(
		"x13x20x00"
		"xCExCFxC5xD0xC0xD6xC8xDFx20xCExD2xCAxCBxCExCDxC5"
		"xCDxC0x5Ex44x45x43x4Cx49x4Ex45x44x2Ex4Ax50x47x7E"
		"x15x0Ex00"
		"x32x30x32x30x30x33x31x39x31x37x32x35x32x34"
		"x19x02x00x32x36"
		"x1Bx08x00x30x30x30x36x30x30x35x36"
		"x27x02x00x31x36");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());
	TEST_NUMBER_EQUAL(Mdb::DeviceContext::Status_Error, context->getStatus());
	TEST_STRING_EQUAL("00060056", context->getSerialNumber());

	// init timeout
	timerEngine->tick(INPAS_INIT_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("19020032361B0000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// init ack
	packetLayer->recvControl(Inpas::Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_NUMBER_EQUAL(Mdb::DeviceContext::Status_Init, context->getStatus());

	// wait response
	packetLayer->recvPacket("x19x02x00x32x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// init response
	packetLayer->recvPacket(
		"x13x1Fx00"
		"xCExCFxC5xD0xC0xD6xC8xDFx20xCExC4xCExC1xD0xC5xCD"
		"xC0x5Ex41x50x50x52x4Fx56x45x44x2Ex4Ax50x47x7E"
		"x15x0Ex00"
		"x32x30x32x30x30x33x31x39x31x35x31x34x31x34"
		"x19x02x00x32x36"
		"x1Bx08x00x30x30x30x36x30x30x35x36"
		"x27x01x00x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());
	TEST_NUMBER_EQUAL(Mdb::DeviceContext::Status_Work, context->getStatus());

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();
	return true;
}


bool InpasCommandLayerTest::gotoStateWait() {
	commandLayer->reset();

	// init timeout
	timerEngine->tick(INPAS_INIT_DELAY);
	timerEngine->execute();
	TEST_STRING_EQUAL("19020032361B0000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());
	TEST_NUMBER_EQUAL(Mdb::DeviceContext::Status_NotFound, context->getStatus());

	// init ack
	packetLayer->recvControl(Inpas::Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_NUMBER_EQUAL(Mdb::DeviceContext::Status_Init, context->getStatus());

	// init response
	packetLayer->recvPacket(
		"x13x1Fx00xCExCFxC5xD0xC0xD6xC8"
		"xDFx20xCExC4xCExC1xD0xC5xCDxC0"
		"x5Ex41x50x50x52x4Fx56x45x44x2E"
		"x4Ax50x47x7Ex15x0Ex00x32x30x32"
		"x30x30x33x31x39x31x35x31x34x31"
		"x34x19x02x00x32x36x1Bx08x00x30"
		"x30x30x36x30x30x35x36x27x01x00"
		"x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());
	TEST_NUMBER_EQUAL(Mdb::DeviceContext::Status_Work, context->getStatus());
	return true;
}

bool InpasCommandLayerTest::testPayment() {
	TEST_NUMBER_EQUAL(true, gotoStateWait());

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();

	// payment request
	commandLayer->sale(0, 50);
	TEST_STRING_EQUAL("000800303030303035303004030036343319010031", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ack
	packetLayer->recvControl(Inpas::Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// wait
	packetLayer->recvPacket("x19x02x00x32x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// approving payment
	packetLayer->recvPacket(
		"x00x03x00x31x30x30x04x03x00x36x34x33x06x0Ex00x32x30"
		"x31x38x31x30x31x36x31x39x31x35x32x33x0Ax10x00x2Ax2Ax2Ax2Ax2A"
		"x2Ax2Ax2Ax2Ax2Ax2Ax2Ax36x36x33x39x0Bx04x00x31x39x30x33x0Dx06"
		"x00x32x34x37x36x32x39x0Ex0Cx00x38x32x38x39x31x33x33x34x39x33"
		"x37x33x0Fx02x00x30x30x13x08x00xCExC4xCExC1xD0xC5xCDxCEx15x0E"
		"x00x32x30x31x38x31x30x31x36x31x39x31x35x32x33x17x02x00x2Dx31"
		"x19x01x00x31x1Ax02x00x2Dx31x1Bx08x00x30x30x32x36x36x32x38x36"
		"x1Cx09x00x31x31x31x31x31x31x31x31x31x27x01x00x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendApproved,1,10,0,0>", result->getString());
	result->clear();

	// sale complete
	commandLayer->saleComplete();
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();

	// payment request
	commandLayer->sale(0, 10);
	TEST_STRING_EQUAL("000800303030303031303004030036343319010031", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ack
	packetLayer->recvControl(Inpas::Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// wait
	packetLayer->recvPacket("x19x02x00x32x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// approving payment
	packetLayer->recvPacket(
		"x00x03x00x31x30x30x04x03x00x36x34x33x06x0Ex00x32x30"
		"x31x38x31x30x31x36x31x39x31x35x32x33x0Ax10x00x2Ax2Ax2Ax2Ax2A"
		"x2Ax2Ax2Ax2Ax2Ax2Ax2Ax36x36x33x39x0Bx04x00x31x39x30x33x0Dx06"
		"x00x32x34x37x36x32x39x0Ex0Cx00x38x32x38x39x31x33x33x34x39x33"
		"x37x33x0Fx02x00x30x30x13x08x00xCExC4xCExC1xD0xC5xCDxCEx15x0E"
		"x00x32x30x31x38x31x30x31x36x31x39x31x35x32x33x17x02x00x2Dx31"
		"x19x01x00x31x1Ax02x00x2Dx31x1Bx08x00x30x30x32x36x36x32x38x36"
		"x1Cx09x00x31x31x31x31x31x31x31x31x31x27x01x00x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendApproved,1,10,0,0>", result->getString());
	result->clear();
	TEST_NUMBER_EQUAL(Mdb::DeviceContext::Status_Work, context->getStatus());

	// sale failed
	commandLayer->saleComplete();
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	return true;
}

bool InpasCommandLayerTest::testPaymentThroughModem() {
	TEST_NUMBER_EQUAL(true, gotoStateWait());

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();

	// payment request
	commandLayer->sale(0, 50);
	TEST_STRING_EQUAL("000800303030303035303004030036343319010031", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ack
	packetLayer->recvControl(Inpas::Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// conn open
	packetLayer->recvPacket("190200363340010031410200313646120039312E3130372E36352E36363B313937393B");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<connect:91.107.65.66,1979,0>", result->getString());
	result->clear();

	// conn opened
	tcpIp->connectComplete();
	TEST_STRING_EQUAL("1902003633410200313643010030", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// data send
	packetLayer->recvPacket(
		"1902003633400100304102003137469300504F5354202F73657276696365"
		"2D73656C6563746F722F20485454502F312E310D0A486F73743A2039312E"
		"3130372E36352E36363A313937390D0A436F6E74656E742D547970653A20"
		"6170706C69636174696F6E2F6F637465742D73747265616D0D0A436F6E74"
		"656E742D4C656E6774683A203334310D0A43616368652D436F6E74726F6C"
		"3A206E6F2D73746F72650D0A0D0A");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL(
		"<send=504F5354202F736572766963652D73656C6563746F722F20485454"
		"502F312E310D0A486F73743A2039312E3130372E36352E36363A31393739"
		"0D0A436F6E74656E742D547970653A206170706C69636174696F6E2F6F63"
		"7465742D73747265616D0D0A436F6E74656E742D4C656E6774683A203334"
		"310D0A43616368652D436F6E74726F6C3A206E6F2D73746F72650D0A0D0A"
		",len=147>", result->getString());
	result->clear();

	// data sended
	tcpIp->sendComplete();
	TEST_STRING_EQUAL("1902003633410200313743010030", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// data recv
	packetLayer->recvPacket("1902003633400100314102003137");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// data recv
	tcpIp->addRecvData("", true);
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<recv=495>", result->getString());
	result->clear();

	// data recv
	tcpIp->addRecvData(
		"02520119020033341B080030303036303035363B20016E98E478134CEB69"
		"D0FB0BFC3BE6442368F702D06917270F38081C2DC4A5270CF3D47E86758F"
		"CA82862CFA1078D145363F599CC385A304BAC1B92032152FB7885367A070"
		"95CE598485070D8962681F267EAB6B82E538514BB36A67189AA38AEA749A"
		"AF0D3FD49E102229A7410D772C5C3706E177C0CBC6C8E406B657E41CD1E8"
		"7B9B33009D0DD82E89268E53F450E8266C6194774113B9A0AC7C1D92661A"
		"0BEC3EA264482BD59B5B3558E01E77A246A9B6B3EA6DBD46C4F7EA08B3C4"
		"790CC04E34F4DB5855EEA95FD73BCA7BBB0F2959B8532588A980FF9247EA"
		"B1F4DC92D2504B2C987E9E8ABDBDF007B44068C9A71CD000EB34CC7EBE51"
		"74A69E504CD80BC16FBB681592509CAD60B251DA814E7A2FDEE77059CB2C"
		"EC7EB21A1BCE6F7E30F4FD1800191009211012000C0791F92E31086C760C"
		"7DA49402E1AE4EFF010031", true);
	TEST_STRING_EQUAL(
		"190200363341020031374301003046550102520119020033341B08003030"
		"3036303035363B20016E98E478134CEB69D0FB0BFC3BE6442368F702D069"
		"17270F38081C2DC4A5270CF3D47E86758FCA82862CFA1078D145363F599C"
		"C385A304BAC1B92032152FB7885367A07095CE598485070D8962681F267E"
		"AB6B82E538514BB36A67189AA38AEA749AAF0D3FD49E102229A7410D772C"
		"5C3706E177C0CBC6C8E406B657E41CD1E87B9B33009D0DD82E89268E53F4"
		"50E8266C6194774113B9A0AC7C1D92661A0BEC3EA264482BD59B5B3558E0"
		"1E77A246A9B6B3EA6DBD46C4F7EA08B3C4790CC04E34F4DB5855EEA95FD7"
		"3BCA7BBB0F2959B8532588A980FF9247EAB1F4DC92D2504B2C987E9E8ABD"
		"BDF007B44068C9A71CD000EB34CC7EBE5174A69E504CD80BC16FBB681592"
		"509CAD60B251DA814E7A2FDEE77059CB2CEC7EB21A1BCE6F7E30F4FD1800"
		"191009211012000C0791F92E31086C760C7DA49402E1AE4EFF010031", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// conn close
	packetLayer->recvPacket("190200363340010030410200313646120039312E3130372E36352E36363B313937393B");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<close>", result->getString());
	result->clear();

	// conn closed
	tcpIp->remoteClose();
	TEST_STRING_EQUAL("1902003633410200313643010030", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// wait
	packetLayer->recvPacket("x19x02x00x32x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// approving payment
	packetLayer->recvPacket(
		"x00x03x00x31x30x30x04x03x00x36x34x33x06x0Ex00x32x30"
		"x31x38x31x30x31x36x31x39x31x35x32x33x0Ax10x00x2Ax2Ax2Ax2Ax2A"
		"x2Ax2Ax2Ax2Ax2Ax2Ax2Ax36x36x33x39x0Bx04x00x31x39x30x33x0Dx06"
		"x00x32x34x37x36x32x39x0Ex0Cx00x38x32x38x39x31x33x33x34x39x33"
		"x37x33x0Fx02x00x30x30x13x08x00xCExC4xCExC1xD0xC5xCDxCEx15x0E"
		"x00x32x30x31x38x31x30x31x36x31x39x31x35x32x33x17x02x00x2Dx31"
		"x19x01x00x31x1Ax02x00x2Dx31x1Bx08x00x30x30x32x36x36x32x38x36"
		"x1Cx09x00x31x31x31x31x31x31x31x31x31x27x01x00x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendApproved,1,10,0,0>", result->getString());
	result->clear();

	// sale complete
	commandLayer->saleComplete();
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();

	// payment request
	commandLayer->sale(0, 10);
	TEST_STRING_EQUAL("000800303030303031303004030036343319010031", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ack
	packetLayer->recvControl(Inpas::Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// wait
	packetLayer->recvPacket("x19x02x00x32x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// approving payment
	packetLayer->recvPacket(
		"x00x03x00x31x30x30x04x03x00x36x34x33x06x0Ex00x32x30"
		"x31x38x31x30x31x36x31x39x31x35x32x33x0Ax10x00x2Ax2Ax2Ax2Ax2A"
		"x2Ax2Ax2Ax2Ax2Ax2Ax2Ax36x36x33x39x0Bx04x00x31x39x30x33x0Dx06"
		"x00x32x34x37x36x32x39x0Ex0Cx00x38x32x38x39x31x33x33x34x39x33"
		"x37x33x0Fx02x00x30x30x13x08x00xCExC4xCExC1xD0xC5xCDxCEx15x0E"
		"x00x32x30x31x38x31x30x31x36x31x39x31x35x32x33x17x02x00x2Dx31"
		"x19x01x00x31x1Ax02x00x2Dx31x1Bx08x00x30x30x32x36x36x32x38x36"
		"x1Cx09x00x31x31x31x31x31x31x31x31x31x27x01x00x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendApproved,1,10,0,0>", result->getString());
	result->clear();

	// sale failed
	commandLayer->saleComplete();
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	return true;
}

bool InpasCommandLayerTest::testPressButtonBeforeCloseSession() {
	TEST_NUMBER_EQUAL(true, gotoStateWait());

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();

	// payment request
	commandLayer->sale(0, 10);
	TEST_STRING_EQUAL("000800303030303031303004030036343319010031", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ack
	packetLayer->recvControl(Inpas::Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// wait
	packetLayer->recvPacket("x19x02x00x32x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// approving payment
	packetLayer->recvPacket(
		"x00x03x00x31x30x30x04x03x00x36x34x33x06x0Ex00x32x30"
		"x31x38x31x30x31x36x31x39x31x35x32x33x0Ax10x00x2Ax2Ax2Ax2Ax2A"
		"x2Ax2Ax2Ax2Ax2Ax2Ax2Ax36x36x33x39x0Bx04x00x31x39x30x33x0Dx06"
		"x00x32x34x37x36x32x39x0Ex0Cx00x38x32x38x39x31x33x33x34x39x33"
		"x37x33x0Fx02x00x30x30x13x08x00xCExC4xCExC1xD0xC5xCDxCEx15x0E"
		"x00x32x30x31x38x31x30x31x36x31x39x31x35x32x33x17x02x00x2Dx31"
		"x19x01x00x31x1Ax02x00x2Dx31x1Bx08x00x30x30x32x36x36x32x38x36"
		"x1Cx09x00x31x31x31x31x31x31x31x31x31x27x01x00x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendApproved,1,10,0,0>", result->getString());
	result->clear();

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();
	return true;
}

bool InpasCommandLayerTest::testStateSessionTimeout() {
	TEST_NUMBER_EQUAL(true, gotoStateWait());

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();

	// session timeout
	timerEngine->tick(MDB_CL_SESSION_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	return true;
}

bool InpasCommandLayerTest::testStateSessionCancelByTerminal() {
	TEST_NUMBER_EQUAL(true, gotoStateWait());

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();

	// cancel
	packetLayer->recvControl(Inpas::Control_EOT);
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	return true;
}

bool InpasCommandLayerTest::testStateRequestTimeout() {
	TEST_NUMBER_EQUAL(true, gotoStateWait());

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();

	// payment request
	commandLayer->sale(0, 50);
	TEST_STRING_EQUAL("000800303030303035303004030036343319010031", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// request timeout
	timerEngine->tick(INPAS_RECV_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=1,VendDenied>", result->getString());
	result->clear();

	// close
	commandLayer->closeSession();
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	return true;
}

bool InpasCommandLayerTest::testStateApprovingTimeout() {
	TEST_NUMBER_EQUAL(true, gotoStateWait());

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();

	// payment request
	commandLayer->sale(0, 50);
	TEST_STRING_EQUAL("000800303030303035303004030036343319010031", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ack
	packetLayer->recvControl(Inpas::Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// wait
	packetLayer->recvPacket("x19x02x00x32x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// approve timeout
	timerEngine->tick(MDB_CL_APPROVING_TIMEOUT);
	timerEngine->execute();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=1,VendDenied>", result->getString());
	result->clear();

	// close
	commandLayer->closeSession();
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();

	// approving payment (after ephor timeout)
	packetLayer->recvPacket(
		"x00x03x00x31x30x30x04x03x00x36x34x33x06x0Ex00x32x30"
		"x31x38x31x30x31x36x31x39x31x35x32x33x0Ax10x00x2Ax2Ax2Ax2Ax2A"
		"x2Ax2Ax2Ax2Ax2Ax2Ax2Ax36x36x33x39x0Bx04x00x31x39x30x33x0Dx06"
		"x00x32x34x37x36x32x39x0Ex0Cx00x38x32x38x39x31x33x33x34x39x33"
		"x37x33x0Fx02x00x30x30x13x08x00xCExC4xCExC1xD0xC5xCDxCEx15x0E"
		"x00x32x30x31x38x31x30x31x36x31x39x31x35x32x33x17x02x00x2Dx31"
		"x19x01x00x31x1Ax02x00x2Dx31x1Bx08x00x30x30x32x36x36x32x38x36"
		"x1Cx09x00x31x31x31x31x31x31x31x31x31x27x01x00x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());
	result->clear();
	return true;
}

bool InpasCommandLayerTest::testStateApprovingCancelByTerminal() {
	TEST_NUMBER_EQUAL(true, gotoStateWait());

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();

	// payment request
	commandLayer->sale(0, 10);
	TEST_STRING_EQUAL("000800303030303031303004030036343319010031", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// cancel
	packetLayer->recvControl(Inpas::Control_EOT);
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=1,VendDenied>", result->getString());
	result->clear();

	// close
	commandLayer->closeSession();
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	return true;
}

//todo: тест не дописан
bool InpasCommandLayerTest::testStateApprovingCancelByUser() {
	TEST_NUMBER_EQUAL(true, gotoStateWait());

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();

	// payment request
	commandLayer->sale(0, 100);
	TEST_STRING_EQUAL("000800303030303031303004030036343319010031", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// cancel
	commandLayer->closeSession();
	packetLayer->recvControl(Inpas::Control_EOT);
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("", result->getString());
	result->clear();

	// close
	commandLayer->closeSession();
	timerEngine->tick(1);
	timerEngine->execute();
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	return true;
}

bool InpasCommandLayerTest::testPaymentCancel() {
	TEST_NUMBER_EQUAL(true, gotoStateWait());

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();

	// payment request
	commandLayer->sale(0, 10);
	TEST_STRING_EQUAL("000800303030303031303004030036343319010031", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ack
	packetLayer->recvControl(Inpas::Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// wait
	packetLayer->recvPacket("x19x02x00x32x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// approving payment
	packetLayer->recvPacket(
		"x00x03x00x31x30x30x04x03x00x36x34x33x06x0Ex00x32x30"
		"x31x38x31x30x31x36x31x39x31x35x32x33x0Ax10x00x2Ax2Ax2Ax2Ax2A"
		"x2Ax2Ax2Ax2Ax2Ax2Ax2Ax36x36x33x39x0Bx04x00x31x39x30x33x0Dx06"
		"x00x32x34x37x36x32x39x0Ex0Cx00x38x32x38x39x31x33x33x34x39x33"
		"x37x33x0Fx02x00x30x30x13x08x00xCExC4xCExC1xD0xC5xCDxCEx15x0E"
		"x00x32x30x31x38x31x30x31x36x31x39x31x35x32x33x17x02x00x2Dx31"
		"x19x01x00x31x1Ax02x00x2Dx31x1Bx08x00x30x30x32x36x36x32x38x36"
		"x1Cx09x00x31x31x31x31x31x31x31x31x31x27x01x00x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendApproved,1,10,0,0>", result->getString());
	result->clear();

	// sale failed
	commandLayer->saleFailed();
	TEST_STRING_EQUAL("00080030303030303130300403003634331902003533", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());
	timerEngine->tick(INPAS_RECV_TIMEOUT);
	timerEngine->execute();

	TEST_STRING_EQUAL("00080030303030303130300403003634331902003533", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());
	timerEngine->tick(INPAS_RECV_TIMEOUT);
	timerEngine->execute();

	TEST_STRING_EQUAL("00080030303030303130300403003634331902003533", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ack
	packetLayer->recvControl(Inpas::Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// wait
	packetLayer->recvPacket("x19x02x00x32x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// cancel payment
	packetLayer->recvPacket(
		"x00x03x00x31x30x30x04x03x00x36x34x33x06x0Ex00x32x30"
		"x31x38x31x30x31x36x31x39x31x35x32x33x0Ax10x00x2Ax2Ax2Ax2Ax2A"
		"x2Ax2Ax2Ax2Ax2Ax2Ax2Ax36x36x33x39x0Bx04x00x31x39x30x33x0Dx06"
		"x00x32x34x37x36x32x39x0Ex0Cx00x38x32x38x39x31x33x33x34x39x33"
		"x37x33x0Fx02x00x30x30x13x08x00xCExC4xCExC1xD0xC5xCDxCEx15x0E"
		"x00x32x30x31x38x31x30x31x36x31x39x31x35x32x33x17x02x00x2Dx31"
		"x19x02x00x35x33x1Ax02x00x2Dx31x1Bx08x00x30x30x32x36x36x32x38"
		"x36x1Cx09x00x31x31x31x31x31x31x31x31x31x27x01x00x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();
	return true;
}

bool InpasCommandLayerTest::testPaymentCancelFailed() {
	TEST_NUMBER_EQUAL(true, gotoStateWait());

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();

	// payment request
	commandLayer->sale(0, 10);
	TEST_STRING_EQUAL("000800303030303031303004030036343319010031", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ack
	packetLayer->recvControl(Inpas::Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// wait
	packetLayer->recvPacket("x19x02x00x32x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// approving payment
	packetLayer->recvPacket(
		"x00x03x00x31x30x30"
		"x04x03x00x36x34x33"
		"x06x0Ex00"
		"x32x30x31x38x31x30x31x36x31x39x31x35x32x33"
		"x0Ax10x00"
		"x2Ax2Ax2Ax2Ax2Ax2Ax2Ax2Ax2Ax2Ax2Ax2Ax36x36x33x39"
		"x0Bx04x00x31x39x30x33"
		"x0Dx06x00x32x34x37x36x32x39"
		"x0Ex0Cx00"
		"x38x32x38x39x31x33x33x34x39x33x37x33"
		"x0Fx02x00x30x30"
		"x13x08x00xCExC4xCExC1xD0xC5xCDxCE"
		"x15x0Ex00"
		"x32x30x31x38x31x30x31x36x31x39x31x35x32x33"
		"x17x02x00x2Dx31"
		"x19x01x00x31"
		"x1Ax02x00x2Dx31"
		"x1Bx08x00x30x30x32x36x36x32x38x36"
		"x1Cx09x00x31x31x31x31x31x31x31x31x31"
		"x27x01x00x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,VendApproved,1,10,0,0>", result->getString());
	result->clear();

	// sale failed
	commandLayer->saleFailed();
	for(uint16_t i = 0; i < INPAS_CANCEL_TRY_NUMBER; i++) {
		TEST_STRING_EQUAL("00080030303030303130300403003634331902003533", packetLayer->getSendData());
		packetLayer->clearSendData();
		TEST_STRING_EQUAL("", result->getString());
		timerEngine->tick(INPAS_RECV_TIMEOUT);
		timerEngine->execute();
	}
	TEST_STRING_EQUAL("<event=1,SessionEnd>", result->getString());
	result->clear();

	// p200 press button
	packetLayer->recvPacket("x00x05x00x32x30x30x30x30x04x03x00x36x34x33x56x06x00x32x30x31x30x30x30");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("<event=1,SessionBegin,2000>", result->getString());
	result->clear();
	return true;
}

bool InpasCommandLayerTest::testQrCode() {
	TEST_NUMBER_EQUAL(true, gotoStateWait());

	// payment request
	commandLayer->printQrCode("header1", "footer1", "text1");
	TEST_STRING_EQUAL("19020034313902003230400100345A0C00307844465E5E74657874317E470F00686561646572310A666F6F74657231", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ack
	packetLayer->recvControl(Inpas::Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	return true;
}

bool InpasCommandLayerTest::testVerification() {
	TEST_NUMBER_EQUAL(true, gotoStateWait());

	// verification request
	commandLayer->verification();
	TEST_STRING_EQUAL("19020035391B0000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// ack
	packetLayer->recvControl(Inpas::Control_ACK);
	TEST_STRING_EQUAL("", result->getString());
	TEST_STRING_EQUAL("", packetLayer->getSendData());

	// wait
	packetLayer->recvPacket("x19x02x00x32x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// verification response
	packetLayer->recvPacket(
		"x06x0Ex00"
		"x32x30x32x30x30x33x31x39x31x38x30x36x30x39"
		"x0Fx03x00x45x52x33"
		"x15x0Ex00"
		"x32x30x32x30x30x33x31x39x31x38x30x36x30x39"
		"x17x02x00x2Dx31"
		"x19x02x00x35x39"
		"x1Ax02x00x2Dx31"
		"x1Bx08x00x30x30x30x36x30x30x35x36"
		"x1Cx01x00x30"
		"x27x01x00x31");
	TEST_STRING_EQUAL("06", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());
	return true;
}
