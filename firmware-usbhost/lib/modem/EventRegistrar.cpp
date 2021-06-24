#include "EventRegistrar.h"

#include "lib/sale_manager/include/SaleManager.h"

#include "common/logger/include/Logger.h"

#define EVENT_VEND_FAIL_MAX 2

EventRegistrar::EventRegistrar(ConfigModem *config, TimerEngine *timers, EventEngineInterface *eventEngine, RealTimeInterface *realtime) :
	config(config),
	timers(timers),
	eventEngine(eventEngine),
	realtime(realtime),
	state(State_Idle)
{
	timer = timers->addTimer<EventRegistrar, &EventRegistrar::procTimer>(this);
	eventEngine->subscribe(this, GlobalId_SaleManager);
}

EventRegistrar::~EventRegistrar() {
	timers->deleteTimer(timer);
}

void EventRegistrar::setObserver(EventObserver *observer) {
	courier.setRecipient(observer);
}

void EventRegistrar::reset() {
	LOG_INFO(LOG_ER, "reset");
	config->getAutomat()->getSMContext()->getErrors()->remove(ConfigEvent::Type_SaleDisabled);
	vendFailed = 0;
	gotoStateSaleDisabled();
}

void EventRegistrar::proc(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_ER, "proc " << envelope->getType());
	switch(state) {
		case State_SaleDisabled: stateSaleDisabledEvent(envelope); return;
		case State_SaleDisabledTooLong: stateSaleDisabledTooLongEvent(envelope); return;
		case State_SaleEnabled: stateSaleEnabledEvent(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << state << "," << envelope->getType());
	}
}

void EventRegistrar::procTimer() {
	LOG_DEBUG(LOG_ER, "procTimer");
	switch(state) {
	case State_SaleDisabled: stateSaleDisabledTimeout(); break;
	default: LOG_ERROR(LOG_SM, "Unwaited timeout " << state);
	}
}

void EventRegistrar::registerCashlessIdNotFoundEvent(uint16_t cashlessId, Mdb::DeviceContext *context) {
	StringBuilder data;
	data << cashlessId;
	config->getEvents()->add(ConfigEvent::Type_CashlessIdNotFound, data.getString());
	context->getErrors()->add(ConfigEvent::Type_CashlessIdNotFound, data.getString());
}

void EventRegistrar::registerPriceListNotFoundEvent(const char *deviceId, uint8_t priceListNumber, Mdb::DeviceContext *context) {
	StringBuilder data;
	data << deviceId << priceListNumber;
	config->getEvents()->add(ConfigEvent::Type_PriceListNotFound, data.getString());
	context->getErrors()->add(ConfigEvent::Type_PriceListNotFound, data.getString());
}

void EventRegistrar::registerPriceNotEqualEvent(const char *selectId, uint32_t expectedPrice, uint32_t actualPrice, Mdb::DeviceContext *context) {
	StringBuilder data;
	data << selectId << "*" << expectedPrice << "*" << actualPrice;
	config->getEvents()->add(ConfigEvent::Type_PriceNotEqual, data.getString());
	context->getErrors()->add(ConfigEvent::Type_PriceNotEqual, data.getString());
}

void EventRegistrar::registerCoinInEvent(uint32_t value, uint8_t route) {
	config->getAutomat()->registerCoinIn(value, route);
	if(config->getAutomat()->getCategoryMoney() == true) {
		if(route == Mdb::CoinChanger::Route_Cashbox || route == Mdb::CoinChanger::Route_Tube) {
			StringBuilder data;
			data << convertDecimalPoint(config->getAutomat()->getDecimalPoint(), 2, value);
			config->getEvents()->add(ConfigEvent::Type_CoinIn, data.getString());
			return;
		}
	}
}

void EventRegistrar::registerBillInEvent(uint32_t value, uint8_t route) {
	config->getAutomat()->registerBillIn(value);
	if(config->getAutomat()->getCategoryMoney() == true) {
		if(route == Mdb::BillValidator::Route_Stacked) {
			StringBuilder data;
			data << convertDecimalPoint(config->getAutomat()->getDecimalPoint(), 2, value);
			config->getEvents()->add(ConfigEvent::Type_BillIn, data.getString());
			return;
		}
	}
}

void EventRegistrar::registerChangeEvent(uint32_t value) {
	config->getAutomat()->registerCoinDispense(value);
	if(config->getAutomat()->getCategoryMoney() == true) {
		StringBuilder data;
		data << convertDecimalPoint(config->getAutomat()->getDecimalPoint(), 2, value);
		config->getEvents()->add(ConfigEvent::Type_ChangeOut, data.getString());
		return;
	}
}

void EventRegistrar::registerDeviceError(EventEnvelope *envelope) {
	Mdb::EventError event(envelope->getType());
	if(event.open(envelope) == false) {
		return;
	}
	config->getEvents()->add((ConfigEvent::Code)event.code, event.data.getString());
}

void EventRegistrar::registerCashCanceledEvent() {
	config->getEvents()->add(ConfigEvent::Type_CashCanceled);
}

void EventRegistrar::registerVendFailedEvent(const char *selectId, Mdb::DeviceContext *context) {
	config->getEvents()->add(ConfigEvent::Type_SaleFailed, selectId);
	vendFailed++; if(vendFailed >= EVENT_VEND_FAIL_MAX) { context->getErrors()->add(ConfigEvent::Type_SaleFailed, selectId); }
}

void EventRegistrar::startSession() {
	sessionStart = config->getRealTime()->getUnixTimestamp();
}

void EventRegistrar::registerSessionClosedByMaster() {
	uint32_t sessionEnd = config->getRealTime()->getUnixTimestamp();
	StringBuilder data;
	data << (sessionEnd - sessionStart);
	config->getEvents()->add(ConfigEvent::Type_SessionClosedByMaster, data.getString());
}

void EventRegistrar::registerSessionClosedByTimeout() {
	uint32_t sessionEnd = config->getRealTime()->getUnixTimestamp();
	StringBuilder data;
	data << (sessionEnd - sessionStart);
	config->getEvents()->add(ConfigEvent::Type_SessionClosedByTimeout, data.getString());
}

void EventRegistrar::registerSessionClosedByTerminal() {
	uint32_t sessionEnd = config->getRealTime()->getUnixTimestamp();
	StringBuilder data;
	data << (sessionEnd - sessionStart);
	config->getEvents()->add(ConfigEvent::Type_SessionClosedByTerminal, data.getString());
}

void EventRegistrar::registerCashlessCanceledEvent(const char *selectId) {
	uint32_t sessionEnd = config->getRealTime()->getUnixTimestamp();
	StringBuilder data;
	data << selectId << "*" << (sessionEnd - sessionStart);
	config->getEvents()->add(ConfigEvent::Type_CashlessCanceled, data.getString());
}

void EventRegistrar::registerCashlessDeniedEvent(const char *selectId) {
	uint32_t sessionEnd = config->getRealTime()->getUnixTimestamp();
	StringBuilder data;
	data << selectId << "*" << (sessionEnd - sessionStart);
	config->getEvents()->add(ConfigEvent::Type_CashlessDenied, data.getString());
}

void EventRegistrar::gotoStateSaleDisabled() {
	LOG_INFO(LOG_ER, "gotoStateSaleDisabled");
	timer->start(AUTOMAT_DISABLE_TIMEOUT);
	state = State_SaleDisabled;
}

void EventRegistrar::stateSaleDisabledEvent(EventEnvelope *event) {
	LOG_INFO(LOG_ER, "stateSaleDisabledEvent");
	switch(event->getType()) {
		case SaleManager::Event_AutomatState: stateSaleDisabledEventAutomatState(event); break;
		default: LOG_ERROR(LOG_ER, "Unwaited event " << state << "," << event->getType());
	}
}

void EventRegistrar::stateSaleDisabledEventAutomatState(EventEnvelope *envelope) {
	LOG_INFO(LOG_ER, "stateSaleDisabledEventAutomatState");
	EventUint8Interface event(SaleManager::Event_AutomatState);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_ER, "Wrong envelope");
		return;
	}
	if(event.getValue() == true) {
		gotoStateSaleEnabled();
	}
}

void EventRegistrar::stateSaleDisabledTimeout() {
	LOG_INFO(LOG_ER, "stateSaleDisabledTimeout");
	config->getAutomat()->getSMContext()->getErrors()->add(ConfigEvent::Type_SaleDisabled, "");
	config->getEvents()->add(ConfigEvent::Type_SaleDisabled);
	gotoStateSaleDisabledTooLong();
	courier.deliver(Event_CriticalError);
}

void EventRegistrar::gotoStateSaleDisabledTooLong() {
	LOG_INFO(LOG_ER, "gotoStateSaleDisabledTooLong");
	state = State_SaleDisabledTooLong;
}

void EventRegistrar::stateSaleDisabledTooLongEvent(EventEnvelope *envelope) {
	LOG_INFO(LOG_ER, "stateSaleDisabledTooLongEvent");
	switch(envelope->getType()) {
		case SaleManager::Event_AutomatState: stateSaleDisabledTooLongEventAutomatState(envelope); break;
		default: LOG_ERROR(LOG_ER, "Unwaited event " << state << "," << envelope->getType());
	}
}

void EventRegistrar::stateSaleDisabledTooLongEventAutomatState(EventEnvelope *envelope) {
	LOG_INFO(LOG_ER, "stateSaleDisabledTooLongEventAutomatState");
	EventUint8Interface event(SaleManager::Event_AutomatState);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_ER, "Wrong envelope");
		return;
	}
	if(event.getValue() == true) {
		config->getAutomat()->getSMContext()->getErrors()->remove(ConfigEvent::Type_SaleDisabled);
		config->getEvents()->add(ConfigEvent::Type_SaleEnabled);
		gotoStateSaleEnabled();
	}
}

void EventRegistrar::gotoStateSaleEnabled() {
	LOG_INFO(LOG_ER, "gotoStateSaleEnabled");
	timer->stop();
	state = State_SaleEnabled;
}

void EventRegistrar::stateSaleEnabledEvent(EventEnvelope *envelope) {
	LOG_INFO(LOG_ER, "stateSaleEnabledEvent");
	switch(envelope->getType()) {
		case SaleManager::Event_AutomatState: stateSaleEnabledEventAutomatState(envelope); break;
		default: LOG_ERROR(LOG_ER, "Unwaited event " << state << "," << envelope->getType());
	}
}

void EventRegistrar::stateSaleEnabledEventAutomatState(EventEnvelope *envelope) {
	LOG_INFO(LOG_ER, "stateSaleEnabledEventAutomatState");
	EventUint8Interface event(SaleManager::Event_AutomatState);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_ER, "Wrong envelope");
		return;
	}
	if(event.getValue() == false) {
		gotoStateSaleDisabled();
	}
}
