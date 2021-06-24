#include <lib/sale_manager/order/SaleManagerOrderCore.h>
#include "lib/sale_manager/include/SaleManager.h"
#include "lib/utils/stm32/HardwareUartForwardController.h"

#include "common/mdb/slave/cashless/MdbSlaveCashless3.h"
#include "common/utils/include/Number.h"
#include "common/logger/include/Logger.h"

#define ORDER_WAIT_TIMEOUT 120000

SaleManagerOrderCore::SaleManagerOrderCore(
	ConfigModem *config,
	ClientContext *client,
	TimerEngine *timerEngine,
	EventEngineInterface *eventEngine,
	OrderMasterInterface *master,
	OrderDeviceInterface *device,
	Fiscal::Register *fiscalRegister,
	LedInterface *leds,
	EventRegistrar *chronicler
) :
	config(config),
	client(client),
	timerEngine(timerEngine),
	eventEngine(eventEngine),
	master(master),
	device(device),
	fiscalRegister(fiscalRegister),
	leds(leds),
	chronicler(chronicler),
	deviceId(eventEngine)
{
	this->context = config->getAutomat()->getSMContext();
	this->context->setStatus(Mdb::DeviceContext::Status_Init);
	this->context->setState(State_Idle);
	this->product = config->getAutomat()->createIterator();
	this->timer = timerEngine->addTimer<SaleManagerOrderCore, &SaleManagerOrderCore::procTimer>(this);

	master->setOrder(&order);
	eventEngine->subscribe(this, GlobalId_OrderMaster);
	device->setOrder(&order);
	eventEngine->subscribe(this, GlobalId_OrderDevice);
}

SaleManagerOrderCore::~SaleManagerOrderCore() {
	timerEngine->deleteTimer(timer);
	delete this->product;
}

void SaleManagerOrderCore::reset() {
	LOG_DEBUG(LOG_SM, "reset");
	master->reset();
	device->reset();
	context->setStatus(Mdb::DeviceContext::Status_Work);
	gotoStateInit();
	EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, false);
	eventEngine->transmit(&event);
}

void SaleManagerOrderCore::shutdown() {
	LOG_DEBUG(LOG_SM, "shutdown");
	context->setState(State_Idle);
	EventInterface event(deviceId, SaleManager::Event_Shutdown);
	eventEngine->transmit(&event);
}

void SaleManagerOrderCore::procTimer() {
	LOG_DEBUG(LOG_SM, "procTimer");
	switch(context->getState()) {
	case State_Vending: stateVendingTimeout(); return;
	default:  LOG_ERROR(LOG_SM, "Unwaited timeout " << context->getState());
	}
}

void SaleManagerOrderCore::proc(EventEnvelope *envelope) {
	switch(context->getState()) {
	case State_Init: stateInitEvent(envelope); break;
	case State_Sale: stateSaleEvent(envelope); break;
	case State_Approving: stateApprovingEvent(envelope); break;
	case State_PinCode: statePinCodeEvent(envelope); break;
	case State_Vending: stateVendingEvent(envelope); break;
	case State_Saving: stateSavingEvent(envelope); break;
	default: LOG_DEBUG(LOG_SM, "Unwaited event " << context->getState());
	}
}

void SaleManagerOrderCore::gotoStateInit() {
	LOG_DEBUG(LOG_SM, "gotoStateInit");
	master->connect();
	device->disable();
	leds->setPayment(LedInterface::State_InProgress);
	context->setState(State_Init);
	EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, false);
	eventEngine->transmit(&event);
}

void SaleManagerOrderCore::stateInitEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateInitEvent");
	switch(envelope->getType()) {
		case OrderMasterInterface::Event_Connected: stateInitEventOrderMasterConnected(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << context->getState() << ", event=" << envelope->getType());
	}
}

void SaleManagerOrderCore::stateInitEventOrderMasterConnected(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateInitEventOrderMasterConnected");
	gotoStateSale();
}

void SaleManagerOrderCore::gotoStateSale() {
	LOG_DEBUG(LOG_SM, "gotoStateSale");
	device->enable();
	leds->setPayment(LedInterface::State_Success);
	context->setState(State_Sale);
	EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, true);
	eventEngine->transmit(&event);
}

void SaleManagerOrderCore::stateSaleEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateSaleEvent");
	switch(envelope->getType()) {
		case OrderDeviceInterface::Event_VendRequest: stateSaleEventOrderDeviceRequest(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << context->getState() << ", event=" << envelope->getType());
	}
}

void SaleManagerOrderCore::stateSaleEventOrderDeviceRequest(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateSaleEventOrderDeviceRequest");
	gotoStateApproving();
}

void SaleManagerOrderCore::gotoStateApproving() {
	LOG_DEBUG(LOG_SM, "gotoStateApproving");
	master->check();
	context->setState(State_Approving);
}

void SaleManagerOrderCore::stateApprovingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateApprovingEvent");
	switch(envelope->getType()) {
		case OrderMasterInterface::Event_Approved: stateApprovingEventOrderMasterApproved(); return;
		case OrderMasterInterface::Event_PinCode: stateApprovingEventOrderMasterPinCode(); return;
		case OrderMasterInterface::Event_Denied: stateApprovingEventOrderMasterDenied(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << context->getState() << ", event=" << envelope->getType());
	}
}

void SaleManagerOrderCore::stateApprovingEventOrderMasterApproved() {
	LOG_DEBUG(LOG_SM, "stateApprovingEventOrderMasterApproved");
	leds->setFiscal(LedInterface::State_Success);
	gotoStateVending();
}

void SaleManagerOrderCore::stateApprovingEventOrderMasterPinCode() {
	LOG_DEBUG(LOG_SM, "stateApprovingEventOrderMasterPinCode");
	gotoStatePinCode();
}

void SaleManagerOrderCore::stateApprovingEventOrderMasterDenied() {
	LOG_DEBUG(LOG_SM, "stateApprovingEventOrderMasterDenied");
	device->denyVend();
	gotoStateSale();
}

void SaleManagerOrderCore::gotoStatePinCode() {
	LOG_DEBUG(LOG_SM, "gotoStatePinCode");
	leds->setFiscal(LedInterface::State_InProgress);
	device->requestPinCode();
	context->setState(State_PinCode);
}

void SaleManagerOrderCore::statePinCodeEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "statePinCodeEvent");
	switch(envelope->getType()) {
		case OrderDeviceInterface::Event_PinCodeCompleted: statePinCodeEventPinCodeCompleted(envelope); return;
		case OrderDeviceInterface::Event_PinCodeCancelled: statePinCodeEventPinCodeCancelled(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << context->getState() << ", event=" << envelope->getType());
	}
}

void SaleManagerOrderCore::statePinCodeEventPinCodeCompleted(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "statePinCodeEventPinCodeCompleted");
	OrderDeviceInterface::EventPinCodeCompleted event;
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		leds->setFiscal(LedInterface::State_Failure);
		return;
	}

	LOG_INFO(LOG_SM, ">>>>>>>>>> PINCODE='" << event.getPinCode() << "'");
	leds->setFiscal(LedInterface::State_Failure);
	master->checkPinCode(event.getPinCode());
	context->setState(State_Approving);
}

void SaleManagerOrderCore::statePinCodeEventPinCodeCancelled() {
	LOG_DEBUG(LOG_SM, "statePinCodeEventPinCodeCancelled");
	leds->setFiscal(LedInterface::State_Failure);
	device->denyVend();
	gotoStateSale();
}

void SaleManagerOrderCore::gotoStateVending() {
	LOG_DEBUG(LOG_SM, "gotoStateVending");
	device->approveVend();
	timer->start(ORDER_WAIT_TIMEOUT);
	context->setState(State_Vending);
}

void SaleManagerOrderCore::stateVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateVendingEvent");
	switch(envelope->getType()) {
		case OrderDeviceInterface::Event_VendCompleted: stateVendingEventVendCompleted(envelope); return;
		case OrderDeviceInterface::Event_VendSkipped: stateVendingEventVendSkipped(envelope); return;
		case OrderDeviceInterface::Event_VendCancelled: stateVendingEventVendCancelled(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << context->getState() << ", event=" << envelope->getType());
	}
}

void SaleManagerOrderCore::stateVendingEventVendCompleted(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateVendingEventVendCompleted");
	EventUint16Interface event(OrderDeviceInterface::Event_VendCompleted);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	LOG_INFO(LOG_SM, ">>>>>>>>>> SALED " << event.getValue());
	timer->stop();
	order.remove(event.getValue());
	gotoStateSaving(event.getValue());
}

void SaleManagerOrderCore::stateVendingEventVendSkipped(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateVendingEventVendSkipped");
	EventUint16Interface event(OrderDeviceInterface::Event_VendSkipped);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	LOG_INFO(LOG_SM, ">>>>>>>>>> SKIPPED " << event.getValue());
	timer->stop();
	order.remove(event.getValue());
	if(order.getQuantity() > 0) {
		gotoStateVending();
		return;
	} else {
		master->complete();
		device->denyVend();
		gotoStateSale();
		return;
	}
}

void SaleManagerOrderCore::stateVendingEventVendCancelled() {
	LOG_DEBUG(LOG_SM, "stateVendingEventVendCancelled");
	timer->stop();
	order.clear();
	gotoStateSale();
}

void SaleManagerOrderCore::stateVendingTimeout() {
	LOG_DEBUG(LOG_SM, "stateVendingTimeout");
	order.clear();
	gotoStateSale();
}

void SaleManagerOrderCore::gotoStateSaving(uint16_t cid) {
	LOG_DEBUG(LOG_SM, "gotoStateSaving");
	char selectId[16];
	Sambery::numberToString<uint16_t>(cid, selectId, sizeof(selectId));
	saleData.setProduct(selectId, 0, "", 0, Fiscal::TaxRate_NDSNone, 1);
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.taxSystem = Fiscal::TaxSystem_None;
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());

	device->disable();
	master->distribute(cid);
	context->setState(State_Saving);
}

void SaleManagerOrderCore::stateSavingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateSavingEvent");
	switch(envelope->getType()) {
		case OrderMasterInterface::Event_Distributed: stateSavingEventOrderMasterSaved(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << context->getState() << ", event=" << envelope->getType());
	}
}

void SaleManagerOrderCore::stateSavingEventOrderMasterSaved(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateSavingEventOrderMasterSaved");
	if(order.getQuantity() > 0) {
		gotoStateVending();
		return;
	} else {
		master->complete();
		device->denyVend();
		gotoStateSale();
		return;
	}
}
