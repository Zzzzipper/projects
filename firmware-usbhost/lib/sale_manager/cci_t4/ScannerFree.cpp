#if 0
#include <lib/sale_manager/cci_t4/ScannerFree.h>
#include "common/utils/include/Hex.h"
#include "common/logger/include/Logger.h"

ScannerFree::ScannerFree(
	CodeScannerInterface *scanner,
	EventEngine *eventEngine
) :
	scanner(scanner),
	eventEngine(eventEngine),
	deviceId(eventEngine),
	state(State_Idle)
{
	scanner->addObserver(this);
}

EventDeviceId ScannerFree::getDeviceId() {
	return deviceId;
}

void ScannerFree::reset() {
	state = State_Wait;
}

bool ScannerFree::isRefundAble() {
	return false;
}

void ScannerFree::disable() {
	scanner->off();
}

void ScannerFree::enable() {
	scanner->on();
}

bool ScannerFree::revalue(uint32_t credit) {
	return false;
}

bool ScannerFree::sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) {
	this->productPrice = productPrice;
	if(state == State_Session) {
		stateApprovingProcCode();
		return true;
	} else {
		gotoStateApproving();
		return true;
	}

	state = State_Approving;
	return true;
}

bool ScannerFree::saleComplete() {
	if(state != State_Vending) {
		return false;
	}
	state = State_Wait;
	return true;
}

bool ScannerFree::saleFailed() {
	if(state != State_Vending) {
		return false;
	}
	state = State_Wait;
	return true;
}

bool ScannerFree::closeSession() {
	state = State_Wait;
	return true;
}

bool ScannerFree::procCode(uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_SCANNER, "procCode " << dataLen);
	LOG_INFO_HEX(LOG_SCANNER, data, dataLen);
	if(state != State_Approving) {
		LOG_ERROR(LOG_SCANNER, "Wrong state " << state);
		return false;
	}

	state = State_Vending;
	MdbMasterCashlessInterface::EventApproved event(deviceId, Fiscal::Payment_Ephor, productPrice);
	eventEngine->transmit(&event);
	return true;
}
#else
#include <lib/sale_manager/cci_t4/ScannerFree.h>
#include "common/utils/include/Hex.h"
#include "common/logger/include/Logger.h"

#define UNITEX_QR_SESSION_TIMEOUT 30000
#define UNITEX_QR_APPROIVING_TIMEOUT 30000
#define UNITEX_QR_VENDING_TIMEOUT 120000

ScannerFree::ScannerFree(
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
	this->timer = timerEngine->addTimer<ScannerFree, &ScannerFree::procTimer>(this);
	scanner->addObserver(this);
}

EventDeviceId ScannerFree::getDeviceId() {
	return deviceId;
}

void ScannerFree::reset() {
	state = State_Wait;
}

bool ScannerFree::isRefundAble() {
	return false;
}

void ScannerFree::disable() {
	scanner->off();
}

void ScannerFree::enable() {
	scanner->on();
}

bool ScannerFree::revalue(uint32_t credit) {
	return false;
}

bool ScannerFree::sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) {
	this->productPrice = productPrice;
	if(state == State_Session) {
		stateApprovingProcCode();
		return true;
	} else {
		gotoStateApproving();
		return true;
	}
}

bool ScannerFree::saleComplete() {
	if(state != State_Vending) {
		return false;
	}
	gotoStateClosing();
	return true;
}

bool ScannerFree::saleFailed() {
	if(state != State_Vending) {
		return false;
	}
	gotoStateClosing();
	return true;
}

bool ScannerFree::closeSession() {
	gotoStateClosing();
	return true;
}

void ScannerFree::procTimer() {
	LOG_DEBUG(LOG_SCANNER, "procTimer");
	switch(state) {
		case State_Session: stateSessionTimeout(); break;
		case State_Approving: stateApprovingTimeout(); break;
		case State_Vending: stateVendintTimeout(); break;
		case State_Closing: stateClosingTimeout(); break;
		default: LOG_ERROR(LOG_SCANNER, "Unwaited timeout " << state);
	}
}

bool ScannerFree::procCode(uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_SCANNER, "procCode " << dataLen);
	LOG_INFO_HEX(LOG_SCANNER, data, dataLen);
	switch(state) {
		case State_Wait: return stateWaitProcCode();
		case State_Approving: return stateApprovingProcCode();
		default: LOG_ERROR(LOG_SCANNER, "Unwaited code " << state); return false;
	}
}

bool ScannerFree::stateWaitProcCode() {
	LOG_DEBUG(LOG_SCANNER, "stateWaitProcCode");
	gotoStateSession();
	EventUint32Interface event(deviceId, MdbMasterCashless::Event_SessionBegin, maxCredit);
	eventEngine->transmit(&event);
	return true;
}

void ScannerFree::gotoStateSession() {
	LOG_DEBUG(LOG_SCANNER, "gotoStateSession");
	timer->start(UNITEX_QR_SESSION_TIMEOUT);
	state = State_Session;
}

void ScannerFree::stateSessionTimeout() {
	LOG_DEBUG(LOG_SCANNER, "stateSessionTimeout");
	state = State_Wait;
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void ScannerFree::gotoStateApproving() {
	LOG_DEBUG(LOG_SCANNER, "gotoStateApproving");
	timer->start(UNITEX_QR_APPROIVING_TIMEOUT);
	state = State_Approving;
}

bool ScannerFree::stateApprovingProcCode() {
	LOG_DEBUG(LOG_SCANNER, "stateApprovingProcCode");
	gotoStateVending();
	MdbMasterCashlessInterface::EventApproved event(deviceId, Fiscal::Payment_Ephor, productPrice);
	eventEngine->transmit(&event);
	return true;
}

void ScannerFree::stateApprovingTimeout() {
	LOG_DEBUG(LOG_SCANNER, "stateApprovingTimeout");
	state = State_Wait;
	EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
	eventEngine->transmit(&event);
}

void ScannerFree::gotoStateVending() {
	LOG_DEBUG(LOG_ECL, "gotoStateVending");
	timer->start(UNITEX_QR_VENDING_TIMEOUT);
	state = State_Vending;
}

void ScannerFree::stateVendintTimeout() {
	LOG_DEBUG(LOG_ECL, "stateVendintTimeout");
	gotoStateClosing(); // todo: gotoStatePaymentCancel
}

void ScannerFree::gotoStateClosing() {
	LOG_DEBUG(LOG_ECL, "gotoStateClosing");
	timer->start(1);
	state = State_Closing;
}

void ScannerFree::stateClosingTimeout() {
	LOG_DEBUG(LOG_ECL, "stateClosingTimeout");
	state = State_Wait;
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}
#endif

