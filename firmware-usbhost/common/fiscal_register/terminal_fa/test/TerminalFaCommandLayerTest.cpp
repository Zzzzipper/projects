#include "fiscal_register/terminal_fa/TerminalFaCommandLayer.h"
#include "fiscal_register/terminal_fa/TerminalFaProtocol.h"
#include "fiscal_register/TestFiscalRegisterEventEngine.h"
#include "timer/include/TimerEngine.h"
#include "uart/include/TestUart.h"
#include "utils/include/Hex.h"
#include "test/include/Test.h"
#include "logger/include/Logger.h"

class TestTerminalFaPacketLayer : public TerminalFa::PacketLayerInterface {
public:
	TestTerminalFaPacketLayer() : recvBuf(256) {}
	virtual ~TestTerminalFaPacketLayer() {}
	void clearSendData() { sendBuf.clear(); }
	const char *getSendData() { return sendBuf.getString(); }
	void recvData(const char *hex) {
		uint16_t len = hexToData(hex, strlen(hex), recvBuf.getData(), recvBuf.getSize());
		recvBuf.setLen(len);
		observer->procRecvData(recvBuf.getData(), recvBuf.getLen());
	}
	void recvError(Error error) {
		observer->procRecvError(error);
	}

	virtual void setObserver(Observer *observer) { this->observer = observer; }
	virtual bool sendPacket(Buffer *data) {
		for(uint16_t i = 0; i < data->getLen(); i++) {
			sendBuf.addHex((*data)[i]);
		}
		return true;
	}

private:
	Observer *observer;
	StringBuilder sendBuf;
	Buffer recvBuf;
};

class TerminalFaCommandLayerTest : public TestSet {
public:
	TerminalFaCommandLayerTest();
	bool init();
	void cleanup();
	bool testCheck();
	bool testCheckAndOpenedDocument();
	bool testCheckAndClosedShift();
	bool testCheckAndOldShift();

private:
	StringBuilder *result;
	TestUart *uart;
	TimerEngine *timerEngine;
	TestFiscalRegisterEventEngine *eventEngine;
	TestTerminalFaPacketLayer *packetLayer;
	TerminalFa::CommandLayer *client;
};

TEST_SET_REGISTER(TerminalFaCommandLayerTest);

TerminalFaCommandLayerTest::TerminalFaCommandLayerTest() {
	TEST_CASE_REGISTER(TerminalFaCommandLayerTest, testCheck);
	TEST_CASE_REGISTER(TerminalFaCommandLayerTest, testCheckAndOpenedDocument);
	TEST_CASE_REGISTER(TerminalFaCommandLayerTest, testCheckAndClosedShift);
	TEST_CASE_REGISTER(TerminalFaCommandLayerTest, testCheckAndOldShift);
}

bool TerminalFaCommandLayerTest::init() {
	result = new StringBuilder;
	uart = new TestUart(256);
	timerEngine = new TimerEngine();
	eventEngine = new TestFiscalRegisterEventEngine(result);
	packetLayer = new TestTerminalFaPacketLayer;
	client = new TerminalFa::CommandLayer(packetLayer, timerEngine, eventEngine);
	return true;
}

void TerminalFaCommandLayerTest::cleanup() {
	delete client;
	delete packetLayer;
	delete eventEngine;
	delete timerEngine;
	delete uart;
	delete result;
}

bool TerminalFaCommandLayerTest::testCheck() {
	// send Status
	Fiscal::Sale saleData;
	saleData.setProduct("7", 0, "“есточино", 5000, Fiscal::TaxRate_NDSNone, 1);
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = 10000;
	saleData.taxSystem = Fiscal::TaxSystem_ENVD;
	client->sale(&saleData, 2);
	TEST_STRING_EQUAL("01", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv Status
	packetLayer->recvData("00353530313031303030303237120315121200000103");

	// send FiscalStorageState
	TEST_STRING_EQUAL("08", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv FiscalStorageState (Ready to register)
	packetLayer->recvData("0003000001001203151224393939393037383930303030373331370D000000");

	// send CheckOpen
	TEST_STRING_EQUAL("23", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckOpen
	packetLayer->recvData("00");

	// send CheckAdd
	TEST_STRING_EQUAL(
		"2B" // номер команды
		"23042300"                   // 1059 Tag_Product (STLV)
		"0604090092A5E1E2AEE7A8ADAE" // 1030 Tag_ProductName
		"370402008813"               // 1079 Tag_ProductPrice
		"FF0302000001"               // 1023 Tag_ProductNumber
		"AF04010006"                 // 1099 Tag_TaxRate
		"BE04010004",                // 1214 Tag_PaymentMethod
		packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckAdd
	packetLayer->recvData("00");

	// send CheckPayment
	TEST_STRING_EQUAL(
		"2D"
		"1F04010008"   // 1055
		"070402008813" // 1031
		"3904010000"   // 1081
		"BF04010000"   // 1215
		"C004010000"   // 1216
		"C104010000"   // 1217
		"F0030000",    // 1008
		packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckPayment
	packetLayer->recvData("00");

	// send CheckClose
	TEST_STRING_EQUAL("24018813000000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckClose
	packetLayer->recvData("000A000D00000029FE4037");
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	return true;
}

bool TerminalFaCommandLayerTest::testCheckAndOpenedDocument() {
	Fiscal::Sale saleData;
	saleData.setProduct("7", 0, "“есточино", 5000, Fiscal::TaxRate_NDSNone, 1);
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = 10000;
	saleData.taxSystem = Fiscal::TaxSystem_ENVD;
	client->sale(&saleData, 2);

	// send Status
	TEST_STRING_EQUAL("01", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv Status
	packetLayer->recvData("00353530313031303030303237120315121200000103");

	// send FiscalStorageState
	TEST_STRING_EQUAL("08", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv FiscalStorageState (OpenedDocument)
	packetLayer->recvData("0003040001001203151224393939393037383930303030373331370D000000");

	// send DocumentReset
	TEST_STRING_EQUAL("10", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv DocumentReset
	packetLayer->recvData("00");

	// send FiscalStorageState (Ready to register)
	TEST_STRING_EQUAL("08", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv FiscalStorageState
	packetLayer->recvData("0003000001001203151224393939393037383930303030373331370D000000");

	// send CheckOpen
	TEST_STRING_EQUAL("23", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckOpen
	packetLayer->recvData("00");

	// send CheckAdd
	TEST_STRING_EQUAL(
		"2B" // номер команды
		"23042300" // STLV 1059
		"0604090092A5E1E2AEE7A8ADAE" // название продукта
		"370402008813" // цена
		"FF0302000001"
		"AF04010006"
		"BE04010004",
		packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckAdd
	packetLayer->recvData("00");

	// send CheckPayment
	TEST_STRING_EQUAL(
		"2D"
		"1F04010008"
		"070402008813"
		"3904010000"
		"BF04010000"
		"C004010000"
		"C104010000"
		"F0030000",
		packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckPayment
	packetLayer->recvData("00");

	// send CheckClose
	TEST_STRING_EQUAL("24018813000000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckClose
	packetLayer->recvData("000A000D00000029FE4037");
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	return true;
}

bool TerminalFaCommandLayerTest::testCheckAndClosedShift() {
	Fiscal::Sale saleData;
	saleData.setProduct("7", 0, "“есточино", 5000, Fiscal::TaxRate_NDSNone, 1);
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = 10000;
	saleData.taxSystem = Fiscal::TaxSystem_ENVD;
	client->sale(&saleData, 2);

	// send Status
	TEST_STRING_EQUAL("01", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv Status
	packetLayer->recvData("00353530313031303030303237120315121200000103");

	// send FiscalStorageState
	TEST_STRING_EQUAL("08", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv FiscalStorageState (ClosedShift)
	packetLayer->recvData("0003000000001203151316393939393037383930303030373331370E000000");

	// send ShiftOpenStart
	TEST_STRING_EQUAL("2101", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv ShiftOpenFinish
	packetLayer->recvData("00");

	// send ShiftOpenFinish
	TEST_STRING_EQUAL("22", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv ShiftOpenStart
	packetLayer->recvData("00030014000000526107B7");

	// send FiscalStorageState (Ready to register)
	TEST_STRING_EQUAL("08", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv FiscalStorageState
	packetLayer->recvData("0003000001001203151224393939393037383930303030373331370D000000");

	// send CheckOpen
	TEST_STRING_EQUAL("23", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckOpen
	packetLayer->recvData("00");

	// send CheckAdd
	TEST_STRING_EQUAL(
		"2B" // номер команды
		"23042300" // STLV 1059
		"0604090092A5E1E2AEE7A8ADAE" // название продукта
		"370402008813" // цена
		"FF0302000001"
		"AF04010006"
		"BE04010004",
		packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckAdd
	packetLayer->recvData("00");

	// send CheckPayment
	TEST_STRING_EQUAL(
		"2D"
		"1F04010008"
		"070402008813"
		"3904010000"
		"BF04010000"
		"C004010000"
		"C104010000"
		"F0030000",
		packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckPayment
	packetLayer->recvData("00");

	// send CheckClose
	TEST_STRING_EQUAL("24018813000000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckClose
	packetLayer->recvData("000A000D00000029FE4037");
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	return true;
}

bool TerminalFaCommandLayerTest::testCheckAndOldShift() {
	Fiscal::Sale saleData;
	saleData.setProduct("7", 0, "“есточино", 5000, Fiscal::TaxRate_NDSNone, 1);
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = 10000;
	saleData.taxSystem = Fiscal::TaxSystem_ENVD;
	client->sale(&saleData, 2);

	// send Status
	TEST_STRING_EQUAL("01", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv Status
	packetLayer->recvData("00353530313031303030303237120315121200000103");

	// send FiscalStorageState
	TEST_STRING_EQUAL("08", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv FiscalStorageState (Ready to register)
	packetLayer->recvData("0003000001001203151224393939393037383930303030373331370D000000");

	// send CheckOpen
	TEST_STRING_EQUAL("23", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckOpen
	packetLayer->recvData("0116");

	// send ShiftCloseStart
	TEST_STRING_EQUAL("2901", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv ShiftCloseStart
	packetLayer->recvData("00");

	// send ShiftCloseFinish
	TEST_STRING_EQUAL("2A", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv ShiftCloseFinish
	packetLayer->recvData("00");

	// send ShiftOpenStart
	TEST_STRING_EQUAL("2101", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv ShiftOpenFinish
	packetLayer->recvData("00");

	// send ShiftOpenFinish
	TEST_STRING_EQUAL("22", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv ShiftOpenStart
	packetLayer->recvData("00030014000000526107B7");

	// send FiscalStorageState (Ready to register)
	TEST_STRING_EQUAL("08", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv FiscalStorageState
	packetLayer->recvData("0003000001001203151224393939393037383930303030373331370D000000");

	// send CheckOpen
	TEST_STRING_EQUAL("23", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckOpen
	packetLayer->recvData("00");

	// send CheckAdd
	TEST_STRING_EQUAL(
		"2B" // номер команды
		"23042300" // STLV 1059
		"0604090092A5E1E2AEE7A8ADAE" // название продукта
		"370402008813" // цена
		"FF0302000001"
		"AF04010006"
		"BE04010004",
		packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckAdd
	packetLayer->recvData("00");

	// send CheckPayment
	TEST_STRING_EQUAL(
		"2D"
		"1F04010008"
		"070402008813"
		"3904010000"
		"BF04010000"
		"C004010000"
		"C104010000"
		"F0030000",
		packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckPayment
	packetLayer->recvData("00");

	// send CheckClose
	TEST_STRING_EQUAL("24018813000000", packetLayer->getSendData());
	packetLayer->clearSendData();
	TEST_STRING_EQUAL("", result->getString());

	// recv CheckClose
	packetLayer->recvData("000A000D00000029FE4037");
	TEST_STRING_EQUAL("", packetLayer->getSendData());
	TEST_STRING_EQUAL("<event=CommandOK>", result->getString());
	return true;
}
