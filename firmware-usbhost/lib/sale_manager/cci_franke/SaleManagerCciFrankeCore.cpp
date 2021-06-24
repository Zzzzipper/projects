#include "SaleManagerCciFrankeCore.h"

#include "lib/sale_manager/include/SaleManager.h"
#include "lib/utils/stm32/HardwareUartForwardController.h"

#include "common/logger/include/Logger.h"

SaleManagerCciFrankeCore::SaleManagerCciFrankeCore(
	ConfigModem *config,
	ClientContext *client,
	TimerEngine *timers,
	EventEngineInterface *eventEngine,
	MdbSlaveCashlessInterface *slaveCashless,
	MdbMasterCashlessStack *masterCashlessStack,
	Fiscal::Register *fiscalRegister
) :
	config(config),
	client(client),
	timers(timers),
	eventEngine(eventEngine),
	slaveCashless(slaveCashless),
	masterCashlessStack(masterCashlessStack),
	fiscalRegister(fiscalRegister),
	deviceId(eventEngine)
{
	this->context = config->getAutomat()->getSMContext();
	this->context->setStatus(Mdb::DeviceContext::Status_Init);
	this->context->setState(State_Idle);
	this->product = config->getAutomat()->createIterator();
	this->timer = timers->addTimer<SaleManagerCciFrankeCore, &SaleManagerCciFrankeCore::procTimer>(this);

	eventEngine->subscribe(this, GlobalId_SlaveCashless1);
	eventEngine->subscribe(this, GlobalId_MasterCashless);
	eventEngine->subscribe(this, GlobalId_FiscalRegister);
}

SaleManagerCciFrankeCore::~SaleManagerCciFrankeCore() {
	timers->deleteTimer(timer);
	delete this->product;
}

void SaleManagerCciFrankeCore::reset() {
	LOG_DEBUG(LOG_SM, "reset");
	slaveCashless->reset();
	masterCashlessStack->reset();
	if(fiscalRegister != NULL) { fiscalRegister->reset(); }
	context->setStatus(Mdb::DeviceContext::Status_Work);
	gotoStateSale();
	EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, true);
	eventEngine->transmit(&event);
}

void SaleManagerCciFrankeCore::shutdown() {
	LOG_DEBUG(LOG_SM, "shutdown");
	context->setState(State_Idle);
	EventInterface event(deviceId, SaleManager::Event_Shutdown);
	eventEngine->transmit(&event);
}

void SaleManagerCciFrankeCore::procTimer() {
	switch(context->getState()) {
	default:;
	}
}

void SaleManagerCciFrankeCore::proc(EventEnvelope *envelope) {
	switch(context->getState()) {
	case State_Sale: stateSaleEvent(envelope); break;
	case State_FreeVending: stateFreeVendingEvent(envelope); break;
	case State_CashlessApproving: stateCashlessApprovingEvent(envelope); break;
	case State_CashlessVending: stateCashlessVendingEvent(envelope); break;
	case State_CashlessClosing: stateCashlessClosingEvent(envelope); break;
	default: LOG_DEBUG(LOG_SM, "Unwaited event " << context->getState());
	}
}

void SaleManagerCciFrankeCore::gotoStateSale() {
	LOG_DEBUG(LOG_SM, "gotoStateSale");
	slaveCashless-> setCredit(0);
	masterCashlessStack->enable();
	context->setState(State_Sale);
}

void SaleManagerCciFrankeCore::stateSaleEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateWaitEvent");
	switch(envelope->getType()) {
		case MdbSlaveCashlessInterface::Event_VendRequest: stateSaleEventVendRequest(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << context->getState() << ", event=" << envelope->getType());
	}
}

void SaleManagerCciFrankeCore::stateSaleEventVendRequest(EventEnvelope *envelope) {
	MdbSlaveCashlessInterface::EventVendRequest event(MdbSlaveCashlessInterface::Event_VendRequest);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		context->incProtocolErrorCount();
		return;
	}

	uint16_t productId = event.getProductId();
	LOG_INFO(LOG_SM, "stateCashlessCreditEventVendRequest " << productId << "," << event.getPrice());
	if(product->findByCashlessId(productId) == false) {
		LOG_ERROR(LOG_SM, "Product " << productId << " not found");
		registerCashlessIdNotFoundEvent(productId);
		slaveCashless->denyVend(true);
		gotoStateSale();
		return;
	}

	ConfigPrice *productPrice = product->getPrice("DA", 1);
	if(productPrice == NULL) {
		LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
		registerPriceListNotFoundEvent("DA", 1);
		slaveCashless->denyVend(true);
		gotoStateSale();
		return;
	}

	price = productPrice->getPrice();
	if(price == 0) {
		slaveCashless->approveVend(price);
		gotoStateFreeVending();
		return;
	}

	if(masterCashlessStack->sale(productId, price, product->getName(), product->getWareId()) == false) {
		LOG_ERROR(LOG_SM, "Sale start failed");
		slaveCashless->denyVend(true);
		gotoStateSale();
		return;
	}

	masterCashlessStack->enable();
	context->setState(State_CashlessApproving);
}

void SaleManagerCciFrankeCore::gotoStateFreeVending() {
	LOG_DEBUG(LOG_SM, "gotoStateFreeVending");
	context->setState(State_FreeVending);
}

void SaleManagerCciFrankeCore::stateFreeVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateFreeVendingEvent");
	switch(envelope->getType()) {
		case MdbSlaveCashlessInterface::Event_VendComplete: stateFreeVendingEventVendComplete(); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateFreeVendingEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerCciFrankeCore::stateFreeVendingEventVendComplete() {
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

void SaleManagerCciFrankeCore::stateFreeVendingEventVendCancel() {
	LOG_INFO(LOG_SM, "stateFreeVendingEventVendCancel");
	gotoStateSale();
}

void SaleManagerCciFrankeCore::stateCashlessApprovingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessApproving");
	switch(envelope->getType()) {
		case MdbMasterCashlessInterface::Event_VendApproved: stateCashlessApprovingEventVendApproved(envelope); return;
		case MdbMasterCashlessInterface::Event_VendDenied: stateCashlessApprovingEventVendDenied(); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateCashlessApprovingEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << context->getState() << ", event=" << envelope->getType());
	}
}

void SaleManagerCciFrankeCore::stateCashlessApprovingEventVendApproved(EventEnvelope *envelope) {
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

	slaveCashless->approveVend(price);
//+++ todo: ��� �������. ������ ���� ��������
	price = event.getValue1();
//+++
	masterCashlessStack->disable();
	context->setState(State_CashlessVending);
}

void SaleManagerCciFrankeCore::stateCashlessApprovingEventVendDenied() {
	LOG_INFO(LOG_SM, "stateCashlessApprovingEventVendDenied");
	slaveCashless->denyVend(true);
	masterCashlessStack->closeSession();
	masterCashlessStack->disable();
	gotoStateSale();
}

void SaleManagerCciFrankeCore::stateCashlessApprovingEventVendCancel() {
	LOG_INFO(LOG_SM, "stateCashlessApprovingEventVendCancel");
	masterCashlessStack->closeSession();
	masterCashlessStack->disable();
	gotoStateSale();
}

void SaleManagerCciFrankeCore::stateCashlessVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessVendingEvent");
	switch(envelope->getType()) {
//		case MdbMasterCashless::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendComplete: stateCashlessVendingEventVendComplete(); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateCashlessVendingEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerCciFrankeCore::stateCashlessVendingEventVendComplete() {
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

void SaleManagerCciFrankeCore::stateCashlessVendingEventVendCancel() {
	LOG_INFO(LOG_SM, "stateCashlessVendingEventVendCancel " << product->getName());
//	registerVendFailedEvent();
	if(masterCashless->saleFailed() == false) {
		masterCashless->reset();
		gotoStateSale();
		return;
	}

	gotoStateSale();
}

void SaleManagerCciFrankeCore::gotoStateCashlessClosing() {
	LOG_DEBUG(LOG_SM, "gotoStateCashlessClosing");
	if(masterCashless->saleComplete() == false) {
		masterCashless->reset();
		gotoStateSale();
		return;
	}
	context->setState(State_CashlessClosing);
}

void SaleManagerCciFrankeCore::stateCashlessClosingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessClosingEvent");
	switch(envelope->getType()) {
//		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateCashlessClosingEventSessionEnd(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerCciFrankeCore::stateCashlessClosingEventSessionEnd(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessClosingEventSessionEnd");
	if(masterCashless->getDeviceId().getValue() == envelope->getDevice()) {
		gotoStateSale();
		return;
	}
}

void SaleManagerCciFrankeCore::registerCashlessIdNotFoundEvent(uint16_t cashlessId) {
	StringBuilder data;
	data << cashlessId;
	config->getEvents()->add(ConfigEvent::Type_CashlessIdNotFound, data.getString());
	context->getErrors()->add(ConfigEvent::Type_CashlessIdNotFound, data.getString());
}

void SaleManagerCciFrankeCore::registerPriceListNotFoundEvent(const char *deviceId, uint8_t priceListNumber) {
	StringBuilder data;
	data << deviceId << priceListNumber;
	config->getEvents()->add(ConfigEvent::Type_PriceListNotFound, data.getString());
	context->getErrors()->add(ConfigEvent::Type_PriceListNotFound, data.getString());
}
