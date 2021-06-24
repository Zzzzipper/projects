#include "sim900/GsmFirmwareUpdater.h"
#include "sim900/test/TestGsmHardware.h"
#include "timer/include/TimerEngine.h"
#include "uart/include/TestUart.h"
#include "utils/include/TestEventObserver.h"
#include "logger/include/Logger.h"
#include "test/include/Test.h"

namespace Gsm {

class TestFirmwareUpdaterObserver : public TestEventObserver {
public:
	TestFirmwareUpdaterObserver(StringBuilder *result) : TestEventObserver(result) {}
	virtual void proc(Event *event) {
		switch(event->getType()) {
		case Dex::DataParser::Event_AsyncOk: *result << "<event=AsyncOk>"; break;
		default: *result << "<event=" << event->getType() << ">";
		}
	}
};

class FirmwareUpdaterTest : public TestSet {
public:
	FirmwareUpdaterTest();
	bool init();
	void cleanup();
	bool testCrc();
	bool test();
	bool testHeaderEraseError();

private:
	StringBuilder *result;
	Gsm::TestHardware *hardware;
	TestUart *uart;
	TimerEngine *timers;
	TestFirmwareUpdaterObserver *observer;
	FirmwareUpdater *updater;
};

TEST_SET_REGISTER(Gsm::FirmwareUpdaterTest);

FirmwareUpdaterTest::FirmwareUpdaterTest() {
	TEST_CASE_REGISTER(FirmwareUpdaterTest, testCrc);
	TEST_CASE_REGISTER(FirmwareUpdaterTest, test);
	TEST_CASE_REGISTER(FirmwareUpdaterTest, testHeaderEraseError);
}

bool FirmwareUpdaterTest::init() {
	this->result = new StringBuilder(1024, 1024);
	this->hardware = new Gsm::TestHardware(result);
	this->uart = new TestUart(256);
	this->timers = new TimerEngine;
	this->observer = new TestFirmwareUpdaterObserver(result);
	this->updater = new FirmwareUpdater(hardware, uart, timers);
	this->updater->setObserver(observer);
	return true;
}

void FirmwareUpdaterTest::cleanup() {
	delete this->updater;
	delete this->observer;
	delete this->timers;
	delete this->uart;
	delete this->hardware;
	delete this->result;
}

bool FirmwareUpdaterTest::testCrc() {
	uint8_t data[] = {
		0x4d, 0x4d, 0x4d, 0x01, 0x38, 0x00, 0x00, 0x00, 0x46, 0x49, 0x4c, 0x45, 0x5f, 0x49, 0x4e, 0x46,
		0x4f, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x00, 0x00, 0xb0, 0x00, 0x10,
		0x98, 0x06, 0x0b, 0x00, 0xff, 0xff, 0xff, 0xff, 0x38, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x38, 0x04, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x4d, 0x4d, 0x4d, 0x02, 0x30, 0x01, 0x00, 0x02,
		0x53, 0x49, 0x4d, 0x38, 0x30, 0x30, 0x4d, 0x33, 0x32, 0x5f, 0x50, 0x43, 0x42, 0x30, 0x31, 0x5f,
		0x67, 0x70, 0x72, 0x73, 0x5f, 0x4d, 0x54, 0x36, 0x32, 0x36, 0x30, 0x5f, 0x53, 0x30, 0x30, 0x2e,
		0x31, 0x33, 0x30, 0x38, 0x42, 0x30, 0x38, 0x53, 0x49, 0x4d, 0x38, 0x30, 0x30, 0x4d, 0x33, 0x32,
		0x2e, 0x62, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x31, 0x33, 0x30, 0x38, 0x42, 0x30, 0x38, 0x53, 0x49, 0x4d, 0x38, 0x30, 0x30, 0x4d, 0x33, 0x32,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x4d, 0x4d, 0x01, 0x7c, 0x00, 0x07, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xc2, 0x00, 0x25, 0x00,
		0x36, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x3b, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x4d, 0x4d, 0x4d, 0x02, 0x20, 0x00, 0x0e, 0x02, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	uint32_t exp = 0x2152;
	uint32_t act = 0;
	for(uint16_t i = 0; i < sizeof(data); i++) {
		act += data[i];
	}
	TEST_NUMBER_EQUAL(exp, act);
	return true;
}

//10/10/2017 18:33:54:105 SEND: 03 00 02 00 00
//10/10/2017 18:33:54:136 SEND:
bool FirmwareUpdaterTest::test() {
#if 0 //todo: ����������� � ������������ GSM-������
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Ok, updater->start(0));
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Start delay
	timers->tick(POWER_BUTTON_PRESS);
	timers->execute();

	uint8_t data1[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
		0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	};
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Busy, updater->procData(data1, sizeof(data1)));

	timers->tick(POWER_BUTTON_DELAY);
	timers->execute();
#else
	uint8_t data1[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
		0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	};

	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Ok, updater->start(0));
	TEST_STRING_EQUAL("<Gsm::Hardware::init><Gsm::Hardware::pressPowerButton>", result->getString());
	result->clear();
#endif

	// Synchronization
	TEST_HEXDATA_EQUAL("B5", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Busy, updater->procData(data1, sizeof(data1)));
	TEST_STRING_EQUAL("", result->getString());

	timers->tick(SYNC_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("B5", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Busy, updater->procData(data1, sizeof(data1)));
	TEST_STRING_EQUAL("", result->getString());

	timers->tick(SYNC_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("B5", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Busy, updater->procData(data1, sizeof(data1)));
	TEST_STRING_EQUAL("", result->getString());

	// Sync OK
	uart->addRecvData(Control_SyncComplete);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", result->getString());

	// Recv data
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Ok, updater->procData(data1, sizeof(data1)));
	TEST_HEXDATA_EQUAL("01"
		"000102030405060708090A0B0C0D0E0F"
		"101112131415161718191A1B1C1D1E1F"
		"202122232425262728292A2B2C2D2E2F"
		"303132333435363738393A3B3C3D3E3F"
		"404142434445464748494A4B4C4D4E4F"
		"505152535455565758595A5B5C5D5E5F"
		"606162636465666768696A6B6C6D6E6F"
		"707172737475767778797A7B7C7D7E7F",
		uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	uint8_t data2[] = {
		0x22, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x22, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
		0x22, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
		0x22, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
		0x22, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
		0x22, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
		0x22, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
		0x22, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	};

	// Erasing
	uart->addRecvData(Control_Erasing);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Busy, updater->procData(data2, sizeof(data2)));
	TEST_STRING_EQUAL("", result->getString());

	uart->addRecvData(Control_Erasing);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Busy, updater->procData(data2, sizeof(data2)));
	TEST_STRING_EQUAL("", result->getString());

	uart->addRecvData(Control_Erasing);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Busy, updater->procData(data2, sizeof(data2)));
	TEST_STRING_EQUAL("", result->getString());

	uart->addRecvData("020008");
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());

	// Writing
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Async, updater->procData(data2, sizeof(data2)));
	TEST_HEXDATA_EQUAL("0380000000"
		"220102030405060708090A0B0C0D0E0F"
		"221112131415161718191A1B1C1D1E1F"
		"222122232425262728292A2B2C2D2E2F"
		"223132333435363738393A3B3C3D3E3F"
		"224142434445464748494A4B4C4D4E4F"
		"225152535455565758595A5B5C5D5E5F"
		"226162636465666768696A6B6C6D6E6F"
		"227172737475767778797A7B7C7D7E7F"
		"101F0000",
		uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_STRING_EQUAL("", result->getString());

	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Busy, updater->procData(data2, sizeof(data2)));

	uart->addRecvData(Control_DataConfirm);
	TEST_STRING_EQUAL("<event=AsyncOk>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", result->getString());

	// Writing
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Async, updater->procData(data2, sizeof(data2)));
	TEST_HEXDATA_EQUAL("0380000000"
		"220102030405060708090A0B0C0D0E0F"
		"221112131415161718191A1B1C1D1E1F"
		"222122232425262728292A2B2C2D2E2F"
		"223132333435363738393A3B3C3D3E3F"
		"224142434445464748494A4B4C4D4E4F"
		"225152535455565758595A5B5C5D5E5F"
		"226162636465666768696A6B6C6D6E6F"
		"227172737475767778797A7B7C7D7E7F"
		"101F0000",
		uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	uart->addRecvData(Control_DataConfirm);
	TEST_STRING_EQUAL("<event=AsyncOk>", result->getString());
	result->clear();
	TEST_STRING_EQUAL("", result->getString());

	// Completing
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Async, updater->complete());
	TEST_STRING_EQUAL("", result->getString());

	// Ending
	TEST_HEXDATA_EQUAL("05", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	uart->addRecvData(Control_DataEndConfirm);
	TEST_STRING_EQUAL("", result->getString());

	// Reset
	TEST_HEXDATA_EQUAL("07", uart->getSendData(), uart->getSendLen());
	uart->addRecvData(Control_ResetConfirm);
	TEST_STRING_EQUAL("<event=AsyncOk>", result->getString());

	return true;
}

bool FirmwareUpdaterTest::testHeaderEraseError() {
#if 0 //todo: ����������� � ������������ GSM-������
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Ok, updater->start(0));
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Start delay
	timers->tick(POWER_BUTTON_PRESS);
	timers->execute();

	uint8_t data1[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
		0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	};
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Busy, updater->procData(data1, sizeof(data1)));

	timers->tick(POWER_BUTTON_DELAY);
	timers->execute();
#else
	uint8_t data1[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
		0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	};

	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Ok, updater->start(0));
#endif

	// Synchronization
	TEST_HEXDATA_EQUAL("B5", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Busy, updater->procData(data1, sizeof(data1)));

	timers->tick(SYNC_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("B5", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Busy, updater->procData(data1, sizeof(data1)));

	timers->tick(SYNC_TIMEOUT);
	timers->execute();
	TEST_HEXDATA_EQUAL("B5", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Busy, updater->procData(data1, sizeof(data1)));

	// Sync OK
	uart->addRecvData(Control_SyncComplete);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	// Recv data
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Ok, updater->procData(data1, sizeof(data1)));
	TEST_HEXDATA_EQUAL("01"
		"000102030405060708090A0B0C0D0E0F"
		"101112131415161718191A1B1C1D1E1F"
		"202122232425262728292A2B2C2D2E2F"
		"303132333435363738393A3B3C3D3E3F"
		"404142434445464748494A4B4C4D4E4F"
		"505152535455565758595A5B5C5D5E5F"
		"606162636465666768696A6B6C6D6E6F"
		"707172737475767778797A7B7C7D7E7F",
		uart->getSendData(), uart->getSendLen());
	uart->clearSendBuffer();

	uint8_t data2[] = {
		0x22, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x22, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
		0x22, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
		0x22, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
		0x22, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
		0x22, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
		0x22, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
		0x22, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	};

	// Erasing
	uart->addRecvData(Control_EraseError);
	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Error, updater->procData(data2, sizeof(data2)));

	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Error, updater->procData(data2, sizeof(data2)));

	TEST_HEXDATA_EQUAL("", uart->getSendData(), uart->getSendLen());
	TEST_NUMBER_EQUAL(Dex::DataParser::Result_Error, updater->procData(data2, sizeof(data2)));
	return true;
}

}
