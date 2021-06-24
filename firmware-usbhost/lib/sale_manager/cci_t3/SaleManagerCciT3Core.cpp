#include <lib/sale_manager/cci_t3/SaleManagerCciT3Core.h>
#include "lib/sale_manager/include/SaleManager.h"
#include "lib/utils/stm32/HardwareUartForwardController.h"

#include "common/logger/include/Logger.h"

SaleManagerCciT3Core::SaleManagerCciT3Core(
	ConfigModem *config,
	ClientContext *client,
	TimerEngine *timerEngine,
	EventEngineInterface *eventEngine,
	OrderDeviceInterface *slaveCashless,
	OrderInterface *scanner,
	MdbMasterCashlessStack *masterCashlessStack,
	Fiscal::Register *fiscalRegister,
	LedInterface *leds,
	EventRegistrar *chronicler
) :
	config(config),
	client(client),
	timerEngine(timerEngine),
	eventEngine(eventEngine),
	slaveCashless(slaveCashless),
	scanner(scanner),
	masterCashlessStack(masterCashlessStack),
	fiscalRegister(fiscalRegister),
	leds(leds),
	chronicler(chronicler),
	deviceId(eventEngine)
{
	this->context = config->getAutomat()->getSMContext();
	this->context->setStatus(Mdb::DeviceContext::Status_Init);
	this->context->setState(State_Idle);
	this->product = config->getAutomat()->createIterator();
	this->timer = timerEngine->addTimer<SaleManagerCciT3Core, &SaleManagerCciT3Core::procTimer>(this);
	this->slaveCashless->setOrder(&order);

	eventEngine->subscribe(this, GlobalId_OrderDevice);
	eventEngine->subscribe(this, GlobalId_Order);
	eventEngine->subscribe(this, GlobalId_MasterCashless);
	eventEngine->subscribe(this, GlobalId_FiscalRegister);
}

SaleManagerCciT3Core::~SaleManagerCciT3Core() {
	timerEngine->deleteTimer(timer);
	delete this->product;
}

void SaleManagerCciT3Core::reset() {
	LOG_DEBUG(LOG_SM, "reset");
	if(scanner != NULL) { scanner->reset(); }
	slaveCashless->reset();
	masterCashlessStack->reset();
	if(fiscalRegister != NULL) { fiscalRegister->reset(); }
//	leds->setPayment(LedInterface::State_InProgress);
//	leds->setPayment(LedInterface::State_Success);
	context->setStatus(Mdb::DeviceContext::Status_Work);
	gotoStateSale();
	EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, true);
	eventEngine->transmit(&event);
}

void SaleManagerCciT3Core::shutdown() {
	LOG_DEBUG(LOG_SM, "shutdown");
	context->setState(State_Idle);
	EventInterface event(deviceId, SaleManager::Event_Shutdown);
	eventEngine->transmit(&event);
}

void SaleManagerCciT3Core::procTimer() {
	switch(context->getState()) {
	default:;
	}
}

void SaleManagerCciT3Core::proc(EventEnvelope *envelope) {
	switch(context->getState()) {
	case State_Sale: stateSaleEvent(envelope); break;
	case State_OrderApproving: stateOrderApprovingEvent(envelope); break;
	case State_OrderVending: stateOrderVendingEvent(envelope); break;
	case State_OrderClosing: stateOrderClosingEvent(envelope); break;
	case State_FreeVending: stateFreeVendingEvent(envelope); break;
	case State_CashlessApproving: stateCashlessApprovingEvent(envelope); break;
	case State_CashlessVending: stateCashlessVendingEvent(envelope); break;
	case State_CashlessClosing: stateCashlessClosingEvent(envelope); break;
	default: LOG_DEBUG(LOG_SM, "Unwaited event " << context->getState());
	}
}

void SaleManagerCciT3Core::gotoStateSale() {
	LOG_DEBUG(LOG_SM, "gotoStateSale");
	masterCashlessStack->enable();
	slaveCashless->enable();
	context->setState(State_Sale);
}

void SaleManagerCciT3Core::stateSaleEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateWaitEvent");
	switch(envelope->getType()) {
		case OrderInterface::Event_OrderApprove: stateSaleEventOrderApprove(envelope); return;
		case OrderDeviceInterface::Event_VendRequest: stateSaleEventVendRequest(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << context->getState() << ", event=" << envelope->getType());
	}
}

void SaleManagerCciT3Core::stateSaleEventOrderApprove(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateSaleEventOrderApprove");
	masterCashlessStack->disable();
	slaveCashless->disable();
	context->setState(State_OrderApproving);
}

void SaleManagerCciT3Core::stateSaleEventVendRequest(EventEnvelope *envelope) {
	EventUint16Interface event(OrderDeviceInterface::Event_VendRequest);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

	uint16_t productId = event.getValue();
	LOG_INFO(LOG_SM, "stateSaleEventVendRequest " << productId);
	if(product->findByCashlessId(productId) == false) {
		LOG_ERROR(LOG_SM, "Product " << productId << " not found");
		chronicler->registerCashlessIdNotFoundEvent(productId, context);
		slaveCashless->denyVend();
		gotoStateSale();
		return;
	}

	ConfigPrice *productPrice = product->getPrice("DA", 1);
	if(productPrice == NULL) {
		LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
		chronicler->registerPriceListNotFoundEvent("DA", 1, context);
		slaveCashless->denyVend();
		gotoStateSale();
		return;
	}

	price = productPrice->getPrice();
	if(price == 0) {
		order.set(product->getCashlessId());
		slaveCashless->approveVend();
		gotoStateFreeVending();
		return;
	}

	if(masterCashlessStack->sale(productId, price, product->getName(), product->getWareId()) == false) {
		LOG_ERROR(LOG_SM, "Sale start failed");
		slaveCashless->denyVend();
		gotoStateSale();
		return;
	}

	context->setState(State_CashlessApproving);
}

void SaleManagerCciT3Core::stateOrderApprovingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateOrderApproving");
	switch(envelope->getType()) {
		case OrderInterface::Event_OrderRequest: stateOrderApprovingEventOrderRequest(envelope); return;
		case OrderInterface::Event_OrderCancel: stateOrderApprovingEventOrderCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << context->getState() << ", event=" << envelope->getType());
	}
}

void SaleManagerCciT3Core::stateOrderApprovingEventOrderRequest(EventEnvelope *envelope) {
	EventUint16Interface event(OrderInterface::Event_OrderRequest);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	LOG_INFO(LOG_SM, "stateOrderApprovingEventOrderRequest " << event.getDevice() << "," << event.getValue());
	uint16_t productId = event.getValue();
	if(product->findByCashlessId(productId) == false) {
		LOG_ERROR(LOG_SM, "Product " << productId << " not found");
		chronicler->registerCashlessIdNotFoundEvent(productId, context);
		if(scanner != NULL) { scanner->saleFailed(); }
		gotoStateOrderClosing();
		return;
	}

	order.set(product->getCashlessId());
	slaveCashless->approveVend();
	context->setState(State_OrderVending);
}

void SaleManagerCciT3Core::stateOrderApprovingEventOrderCancel() {
	LOG_INFO(LOG_SM, "stateOrderApprovingEventOrderCancel");
//todo: отмена заказа
	gotoStateSale();
}

void SaleManagerCciT3Core::stateOrderVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateOrderVendingEvent");
	switch(envelope->getType()) {
		case OrderDeviceInterface::Event_VendCompleted: stateOrderVendingEventVendComplete(); return;
		case OrderDeviceInterface::Event_VendCancelled: stateOrderVendingEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerCciT3Core::stateOrderVendingEventVendComplete() {
	LOG_INFO(LOG_SM, "stateOrderVendingEventVendComplete " << client->getLoyalityType() << "," << client->getLoyalityLen());
	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), 0, Fiscal::TaxRate_NDSNone, 1);
	saleData.device.set("DA");
	saleData.priceList = 1;
	saleData.paymentType = Fiscal::Payment_Cashless;
	saleData.credit = 0;
	saleData.taxSystem = Fiscal::TaxSystem_None;
	saleData.loyalityType = client->getLoyalityType();
	saleData.loyalityCode.set(client->getLoyalityCode(), client->getLoyalityLen());
	client->completeVending();

	config->getAutomat()->sale(&saleData);
	if(scanner != NULL) { scanner->saleComplete(); }
	gotoStateOrderClosing();
}

void SaleManagerCciT3Core::stateOrderVendingEventVendCancel() {
	LOG_INFO(LOG_SM, "stateOrderVendingEventVendCancel " << product->getName());
	//	registerVendFailedEvent();
	if(scanner != NULL) { scanner->saleFailed(); }
	gotoStateOrderClosing();
}

void SaleManagerCciT3Core::gotoStateOrderClosing() {
	LOG_DEBUG(LOG_SM, "gotoStateOrderClosing");
	slaveCashless->disable();
	context->setState(State_OrderClosing);
}

void SaleManagerCciT3Core::stateOrderClosingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateOrderClosingEvent");
	switch(envelope->getType()) {
		case OrderInterface::Event_OrderEnd: stateOrderClosingEventOrderEnd(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerCciT3Core::stateOrderClosingEventOrderEnd(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateOrderClosingEventSessionEnd");
	gotoStateSale();
}

void SaleManagerCciT3Core::gotoStateFreeVending() {
	LOG_DEBUG(LOG_SM, "gotoStateFreeVending");
	context->setState(State_FreeVending);
}

void SaleManagerCciT3Core::stateFreeVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateFreeVendingEvent");
	switch(envelope->getType()) {
		case OrderDeviceInterface::Event_VendCompleted: stateFreeVendingEventVendComplete(); return;
		case OrderDeviceInterface::Event_VendCancelled: stateFreeVendingEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerCciT3Core::stateFreeVendingEventVendComplete() {
	LOG_INFO(LOG_SM, "stateFreeVendingEventVendComplete " << client->getLoyalityType() << "," << client->getLoyalityLen());
	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), 0, Fiscal::TaxRate_NDSNone, 1);
	saleData.device.set("DA");
	saleData.priceList = 1;
	saleData.paymentType = Fiscal::Payment_Cashless;
	saleData.credit = 0;
	saleData.taxSystem = Fiscal::TaxSystem_None;
	saleData.loyalityType = client->getLoyalityType();
	saleData.loyalityCode.set(client->getLoyalityCode(), client->getLoyalityLen());
	client->completeVending();

	config->getAutomat()->sale(&saleData);
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
	gotoStateSale();
}

void SaleManagerCciT3Core::stateFreeVendingEventVendCancel() {
	LOG_INFO(LOG_SM, "stateFreeVendingEventVendCancel");
	gotoStateSale();
}

void SaleManagerCciT3Core::stateCashlessApprovingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessApproving");
	switch(envelope->getType()) {
		case MdbMasterCashlessInterface::Event_VendApproved: stateCashlessApprovingEventVendApproved(envelope); return;
		case MdbMasterCashlessInterface::Event_VendDenied: stateCashlessApprovingEventVendDenied(); return;
//		case MdbSlaveCashless::Event_VendCancel: stateCashlessApprovingEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << context->getState() << ", event=" << envelope->getType());
	}
}

void SaleManagerCciT3Core::stateCashlessApprovingEventVendApproved(EventEnvelope *envelope) {
	LOG_INFO(LOG_SM, "stateCashlessApprovingEventVendApproved");
	MdbMasterCashlessInterface::EventApproved event;
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	LOG_INFO(LOG_SM, "stateCashlessApprovingEventVendApproved " << event.getDevice() << "," << event.getValue1());
	masterCashless = masterCashlessStack->find(event.getDevice());
	if(masterCashless == NULL) {
		LOG_ERROR(LOG_SM, "Unknown cashless " << event.getDevice());
		return;
	}

	order.set(product->getCashlessId());
	slaveCashless->approveVend();
	masterCashlessStack->disable();
	context->setState(State_CashlessVending);
}

void SaleManagerCciT3Core::stateCashlessApprovingEventVendDenied() {
	LOG_INFO(LOG_SM, "stateCashlessApprovingEventVendDenied");
	slaveCashless->denyVend();
	masterCashlessStack->closeSession();
	masterCashlessStack->disable();
	gotoStateSale();
}

void SaleManagerCciT3Core::stateCashlessApprovingEventVendCancel() {
	LOG_INFO(LOG_SM, "stateCashlessApprovingEventVendCancel");
	masterCashlessStack->closeSession();
	masterCashlessStack->disable();
	gotoStateSale();
}

void SaleManagerCciT3Core::stateCashlessVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessVendingEvent");
	switch(envelope->getType()) {
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case OrderDeviceInterface::Event_VendCompleted: stateCashlessVendingEventVendComplete(); return;
		case OrderDeviceInterface::Event_VendCancelled: stateCashlessVendingEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerCciT3Core::stateCashlessVendingEventVendComplete() {
	LOG_INFO(LOG_SM, "stateCashlessVendingEventVendComplete " << client->getLoyalityType() << "," << client->getLoyalityLen());
	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), price, product->getTaxRate(), 1);
	saleData.device.set("DA");
	saleData.priceList = 1;
	saleData.paymentType = Fiscal::Payment_Cashless;
	saleData.credit = price;
	saleData.taxSystem = config->getAutomat()->getTaxSystem();
	saleData.loyalityType = client->getLoyalityType();
	saleData.loyalityCode.set(client->getLoyalityCode(), client->getLoyalityLen());
	client->completeVending();

	config->getAutomat()->sale(&saleData);
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
	masterCashlessStack->closeSession();//todo: closeOther(deviceId);
	gotoStateCashlessClosing();
}

void SaleManagerCciT3Core::stateCashlessVendingEventVendCancel() {
	LOG_INFO(LOG_SM, "stateCashlessVendingEventVendCancel " << product->getName());
//	registerVendFailedEvent();
	if(masterCashless->saleFailed() == false) {
		masterCashless->reset();
		gotoStateSale();
		return;
	}

	gotoStateSale();
}

void SaleManagerCciT3Core::gotoStateCashlessClosing() {
	LOG_DEBUG(LOG_SM, "gotoStateCashlessClosing");
	if(masterCashless->saleComplete() == false) {
		masterCashless->reset();
		gotoStateSale();
		return;
	}
	context->setState(State_CashlessClosing);
}

void SaleManagerCciT3Core::stateCashlessClosingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessClosingEvent");
	switch(envelope->getType()) {
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateCashlessClosingEventSessionEnd(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerCciT3Core::stateCashlessClosingEventSessionEnd(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessClosingEventSessionEnd");
	if(masterCashless->getDeviceId().getValue() == envelope->getDevice()) {
		gotoStateSale();
		return;
	}
}

void SaleManagerCciT3Core::unwaitedEventSessionBegin(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "unwaitedEventSessionBegin");
	MdbMasterCashlessInterface *cashless = masterCashlessStack->find(envelope->getDevice());
	if(cashless == NULL) {
		LOG_ERROR(LOG_SM, "Unknown cashless " << envelope->getDevice());
		context->incProtocolErrorCount();
		return;
	}
	cashless->closeSession();
}
