#include "CodeScanner.h"

#if defined(ARM)
#include "uart/stm32/include/uart.h"
#endif
#include "logger/RemoteLogger.h"
#include "logger/include/Logger.h"

#define SCANNER_MANUFACTURER "EFR"
#define SCANNER_MODEL "SCANNER"

CodeScannerFake::CodeScannerFake() :
	observers(5)
{
//	this->observers = new List<Observer>(5);
}

void CodeScannerFake::addObserver(Observer *observer) {
	observers.add(observer);
}

void CodeScannerFake::test() {
	testUnitex1();
//	testNefteMag();
//	testErpOrder();
}

void CodeScannerFake::procCode(uint8_t *code, uint32_t codeLen) {
	LOG_INFO(LOG_SCANNER, "code " << codeLen);
	LOG_INFO_HEX(LOG_SCANNER, code, codeLen);
	for(uint16_t i = 0; i < observers.getSize(); i++) {
		REMOTE_LOG(RLOG_SCANNER, "$");
		Observer *observer = observers.get(i);
		if(observer->procCode(code, codeLen) == true) {
			return;
		}
	}
}

void CodeScannerFake::testUnitex1() {
	//0003 0001 19 06 05 19 06 05 19 06 05 000066 000201 01 63fe
	//0006 0001 20 01 28 20 01 28 20 01 28 000050 000004 01 a0e0
	//0006 0001 20 01 28 20 01 28 20 01 28 000051 000005 01 a694
	uint8_t test[] = {
			'0', '0', '0', '3', '0', '0', '0', '1',
			'1', '9', '0', '6', '0', '5',
			'1', '9', '0', '6', '0', '5',
			'1', '9', '0', '6', '0', '5',
			'0', '0', '0', '0', '6', '6',
			'0', '0', '0', '0', '0', '1',
			'0', '1', '6', '3', 'f', 'e' };
	uint32_t testLen = sizeof(test);
	LOG_INFO(LOG_SCANNER, "Test Unitex1 " << testLen);
	procCode(test, testLen);
}

void CodeScannerFake::testNefteMag() {
	uint8_t test[] = { '1', '0', '0', '4', '0',
			'A', 'A', 'E', 'C', 'A', 'w', 'Q', 'F', 'B', 'g',
			'c', 'I', 'C', 'Q', 'o', 'L', 'D', 'A', '0', 'O',
			'D', 'w', 'A', 'B', 'A', 'g', 'M', 'E', 'B', 'Q',
			'Y', 'H', 'C', 'A', 'k', 'K', 'C', 'w', 'w', 'N',
			'D', 'g', '8', 'A', 'A', 'Q', 'I', 'D', 'B', 'A', //50
			'U', 'G', 'B', 'w', 'g', 'J', 'C', 'g', 's', 'M',
			'D', 'Q', '4', 'P', 'A', 'A', 'E', 'C', 'A', 'w',
			'Q', 'F', 'B', 'g', 'c', 'I', 'C', 'Q', 'o', 'L',
			'D', 'A', '0', 'O', 'D', 'w', 'A', 'B', 'A', 'g',
			'M', 'E', 'B', 'Q', 'Y', 'H', 'C', 'A', 'k', 'K', //100
			'C', 'w', 'w', 'N', 'D', 'g', '8', 'A', 'A', 'Q',
			'I', 'D', 'B', 'A', 'U', 'G', 'B', 'w', 'g', 'J',
			'C', 'g', 's', 'M', 'D', 'Q', '4', 'P', 'A', 'A',
			'E', 'C', 'A', 'w', 'Q', 'F', 'B', 'g', 'c', 'I',
			'C', 'Q', 'o', 'L', 'D', 'A', '0', 'O', 'D', 'w', //150
			'A', 'B', 'A', 'g', 'M', 'E', 'B', 'Q', 'Y', 'H',
			'C', 'A', 'k', 'K', 'C', 'w', 'w', 'N', 'D', 'g',
			'8', '=' };
	uint32_t testLen = sizeof(test);
	LOG_INFO(LOG_SCANNER, "Test NefteMag " << testLen);
	procCode(test, testLen);
}

void CodeScannerFake::testErpOrder() {
	uint8_t test[] = { '1', '0', '0', '5', '0',  '1', '1', '1', '1', '1' };
	uint32_t testLen = sizeof(test);
	LOG_INFO(LOG_SCANNER, "Test ErpOrder " << testLen);
	procCode(test, testLen);
}

CodeScanner::CodeScanner(
	Mdb::DeviceContext *context,
	AbstractUart *uart,
	TimerEngine *timers
) :
	context(context),
	timers(timers),
	uart(uart),
	observers(5),
	data(128)
{
	this->context->setManufacturer((uint8_t*)SCANNER_MANUFACTURER, sizeof(SCANNER_MANUFACTURER));
	this->context->setModel((uint8_t*)SCANNER_MODEL, sizeof(SCANNER_MODEL));
	this->context->setState(0);
	this->context->setStatus(Mdb::DeviceContext::Status_NotFound);
	this->timer = timers->addTimer<CodeScanner, &CodeScanner::procTimer>(this);
	uart->setReceiveHandler(this);
}

void CodeScanner::addObserver(Observer *observer) {
	observers.add(observer);
}

void CodeScanner::on() {
	LOG_INFO(LOG_SCANNER, "on");
#if defined(ARM)
	RELE1_ON;
#endif
}

void CodeScanner::off() {
	LOG_INFO(LOG_SCANNER, "off");
#if defined(ARM)
	RELE1_OFF;
#endif
}

void CodeScanner::test() {
	LOG_INFO(LOG_SCANNER, "test");
	uint8_t test[] = { '1', '1', '1', '1', '1' };
	data.clear();
	data.add(test, sizeof(test));
	procCode(data.getData(), data.getLen());
}

void CodeScanner::handle() {
	while(uart->isEmptyReceiveBuffer() == false) {
		uint8_t b = uart->receive();
		LOG_TRACE(LOG_SCANNER, "lex=" << b);
		if(b == 0x0A || b == 0x0D) {
			if(data.getLen() > 0) {
				procCode(data.getData(), data.getLen());
			}
			timer->stop();
			data.clear();
		} else {
			data.addUint8(b);
			timer->start(1000);
		}
	}
}

void CodeScanner::procTimer() {
	LOG_INFO(LOG_SCANNER, "procTimer " << data.getLen());
	data.clear();
}

void CodeScanner::procCode(uint8_t *code, uint32_t codeLen) {
	LOG_INFO(LOG_SCANNER, "code " << codeLen);
	LOG_INFO_HEX(LOG_SCANNER, code, codeLen);
	REMOTE_LOG_IN(RLOG_SCANNER, code, codeLen);
	context->setStatus(Mdb::DeviceContext::Status_Work);
	for(uint16_t i = 0; i < observers.getSize(); i++) {
		Observer *observer = observers.get(i);
		if(observer->procCode(code, codeLen) == true) {
			return;
		}
	}
}
