#include <lib/sale_manager/mdb_master/SaleManagerMdbMasterCore.h>
#include "lib/sale_manager/include/SaleManager.h"

#include "common/logger/include/Logger.h"

#include <string.h>

#define INIT_TIMEOUT 30000
#define SERVICE_TIMEOUT 120000
#define SHUTDOWN_TIMEOUT 2000
//#define REFUNDABLE

SaleManagerMdbMasterCore::SaleManagerMdbMasterCore(SaleManagerMdbMasterParams *c) :
	config(c->config),
	timers(c->timers),
	eventEngine(c->eventEngine),
	fiscalRegister(c->fiscalRegister),
	leds(c->leds),
	chronicler(c->chronicler),
	slaveCashless1(c->slaveCashless1),
	slaveCoinChanger(c->slaveCoinChanger),
	slaveBillValidator(c->slaveBillValidator),
	masterCoinChanger(c->masterCoinChanger),
	masterBillValidator(c->masterBillValidator),
	masterCashlessStack(c->masterCashlessStack),
	deviceId(c->eventEngine)
{
	this->context = config->getAutomat()->getSMContext();
	this->context->setStatus(Mdb::DeviceContext::Status_NotFound);
	this->context->setState(State_Idle);
	this->product = config->getAutomat()->createIterator();
	this->timer = timers->addTimer<SaleManagerMdbMasterCore, &SaleManagerMdbMasterCore::procTimer>(this);

	LOG_DEBUG(LOG_MODEM, "subscribe to " << fiscalRegister->getDeviceId().getValue());
	eventEngine->subscribe(this, GlobalId_FiscalRegister, fiscalRegister->getDeviceId());
	eventEngine->subscribe(this, GlobalId_SlaveCoinChanger);
	eventEngine->subscribe(this, GlobalId_SlaveBillValidator);
	eventEngine->subscribe(this, GlobalId_SlaveCashless1);
	eventEngine->subscribe(this, GlobalId_MasterCoinChanger);
	eventEngine->subscribe(this, GlobalId_MasterBillValidator);
	eventEngine->subscribe(this, GlobalId_MasterCashless);
}

SaleManagerMdbMasterCore::~SaleManagerMdbMasterCore() {
	timers->deleteTimer(this->timer);
	delete product;
}

void SaleManagerMdbMasterCore::reset() {
	LOG_DEBUG(LOG_SM, "reset");
	cashCredit = 0;
	cashlessCredit = 0;
	slaveCashless1->reset();
	fiscalRegister->reset();
	masterCoinChanger->reset();
	masterBillValidator->reset();
	masterCashlessStack->reset();
	timer->start(INIT_TIMEOUT);
	leds->setPayment(LedInterface::State_InProgress);
	context->setStatus(Mdb::DeviceContext::Status_Init);
	context->setState(State_Init);
}

void SaleManagerMdbMasterCore::shutdown() {
	masterCoinChanger->disable();
	masterBillValidator->disable();
	masterCashlessStack->disable();
	timer->start(SHUTDOWN_TIMEOUT);
	context->setState(State_Shutdown);
}

void SaleManagerMdbMasterCore::service() {
	if(context->getState() == State_Service) {
		timer->stop();
		gotoStateSale();
		return;
	}
	if(context->getState() != State_Sale) {
		LOG_ERROR(LOG_SM, "Wrong state " << context->getState());
		context->incProtocolErrorCount();
		return;
	}
	timer->start(SERVICE_TIMEOUT);
	context->setState(State_Service);
}

void SaleManagerMdbMasterCore::testCredit() {
	slaveCoinChanger->deposite(0x43, 0x00);
}

void SaleManagerMdbMasterCore::procTimer() {
	LOG_DEBUG(LOG_SM, "procTimer " << context->getState());
	switch(context->getState()) {
	case State_Init: stateInitTimeout(); break;
	case State_Service: stateServiceTimeout(); break;
	case State_Shutdown: stateShutdownTimeout(); break;
	case State_CashlessApproving: stateCashlessApprovingTimeout(); break;
	default: LOG_ERROR(LOG_SM, "Unwaited timeout " << context->getState());
	}
}

void SaleManagerMdbMasterCore::proc(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "proc " << context->getState() << "," << envelope->getType() << "," << envelope->getDevice());
	switch(envelope->getType()) {
		case MdbSlaveCoinChanger::Event_Error: chronicler->registerDeviceError(envelope); return;
	}

	switch(context->getState()) {
		case State_Idle: LOG_DEBUG(LOG_SM, "Ignore event " << context->getState() << "," << envelope->getType()); return;
		case State_Init: stateInitEvent(envelope); return;
		case State_Sale: stateSaleEvent(envelope); return;
		case State_CashCredit: stateCashCreditEvent(envelope); return;
		case State_CashVending: stateCashVendingEvent(envelope); return;
		case State_CashChange: stateCashChangeEvent(envelope); return;
		case State_CashCheckPrinting: stateCashCheckPrintingEvent(envelope); return;
		case State_CashRefund: stateCashRefundEvent(envelope); return;
		case State_TokenCredit: stateTokenCredit(envelope); return;
		case State_TokenVending: stateTokenVendingEvent(envelope); return;
		case State_TokenCheckPrinting: stateTokenCheckPrintingEvent(envelope); return;
		case State_CashlessStackApproving: stateCashlessStackApprovingEvent(envelope); return;
		case State_CashlessStackHoldingApproving: stateCashlessStackHoldingApprovingEvent(envelope); return;
		case State_CashlessCredit: stateCashlessCreditEvent(envelope); return;
		case State_CashlessRevaluing: stateCashlessRevaluingEvent(envelope); return;
		case State_CashlessApproving: stateCashlessApprovingEvent(envelope); return;
		case State_CashlessHoldingApproving: stateCashlessHoldingApprovingEvent(envelope); return;
		case State_CashlessHoldingCredit: stateCashlessHoldingCreditEvent(envelope); return;
		case State_CashlessVending: stateCashlessVendingEvent(envelope); return;
		case State_CashlessCheckPrinting: stateCashlessCheckPrintingEvent(envelope); return;
		case State_CashlessClosing: stateCashlessClosingEvent(envelope); return;
		case State_Service: stateServiceEvent(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateInitEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateInitEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Ready: slaveBillValidator->reset(); return;
		case MdbMasterCoinChanger::Event_Ready: slaveCoinChanger->reset(); return;
		case MdbSlaveCashlessInterface::Event_Reset: stateInitEventSlaveCashlessReset(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateInitEventSlaveCashlessReset() {
	LOG_DEBUG(LOG_SM, "stateInitEventSlaveCashlessReset");
	if(masterCoinChanger->isInited() == true) {
		setMaxBill();
	}
	if(slaveCoinChanger->isReseted() == false && masterCoinChanger->isInited() == true) {
		slaveCoinChanger->reset();
	}
	if(slaveBillValidator->isReseted() == false && masterBillValidator->isInited() == true) {
		slaveBillValidator->reset();
	}

#if defined(AUTOMAT_PALMA)
	if(slaveCashless1->isEnable() == true) {
#else
	if(slaveCoinChanger->isEnable() == true) {
#endif
		masterCoinChanger->enable();
	} else {
		masterCoinChanger->disable();
	}
	if(slaveBillValidator->isEnable() == true) {
		masterBillValidator->enable();
	} else {
		masterBillValidator->disable();
	}

	timer->stop();
	leds->setPayment(LedInterface::State_Success);
	context->setStatus(Mdb::DeviceContext::Status_Disabled);
	context->setState(State_Sale);
}

void SaleManagerMdbMasterCore::stateInitTimeout() {
	LOG_DEBUG(LOG_SM, "stateInitEvent");
	leds->setPayment(LedInterface::State_Failure);
}

void SaleManagerMdbMasterCore::gotoStateSale() {
	LOG_DEBUG(LOG_SM, "gotoStateSale");
	if(cashCredit > 0) {
		gotoStateCashCredit();
		return;
	}

	if(masterCoinChanger->isInited() == true) {
		setMaxBill();
	}
	if(slaveCoinChanger->isReseted() == false && masterCoinChanger->isInited() == true) {
		slaveCoinChanger->reset();
	}
	if(slaveBillValidator->isReseted() == false && masterBillValidator->isInited() == true) {
		slaveBillValidator->reset();
	}

#if defined(AUTOMAT_PALMA)
	if(slaveCashless1->isEnable() == true) {
#else
	if(slaveCoinChanger->isEnable() == true) {
#endif
		masterCoinChanger->enable();
	} else {
		masterCoinChanger->disable();
	}
	if(slaveBillValidator->isEnable() == true) {
		masterBillValidator->enable();
	} else {
		masterBillValidator->disable();
	}
	if(slaveCashless1->isEnable() == true) {
		stateSaleEventCashlessEnable();
	} else {
		stateSaleEventCashlessDisable();
	}

	saleNum = 0;
	context->setState(State_Sale);
}

void SaleManagerMdbMasterCore::stateSaleEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateSaleEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Ready: slaveBillValidator->reset(); return;
		case MdbMasterBillValidator::Event_Deposite: stateSaleEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_Ready: slaveCoinChanger->reset(); return;
		case MdbMasterCoinChanger::Event_Deposite: stateSaleEventCashCredit(envelope); return;
		case MdbMasterCoinChanger::Event_DepositeToken: stateSaleEventTokenCredit(); return;
		case MdbMasterCoinChanger::Event_EscrowRequest: slaveCoinChanger->escrowRequest(); return;
		case MdbMasterCashlessInterface::Event_Ready: stateSaleEventCashlessReady(); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: stateSaleEventCashlessCredit(envelope); return;
		case MdbSlaveBillValidator::Event_Enable: masterBillValidator->enable(); return;
		case MdbSlaveBillValidator::Event_Disable: masterBillValidator->disable(); return;
		case MdbSlaveCoinChanger::Event_Enable: masterCoinChanger->enable(); return;
		case MdbSlaveCoinChanger::Event_Disable: masterCoinChanger->disable(); return;
		case MdbSlaveCashlessInterface::Event_Enable: stateSaleEventCashlessEnable(); return;
		case MdbSlaveCashlessInterface::Event_Disable: stateSaleEventCashlessDisable(); return;
		case MdbSlaveCashlessInterface::Event_VendRequest: stateSaleEventCashlessVendRequest(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

bool SaleManagerMdbMasterCore::parseCashCredit(EventEnvelope *envelope, uint32_t *incomingCredit) {
	Mdb::EventDeposite event1(MdbMasterBillValidator::Event_Deposite);
	if(event1.open(envelope) == true) {
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

void SaleManagerMdbMasterCore::stateSaleEventCashCredit(EventEnvelope *envelope) {
	uint32_t incomingCredit = 0;
	if(parseCashCredit(envelope, &incomingCredit) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_SM, "stateEnabledEventCredit " << incomingCredit);
	cashCredit = incomingCredit;
	gotoStateCashCredit();
}

void SaleManagerMdbMasterCore::stateSaleEventTokenCredit() {
	LOG_DEBUG(LOG_SM, "stateSaleEventTokenCredit");
	gotoStateTokenCredit();
}

void SaleManagerMdbMasterCore::stateSaleEventCashlessReady() {
	LOG_DEBUG(LOG_SM, "stateSaleEventCashlessReady");
	masterCashlessStack->enable();
}

void SaleManagerMdbMasterCore::stateSaleEventCashlessCredit(EventEnvelope *envelope) {
	EventUint32Interface event(MdbMasterCashlessInterface::Event_SessionBegin);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_SM, "stateSaleEventCashlessCredit " << event.getDevice() << "," << event.getValue());
	masterCashless = masterCashlessStack->find(event.getDevice());
	if(masterCashless == NULL) {
		LOG_ERROR(LOG_SM, "Unknown cashless " << event.getDevice());
		context->incProtocolErrorCount();
		return;
	}

	cashlessCredit = event.getValue();
#ifdef REFUNDABLE
	if(masterCashless->isRefundAble() == false) {
		masterCoinChanger->disable();
		masterBillValidator->disable();
	} else {
		masterCoinChanger->enable();
		masterBillValidator->enable();
	}
#else
	masterCoinChanger->disable();
	masterBillValidator->disable();
#endif
	slaveCashless1->setCredit(cashlessCredit);
	chronicler->startSession();
	context->setState(State_CashlessCredit);
}

void SaleManagerMdbMasterCore::stateSaleEventCashlessEnable() {
	LOG_INFO(LOG_SM, "stateSaleEventCashlessEnable");
#if defined(AUTOMAT_PALMA)
	masterCoinChanger->enable();
#endif
	masterCashlessStack->enable();
	context->setStatus(Mdb::DeviceContext::Status_Enabled);
	EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, true);
	eventEngine->transmit(&event);
}

void SaleManagerMdbMasterCore::stateSaleEventCashlessDisable() {
	LOG_INFO(LOG_SM, "stateSaleEventCashlessDisable");
#if defined(AUTOMAT_PALMA)
	masterCoinChanger->disable();
#endif
	masterCashlessStack->disable();
	context->setStatus(Mdb::DeviceContext::Status_Disabled);
	EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, false);
	eventEngine->transmit(&event);
}

void SaleManagerMdbMasterCore::stateSaleEventCashlessVendRequest(EventEnvelope *envelope) {
	MdbSlaveCashlessInterface::EventVendRequest event(MdbSlaveCashlessInterface::Event_VendRequest);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_SM, "stateCashlessCreditEventVendRequest " << event.getProductId() << "," << event.getPrice());
	if(findProduct(event.getProductId(), event.getPrice(), "DA", 1) == false) {
		LOG_ERROR(LOG_SM, "Product not found");
		slaveCashless1->denyVend(true);
		gotoStateSale();
		return;
	}

	if(masterCashlessStack->sale(product->getCashlessId(), price, product->getName(), product->getWareId()) == false) {
		slaveCashless1->denyVend(true);
		gotoStateSale();
		return;
	}

	cashlessCredit = price;
	chronicler->startSession();
	gotoStateCashlessStackApproving();
}

void SaleManagerMdbMasterCore::gotoStateCashCredit() {
	LOG_DEBUG(LOG_SM, "gotoStateCashCredit");
	if(cashCredit < config->getAutomat()->getMaxCredit()) {
		masterCoinChanger->enable();
		masterBillValidator->enable();
	} else {
		masterCoinChanger->disable();
		masterBillValidator->disable();
	}
	masterCashlessStack->disable();
	slaveCashless1->setCredit(cashCredit);
	context->setState(State_CashCredit);
}

void SaleManagerMdbMasterCore::stateCashCreditEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashCreditEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: stateCashCreditEventCredit(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: stateCashCreditEventCredit(envelope); return;
		case MdbMasterCoinChanger::Event_DepositeToken: gotoStateTokenCredit(); return;
		case MdbMasterCoinChanger::Event_EscrowRequest: stateCashCreditEventEscrowRequest(); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendRequest: stateCashCreditEventVendRequest(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateCashCreditEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateCashCreditEventCredit(EventEnvelope *envelope) {
	uint32_t incomingCredit = 0;
	if(parseCashCredit(envelope, &incomingCredit) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_SM, "stateCashCreditEventCredit " << cashCredit << "," << incomingCredit);
	cashCredit += incomingCredit;
	if(cashCredit >= config->getAutomat()->getMaxCredit()) {
		masterCoinChanger->disable();
		masterBillValidator->disable();
	}

	slaveCashless1->setCredit(cashCredit);
}

void SaleManagerMdbMasterCore::stateCashCreditEventEscrowRequest() {
	LOG_INFO(LOG_SM, "stateCashCreditEventEscrowRequest");
	if(config->getAutomat()->getCreditHolding() == false || (config->getAutomat()->getMultiVend() == true && saleNum > 0)) {
		slaveCashless1->cancelVend();
		chronicler->registerCashCanceledEvent();
		gotoStateCashRefund();
		return;
	}
}

void SaleManagerMdbMasterCore::stateCashCreditEventVendRequest(EventEnvelope *envelope) {
	MdbSlaveCashlessInterface::EventVendRequest event(MdbSlaveCashlessInterface::Event_VendRequest);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_SM, "stateCreditEventVendRequest " << event.getProductId() << "," << event.getPrice());
	if(findProduct(event.getProductId(), event.getPrice(), "CA", 0) == false) {
		LOG_ERROR(LOG_SM, "Product not found");
		slaveCashless1->denyVend(true);
		if(config->getAutomat()->getCreditHolding() == true) {
			gotoStateSale();
			return;
		} else {
			gotoStateCashRefund();
			return;
		}
	}

	if(price > cashCredit) {
		LOG_ERROR(LOG_SM, "Not enough credit (exp= " << price << ", act=" << cashCredit << ")");
		slaveCashless1->denyVend(true);
		gotoStateSale();
		return;
	}

	approveProduct();
	masterCoinChanger->disable();
	masterBillValidator->disable();
	context->setState(State_CashVending);
}

void SaleManagerMdbMasterCore::stateCashCreditEventVendCancel() {
	LOG_DEBUG(LOG_SM, "stateCreditEventVendCancel");
	if(config->getAutomat()->getCreditHolding() == false || (config->getAutomat()->getMultiVend() == true && saleNum > 0)) {
		chronicler->registerCashCanceledEvent();
		gotoStateCashRefund();
		return;
	} else {
		slaveCashless1->setCredit(cashCredit);
		return;
	}
}

void SaleManagerMdbMasterCore::stateCashVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashVendingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventBillDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCoinDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendComplete: stateCashVendingEventComplete(); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateCashVendingEventCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::unwaitedEventCoinDeposite(EventEnvelope *envelope) {
	MdbMasterCoinChanger::EventCoin event(MdbMasterCoinChanger::Event_Deposite);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_SM, "unwaitedEventCoinDeposite " << cashCredit << "," << event.getNominal());
	cashCredit += event.getNominal();
	chronicler->registerCoinInEvent(event.getNominal(), event.getRoute());
}

void SaleManagerMdbMasterCore::unwaitedEventBillDeposite(EventEnvelope *envelope) {
	Mdb::EventDeposite event(MdbMasterBillValidator::Event_Deposite);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_SM, "unwaitedEventBillDeposite " << cashCredit << "," << event.getNominal());
	cashCredit += event.getNominal();
	chronicler->registerBillInEvent(event.getNominal(), event.getRoute());
}

void SaleManagerMdbMasterCore::unwaitedEventSessionBegin(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "unwaitedEventSessionBegin");
	MdbMasterCashlessInterface *cashless = masterCashlessStack->find(envelope->getDevice());
	if(cashless == NULL) {
		LOG_ERROR(LOG_SM, "Unknown cashless " << envelope->getDevice());
		context->incProtocolErrorCount();
		return;
	}
	cashless->closeSession();
}

void SaleManagerMdbMasterCore::stateCashVendingEventComplete() {
	LOG_INFO(LOG_SM, "stateCashVendingEventComplete");
	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), price, product->getTaxRate(), 1);
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	if(config->getAutomat()->getMultiVend() == true) { saleData.credit = price; } else { saleData.credit = cashCredit; }
	saleData.taxSystem = config->getAutomat()->getTaxSystem();

	cashCredit -= price;
	saleNum++;
	config->getAutomat()->sale(&saleData);
	if(config->getAutomat()->getMultiVend() == true) {
		gotoStateCashCheckPrinting();
	} else {
		gotoStateCashChange();
	}
}

void SaleManagerMdbMasterCore::stateCashVendingEventCancel() {
	LOG_INFO(LOG_SM, "stateCashVendingEventCancel");
	chronicler->registerVendFailedEvent(product->getId(), context);
	gotoStateCashRefund();
}

void SaleManagerMdbMasterCore::gotoStateCashChange() {
	LOG_INFO(LOG_SM, "gotoStateCashChange");
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

void SaleManagerMdbMasterCore::stateCashChangeEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashChangeEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventBillDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCoinDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbMasterCoinChanger::Event_Dispense: gotoStateCashChange(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::gotoStateCashCheckPrinting() {
	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>TICKET " << saleData.getProductNum() << "," << saleData.credit << "," << saleData.getPrice() << ":" << config->getAutomat()->getDecimalPoint());
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
	context->setState(State_CashCheckPrinting);
}

void SaleManagerMdbMasterCore::stateCashCheckPrintingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashCheckPrintingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventBillDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCoinDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case Fiscal::Register::Event_CommandOK: stateCashCheckPrintingEventRegisterComplete();  return;
		case Fiscal::Register::Event_CommandError: stateCashCheckPrintingEventRegisterComplete(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateCashCheckPrintingEventRegisterComplete() {
	gotoStateSale();
}

void SaleManagerMdbMasterCore::gotoStateCashRefund() {
	LOG_INFO(LOG_SM, "gotoStateCashRefund");
	if(masterCoinChanger->isInited() == true && cashCredit > 0) {
		LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>DISPENSE " << cashCredit);
		masterCoinChanger->dispense(cashCredit);
		chronicler->registerChangeEvent(cashCredit);
		cashCredit = 0;
		context->setState(State_CashRefund);
		return;
	}

	gotoStateSale();
}

void SaleManagerMdbMasterCore::stateCashRefundEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashDispenseEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventBillDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCoinDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbMasterCoinChanger::Event_Dispense: gotoStateCashRefund(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::gotoStateTokenCredit() {
	LOG_DEBUG(LOG_SM, "gotoStateTokenCredit");
	masterCoinChanger->disable();
	masterBillValidator->disable();
	masterCashlessStack->disable();
	slaveCashless1->setCredit(config->getAutomat()->getMaxCredit());
	context->setState(State_TokenCredit);
}

void SaleManagerMdbMasterCore::stateTokenCredit(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateTokenCredit");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventBillDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCoinDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendRequest: stateTokenCreditEventVendRequest(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateTokenCreditEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateTokenCreditEventVendRequest(EventEnvelope *envelope) {
	MdbSlaveCashlessInterface::EventVendRequest event(MdbSlaveCashlessInterface::Event_VendRequest);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_SM, "stateCreditEventVendRequest " << event.getProductId() << "," << event.getPrice());
	if(findProduct(event.getProductId(), event.getPrice(), "CA", 0) == false) {
		LOG_ERROR(LOG_SM, "Product not found");
		slaveCashless1->denyVend(true);
		gotoStateSale();
		return;
	}

	slaveCashless1->approveVend(price);
	masterCoinChanger->disable();
	masterBillValidator->disable();
	context->setState(State_TokenVending);
}

void SaleManagerMdbMasterCore::stateTokenCreditEventVendCancel() {
	LOG_DEBUG(LOG_SM, "stateTokenCreditEventVendCancel");
	gotoStateSale();
}

void SaleManagerMdbMasterCore::stateTokenVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateTokenVendingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventBillDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCoinDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendComplete: stateTokenVendingEventComplete(); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateTokenVendingEventCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateTokenVendingEventComplete() {
	LOG_INFO(LOG_SM, "stateTokenVendingEventComplete");
	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), price, product->getTaxRate(), 1);
	saleData.device.set("TA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Token;
	saleData.credit = price;
	saleData.taxSystem = config->getAutomat()->getTaxSystem();

	config->getAutomat()->sale(&saleData);
	gotoStateTokenCheckPrinting();
}

void SaleManagerMdbMasterCore::stateTokenVendingEventCancel() {
	LOG_INFO(LOG_SM, "stateTokenVendingEventCancel");
	chronicler->registerVendFailedEvent(product->getId(), context);
	gotoStateSale();
}

void SaleManagerMdbMasterCore::gotoStateTokenCheckPrinting() {
	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>TICKET " << saleData.getProductNum() << "," << saleData.credit << "," << saleData.getPrice() << ":" << config->getAutomat()->getDecimalPoint());
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
	context->setState(State_TokenCheckPrinting);
}

void SaleManagerMdbMasterCore::stateTokenCheckPrintingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateTokenCheckPrintingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventBillDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCoinDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case Fiscal::Register::Event_CommandOK: gotoStateSale(); return;
		case Fiscal::Register::Event_CommandError: gotoStateSale(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::gotoStateCashlessStackApproving() {
	LOG_DEBUG(LOG_SM, "gotoStateCashlessStackApproving");
	if(config->getAutomat()->getCashless2Click() == true) {	timer->start(MDBMASTER_APPROVING_TIMEUT); }
	context->setState(State_CashlessStackApproving);
}

void SaleManagerMdbMasterCore::stateCashlessStackApprovingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessStackApprovingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventBillDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCoinDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbMasterCashlessInterface::Event_VendApproved: stateCashlessStackApprovingEventVendApproved(envelope); return;
		case MdbMasterCashlessInterface::Event_VendDenied: stateCashlessStackApprovingEventVendDenied(); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateCashlessStackApprovingEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateCashlessStackApprovingEventVendApproved(EventEnvelope *envelope) {
	LOG_INFO(LOG_SM, "stateCashlessStackApprovingEventVendApproved");
	MdbMasterCashlessInterface::EventApproved event;
	if(event.open(envelope) == false) {
		chronicler->registerCashlessCanceledEvent(product->getId());
		stateCashlessStackApprovingFailed();
		return;
	}

	LOG_INFO(LOG_SM, "approved " << event.getDevice() << "," << event.getValue1());
	masterCashless = masterCashlessStack->find(event.getDevice());
	if(masterCashless == NULL) {
		LOG_ERROR(LOG_SM, "Unknown cashless " << event.getDevice());
		chronicler->registerCashlessCanceledEvent(product->getId());
		stateCashlessStackApprovingFailed();
		return;
	}

	saleData.paymentType = event.getType1();
	saleData.credit = event.getValue1();
	approvedPrice = event.getValue1();
	approveProduct();
	timer->stop();
	context->setState(State_CashlessVending);
}

void SaleManagerMdbMasterCore::stateCashlessStackApprovingEventVendDenied() {
	LOG_INFO(LOG_SM, "stateCashlessStackApprovingEventVendDenied");
	chronicler->registerCashlessDeniedEvent(product->getId());
	stateCashlessStackApprovingFailed();
}

void SaleManagerMdbMasterCore::stateCashlessStackApprovingFailed() {
	LOG_INFO(LOG_SM, "stateCashlessStackApprovingFailed");
	slaveCashless1->denyVend(true);
	masterCashlessStack->closeSession();
	timer->stop();
	gotoStateSale();
}

void SaleManagerMdbMasterCore::stateCashlessStackApprovingEventVendCancel() {
	LOG_INFO(LOG_SM, "stateCashlessStackApprovingEventVendCancel");
	chronicler->registerCashlessCanceledEvent(product->getId());
	masterCashlessStack->closeSession();
	timer->stop();
	gotoStateSale();
}

void SaleManagerMdbMasterCore::stateCashlessStackApprovingTimeout() {
	LOG_INFO(LOG_SM, "stateCashlessStackApprovingTimeout");
	slaveCashless1->denyVend(true);
	context->setState(State_CashlessStackHoldingApproving);
}

void SaleManagerMdbMasterCore::stateCashlessStackHoldingApprovingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessStackHoldingApprovingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventBillDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCoinDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbMasterCashlessInterface::Event_VendApproved: stateCashlessStackHoldingApprovingEventVendApproved(envelope); return;
		case MdbMasterCashlessInterface::Event_VendDenied: stateCashlessStackHoldingApprovingEventVendDenied(); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateCashlessStackHoldingApprovingEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateCashlessStackHoldingApprovingEventVendApproved(EventEnvelope *envelope) {
	LOG_INFO(LOG_SM, "stateCashlessStackHoldingApprovingEventVendApproved");
	MdbMasterCashlessInterface::EventApproved event;
	if(event.open(envelope) == false) {
		chronicler->registerCashlessCanceledEvent(product->getId());
		stateCashlessStackHoldingApprovingFailed();
		return;
	}

	LOG_INFO(LOG_SM, "approved " << event.getDevice() << "," << event.getValue1());
	masterCashless = masterCashlessStack->find(event.getDevice());
	if(masterCashless == NULL) {
		LOG_ERROR(LOG_SM, "Unknown cashless " << event.getDevice());
		chronicler->registerCashlessCanceledEvent(product->getId());
		stateCashlessStackHoldingApprovingFailed();
		return;
	}

	cashlessCredit = price;
	saleData.paymentType = event.getType1();
	saleData.credit = event.getValue1();
	approvedPrice = event.getValue1();
	slaveCashless1->setCredit(cashlessCredit);
	context->setState(State_CashlessHoldingCredit);
}

void SaleManagerMdbMasterCore::stateCashlessStackHoldingApprovingEventVendDenied() {
	LOG_INFO(LOG_SM, "stateCashlessStackHoldingApprovingEventVendDenied");
	chronicler->registerCashlessDeniedEvent(product->getId());
	stateCashlessStackHoldingApprovingFailed();
}

void SaleManagerMdbMasterCore::stateCashlessStackHoldingApprovingFailed() {
	LOG_INFO(LOG_SM, "stateCashlessStackHoldingApprovingFailed");
	masterCashlessStack->closeSession();
	gotoStateSale();
}

void SaleManagerMdbMasterCore::stateCashlessStackHoldingApprovingEventVendCancel() {
	LOG_INFO(LOG_SM, "stateCashlessStackHoldingApprovingEventVendCancel");
	chronicler->registerCashlessCanceledEvent(product->getId());
	masterCashlessStack->closeSession();
	gotoStateSale();
}

void SaleManagerMdbMasterCore::stateCashlessCreditEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessCreditEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: stateCashlessCreditEventDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: stateCashlessCreditEventDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateCashlessCreditEventSessionEnd(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendRequest: stateCashlessCreditEventVendRequest(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateCashlessCreditEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateCashlessCreditEventDeposite(EventEnvelope *envelope) {
	revalue = 0;
	if(parseCashCredit(envelope, &revalue) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

#if 0
	if(masterCashless->isRefundAble() == false) {
		LOG_INFO(LOG_SM, "Unwaited cash");
		cashCredit += revalue;
		return;
	}

	LOG_INFO(LOG_SM, "stateCashlessCreditEventDeposite " << cashlessCredit << "," << revalue);
	masterCashless->revalue(revalue);
	context->setState(State_CashlessRevaluing);
#else
	LOG_INFO(LOG_SM, "Unwaited cash");
	cashCredit += revalue;
	return;
#endif
}

void SaleManagerMdbMasterCore::stateCashlessCreditEventSessionEnd(EventEnvelope *envelope) {
	LOG_INFO(LOG_SM, "stateCashlessCreditEventSessionEnd");
	if(masterCashless->getDeviceId().getValue() == envelope->getDevice()) {
		slaveCashless1->cancelVend();
		chronicler->registerSessionClosedByTerminal();
		gotoStateSale();
		return;
	}
}

void SaleManagerMdbMasterCore::stateCashlessCreditEventVendRequest(EventEnvelope *envelope) {
	MdbSlaveCashlessInterface::EventVendRequest event(MdbSlaveCashlessInterface::Event_VendRequest);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_SM, "stateCashlessCreditEventVendRequest " << event.getProductId() << "," << event.getPrice());
	if(findProduct(event.getProductId(), event.getPrice(), "DA", 1) == false) {
		LOG_ERROR(LOG_SM, "Product not found");
		slaveCashless1->denyVend(true);
		masterCashless->closeSession();
		gotoStateSale();
		return;
	}

	if(price > cashlessCredit) {
		LOG_ERROR(LOG_SM, "Not enough credit (exp= " << price << ", act=" << cashlessCredit << ")");
		slaveCashless1->denyVend(true);
		masterCashless->closeSession();
		gotoStateSale();
		return;
	}

	if(masterCashless->sale(product->getCashlessId(), price, product->getName(), product->getWareId()) == false) {
		slaveCashless1->denyVend(true);
		masterCashless->closeSession();
		gotoStateSale();
		return;
	}

	chronicler->startSession();
	gotoStateCashlessApproving();
}

void SaleManagerMdbMasterCore::stateCashlessCreditEventVendCancel() {
	LOG_INFO(LOG_SM, "stateCashlessCreditEventVendCancel");
	masterCashless->closeSession();
	chronicler->registerSessionClosedByMaster();
	gotoStateSale();
}

void SaleManagerMdbMasterCore::stateCashlessRevaluingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessRevaluingEvent");
	switch(envelope->getType()) {
		case MdbMasterCashlessInterface::Event_RevalueApproved: stateCashlessRevaluingEventRevalueApproved(); return;
		case MdbMasterCashlessInterface::Event_RevalueDenied: stateCashlessRevaluingEventRevalueDenied(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateCashlessRevaluingEventRevalueApproved() {
	LOG_DEBUG(LOG_SM, "stateCashlessRevaluingEventRevalueApproved");
	cashlessCredit += revalue;
	slaveCashless1->setCredit(cashlessCredit);
	context->setState(State_CashlessCredit);
}

void SaleManagerMdbMasterCore::stateCashlessRevaluingEventRevalueDenied() {
	LOG_DEBUG(LOG_SM, "stateCashlessRevaluingEventRevalueDenied");
	masterCoinChanger->dispense(revalue);
	context->setState(State_CashlessCredit);
}

void SaleManagerMdbMasterCore::gotoStateCashlessApproving() {
	LOG_DEBUG(LOG_SM, "gotoStateCashlessApproving");
	if(config->getAutomat()->getCashless2Click() == true) {	timer->start(MDBMASTER_APPROVING_TIMEUT); }
	context->setState(State_CashlessApproving);
}

void SaleManagerMdbMasterCore::stateCashlessApprovingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessApprovingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventBillDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCoinDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbMasterCashlessInterface::Event_VendApproved: stateCashlessApprovingEventVendApproved(envelope); return;
		case MdbMasterCashlessInterface::Event_VendDenied: stateCashlessApprovingEventVendDenied(); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateCashlessApprovingEventSessionEnd(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateCashlessApprovingEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateCashlessApprovingEventVendApproved(EventEnvelope *envelope) {
	LOG_INFO(LOG_SM, "stateCashlessApprovingEventVendApproved");
	MdbMasterCashlessInterface::EventApproved event;
	if(event.open(envelope) == false) {
		stateCashlessApprovingFailed();
		return;
	}

	saleData.paymentType = event.getType1();
	saleData.credit = event.getValue1();
	approvedPrice = event.getValue1();
	approveProduct();
	timer->stop();
	context->setState(State_CashlessVending);
}

void SaleManagerMdbMasterCore::stateCashlessApprovingEventVendDenied() {
	LOG_INFO(LOG_SM, "stateCashlessApprovingEventVendDenied");
	chronicler->registerCashlessDeniedEvent(product->getId());
	stateCashlessApprovingFailed();
}

void SaleManagerMdbMasterCore::stateCashlessApprovingFailed() {
	slaveCashless1->denyVend(true);
	masterCashless->closeSession();
	timer->stop();
	context->setState(State_CashlessClosing);
}

void SaleManagerMdbMasterCore::stateCashlessApprovingEventSessionEnd(EventEnvelope *envelope) {
	LOG_INFO(LOG_SM, "stateCashlessApprovingEventSessionEnd");
	if(masterCashless->getDeviceId().getValue() == envelope->getDevice()) {
		slaveCashless1->denyVend(true);
		timer->stop();
		gotoStateSale();
		return;
	}
}

void SaleManagerMdbMasterCore::stateCashlessApprovingEventVendCancel() {
	LOG_INFO(LOG_SM, "stateCashlessApprovingEventVendCancel");
	masterCashless->closeSession();
	chronicler->registerCashlessCanceledEvent(product->getId());
	timer->stop();
	gotoStateSale();
}

void SaleManagerMdbMasterCore::stateCashlessApprovingTimeout() {
	LOG_INFO(LOG_SM, "stateCashlessApprovingTimeout");
	slaveCashless1->denyVend(true);
	context->setState(State_CashlessHoldingApproving);
}

void SaleManagerMdbMasterCore::stateCashlessHoldingApprovingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessHoldingApprovingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventBillDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCoinDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbMasterCashlessInterface::Event_VendApproved: stateCashlessHoldingApprovingEventVendApproved(envelope); return;
		case MdbMasterCashlessInterface::Event_VendDenied: stateCashlessHoldingApprovingEventVendDenied(); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateCashlessHoldingApprovingEventSessionEnd(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateCashlessHoldingApprovingEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateCashlessHoldingApprovingEventVendApproved(EventEnvelope *envelope) {
	LOG_INFO(LOG_SM, "stateCashlessHoldingApprovingEventVendApproved");
	MdbMasterCashlessInterface::EventApproved event;
	if(event.open(envelope) == false) {
		stateCashlessHoldingApprovingFailed();
		return;
	}

	cashlessCredit = price;
	saleData.paymentType = event.getType1();
	saleData.credit = event.getValue1();
	approvedPrice = event.getValue1();
	slaveCashless1->setCredit(cashlessCredit);
	context->setState(State_CashlessHoldingCredit);
}

void SaleManagerMdbMasterCore::stateCashlessHoldingApprovingEventVendDenied() {
	LOG_INFO(LOG_SM, "stateCashlessHoldingApprovingEventVendDenied");
	chronicler->registerCashlessDeniedEvent(product->getId());
	stateCashlessHoldingApprovingFailed();
}

void SaleManagerMdbMasterCore::stateCashlessHoldingApprovingFailed() {
	masterCashless->closeSession();
	context->setState(State_CashlessClosing);
}

void SaleManagerMdbMasterCore::stateCashlessHoldingApprovingEventSessionEnd(EventEnvelope *envelope) {
	LOG_INFO(LOG_SM, "stateCashlessHoldingApprovingEventSessionEnd");
	if(masterCashless->getDeviceId().getValue() == envelope->getDevice()) {
		gotoStateSale();
		return;
	}
}

void SaleManagerMdbMasterCore::stateCashlessHoldingApprovingEventVendCancel() {
	LOG_INFO(LOG_SM, "stateCashlessHoldingApprovingEventVendCancel");
	masterCashless->closeSession();
	chronicler->registerCashlessCanceledEvent(product->getId());
	gotoStateSale();
}

void SaleManagerMdbMasterCore::stateCashlessHoldingCreditEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessHoldingCreditEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: stateCashlessHoldingCreditEventDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: stateCashlessHoldingCreditEventDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateCashlessHoldingCreditEventSessionEnd(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendRequest: stateCashlessHoldingCreditEventVendRequest(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateCashlessHoldingCreditEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateCashlessHoldingCreditEventDeposite(EventEnvelope *envelope) {
	revalue = 0;
	if(parseCashCredit(envelope, &revalue) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

#if 0
	if(masterCashless->isRefundAble() == false) {
		LOG_INFO(LOG_SM, "Unwaited cash");
		cashCredit += revalue;
		return;
	}

	LOG_INFO(LOG_SM, "stateCashlessCreditEventDeposite " << cashlessCredit << "," << revalue);
	masterCashless->revalue(revalue);
	context->setState(State_CashlessRevaluing);
#else
	LOG_INFO(LOG_SM, "Unwaited cash");
	cashCredit += revalue;
	return;
#endif
}

void SaleManagerMdbMasterCore::stateCashlessHoldingCreditEventSessionEnd(EventEnvelope *envelope) {
	LOG_INFO(LOG_SM, "stateCashlessHoldingEventSessionEnd");
	if(masterCashless->getDeviceId().getValue() == envelope->getDevice()) {
		slaveCashless1->cancelVend();
		chronicler->registerSessionClosedByTerminal();
		gotoStateSale();
		return;
	}
}

void SaleManagerMdbMasterCore::stateCashlessHoldingCreditEventVendRequest(EventEnvelope *envelope) {
	MdbSlaveCashlessInterface::EventVendRequest event(MdbSlaveCashlessInterface::Event_VendRequest);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_SM, "stateCashlessHoldingEventVendRequest " << event.getProductId() << "," << event.getPrice());
	if(findProduct(event.getProductId(), event.getPrice(), "DA", 1) == false) {
		LOG_ERROR(LOG_SM, "Product not found");
		slaveCashless1->denyVend(true);
		masterCashless->closeSession();
		gotoStateSale();
		return;
	}

	if(price != cashlessCredit) {
		LOG_ERROR(LOG_SM, "Not enough credit (exp= " << price << ", act=" << cashlessCredit << ")");
		slaveCashless1->denyVend(true);
		masterCashless->closeSession();
		gotoStateSale();
		return;
	}

	approveProduct();
	context->setState(State_CashlessVending);
}

void SaleManagerMdbMasterCore::stateCashlessHoldingCreditEventVendCancel() {
	LOG_INFO(LOG_SM, "stateCashlessHoldingEventVendCancel");
	masterCashless->closeSession();
	chronicler->registerSessionClosedByMaster();
	gotoStateSale();
}

void SaleManagerMdbMasterCore::stateCashlessVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessVendingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventBillDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCoinDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendComplete: stateCashlessVendingEventVendComplete(); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateCashlessVendingEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateCashlessVendingEventVendComplete() {
	LOG_INFO(LOG_SM, "stateCashlessVendingEventVendComplete");
	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), approvedPrice, product->getTaxRate(), 1);
	saleData.device.set("DA");
	saleData.priceList = 1;
	saleData.taxSystem = config->getAutomat()->getTaxSystem();

	cashlessCredit -= price;
	config->getAutomat()->sale(&saleData);
	gotoStateCashlessCheckPrinting();
}

void SaleManagerMdbMasterCore::stateCashlessVendingEventVendCancel() {
	LOG_INFO(LOG_SM, "stateCashlessVendingEventVendCancel " << product->getName());
	chronicler->registerVendFailedEvent(product->getId(), context);
	cashlessCredit = 0;
	if(masterCashless->saleFailed() == false) {
		masterCashless->reset();
		gotoStateSale();
		return;
	}
	context->setState(State_CashlessClosing);
}

void SaleManagerMdbMasterCore::gotoStateCashlessCheckPrinting() {
	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>TICKET " << saleData.getProductNum() << "," << saleData.credit << "," << saleData.getPrice() << ":" << config->getAutomat()->getDecimalPoint());
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
	context->setState(State_CashlessCheckPrinting);
}

void SaleManagerMdbMasterCore::stateCashlessCheckPrintingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessCheckPrintingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventBillDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCoinDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case Fiscal::Register::Event_CommandOK: gotoStateCashlessClosing(); return;
		case Fiscal::Register::Event_CommandError: gotoStateCashlessClosing(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::gotoStateCashlessClosing() {
	LOG_DEBUG(LOG_SM, "gotoStateCashlessClosing");
	if(masterCashless->saleComplete() == false) {
		masterCashless->reset();
		gotoStateSale();
		return;
	}
	context->setState(State_CashlessClosing);
}

void SaleManagerMdbMasterCore::stateCashlessClosingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessClosingEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Deposite: unwaitedEventBillDeposite(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: unwaitedEventCoinDeposite(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateCashlessClosingEventSessionEnd(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateCashlessClosingEventSessionEnd(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessClosingEventSessionEnd");
	if(masterCashless->getDeviceId().getValue() == envelope->getDevice()) {
		gotoStateSale();
		return;
	}
}

void SaleManagerMdbMasterCore::stateServiceEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateServiceEvent");
	switch(envelope->getType()) {
		case MdbMasterBillValidator::Event_Ready: slaveBillValidator->reset(); return;
		case MdbMasterCoinChanger::Event_Ready: slaveCoinChanger->reset(); return;
		case MdbSlaveBillValidator::Event_Enable: masterBillValidator->enable(); return;
		case MdbSlaveBillValidator::Event_Disable: masterBillValidator->disable(); return;
		case MdbSlaveCoinChanger::Event_Enable: masterCoinChanger->enable(); return;
		case MdbSlaveCoinChanger::Event_Disable: masterCoinChanger->disable(); return;
		case MdbSlaveCoinChanger::Event_DispenseCoin: stateServiceEventDispenseCoin(envelope); return;
		case MdbMasterCoinChanger::Event_DispenseCoin: slaveCoinChanger->dispenseComplete(); return;
		case MdbMasterCoinChanger::Event_Deposite: stateServiceEventDeposite(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbMasterCore::stateServiceEventDispenseCoin(EventEnvelope *envelope) {
	EventUint8Interface event(MdbSlaveCoinChanger::Event_DispenseCoin);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>> DISPENSE COIN " << event.getValue());
	masterCoinChanger->dispenseCoin(event.getValue());
	timer->start(SERVICE_TIMEOUT);
}

void SaleManagerMdbMasterCore::stateServiceEventDeposite(EventEnvelope *envelope) {
	MdbMasterCoinChanger::EventCoin event(MdbMasterCoinChanger::Event_Deposite);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>> DEPOSITE COIN " << event.getNominal());
	slaveCoinChanger->deposite(event.getByte1(), event.getByte2());
	timer->start(SERVICE_TIMEOUT);
}

void SaleManagerMdbMasterCore::stateServiceTimeout() {
	LOG_DEBUG(LOG_SM, "stateServiceTimeout");
	timer->stop();
	gotoStateSale();
}

void SaleManagerMdbMasterCore::stateShutdownTimeout() {
	LOG_DEBUG(LOG_SM, "stateShutdownTimeout");
	context->setState(State_Idle);
	EventInterface event(deviceId, SaleManager::Event_Shutdown);
	eventEngine->transmit(&event);
}

bool SaleManagerMdbMasterCore::findProduct(uint16_t eventId, uint32_t eventPrice, const char *deviceId, uint8_t priceListNumber) {
	if(config->getAutomat()->getPriceHolding() <= 0) {
		uint16_t productId = eventId;
		if(product->findByCashlessId(productId) == false) {
			LOG_ERROR(LOG_SM, "Product " << productId << " not found");
			chronicler->registerCashlessIdNotFoundEvent(productId, context);
			return false;
		}

		ConfigPrice *productPrice = product->getPrice(deviceId, priceListNumber);
		if(productPrice == NULL) {
			LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
			chronicler->registerPriceListNotFoundEvent("DA", 1, context);
			return false;
		}

		price = eventPrice;
		if(productPrice->getPrice() != price) {
			LOG_ERROR(LOG_SM, "Product " << productId << " wrong price (exp=" << productPrice->getPrice() << ", act=" << price << ")");
			chronicler->registerPriceNotEqualEvent(product->getId(), productPrice->getPrice(), price, context);
		}
	} else {
		uint16_t productId = eventPrice;
		if(product->findByCashlessId(productId) == false) {
			LOG_ERROR(LOG_SM, "Product " << productId << " not found");
			chronicler->registerCashlessIdNotFoundEvent(productId, context);
			return false;
		}

		ConfigPrice *productPrice = product->getPrice(deviceId, priceListNumber);
		if(productPrice == NULL) {
			LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
			chronicler->registerPriceListNotFoundEvent("DA", 1, context);
			return false;
		}

		price = productPrice->getPrice();
	}

	return true;
}

void SaleManagerMdbMasterCore::approveProduct() {
	if(config->getAutomat()->getPriceHolding() <= 0) {
		slaveCashless1->approveVend(price);
	} else {
		slaveCashless1->approveVend(product->getCashlessId());
	}
}

void SaleManagerMdbMasterCore::setMaxBill() {
	uint32_t maxChange = config->getAutomat()->getCCContext()->getInTubeValue();
	config->getAutomat()->getBVContext()->setMaxChange(maxChange);
}
