#include "ErpCashless.h"

#include "common/logger/include/Logger.h"

#define SESSION_TIMEOUT 60000

ErpCashless::ErpCashless(
	Mdb::DeviceContext *context,
	TimerEngine *timerEngine,
	EventEngineInterface *eventEngine
) :
	context(context),
	timerEngine(timerEngine),
	eventEngine(eventEngine),
	deviceId(eventEngine),
	enabled(false),
	credit(0)
{
	this->context->setState(State_Idle);
	this->timer = timerEngine->addTimer<ErpCashless, &ErpCashless::procTimer>(this);
}

bool ErpCashless::deposite(uint32_t credit) {
	LOG_DEBUG(LOG_MODEM, "deposite " << credit << " (" << context->getMasterDecimalPoint() << "/" << context->getDecimalPoint() << ")");
	this->credit = credit;
	if(context->getState() == State_Enabled && credit > 0) {
		gotoStateSession();
	}
	return true;
}

EventDeviceId ErpCashless::getDeviceId() {
	return deviceId;
}

void ErpCashless::reset() {
	LOG_DEBUG(LOG_MODEM, "reset");
	credit = 0;
	enabled = false;
	context->setState(State_Disabled);
}

bool ErpCashless::isRefundAble() {
	return false;
}

void ErpCashless::disable() {
	LOG_DEBUG(LOG_MODEM, "disable");
	enabled = false;
	if(context->getState() == State_Enabled) {
		gotoStateEnabled();
	}
}

void ErpCashless::enable() {
	LOG_DEBUG(LOG_MODEM, "enable");
	enabled = true;
	if(context->getState() == State_Disabled) {
		gotoStateEnabled();
	}
}

bool ErpCashless::revalue(uint32_t credit) {
	return false;
}

bool ErpCashless::sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) {
	LOG_DEBUG(LOG_MODEM, "sale " << productPrice << " (" << context->getMasterDecimalPoint() << "/" << context->getDecimalPoint() << ")");
	if(context->getState() != State_Session) {
		return false;
	}
	timer->stop();
	uint32_t price = productPrice;
	if(credit >= price) {
		context->setState(State_Vending);
		MdbMasterCashlessInterface::EventApproved event(deviceId, Fiscal::Payment_Ephor, productPrice);
		eventEngine->transmit(&event);
	} else {
		credit = 0;
		gotoStateEnabled();
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
	}
	return true;
}

bool ErpCashless::saleComplete() {
	LOG_DEBUG(LOG_MODEM, "saleComplete");
	if(context->getState() != State_Vending) {
		return false;
	}
	credit = 0;
	gotoStateEnabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
	return true;
}

bool ErpCashless::saleFailed() {
	LOG_DEBUG(LOG_MODEM, "saleFailed");
	if(context->getState() != State_Vending) {
		return false;
	}
	credit = 0;
	gotoStateEnabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
	return true;
}

bool ErpCashless::closeSession() {
	LOG_DEBUG(LOG_MODEM, "closeSession");
	timer->stop();
	credit = 0;
	gotoStateEnabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
	return true;
}

void ErpCashless::procTimer() {
	LOG_DEBUG(LOG_MODEM, "procTimer " << context->getState());
	switch(context->getState()) {
		case State_Session: stateSessionTimeout(); break;
		default: LOG_ERROR(LOG_SM, "Unwaited timeout " << context->getState());
	}
}

void ErpCashless::gotoStateEnabled() {
	if(enabled == true) {
		if(credit > 0) {
			gotoStateSession();
			return;
		} else {
			context->setState(State_Enabled);
			return;
		}
	} else {
		context->setState(State_Disabled);
		return;
	}
}

void ErpCashless::gotoStateSession() {
	timer->start(SESSION_TIMEOUT);
	context->setState(State_Session);
	EventUint32Interface event(deviceId, MdbMasterCashless::Event_SessionBegin, credit);
	eventEngine->transmit(&event);
}

void ErpCashless::stateSessionTimeout() {
	gotoStateEnabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}
