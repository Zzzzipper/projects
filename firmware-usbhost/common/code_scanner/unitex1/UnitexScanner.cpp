#include "common/code_scanner/unitex1/UnitexProtocol.h"
#include "common/code_scanner/unitex1/UnitexScanner.h"
#include "common/utils/include/Hex.h"
#include "logger/RemoteLogger.h"
#include "common/logger/include/Logger.h"

#define UNITEX_QR_SESSION_TIMEOUT 30000
#define UNITEX_QR_APPROIVING_TIMEOUT 30000
#define UNITEX_QR_VENDING_TIMEOUT 120000
#define UNITEX_POOL_SIZE 20

void UnitexSale::clear() {
	this->year = 0;
	this->month = 0;
	this->day = 0;
	this->checkNum = 0;
	this->cashlessId = 0;
}

bool operator==(const UnitexSale &s1, const UnitexSale &s2) {
	if(s1.year != s2.year) { return false; }
	if(s1.month != s2.month) { return false; }
	if(s1.day != s2.day) { return false; }
	if(s1.checkNum != s2.checkNum) { return false; }
	if(s1.cashlessId != s2.cashlessId) { return false; }
	return true;
}

UnitexSales::UnitexSales() {
	pool = new Fifo<UnitexSale*>(UNITEX_POOL_SIZE);
	for(uint16_t i = 0; i < UNITEX_POOL_SIZE; i++) {
		UnitexSale *sale = new UnitexSale;
		pool->push(sale);
	}
	fifo = new Fifo<UnitexSale*>(UNITEX_POOL_SIZE);
}

UnitexSales::~UnitexSales() {
	delete fifo;
	delete pool;
}

void UnitexSales::clear() {
	while(fifo->isEmpty() == false) {
		UnitexSale *entry = fifo->pop();
		pool->push(entry);
	}
}

bool UnitexSales::push(UnitexSale *saleData) {
	if(isUsed(saleData) == true) {
		return false;
	}

	if(fifo->isFull() == true) {
		UnitexSale *entry = fifo->pop();
		pool->push(entry);
	}

	UnitexSale *entry = pool->pop();
	if(entry == NULL) {
		return false;
	}

	*entry = *saleData;
	fifo->push(entry);
	return true;
}

bool UnitexSales::isUsed(UnitexSale *saleData) {
	for(uint32_t i = 0; i < fifo->getSize(); i++) {
		if(*(fifo->get(i)) == *saleData) {
			return true;
		}
	}

	return false;
}

UnitexScanner::UnitexScanner(
	CodeScannerInterface *scanner,
	TimerEngine *timerEngine,
	EventEngine *eventEngine,
	uint32_t maxCredit
) :
	scanner(scanner),
	timerEngine(timerEngine),
	eventEngine(eventEngine),
	deviceId(eventEngine),
	maxCredit(maxCredit),
	state(State_Idle)
{
	this->timer = timerEngine->addTimer<UnitexScanner, &UnitexScanner::procTimer>(this);
	scanner->addObserver(this);
}

UnitexScanner::~UnitexScanner() {
	timerEngine->deleteTimer(this->timer);
}

EventDeviceId UnitexScanner::getDeviceId() {
	return deviceId;
}

void UnitexScanner::reset() {
	LOG_INFO(LOG_SCANNER, "reset");
	state = State_Wait;
}

bool UnitexScanner::isRefundAble() {
	return false;
}

void UnitexScanner::disable() {
	LOG_INFO(LOG_SCANNER, "off");
	scanner->off();
}

void UnitexScanner::enable() {
	LOG_INFO(LOG_SCANNER, "on");
	scanner->on();
}

bool UnitexScanner::revalue(uint32_t credit) {
	return false;
}

bool UnitexScanner::sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) {
	LOG_INFO(LOG_SCANNER, "sale " << state);
	this->productId = productId;
	this->productPrice = productPrice;
	if(state == State_Session) {
		stateApprovingProcCode();
		return true;
	} else {
		gotoStateApproving();
		return true;
	}
}

bool UnitexScanner::saleComplete() {
	LOG_INFO(LOG_SCANNER, "saleComplete");
	if(state != State_Vending) {
		return false;
	}
	gotoStateClosing();
	return true;
}

bool UnitexScanner::saleFailed() {
	LOG_INFO(LOG_SCANNER, "saleFailed");
	if(state != State_Vending) {
		return false;
	}
	gotoStateClosing(); // todo: gotoStatePaymentCancel
	return true;
}

bool UnitexScanner::closeSession() {
	LOG_INFO(LOG_SCANNER, "closeSession");
	gotoStateClosing();
	return true;
}

void UnitexScanner::procTimer() {
	LOG_DEBUG(LOG_SCANNER, "procTimer");
	switch(state) {
		case State_Session: stateSessionTimeout(); break;
		case State_Approving: stateApprovingTimeout(); break;
		case State_Vending: stateVendintTimeout(); break;
		case State_Closing: stateClosingTimeout(); break;
		default: LOG_ERROR(LOG_SCANNER, "Unwaited timeout " << state);
	}
}

//0003 0001 19 06 05 19 06 05 19 06 05 000066 000201 01 63fe
bool UnitexScanner::procCode(uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_SCANNER, "procCode " << state << "," << dataLen);
	LOG_INFO_HEX(LOG_SCANNER, data, dataLen);
	uint16_t reqLen = sizeof(Unitex::CodeRequest);
	if(dataLen != reqLen) {
		LOG_ERROR(LOG_SCANNER, "Wrong code len " << dataLen << "<>" << reqLen);
		return false;
	}

	Unitex::CodeRequest *req = (Unitex::CodeRequest*)data;
	code.year = req->year1.get();
	code.month = req->month1.get();
	code.day = req->day1.get();
	code.checkNum = req->checkNum.get();
	code.cashlessId = req->cashlessId.get();
	LOG_WARN(LOG_SCANNER, "data " << code.year << "." << code.month << "." << code.day << "," << code.checkNum << "," << code.cashlessId);

	switch(state) {
		case State_Wait: return stateWaitProcCode();
		case State_Approving: return stateApprovingProcCode();
		default: LOG_ERROR(LOG_SCANNER, "Unwaited code " << state); return false;
	}
}

bool UnitexScanner::stateWaitProcCode() {
	LOG_DEBUG(LOG_SCANNER, "stateWaitProcCode");
	if(sales.isUsed(&code) == true) {
		LOG_INFO(LOG_SCANNER, "already used " << code.checkNum);
		return true;
	}
	gotoStateSession();
	EventUint32Interface event(deviceId, MdbMasterCashless::Event_SessionBegin, maxCredit);
	eventEngine->transmit(&event);
	return true;
}

void UnitexScanner::gotoStateSession() {
	LOG_DEBUG(LOG_SCANNER, "gotoStateSession");
	timer->start(UNITEX_QR_SESSION_TIMEOUT);
	state = State_Session;
}

void UnitexScanner::stateSessionTimeout() {
	LOG_DEBUG(LOG_SCANNER, "stateSessionTimeout");
	state = State_Wait;
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void UnitexScanner::gotoStateApproving() {
	LOG_DEBUG(LOG_SCANNER, "gotoStateApproving");
	timer->start(UNITEX_QR_APPROIVING_TIMEOUT);
	state = State_Approving;
}

bool UnitexScanner::stateApprovingProcCode() {
	if(productId != code.cashlessId) {
		LOG_ERROR(LOG_SCANNER, "Wrong productId " << productId << "<>" << code.cashlessId);
		gotoStateSession();
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
		return false;
	}

	if(sales.push(&code) == false) {
		LOG_ERROR(LOG_SCANNER, "Product already saled");
		gotoStateSession();
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
		return false;
	}

	gotoStateVending();
	MdbMasterCashlessInterface::EventApproved event(deviceId, Fiscal::Payment_Ephor, productPrice);
	eventEngine->transmit(&event);
	return true;
}

void UnitexScanner::stateApprovingTimeout() {
	LOG_DEBUG(LOG_SCANNER, "stateApprovingTimeout");
	state = State_Wait;
	EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
	eventEngine->transmit(&event);
}

void UnitexScanner::gotoStateVending() {
	LOG_DEBUG(LOG_ECL, "gotoStateVending");
	timer->start(UNITEX_QR_VENDING_TIMEOUT);
	state = State_Vending;
}

void UnitexScanner::stateVendintTimeout() {
	LOG_DEBUG(LOG_ECL, "stateVendintTimeout");
	gotoStateClosing(); // todo: gotoStatePaymentCancel
}

void UnitexScanner::gotoStateClosing() {
	LOG_DEBUG(LOG_ECL, "gotoStateClosing");
	timer->start(1);
	state = State_Closing;
}

void UnitexScanner::stateClosingTimeout() {
	LOG_DEBUG(LOG_ECL, "stateClosingTimeout");
	state = State_Wait;
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}
