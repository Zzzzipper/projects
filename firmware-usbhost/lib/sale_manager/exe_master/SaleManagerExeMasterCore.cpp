#include "SaleManagerExeMasterCore.h"

#include "lib/sale_manager/include/SaleManager.h"
#include "lib/rfid/RfidReader.h"

#include "common/logger/include/Logger.h"

#include <string.h>

#define SHUTDOWN_TIMEOUT 2000

#define REFUNDABLE

SaleManagerExeMasterCore::SaleManagerExeMasterCore(SaleManagerExeMasterParams *c) :
	config(c->config),
	client(c->client),
	timers(c->timers),
	eventEngine(c->eventEngine),
	fiscalRegister(c->fiscalRegister),
	leds(c->leds),
	chronicler(c->chronicler),
	deviceId(c->eventEngine)
{
	this->context = config->getAutomat()->getSMContext();
	this->context->setStatus(Mdb::DeviceContext::Status_NotFound);
	this->context->setState(State_Idle);

	this->executive = c->executive;
	this->masterCoinChanger = c->masterCoinChanger;
	this->masterBillValidator = c->masterBillValidator;
	this->masterCashlessStack = c->masterCashlessStack;

	this->product = config->getAutomat()->createIterator();
	this->timer = timers->addTimer<SaleManagerExeMasterCore, &SaleManagerExeMasterCore::procTimer>(this);

	eventEngine->subscribe(this, GlobalId_MasterCoinChanger);
	eventEngine->subscribe(this, GlobalId_MasterBillValidator);
	eventEngine->subscribe(this, GlobalId_MasterCashless);
	eventEngine->subscribe(this, GlobalId_Executive);
	eventEngine->subscribe(this, GlobalId_FiscalRegister);
}

SaleManagerExeMasterCore::~SaleManagerExeMasterCore() {
	timers->deleteTimer(this->timer);
	delete product;
}

EventDeviceId SaleManagerExeMasterCore::getDeviceId() {
	return deviceId;
}

void SaleManagerExeMasterCore::reset() {
	LOG_DEBUG(LOG_SM, "reset");
	cashCredit = 0;
	cashlessCredit = 0;
	vendFailed = 0;
	executive->setDicimalPoint(config->getAutomat()->getDecimalPoint());
	if(config->getAutomat()->getShowChange() == true) {
		executive->setChange(false);
	} else {
		executive->setChange(true);
	}
	executive->setScalingFactor(1);
	executive->reset();
	masterCoinChanger->reset();
	masterBillValidator->reset();
	masterCashlessStack->reset();
	if(fiscalRegister != NULL) { fiscalRegister->reset(); }
	leds->setPayment(LedInterface::State_InProgress);
	context->setStatus(Mdb::DeviceContext::Status_Init);
	context->setState(State_Disabled);
}

void SaleManagerExeMasterCore::shutdown() {
	LOG_DEBUG(LOG_SM, "shutdown");
	masterCoinChanger->disable();
	masterBillValidator->disable();
	masterCashlessStack->disable();
	timer->start(SHUTDOWN_TIMEOUT);
	context->setState(State_Shutdown);
}

void SaleManagerExeMasterCore::procTimer() {
	LOG_DEBUG(LOG_SM, "procTimer " << context->getState());
	switch(context->getState()) {
		case State_Shutdown: stateShutdownTimeout(); return;
		case State_CashlessStackApproving: stateCashlessStackApprovingTimeout(); return;
		case State_CashlessApproving: stateCashlessAprrovingTimeout(); return;
		case State_CashlessHolding: stateCashlessHoldingTimeout(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited timeout " << context->getState());
	}
}

void SaleManagerExeMasterCore::proc(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "proc " << context->getState() << "," << envelope->getType());
	switch(context->getState()) {
		case State_Idle: LOG_DEBUG(LOG_SM, "Ignore event " << context->getState() << "," << envelope->getType()); return;
		case State_Disabled: stateDisabledEvent(envelope); return;
		case State_Enabled: stateEnabledEvent(envelope); return;
		case State_CashCredit: stateCashCreditEvent(envelope); return;
		case State_CashPrice: stateCashPriceEvent(envelope); return;
		case State_CashVending: stateCashVendingEvent(envelope); return;
		case State_CashChange: stateCashChangeEvent(envelope); return;
		case State_CashCheckPrinting: stateCashCheckPrintingEvent(envelope); return;
		case State_CashRefund: stateCashRefundEvent(envelope); return;
		case State_TokenCredit: stateTokenCredit(envelope); return;
		case State_TokenPrice: stateTokenPriceEvent(envelope); return;
		case State_TokenVending: stateTokenVendingEvent(envelope); return;
		case State_TokenCheckPrinting: stateTokenCheckPrintingEvent(envelope); return;
		case State_CashlessStackPrice: stateCashlessStackPriceEvent(envelope); return;
		case State_CashlessStackApproving: stateCashlessStackApprovingEvent(envelope); return;
		case State_CashlessStackHolding: stateCashlessStackHoldingEvent(envelope); return;
		case State_CashlessCredit: stateCashlessCreditEvent(envelope); return;
		case State_CashlessRevaluing: stateCashlessRevaluingEvent(envelope); return;
		case State_CashlessPrice: stateCashlessPriceEvent(envelope); return;
		case State_CashlessApproving: stateCashlessApprovingEvent(envelope); return;
		case State_CashlessHoldingApproving: stateCashlessHoldingApprovingEvent(envelope); return;
		case State_CashlessHolding: stateCashlessHoldingEvent(envelope); return;
		case State_CashlessVending: stateCashlessVendingEvent(envelope); return;
		case State_CashlessCheckPrinting: stateCashlessCheckPrintingEvent(envelope); return;
		case State_CashlessClosing: stateCashlessClosingEvent(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::updateChangeStatus() {
	LOG_DEBUG(LOG_SM, "updateChangeStatus");
	if(config->getAutomat()->getShowChange() == true) {
		executive->setChange(masterCoinChanger->hasChange());
	}
	executive->setCredit(cashCredit);
}

void SaleManagerExeMasterCore::gotoStateDisabled() {
	LOG_DEBUG(LOG_SM, "gotoStateDisabled");
	masterCoinChanger->disable();
	masterBillValidator->disable();
	masterCashlessStack->disable();
	context->setStatus(Mdb::DeviceContext::Status_Disabled);
	context->setState(State_Disabled);
	EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, false);
	eventEngine->transmit(&event);
}

void SaleManagerExeMasterCore::stateDisabledEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateDisabledEvent");
	switch(envelope->getType()) {
		case ExeMasterInterface::Event_Ready: stateDisabledEventExeMasterReady(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateDisabledEventExeMasterReady() {
	LOG_DEBUG(LOG_SM, "stateDisabledEventExeMasterReady");
	leds->setPayment(LedInterface::State_Success);
	gotoStateEnabled();
}

void SaleManagerExeMasterCore::gotoStateEnabled() {
	LOG_DEBUG(LOG_SM, "gotoStateEnabled");
	if(cashCredit > 0) {
		gotoStateCashCredit();
		return;
	}
	if(executive->isEnabled() == false) {
		gotoStateDisabled();
		return;
	}
	if(config->getAutomat()->getShowChange() == true) {
		executive->setChange(masterCoinChanger->hasChange());
	}
	executive->setCredit(cashCredit);
	if(masterCoinChanger->isInited() == true) {
		config->getAutomat()->getBVContext()->setMaxChange(config->getAutomat()->getCCContext()->getInTubeValue());
	}
	masterCoinChanger->enable();
	masterBillValidator->enable();
	masterCashlessStack->enable();
	saleNum = 0;
	context->setStatus(Mdb::DeviceContext::Status_Enabled);
	context->setState(State_Enabled);
	EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, true);
	eventEngine->transmit(&event);
}

void SaleManagerExeMasterCore::stateEnabledEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateEnabledEvent");
	switch(envelope->getType()) {
		case ExeMasterInterface::Event_NotReady: stateEnabledEventNotReady(); return;
		case ExeMasterInterface::Event_VendRequest: stateEnabledEventVendRequest(envelope); return;
		case MdbMasterBillValidator::Event_Deposite: stateEnabledEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_Ready: updateChangeStatus(); return;
		case MdbMasterCoinChanger::Event_Deposite: stateEnabledEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_DepositeToken: stateEnabledEventTokenCredit(); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: stateEnabledEventCashlessCredit(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateEnabledEventNotReady() {
	LOG_DEBUG(LOG_SM, "stateEnabledEventNotReady");
	gotoStateDisabled();
}

void SaleManagerExeMasterCore::stateEnabledEventVendRequest(EventEnvelope *envelope) {
	EventUint8Interface event(ExeMasterInterface::Event_VendRequest);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	productId = event.getValue();
	LOG_INFO(LOG_SM, "stateEnabledEventVendRequest " << productId);
	if(product->findByCashlessId(productId) == false) {
		LOG_ERROR(LOG_SM, "Product " << productId << " not found");
		return;
	}

	DateTime datetime;
	config->getEvents()->getRealTime()->getDateTime(&datetime);
	ConfigPrice *productPrice = product->getIndexByDateTime("CA", &datetime);
	if(config->getAutomat()->getCashless2Click() == true || cashCredit == 0) {
		if(productPrice != NULL && productPrice->getPrice() == 0) {
			price = client->calcDiscount(productPrice->getPrice());
			gotoStateCashPrice();
			return;
		}

		productPrice = product->getIndexByDateTime("DA", &datetime);
		if(productPrice == NULL) {
			LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
			return;
		}

		price = client->calcDiscount(productPrice->getPrice());
		gotoStateCashlessStackPrice();
	} else {
		if(productPrice == NULL) {
			LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
			return;
		}

		price = client->calcDiscount(productPrice->getPrice());
		gotoStateCashPrice();
	}
}

bool SaleManagerExeMasterCore::parseCashCredit(EventEnvelope *envelope, uint32_t *incomingCredit) {
	Mdb::EventDeposite event1(MdbMasterBillValidator::Event_Deposite);
	if(event1.open(envelope) == true && event1.getRoute() == Mdb::BillValidator::Route_Stacked) {
		*incomingCredit = event1.getNominal();
		chronicler->registerBillInEvent(event1.getNominal(), event1.getRoute());
		return true;
	}
	MdbMasterCoinChanger::EventCoin event2(MdbMasterCoinChanger::Event_Deposite);
	if(event2.open(envelope) == true) {
		*incomingCredit = event2.getNominal();
		chronicler->registerCoinInEvent(event2.getNominal(), event2.getRoute());
		return true;
	}
	return false;
}

void SaleManagerExeMasterCore::stateEnabledEventCashCredit(EventEnvelope *envelope) {
	uint32_t incomingCredit = 0;
	if(parseCashCredit(envelope, &incomingCredit) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	LOG_INFO(LOG_SM, "stateEnabledEventCashCredit " << incomingCredit);
	cashCredit = incomingCredit;
	gotoStateCashCredit();
}

void SaleManagerExeMasterCore::stateEnabledEventTokenCredit() {
	LOG_DEBUG(LOG_SM, "stateEnabledEventTokenCredit");
	gotoStateTokenCredit();
}

void SaleManagerExeMasterCore::gotoStateCashCredit() {
	LOG_DEBUG(LOG_SM, "gotoStateCashCredit");
	if(cashCredit < config->getAutomat()->getMaxCredit()) {
		masterCoinChanger->enable();
		masterBillValidator->enable();
	} else {
		masterCoinChanger->disable();
		masterBillValidator->disable();
	}
#ifdef REFUNDABLE
	masterCashlessStack->disableNotRefundAble();
#else
	masterCashlessStack->disable();
#endif
	executive->setCredit(cashCredit);
	context->setState(State_CashCredit);
}

void SaleManagerExeMasterCore::stateCashCreditEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashCreditEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: stateCashCreditEventCredit(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: stateCashCreditEventCredit(envelope); return;
		case MdbMasterCoinChanger::Event_DepositeToken: gotoStateTokenCredit(); return;
		case MdbMasterCoinChanger::Event_EscrowRequest: stateCashCreditEventEscrowRequest(); return;
#ifdef REFUNDABLE
		case MdbMasterCashlessInterface::Event_SessionBegin: stateCashCreditEventCashlessCredit(envelope); return;
#else
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
#endif
		case ExeMasterInterface::Event_VendRequest: stateCashCreditEventVendRequest(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateCashCreditEventCredit(EventEnvelope *envelope) {
	uint32_t incomingCredit = 0;
	if(parseCashCredit(envelope, &incomingCredit) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	LOG_DEBUG(LOG_SM, "stateCashCreditEventCredit " << cashCredit << "," << incomingCredit);
	cashCredit += incomingCredit;
	if(cashCredit >= config->getAutomat()->getMaxCredit()) {
		masterBillValidator->disable();
		masterCoinChanger->disable();
	}

	executive->setCredit(cashCredit);
}

void SaleManagerExeMasterCore::stateCashCreditEventEscrowRequest() {
	LOG_DEBUG(LOG_SM, "stateCashCreditEventEscrowRequest");
	if(config->getAutomat()->getCreditHolding() == false || (config->getAutomat()->getMultiVend() == true && saleNum > 0)) {
		chronicler->registerCashCanceledEvent();
		gotoStateCashRefund();
	}
}

void SaleManagerExeMasterCore::stateCashCreditEventVendRequest(EventEnvelope *envelope) {
	EventUint8Interface event(ExeMasterInterface::Event_VendRequest);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	productId = event.getValue();
	LOG_INFO(LOG_SM, "stateCashCreditEventVendRequest " << productId);
	if(product->findByCashlessId(productId) == false) {
		LOG_ERROR(LOG_SM, "Product " << productId << " not found");
		chronicler->registerCashlessIdNotFoundEvent(productId, context);
		return;
	}

	DateTime datetime;
	config->getEvents()->getRealTime()->getDateTime(&datetime);
	ConfigPrice *productPrice = product->getIndexByDateTime("CA", &datetime);
	if(productPrice == NULL) {
		LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
		chronicler->registerPriceListNotFoundEvent("CA", 0, context);
		return;
	}

	price = client->calcDiscount(productPrice->getPrice());
	executive->setPrice(price);
	context->setState(State_CashPrice);
}

void SaleManagerExeMasterCore::gotoStateCashPrice() {
	LOG_DEBUG(LOG_SM, "gotoStateCashPrice");
	executive->setPrice(price);
	context->setState(State_CashPrice);
}

void SaleManagerExeMasterCore::stateCashPriceEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashPriceEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case ExeMasterInterface::Event_VendPrice: stateCashPriceEventVendPrice(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateCashPriceEventVendPrice() {
	if(price > cashCredit) {
		LOG_INFO(LOG_SM, "Not enough credit (exp= " << price << ", act=" << cashCredit << ")");
		executive->denyVend(price);
		gotoStateEnabled();
		return;
	}

	executive->approveVend(price);
	masterCoinChanger->disable();
	masterBillValidator->disable();
#ifdef REFUNDABLE
	masterCashlessStack->disableNotRefundAble();
#else
	masterCashlessStack->disable();
#endif
	context->setState(State_CashVending);
}

void SaleManagerExeMasterCore::stateCashVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashVendingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case ExeMasterInterface::Event_VendComplete: stateCashVendingEventComplete(); return;
		case ExeMasterInterface::Event_VendFailed: stateCashVendingEventVendFailed(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::unwaitedEventCashCredit(EventEnvelope *envelope) {
	uint32_t incomingCredit = 0;
	if(parseCashCredit(envelope, &incomingCredit) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	cashCredit += incomingCredit;
	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>> DEPOSITE" << cashCredit);
}

void SaleManagerExeMasterCore::unwaitedEventSessionBegin(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "unwaitedEventSessionBegin");
	MdbMasterCashlessInterface *cashless = masterCashlessStack->find(envelope->getDevice());
	if(cashless == NULL) {
		LOG_ERROR(LOG_SM, "Unknown cashless " << envelope->getDevice());
		context->incProtocolErrorCount();
		return;
	}
	cashless->closeSession();
}

void SaleManagerExeMasterCore::stateCashVendingEventComplete() {
	LOG_INFO(LOG_SM, "stateCashVendingEventComplete");
	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), price, product->getTaxRate(), 1);
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = cashCredit;
	saleData.taxSystem = config->getAutomat()->getTaxSystem();
	saleData.loyalityType = client->getLoyalityType();
	saleData.loyalityCode.set(client->getLoyalityCode(), client->getLoyalityLen());
	client->completeVending();

	cashCredit -= price;
	saleNum++;
	config->getAutomat()->sale(&saleData);
	if(config->getAutomat()->getMultiVend() == true) {
		gotoStateCashCheckPrinting();
	} else {
		gotoStateCashChange();
	}
}

void SaleManagerExeMasterCore::stateCashVendingEventVendFailed() {
	LOG_DEBUG(LOG_SM, "stateCashVendingEventVendFailed");
	chronicler->registerVendFailedEvent(product->getId(), context);
	gotoStateCashRefund();
}

void SaleManagerExeMasterCore::gotoStateCashChange() {
	LOG_DEBUG(LOG_SM, "gotoStateCashChange");
	if(masterCoinChanger->isInited() == true && cashCredit > 0) {
		LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>DISPENSE " << cashCredit);
		masterCoinChanger->dispense(cashCredit);
		chronicler->registerChangeEvent(cashCredit);
		cashCredit = 0;
		context->setState(State_CashChange);
		return;
	}

	gotoStateCashCheckPrinting();
}

void SaleManagerExeMasterCore::stateCashChangeEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashChangeEvent");
	switch(envelope->getType()) {
		case MdbMasterCoinChanger::Event_Dispense: {
			LOG_INFO(LOG_SM, "Dispense complete");
			gotoStateCashCheckPrinting();
			return;
		}
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::gotoStateCashCheckPrinting() {
	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>TICKET " << saleData.getProductNum() << "," << saleData.credit << "," << saleData.getPrice() << ":" << config->getAutomat()->getDecimalPoint());
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
	context->setState(State_CashCheckPrinting);
}

void SaleManagerExeMasterCore::stateCashCheckPrintingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashCheckPrintingEvent");
	switch(envelope->getType()) {
		case Fiscal::Register::Event_CommandOK: gotoStateEnabled(); return;
		case Fiscal::Register::Event_CommandError: gotoStateEnabled(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::gotoStateCashRefund() {
	LOG_DEBUG(LOG_SM, "gotoStateCashRefund");
	if(masterCoinChanger->isInited() == true && cashCredit > 0) {
		LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>DISPENSE " << cashCredit);
		masterCoinChanger->dispense(cashCredit);
		chronicler->registerChangeEvent(cashCredit);
		cashCredit = 0;
		context->setState(State_CashRefund);
		return;
	}

	gotoStateEnabled();
}

void SaleManagerExeMasterCore::stateCashRefundEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashRefundEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbMasterCoinChanger::Event_Dispense: gotoStateCashRefund(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateEnabledEventCashlessCredit(EventEnvelope *envelope) {
	EventUint32Interface event(MdbMasterCashlessInterface::Event_SessionBegin);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	LOG_INFO(LOG_SM, "stateSaleEventCashlessCredit " << event.getDevice() << "," << event.getValue());
	masterCashless = masterCashlessStack->find(event.getDevice());
	if(masterCashless == NULL) {
		LOG_ERROR(LOG_SM, "Unknown cashless " << event.getDevice());
		return;
	}

	cashlessCredit = event.getValue();
#ifdef REFUNDABLE
	if(masterCashless->isRefundAble() == true) {
		masterCoinChanger->enable();
		masterBillValidator->enable();
	} else {
		masterCoinChanger->disable();
		masterBillValidator->disable();
	}
#else
	masterCoinChanger->disable();
	masterBillValidator->disable();
#endif
	executive->setCredit(cashlessCredit);
	chronicler->startSession();
	context->setState(State_CashlessCredit);
}

void SaleManagerExeMasterCore::stateCashCreditEventCashlessCredit(EventEnvelope *envelope) {
	EventUint32Interface event(MdbMasterCashlessInterface::Event_SessionBegin);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	LOG_INFO(LOG_SM, "stateSaleEventCashlessCredit " << event.getDevice() << "," << event.getValue());
	masterCashless = masterCashlessStack->find(event.getDevice());
	if(masterCashless == NULL) {
		LOG_ERROR(LOG_SM, "Unknown cashless " << event.getDevice());
		return;
	}

	cashlessCredit = event.getValue();
	if(masterCashless->isRefundAble() == false) {
		masterCoinChanger->disable();
		masterBillValidator->disable();
		executive->setCredit(cashlessCredit);
		chronicler->startSession();
		context->setState(State_CashlessCredit);
		return;
	} else {
		masterCoinChanger->enable();
		masterBillValidator->enable();
		revalue = cashCredit;
		masterCashless->revalue(revalue);
		cashCredit = 0;
		context->setState(State_CashlessRevaluing);
		return;
	}

}

void SaleManagerExeMasterCore::gotoStateTokenCredit() {
	LOG_DEBUG(LOG_SM, "gotoStateTokenCredit");
	masterCoinChanger->disable();
	masterBillValidator->disable();
	masterCashlessStack->disable();
	executive->setCredit(config->getAutomat()->getMaxCredit());
	context->setState(State_TokenCredit);
}

void SaleManagerExeMasterCore::stateTokenCredit(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateTokenCredit");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case ExeMasterInterface::Event_VendRequest: stateTokenCreditEventVendRequest(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateTokenCreditEventVendRequest(EventEnvelope *envelope) {
	EventUint8Interface event(ExeMasterInterface::Event_VendRequest);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	productId = event.getValue();
	LOG_INFO(LOG_SM, "stateCashCreditEventVendRequest " << productId);
	if(product->findByCashlessId(productId) == false) {
		LOG_ERROR(LOG_SM, "Product " << productId << " not found");
		chronicler->registerCashlessIdNotFoundEvent(productId, context);
		gotoStateCashRefund();
		return;
	}

	DateTime datetime;
	config->getEvents()->getRealTime()->getDateTime(&datetime);
	ConfigPrice *productPrice = product->getIndexByDateTime("CA", &datetime);
	if(productPrice == NULL) {
		LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
		chronicler->registerPriceListNotFoundEvent("CA", 0, context);
		gotoStateCashRefund();
		return;
	}

	price = client->calcDiscount(productPrice->getPrice());
	executive->setPrice(price);
	context->setState(State_TokenPrice);
}

void SaleManagerExeMasterCore::stateTokenPriceEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateTokenPriceEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case ExeMasterInterface::Event_VendPrice: stateTokenPriceEventVendPrice(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateTokenPriceEventVendPrice() {
	executive->approveVend(price);
	context->setState(State_TokenVending);
}

void SaleManagerExeMasterCore::stateTokenVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateTokenVendingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case ExeMasterInterface::Event_VendComplete: stateTokenVendingEventComplete(); return;
		case ExeMasterInterface::Event_VendFailed: stateTokenVendingEventFailed(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateTokenVendingEventComplete() {
	LOG_INFO(LOG_SM, "stateTokenVendingEventComplete");
	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), price, product->getTaxRate(), 1);
	saleData.device.set("TA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Token;
	saleData.credit = price;
	saleData.taxSystem = config->getAutomat()->getTaxSystem();
	saleData.loyalityType = client->getLoyalityType();
	saleData.loyalityCode.set(client->getLoyalityCode(), client->getLoyalityLen());
	client->completeVending();

	config->getAutomat()->sale(&saleData);
	gotoStateTokenCheckPrinting();
}

void SaleManagerExeMasterCore::stateTokenVendingEventFailed() {
	LOG_INFO(LOG_SM, "stateTokenVendingEventFailed");
	chronicler->registerVendFailedEvent(product->getId(), context);
	gotoStateEnabled();
}

void SaleManagerExeMasterCore::gotoStateTokenCheckPrinting() {
	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>TICKET " << saleData.getProductNum() << "," << saleData.credit << "," << saleData.getPrice() << ":" << config->getAutomat()->getDecimalPoint());
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
	context->setState(State_TokenCheckPrinting);
}

void SaleManagerExeMasterCore::stateTokenCheckPrintingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateTokenCheckPrintingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case Fiscal::Register::Event_CommandOK: gotoStateEnabled(); return;
		case Fiscal::Register::Event_CommandError: gotoStateEnabled(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::gotoStateCashlessStackPrice() {
	LOG_DEBUG(LOG_SM, "gotoStateCashlessStackPrice");
	executive->setPrice(price);
	context->setState(State_CashlessStackPrice);
}

void SaleManagerExeMasterCore::stateCashlessStackPriceEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessStackPriceEvent");
	switch(envelope->getType()) {
		case ExeMasterInterface::Event_VendPrice: stateCashlessStackPriceEventVendPrice(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateCashlessStackPriceEventVendPrice() {
	LOG_DEBUG(LOG_SM, "stateCashlessStackPriceEventVendPrice");
	if(masterCashlessStack->sale(productId, price, product->getName(), product->getWareId()) == true) {
		cashlessCredit = price;
		gotoStateCashlessStackApproving();
		return;
	} else {
		executive->denyVend(price);
		gotoStateEnabled();
		return;
	}
}

void SaleManagerExeMasterCore::gotoStateCashlessStackApproving() {
	LOG_DEBUG(LOG_SM, "gotoStateCashlessStackApproving");
	chronicler->startSession();
	if(config->getAutomat()->getCashless2Click() == true) { timer->start(EXEMASTER_APPROVING_TIMEOUT); }
	context->setState(State_CashlessStackApproving);
}

void SaleManagerExeMasterCore::stateCashlessStackApprovingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessStackApprovingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: stateCashlessStackApprovingEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: stateCashlessStackApprovingEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_DepositeToken: stateCashlessStackApprovingEventTokenCredit(); return;
		case MdbMasterCashlessInterface::Event_VendApproved: stateCashlessStackApprovingEventVendApproved(envelope); return;
		case MdbMasterCashlessInterface::Event_VendDenied: stateCashlessStackApprovingEventVendDenied(); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateCashlessStackApprovingEventVendDenied(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateCashlessStackApprovingEventCashCredit(EventEnvelope *envelope) {
	uint32_t incomingCredit = 0;
	if(parseCashCredit(envelope, &incomingCredit) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	LOG_INFO(LOG_SM, "stateCashlessStackApprovingEventCashCredit " << incomingCredit);
	cashCredit = incomingCredit;
	executive->denyVend(price);
	masterCashlessStack->closeSession();
	timer->stop();
	gotoStateCashCredit();
}

void SaleManagerExeMasterCore::stateCashlessStackApprovingEventTokenCredit() {
	LOG_DEBUG(LOG_SM, "stateCashlessStackApprovingEventTokenCredit");
	executive->denyVend(price);
	masterCashlessStack->closeSession();
	timer->stop();
	gotoStateTokenCredit();
}

void SaleManagerExeMasterCore::stateCashlessStackApprovingEventVendApproved(EventEnvelope *envelope) {
	LOG_INFO(LOG_SM, "stateCashlessStackApprovingEventVendApproved");
	MdbMasterCashlessInterface::EventApproved event;
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		stateCashlessStackApprovingFailed();
		return;
	}

	LOG_INFO(LOG_SM, "approved " << event.getDevice() << "," << event.getValue1());
	masterCashless = masterCashlessStack->find(event.getDevice());
	if(masterCashless == NULL) {
		LOG_ERROR(LOG_SM, "Unknown cashless " << event.getDevice());
		stateCashlessStackApprovingFailed();
		return;
	}

	saleData.paymentType = event.getType1();
	saleData.credit = event.getValue1();
	approvedPrice = event.getValue1();
	executive->approveVend(price);
	masterCoinChanger->disable();
	masterBillValidator->disable();
	masterCashlessStack->disable();
	timer->stop();
	gotoStateCashlessVending();
}

void SaleManagerExeMasterCore::stateCashlessStackApprovingEventVendDenied() {
	LOG_DEBUG(LOG_SM, "stateCashlessStackApprovingEventVendDenied");
	chronicler->registerCashlessDeniedEvent(product->getId());
	stateCashlessStackApprovingFailed();
}

void SaleManagerExeMasterCore::stateCashlessStackApprovingFailed() {
	executive->denyVend(price);
	masterCashlessStack->closeSession();
	timer->stop();
	context->setState(State_CashlessClosing);
}

void SaleManagerExeMasterCore::stateCashlessStackApprovingTimeout() {
	LOG_DEBUG(LOG_SM, "stateCashlessStackApprovingTimeout");
	executive->denyVend(0);
	context->setState(State_CashlessStackHolding);
}

void SaleManagerExeMasterCore::stateCashlessStackHoldingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessStackHoldingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: stateCashlessStackHoldingEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: stateCashlessStackHoldingEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_DepositeToken: stateCashlessStackHoldingEventTokenCredit(); return;
		case MdbMasterCashlessInterface::Event_VendApproved: stateCashlessStackHoldingEventVendApproved(envelope); return;
		case MdbMasterCashlessInterface::Event_VendDenied: stateCashlessStackHoldingEventVendDenied(); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateCashlessStackHoldingEventVendDenied(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateCashlessStackHoldingEventCashCredit(EventEnvelope *envelope) {
	uint32_t incomingCredit = 0;
	if(parseCashCredit(envelope, &incomingCredit) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	LOG_INFO(LOG_SM, "stateCashlessStackHoldingEventCashCredit " << incomingCredit);
	cashCredit = incomingCredit;
	masterCashlessStack->closeSession();
	gotoStateCashCredit();
}

void SaleManagerExeMasterCore::stateCashlessStackHoldingEventTokenCredit() {
	LOG_DEBUG(LOG_SM, "stateCashlessStackHoldingEventTokenCredit");
	masterCashlessStack->closeSession();
	gotoStateTokenCredit();
}

void SaleManagerExeMasterCore::stateCashlessStackHoldingEventVendApproved(EventEnvelope *envelope) {
	LOG_INFO(LOG_SM, "stateCashlessStackHoldingEventVendApproved");
	MdbMasterCashlessInterface::EventApproved event;
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		stateCashlessStackHoldingFailed();
		return;
	}

	LOG_INFO(LOG_SM, "approved " << event.getDevice() << "," << event.getValue1());
	masterCashless = masterCashlessStack->find(event.getDevice());
	if(masterCashless == NULL) {
		LOG_ERROR(LOG_SM, "Unknown cashless " << event.getDevice());
		stateCashlessStackHoldingFailed();
		return;
	}

	cashlessCredit = price;
	saleData.paymentType = event.getType1();
	saleData.credit = event.getValue1();
	approvedPrice = event.getValue1();
	masterCoinChanger->disable();
	masterBillValidator->disable();
	masterCashlessStack->disable();
	gotoStateCashlessHolding();
}

void SaleManagerExeMasterCore::stateCashlessStackHoldingEventVendDenied() {
	LOG_DEBUG(LOG_SM, "stateCashlessStackHoldingEventVendDenied");
	chronicler->registerCashlessDeniedEvent(product->getId());
	stateCashlessStackHoldingFailed();
}

void SaleManagerExeMasterCore::stateCashlessStackHoldingFailed() {
	executive->denyVend(price);
	masterCashlessStack->closeSession();
	context->setState(State_CashlessClosing);
}

void SaleManagerExeMasterCore::stateCashlessCreditEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessCreditEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: stateCashlessCreditEventDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: stateCashlessCreditEventDeposite(envelope); return;
		case ExeMasterInterface::Event_VendRequest: stateCashlessCreditEventVendRequest(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateCashlessCreditEventSessionEnd(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateCashlessCreditEventDeposite(EventEnvelope *envelope) {
	revalue = 0;
	if(parseCashCredit(envelope, &revalue) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	LOG_INFO(LOG_SM, "stateCashlessCreditEventDeposite " << cashlessCredit << "," << revalue);
	masterCashless->revalue(revalue);
	context->setState(State_CashlessRevaluing);
}

void SaleManagerExeMasterCore::stateCashlessCreditEventVendRequest(EventEnvelope *envelope) {
	EventUint8Interface event(ExeMasterInterface::Event_VendRequest);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event open failed");
		return;
	}

	productId = event.getValue();
	LOG_INFO(LOG_SM, "stateCashlessCreditEventVendRequest " << productId);
	if(product->findByCashlessId(productId) == false) {
		LOG_ERROR(LOG_SM, "Product " << productId << " not found");
		chronicler->registerCashlessIdNotFoundEvent(productId, context);
		masterCashless->closeSession();
		gotoStateEnabled();
		return;
	}

	DateTime datetime;
	config->getEvents()->getRealTime()->getDateTime(&datetime);
	ConfigPrice *productPrice = product->getIndexByDateTime("DA", &datetime);
	if(productPrice == NULL) {
		LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
		chronicler->registerPriceListNotFoundEvent("DA", 1, context);
		masterCashless->closeSession();
		gotoStateEnabled();
		return;
	}

	price = client->calcDiscount(productPrice->getPrice());
	executive->setPrice(price);
	context->setState(State_CashlessPrice);
}

void SaleManagerExeMasterCore::stateCashlessCreditEventSessionEnd() {
	LOG_DEBUG(LOG_SM, "stateCashlessCreditEventSessionEnd");
	gotoStateEnabled();
}

void SaleManagerExeMasterCore::stateCashlessPriceEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessPriceEvent");
	switch(envelope->getType()) {
		case ExeMasterInterface::Event_VendPrice: stateCashlessPriceEventVendPrice(); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateCashlessPriceEventSessionEnd(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateCashlessPriceEventVendPrice() {
	if(price > cashlessCredit) {
		LOG_ERROR(LOG_SM, "Not enough credit (exp= " << price << ", act=" << cashlessCredit << ")");
		masterCashless->closeSession();
		gotoStateEnabled();
		return;
	}

	masterCashless->sale(productId, price, product->getName(), product->getWareId());
	chronicler->startSession();
	gotoStateCashlessApproving();
}

void SaleManagerExeMasterCore::stateCashlessPriceEventSessionEnd() {
	LOG_DEBUG(LOG_SM, "stateCashlessPriceEventSessionEnd");
	executive->denyVend(price);
	chronicler->registerCashlessCanceledEvent(product->getId());
	gotoStateEnabled();
}

void SaleManagerExeMasterCore::stateCashlessRevaluingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessRevaluingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCashlessInterface::Event_RevalueApproved: stateCashlessRevaluingEventRevalueApproved(); return;
		case MdbMasterCashlessInterface::Event_RevalueDenied: stateCashlessRevaluingEventRevalueDenied(); return;
		default: LOG_ERROR(LOG_MDBSCL, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateCashlessRevaluingEventRevalueApproved() {
	LOG_DEBUG(LOG_SM, "stateCashlessRevaluingEventRevalueApproved " << cashlessCredit << "," << revalue);
	cashlessCredit += revalue;
	executive->setCredit(cashlessCredit);
	if(cashCredit == 0) {
		context->setState(State_CashlessCredit);
		return;
	} else {
		revalue = cashCredit;
		masterCashless->revalue(revalue);
		cashCredit = 0;
		context->setState(State_CashlessRevaluing);
		return;
	}
}

void SaleManagerExeMasterCore::stateCashlessRevaluingEventRevalueDenied() {
	LOG_DEBUG(LOG_SM, "stateCashlessRevaluingEventRevalueDenied");
	cashCredit += revalue;
	executive->setCredit(cashlessCredit);
	context->setState(State_CashlessCredit);
}

void SaleManagerExeMasterCore::gotoStateCashlessApproving() {
	LOG_DEBUG(LOG_SM, "gotoStateCashlessApproving");
	if(config->getAutomat()->getCashless2Click() == true) {	timer->start(EXEMASTER_APPROVING_TIMEOUT); }
	context->setState(State_CashlessApproving);
}

void SaleManagerExeMasterCore::stateCashlessApprovingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessApprovingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCashCredit(envelope); return;
		case MdbMasterCashlessInterface::Event_VendApproved: stateCashlessApprovingEventVendApproved(envelope); return;
		case MdbMasterCashlessInterface::Event_VendDenied: stateCashlessApprovingEventVendDenied(); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateCashlessApprovingEventVendDenied(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateCashlessApprovingEventVendApproved(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessApprovingEventVendApproved");
	MdbMasterCashlessInterface::EventApproved event;
	if(event.open(envelope) == false) {
		stateCashlessApprovingFailed();
		return;
	}

	saleData.paymentType = event.getType1();
	saleData.credit = event.getValue1();
	approvedPrice = event.getValue1();
	executive->approveVend(price);
	timer->stop();
	gotoStateCashlessVending();
}

void SaleManagerExeMasterCore::stateCashlessApprovingEventVendDenied() {
	LOG_DEBUG(LOG_SM, "stateCashlessApprovingEventVendDenied");
	chronicler->registerCashlessDeniedEvent(product->getId());
	stateCashlessApprovingFailed();
}

void SaleManagerExeMasterCore::stateCashlessApprovingFailed() {
	executive->denyVend(price);
	masterCashless->closeSession();
	timer->stop();
	context->setState(State_CashlessClosing);
}

void SaleManagerExeMasterCore::stateCashlessAprrovingTimeout() {
	LOG_DEBUG(LOG_SM, "stateCashlessAprrovingTimeout");
	executive->denyVend(0);
	gotoStateCashlessHoldingApproving();
}

void SaleManagerExeMasterCore::gotoStateCashlessHoldingApproving() {
	LOG_DEBUG(LOG_SM, "gotoStateCashlessHoldingApproving");
	context->setState(State_CashlessHoldingApproving);
}

void SaleManagerExeMasterCore::stateCashlessHoldingApprovingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessHoldingApprovingEvent");
	switch(envelope->getType()) {
		case MdbMasterCashlessInterface::Event_VendApproved: stateCashlessHoldingApprovingEventVendApproved(envelope); return;
		case MdbMasterCashlessInterface::Event_VendDenied: stateCashlessHoldingApprovingEventVendDenied(); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateCashlessHoldingApprovingEventVendDenied(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateCashlessHoldingApprovingEventVendApproved(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessHoldingApprovingEventVendApproved");
	MdbMasterCashlessInterface::EventApproved event;
	if(event.open(envelope) == false) {
		stateCashlessHoldingApprovingFailed();
		return;
	}

	cashlessCredit = price;
	saleData.paymentType = event.getType1();
	saleData.credit = event.getValue1();
	approvedPrice = event.getValue1();
	gotoStateCashlessHolding();
}

void SaleManagerExeMasterCore::stateCashlessHoldingApprovingEventVendDenied() {
	LOG_DEBUG(LOG_SM, "stateCashlessHoldingApprovingEventVendDenied");
	chronicler->registerCashlessDeniedEvent(product->getId());
	stateCashlessHoldingApprovingFailed();
}

void SaleManagerExeMasterCore::stateCashlessHoldingApprovingFailed() {
	masterCashless->closeSession();
	context->setState(State_CashlessClosing);
}

void SaleManagerExeMasterCore::gotoStateCashlessHolding() {
	LOG_DEBUG(LOG_SM, "gotoStateCashlessHolding");
	executive->setCredit(cashlessCredit);
	chronicler->startSession();
	timer->start(EXEMASTER_HOLDING_TIMEOUT);
	context->setState(State_CashlessHolding);
}

void SaleManagerExeMasterCore::stateCashlessHoldingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessHoldingEvent");
	switch(envelope->getType()) {
		case ExeMasterInterface::Event_VendRequest: stateCashlessHoldingEventVendRequest(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateCashlessHoldingEventVendRequest(EventEnvelope *envelope) {
	EventUint8Interface event(ExeMasterInterface::Event_VendRequest);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event open failed");
		return;
	}

	productId = event.getValue();
	LOG_INFO(LOG_SM, "stateCashlessCreditEventVendRequest " << productId);
	if(product->findByCashlessId(productId) == false) {
		LOG_ERROR(LOG_SM, "Product " << productId << " not found");
		chronicler->registerCashlessIdNotFoundEvent(productId, context);
		executive->denyVend(price);
		return;
	}

	DateTime datetime;
	config->getEvents()->getRealTime()->getDateTime(&datetime);
	ConfigPrice *productPrice = product->getIndexByDateTime("DA", &datetime);
	if(productPrice == NULL) {
		LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
		chronicler->registerPriceListNotFoundEvent("DA", 1, context);
		executive->denyVend(price);
		return;
	}

	price = client->calcDiscount(productPrice->getPrice());
	if(price != cashlessCredit) {
		LOG_ERROR(LOG_SM, "Price not equal");
		cashlessCredit = 0;
		executive->denyVend(price);
		masterCashless->saleFailed();
		timer->stop();
		gotoStateEnabled();
		return;
	}

	executive->approveVend(price);
	timer->stop();
	gotoStateCashlessVending();
}

void SaleManagerExeMasterCore::stateCashlessHoldingTimeout() {
	LOG_DEBUG(LOG_SM, "stateCashlessHoldingTimeout");
	cashlessCredit = 0;
	masterCashless->saleFailed();
	chronicler->registerSessionClosedByTimeout();
	context->setState(State_CashlessClosing);
}

void SaleManagerExeMasterCore::gotoStateCashlessVending() {
	LOG_DEBUG(LOG_SM, "gotoStateCashlessVending");
	context->setState(State_CashlessVending);
}

void SaleManagerExeMasterCore::stateCashlessVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessVendingEvent");
	switch(envelope->getType()) {
		case ExeMasterInterface::Event_VendComplete: stateCashlessVendingEventVendComplete(); return;
		case ExeMasterInterface::Event_VendFailed: stateCashlessVendingEventVendFailed(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateCashlessVendingEventVendComplete() {
	LOG_INFO(LOG_SM, "stateCashlessVendingEventVendComplete");
	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), approvedPrice, product->getTaxRate(), 1);
	saleData.device.set("DA");
	saleData.priceList = 1;
	saleData.taxSystem = config->getAutomat()->getTaxSystem();
	saleData.loyalityType = client->getLoyalityType();
	saleData.loyalityCode.set(client->getLoyalityCode(), client->getLoyalityLen());
	client->completeVending();

	cashlessCredit -= price;
	config->getAutomat()->sale(&saleData);
	gotoStateCashlessCheckPrinting();
}

void SaleManagerExeMasterCore::stateCashlessVendingEventVendFailed() {
	LOG_DEBUG(LOG_SM, "stateCashlessVendingEventVendFailed");
	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>VEND FAILED " << product->getName());
	cashlessCredit = 0;
	masterCashless->saleFailed();
	chronicler->registerVendFailedEvent(product->getId(), context);
	context->setState(State_CashlessClosing);
}

void SaleManagerExeMasterCore::gotoStateCashlessCheckPrinting() {
	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>TICKET " << saleData.getProductNum() << "," << saleData.credit << "," << saleData.getPrice() << ":" << config->getAutomat()->getDecimalPoint());
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
	context->setState(State_CashlessCheckPrinting);
}

void SaleManagerExeMasterCore::stateCashlessCheckPrintingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessCheckPrintingEvent");
	switch(envelope->getType()) {
		case Fiscal::Register::Event_CommandOK: gotoStateCashlessClosing(); return;
		case Fiscal::Register::Event_CommandError: gotoStateCashlessClosing(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::gotoStateCashlessClosing() {
	LOG_DEBUG(LOG_SM, "gotoStateCashlessClosing");
	masterCashless->saleComplete();
	context->setState(State_CashlessClosing);
}

void SaleManagerExeMasterCore::stateCashlessClosingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessClosingEvent");
	switch(envelope->getType()) {
		case MdbMasterCashlessInterface::Event_SessionEnd: stateCashlessClosingEventSessionEnd(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerExeMasterCore::stateCashlessClosingEventSessionEnd() {
	LOG_DEBUG(LOG_SM, "stateCashlessClosingEventSessionEnd");
	gotoStateEnabled();
}

void SaleManagerExeMasterCore::stateShutdownTimeout() {
	LOG_DEBUG(LOG_SM, "stateShutdownTimeout");
	EventInterface event(SaleManager::Event_Shutdown);
	eventEngine->transmit(&event);
	context->setState(State_Idle);
}
