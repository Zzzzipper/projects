#include "SaleManagerMdbSlaveCore.h"
#include "EvadtsEventProcessor.h"

#include "common/logger/include/Logger.h"

#include "lib/sale_manager/include/SaleManager.h"
#include "lib/utils/stm32/HardwareUartForwardController.h"

//#define DISABLE_BRIDGE

SaleManagerMdbSlaveCore::SaleManagerMdbSlaveCore(SaleManagerMdbSlaveParams *params) :
	config(params->config),
	timers(params->timers),
	eventEngine(params->eventEngine),
	leds(params->leds),
	chronicler(params->chronicler),
	slaveCoinChanger(params->slaveCoinChanger),
	slaveBillValidator(params->slaveBillValidator),
	slaveCashless1(params->slaveCashless1),
	slaveCashless2(params->slaveCashless2),
	slaveGateway(params->slaveGateway),
	masterCashlessStack(params->externCashless),
	fiscalRegister(params->fiscalRegister),
	rebootReason(params->rebootReason),
	deviceId(params->eventEngine),
	enabled(false)
{
	this->context = config->getAutomat()->getSMContext();
	this->context->setStatus(Mdb::DeviceContext::Status_NotFound);
	this->context->setState(State_Idle);

	this->timer = timers->addTimer<SaleManagerMdbSlaveCore, &SaleManagerMdbSlaveCore::procTimer>(this);

	eventEngine->subscribe(this, GlobalId_SnifferCoinChanger);
	eventEngine->subscribe(this, GlobalId_SnifferBillValidator);
	eventEngine->subscribe(this, GlobalId_SnifferCashless);
	eventEngine->subscribe(this, GlobalId_SlaveCashless1);
	eventEngine->subscribe(this, GlobalId_SlaveComGateway);
	eventEngine->subscribe(this, GlobalId_MasterCashless);

	product = config->getAutomat()->createIterator();
}

SaleManagerMdbSlaveCore::~SaleManagerMdbSlaveCore() {
	delete product;
}

void SaleManagerMdbSlaveCore::reset() {
	LOG_INFO(LOG_SM, "reset");
	slaveCoinChanger->reset();
	slaveBillValidator->reset();
	slaveCashless1->reset();
	slaveCashless2->reset();
	if(slaveGateway != NULL) { slaveGateway->reset(); }
	if(masterCashlessStack != NULL) { masterCashlessStack->reset(); }
	if(fiscalRegister != NULL) { fiscalRegister->reset(); }
	leds->setPayment(LedInterface::State_InProgress);
	context->setStatus(Mdb::DeviceContext::Status_Work);
	if(rebootReason == Reboot::Reason_PowerDown) {
#ifndef DISABLE_BRIDGE
		LOG_INFO(LOG_SM, "Start");
		HardwareUartForwardController::start();
#endif
		context->setState(State_Sale);
		return;
	} else {
		gotoStateDelay();
		return;
	}
}

void SaleManagerMdbSlaveCore::service() {
	LOG_INFO(LOG_SM, "service");
}

void SaleManagerMdbSlaveCore::shutdown() {
	LOG_INFO(LOG_SM, "shutdown");
#ifndef DISABLE_BRIDGE
	HardwareUartForwardController::stop();
#endif
	context->setState(State_Idle);
	EventInterface event(SaleManager::Event_Shutdown);
	eventEngine->transmit(&event);
}

void SaleManagerMdbSlaveCore::procTimer() {
	LOG_DEBUG(LOG_SM, "procTimer " << context->getState());
	switch(context->getState()) {
	case State_Delay: stateDelayTimeout(); break;
	default: LOG_ERROR(LOG_SM, "Unwaited timeout " << context->getState());
	}
}

void SaleManagerMdbSlaveCore::proc(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "proc " << context->getState() << "," << envelope->getType());
	switch(envelope->getType()) {
		case MdbSlaveCashlessInterface::Event_CashSale: procEventCashSale(envelope); return;
		case MdbSlaveComGateway::Event_ReportTranscation: procReportTransaction(envelope); return;
		case MdbSlaveComGateway::Event_ReportEvent: procReportEvent(envelope); return;
		case MdbSnifferBillValidator::Event_DepositeBill: procDepositeBill(envelope); return;
		case MdbSnifferCoinChanger::Event_DepositeCoin: procDepositeCoin(envelope); return;
	}

	switch(context->getState()) {
		case State_Idle: LOG_DEBUG(LOG_SM, "Ignore event " << context->getState() << "," << envelope->getType()); return;
		case State_Sale: stateSaleEvent(envelope); return;
		case State_CashlessStackApproving: stateCashlessStackApprovingEvent(envelope); return;
		case State_ExternCredit: stateExternCreditEvent(envelope); return;
		case State_ExternApproving: stateExternApprovingEvent(envelope); return;
		case State_ExternVending: stateExternVendingEvent(envelope); return;
		case State_ExternClosing: stateExternClosingEvent(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbSlaveCore::procEventCashSale(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateSaleEventCashSale");
	if(slaveGateway != NULL && slaveGateway->isInited() == true) {
		return;
	}

	MdbSlaveCashlessInterface::EventVendRequest vendRequest(MdbSlaveCashlessInterface::Event_CashSale);
	if(vendRequest.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event open failed");
		return;
	}

	uint16_t productId = vendRequest.getProductId();
	uint32_t productPrice = vendRequest.getPrice();
	LOG_INFO(LOG_SM, "stateSaleEventCashSale " << productId << "," << productPrice);
	if(product->findByCashlessId(productId) == false) {
		LOG_ERROR(LOG_SM, "Product " << productId << " not found");
		chronicler->registerCashlessIdNotFoundEvent(productId, context);
		return;
	}

	ConfigPrice *price = product->getPrice("CA", 0);
	if(price == NULL) {
		LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
		chronicler->registerPriceListNotFoundEvent("CA", 0, context);
		return;
	}

	if(price->getPrice() != productPrice) {
		LOG_ERROR(LOG_SM, "Product " << productId << " wrong price (exp=" << price->getPrice() << ", act=" << productPrice << ")");
		chronicler->registerPriceNotEqualEvent(product->getId(), price->getPrice(), productPrice, context);
	}

	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), productPrice, product->getTaxRate(), 1);
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = productPrice;
	saleData.taxSystem = config->getAutomat()->getTaxSystem();

	config->getAutomat()->sale(&saleData);
	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>TICKET " << saleData.credit << "," << saleData.getPrice() << ":" << config->getAutomat()->getDecimalPoint());
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
}

void SaleManagerMdbSlaveCore::procReportTransaction(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "procReportTransaction");
	MdbSlaveComGateway::ReportTransaction event;
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event open failed");
		return;
	}

	switch(event.data.transactionType) {
	case Mdb::ComGateway::TransactionType_PaidVend: procReportTransactionPaidVend(&event); return;
	case Mdb::ComGateway::TransactionType_TokenVend: procReportTransactionTokenVend(&event); return;
	default: return;
	}
}

void SaleManagerMdbSlaveCore::procReportTransactionPaidVend(MdbSlaveComGateway::ReportTransaction *event) {
	uint16_t productId = event->data.itemNumber;
	uint32_t price = event->data.price;
	LOG_INFO(LOG_SM, "procReportTransactionPaidVend " << productId << "," << price);
	if(product->findByCashlessId(productId) == false) {
		LOG_ERROR(LOG_SM, "Product " << productId << " not found");
		chronicler->registerCashlessIdNotFoundEvent(productId, context);
		return;
	}

	Fiscal::PaymentType paymentType = Fiscal::Payment_Cash;
	const char *paymentDevice = "CA";
	uint16_t priceListNumber = 0;
	uint32_t credit = event->data.cashInBills + event->data.cashInCashbox + event->data.cashInCoinTubes;
	if(event->data.valueInCashless1 > 0 || event->data.valueInCashless2 > 0) {
		paymentType = Fiscal::Payment_Cashless;
		paymentDevice = "DA";
		priceListNumber = 1;
		credit = price;
	}

	ConfigPrice *productPrice = product->getPrice(paymentDevice, priceListNumber);
	if(productPrice == NULL) {
		LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
		chronicler->registerPriceListNotFoundEvent("CA", 0, context);
		return;
	}

	if(productPrice->getPrice() != price) {
		LOG_ERROR(LOG_SM, "Product " << productId << " wrong price (exp=" << productPrice->getPrice() << ", act=" << price << ")");
		chronicler->registerPriceNotEqualEvent(product->getId(), productPrice->getPrice(), price, context);
	}

	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), price, product->getTaxRate(), 1);
	saleData.device.set(paymentDevice);
	saleData.priceList = priceListNumber;
	saleData.paymentType = paymentType;
	saleData.credit = credit;
	saleData.taxSystem = config->getAutomat()->getTaxSystem();

	config->getAutomat()->sale(&saleData);
	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>TICKET " << saleData.getProductNum() << "," << saleData.credit << "," << saleData.getPrice() << ":" << config->getAutomat()->getDecimalPoint());
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
}

void SaleManagerMdbSlaveCore::procReportTransactionTokenVend(MdbSlaveComGateway::ReportTransaction *event) {
	uint16_t productId = event->data.itemNumber;
	uint32_t price = event->data.price;
	LOG_INFO(LOG_SM, "procReportTransactionTokenVend " << productId << "," << price);
	if(product->findByCashlessId(productId) == false) {
		LOG_ERROR(LOG_SM, "Product " << productId << " not found");
		chronicler->registerCashlessIdNotFoundEvent(productId, context);
		return;
	}

	Fiscal::PaymentType paymentType = Fiscal::Payment_Token;
	const char *paymentDevice = "TA";
	uint16_t priceListNumber = 0;

	ConfigPrice *productPrice = product->getPrice("CA", 0);
	if(productPrice == NULL) {
		LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
		chronicler->registerPriceListNotFoundEvent("CA", 0, context);
		return;
	}

	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), productPrice->getPrice(), product->getTaxRate(), 1);
	saleData.device.set(paymentDevice);
	saleData.priceList = priceListNumber;
	saleData.paymentType = paymentType;
	saleData.credit = productPrice->getPrice();
	saleData.taxSystem = config->getAutomat()->getTaxSystem();

	config->getAutomat()->sale(&saleData);
	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>TICKET " << saleData.getProductNum() << "," << saleData.credit << "," << saleData.getPrice() << ":" << config->getAutomat()->getDecimalPoint());
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
}

void SaleManagerMdbSlaveCore::procReportEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "procReportEvent");
	MdbSlaveComGateway::ReportEvent event;
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event open failed");
		return;
	}

	LOG_INFO(LOG_SM, "ReportEvent " << event.data.code.get() << "=" << event.data.activity);
	EvadtsEventProcessor::proc(event.data.code.get(), event.data.activity, event.data.duration, &event.data.datetime, config);
}

void SaleManagerMdbSlaveCore::procDepositeBill(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "procDepositeBill");
	Mdb::EventDeposite event(MdbSnifferBillValidator::Event_DepositeBill);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event open failed");
		return;
	}

	LOG_INFO(LOG_SM, "DepositeBill " << event.getRoute() << "," << event.getNominal());
	chronicler->registerBillInEvent(event.getNominal(), event.getRoute());
}

void SaleManagerMdbSlaveCore::procDepositeCoin(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "procDepositeCoin");
	Mdb::EventDeposite event(MdbSnifferCoinChanger::Event_DepositeCoin);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event open failed");
		return;
	}

	LOG_INFO(LOG_SM, "DepositeCoin " << event.getRoute() << "," << event.getNominal());
	chronicler->registerCoinInEvent(event.getNominal(), event.getRoute());
}

void SaleManagerMdbSlaveCore::procDeviceStateChange() {
	LOG_DEBUG(LOG_SM, "procDeviceStateChange");
	if(slaveBillValidator->isEnable() == true || slaveCoinChanger->isEnable() == true || slaveCashless1->isEnable() == true || slaveCashless2->isEnable() == true) {
		if(enabled == false) {
			enabled = true;
			leds->setPayment(LedInterface::State_Success);
			EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, true);
			eventEngine->transmit(&event);
		}
	} else {
		if(enabled == true) {
			enabled = false;
			leds->setPayment(LedInterface::State_Success);
			EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, false);
			eventEngine->transmit(&event);
		}
	}
}

void SaleManagerMdbSlaveCore::gotoStateDelay() {
	LOG_DEBUG(LOG_SM, "gotoStateDelay");
#ifndef DISABLE_BRIDGE
	HardwareUartForwardController::stop();
#endif
	timer->start(15000);
	context->setState(State_Delay);
}

void SaleManagerMdbSlaveCore::stateDelayTimeout() {
	LOG_DEBUG(LOG_SM, "stateDelayTimeout");
#ifndef DISABLE_BRIDGE
	LOG_INFO(LOG_SM, "Start");
	HardwareUartForwardController::start();
#endif
	gotoStateSale();
}

void SaleManagerMdbSlaveCore::gotoStateSale() {
	LOG_DEBUG(LOG_SM, "gotoStateSale");
	if(slaveCashless2->isEnable() == true) {
		stateSaleEventCashlessEnable();
	} else {
		stateSaleEventCashlessDisable();
	}
	context->setState(State_Sale);
}

void SaleManagerMdbSlaveCore::stateSaleEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateSaleEvent");
	switch(envelope->getType()) {
		case MdbSnifferBillValidator::Event_Enable: procDeviceStateChange(); return;
		case MdbSnifferBillValidator::Event_Disable: procDeviceStateChange(); return;
		case MdbSnifferCoinChanger::Event_Enable: procDeviceStateChange(); return;
		case MdbSnifferCoinChanger::Event_Disable: procDeviceStateChange(); return;
		case MdbSnifferCashless::Event_Enable: procDeviceStateChange(); return;
		case MdbSnifferCashless::Event_Disable: procDeviceStateChange(); return;
		case MdbSnifferCashless::Event_VendComplete: stateSaleEventCashlessSale(envelope); return;
		case MdbSlaveCashlessInterface::Event_Enable: stateSaleEventCashlessEnable(); return;
		case MdbSlaveCashlessInterface::Event_Disable: stateSaleEventCashlessDisable(); return;
		case MdbSlaveCashlessInterface::Event_VendRequest: stateSaleEventVendRequest(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionBegin: stateSaleEventSessionBegin(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbSlaveCore::stateSaleEventCashlessEnable() {
	LOG_DEBUG(LOG_SM, "stateSaleEventCashlessEnable");
	if(masterCashlessStack != NULL) { masterCashlessStack->enable(); }
	procDeviceStateChange();
}

void SaleManagerMdbSlaveCore::stateSaleEventCashlessDisable() {
	LOG_DEBUG(LOG_SM, "stateSaleEventCashlessDisable");
	if(masterCashlessStack != NULL) { masterCashlessStack->disable(); }
	procDeviceStateChange();
}

void SaleManagerMdbSlaveCore::stateSaleEventVendRequest(EventEnvelope *envelope) {
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
		chronicler->registerCashlessIdNotFoundEvent(productId, context);
		slaveCashless2->denyVend(true);
		return;
	}

	ConfigPrice *productPrice = product->getPrice("DA", 1);
	if(productPrice == NULL) {
		LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
		chronicler->registerPriceListNotFoundEvent("DA", 1, context);
		slaveCashless2->denyVend(true);
		return;
	}

	price = event.getPrice();
	if(productPrice->getPrice() != price) {
		LOG_ERROR(LOG_SM, "Product " << productId << " wrong price (exp=" << productPrice->getPrice() << ", act=" << price << ")");
		chronicler->registerPriceNotEqualEvent(product->getId(), productPrice->getPrice(), price, context);
	}

	if(masterCashlessStack->sale(productId, price, product->getName(), product->getWareId()) == false) {
		LOG_ERROR(LOG_SM, "Always idle not supported");
		slaveCashless2->denyVend(true);
		return;
	}

	cashlessCredit = price;
	gotoStateCashlessStackApproving();
}

void SaleManagerMdbSlaveCore::stateSaleEventCashlessSale(EventEnvelope *envelope) {
	if(slaveGateway != NULL && slaveGateway->isInited() == true) {
		return;
	}

	MdbSnifferCashless::EventVend vend;
	if(vend.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event open failed");
		return;
	}

	uint16_t productId = vend.getProductId();
	uint32_t productPrice = vend.getPrice();
	LOG_INFO(LOG_SM, "stateSaleEventCashlessSale " << productId << "," << productPrice);
	if(product->findByCashlessId(productId) == false) {
		LOG_ERROR(LOG_SM, "Product " << productId << " not found");
		chronicler->registerCashlessIdNotFoundEvent(productId, context);
		return;
	}

	ConfigPrice *price = product->getPrice("DA", 1);
	if(price == NULL) {
		LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
		chronicler->registerPriceListNotFoundEvent("DA", 1, context);
		return;
	}

	if(price->getPrice() != productPrice) {
		LOG_ERROR(LOG_SM, "Product " << productId << " wrong price (exp=" << price->getPrice() << ", act=" << productPrice << ")");
		chronicler->registerPriceNotEqualEvent(product->getId(), price->getPrice(), productPrice, context);
	}

	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), productPrice, product->getTaxRate(), 1);
	saleData.device.set("DA");
	saleData.priceList = 1;
	saleData.paymentType = Fiscal::Payment_Cashless;
	saleData.credit = productPrice;
	saleData.taxSystem = config->getAutomat()->getTaxSystem();

	config->getAutomat()->sale(&saleData);
	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>TICKET " << saleData.getProductNum() << "," << saleData.credit << "," << saleData.getPrice() << ":" << config->getAutomat()->getDecimalPoint());
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
}

void SaleManagerMdbSlaveCore::stateSaleEventSessionBegin(EventEnvelope *envelope) {
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
	slaveCashless2->setCredit(cashlessCredit);
	context->setState(State_ExternCredit);
}

void SaleManagerMdbSlaveCore::gotoStateCashlessStackApproving() {
	LOG_DEBUG(LOG_SM, "gotoStateCashlessStackApproving");
	chronicler->startSession();
	context->setState(State_CashlessStackApproving);
}

void SaleManagerMdbSlaveCore::stateCashlessStackApprovingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessStackApprovingEvent");
	switch(envelope->getType()) {
		case MdbMasterCashlessInterface::Event_VendApproved: stateCashlessStackApprovingEventVendApproved(envelope); return;
		case MdbMasterCashlessInterface::Event_VendDenied: stateCashlessStackApprovingEventVendDenied(); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateCashlessStackApprovingEventVendDenied(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbSlaveCore::stateCashlessStackApprovingEventVendApproved(EventEnvelope *envelope) {
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
	approvePrice = event.getValue1();
	slaveCashless2->approveVend(price);
	masterCashlessStack->disable();
	context->setState(State_ExternVending);
}

void SaleManagerMdbSlaveCore::stateCashlessStackApprovingEventVendDenied() {
	LOG_DEBUG(LOG_SM, "stateCashlessStackApprovingEventVendDenied");
	chronicler->registerCashlessDeniedEvent(product->getId());
	stateCashlessStackApprovingFailed();
}

void SaleManagerMdbSlaveCore::stateCashlessStackApprovingFailed() {
	slaveCashless2->denyVend(true);
	masterCashlessStack->closeSession();
	gotoStateSale();
}

void SaleManagerMdbSlaveCore::stateExternCreditEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateExternCreditEvent");
	switch(envelope->getType()) {
		case MdbMasterCashlessInterface::Event_SessionBegin: unwaitedEventSessionBegin(envelope); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateExternCreditEventSessionEnd(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendRequest: stateExternCreditEventVendRequest(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateExternCreditEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbSlaveCore::stateExternCreditEventSessionEnd(EventEnvelope *envelope) {
	LOG_INFO(LOG_SM, "stateExternCreditEventSessionEnd");
	if(masterCashless->getDeviceId().getValue() == envelope->getDevice()) {
		slaveCashless2->cancelVend();
		gotoStateSale();
		return;
	}
}

void SaleManagerMdbSlaveCore::stateExternCreditEventVendRequest(EventEnvelope *envelope) {
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
		chronicler->registerCashlessIdNotFoundEvent(productId, context);
		slaveCashless2->denyVend(true);
		masterCashless->closeSession();
		gotoStateSale();
		return;
	}

	ConfigPrice *productPrice = product->getPrice("DA", 1);
	if(productPrice == NULL) {
		LOG_ERROR(LOG_SM, "Product " << productId << " price undefined");
		chronicler->registerPriceListNotFoundEvent("DA", 1, context);
		slaveCashless2->denyVend(true);
		masterCashless->closeSession();
		gotoStateSale();
		return;
	}

	price = event.getPrice();
	if(productPrice->getPrice() != price) {
		LOG_ERROR(LOG_SM, "Product " << productId << " wrong price (exp=" << productPrice->getPrice() << ", act=" << price << ")");
		chronicler->registerPriceNotEqualEvent(product->getId(), productPrice->getPrice(), price, context);
	}

	if(price > cashlessCredit) {
		LOG_ERROR(LOG_SM, "Not enough credit (exp= " << price << ", act=" << cashlessCredit << ")");
		slaveCashless2->denyVend(true);
		masterCashless->closeSession();
		gotoStateSale();
		return;
	}

	if(masterCashless->sale(productId, price, product->getName(), product->getWareId()) == false) {
		slaveCashless2->denyVend(true);
		masterCashless->closeSession();
		gotoStateSale();
		return;
	}
	context->setState(State_ExternApproving);
}

void SaleManagerMdbSlaveCore::stateExternCreditEventVendCancel() {
	LOG_INFO(LOG_SM, "stateExternCreditEventVendCancel");
	masterCashless->closeSession();
	gotoStateSale();
}

void SaleManagerMdbSlaveCore::stateExternApprovingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateExternApprovingEvent");
	switch(envelope->getType()) {
		case MdbMasterCashlessInterface::Event_VendApproved: stateExternApprovingEventVendApproved(envelope); return;
		case MdbMasterCashlessInterface::Event_VendDenied: stateExternApprovingEventVendDenied(); return;
		case MdbMasterCashlessInterface::Event_SessionEnd: stateExternApprovingEventSessionEnd(envelope); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateExternApprovingEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbSlaveCore::stateExternApprovingEventVendApproved(EventEnvelope *envelope) {
	LOG_INFO(LOG_SM, "stateExternApprovingEventVendApproved");
	MdbMasterCashlessInterface::EventApproved event;
	if(event.open(envelope) == false) {
		stateExternApprovingFailed();
		return;
	}

	saleData.paymentType = event.getType1();
	saleData.credit = event.getValue1();
	approvePrice = event.getValue1();
	slaveCashless2->approveVend(price);
	context->setState(State_ExternVending);
}

void SaleManagerMdbSlaveCore::stateExternApprovingEventVendDenied() {
	LOG_INFO(LOG_SM, "stateExternApprovingEventVendDenied");
	stateExternApprovingFailed();
}

void SaleManagerMdbSlaveCore::stateExternApprovingFailed() {
	slaveCashless2->denyVend(true);
	masterCashless->closeSession();
	context->setState(State_ExternClosing);
}

void SaleManagerMdbSlaveCore::stateExternApprovingEventVendCancel() {
	LOG_INFO(LOG_SM, "stateExternApprovingEventVendCancel");
	masterCashless->closeSession();
	gotoStateSale();
}

void SaleManagerMdbSlaveCore::stateExternApprovingEventSessionEnd(EventEnvelope *envelope) {
	LOG_INFO(LOG_SM, "stateExternApprovingEventSessionEnd");
	if(masterCashless->getDeviceId().getValue() == envelope->getDevice()) {
		slaveCashless2->denyVend(true);
		gotoStateSale();
		return;
	}
}

void SaleManagerMdbSlaveCore::stateExternVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateExternVendingEvent");
	switch(envelope->getType()) {
		case MdbSlaveCashlessInterface::Event_VendComplete: stateExternVendingEventVendComplete(); return;
		case MdbSlaveCashlessInterface::Event_VendCancel: stateExternVendingEventVendCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbSlaveCore::stateExternVendingEventVendComplete() {
	LOG_INFO(LOG_SM, "stateExternVendingEventVendComplete");
	if(slaveGateway != NULL && slaveGateway->isInited() == true) {
		cashlessCredit = 0;
		gotoStateExternClosing();
		return;
	}

	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), approvePrice, product->getTaxRate(), 1);
	saleData.device.set("DA");
	saleData.priceList = 1;
	saleData.taxSystem = config->getAutomat()->getTaxSystem();

	cashlessCredit = 0;
	config->getAutomat()->sale(&saleData);
	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>TICKET " << saleData.getProductNum() << "," << saleData.credit << "," << saleData.getPrice() << ":" << config->getAutomat()->getDecimalPoint());
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
	gotoStateExternClosing();
}

void SaleManagerMdbSlaveCore::stateExternVendingEventVendCancel() {
	LOG_INFO(LOG_SM, "stateExternVendingEventVendCancel " << product->getName());
	cashlessCredit = 0;
	if(masterCashless->saleFailed() == false) {
		masterCashless->reset();
		gotoStateSale();
		return;
	}
	context->setState(State_ExternClosing);
}

void SaleManagerMdbSlaveCore::gotoStateExternClosing() {
	LOG_DEBUG(LOG_SM, "gotoStateExternClosing");
	if(masterCashless->saleComplete() == false) {
		masterCashless->reset();
		gotoStateSale();
		return;
	}
	context->setState(State_ExternClosing);
}

void SaleManagerMdbSlaveCore::stateExternClosingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateExternClosingEvent");
	switch(envelope->getType()) {
		case MdbMasterCashlessInterface::Event_SessionEnd: stateExternClosingEventSessionEnd(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbSlaveCore::stateExternClosingEventSessionEnd(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashlessClosingEventSessionEnd");
	if(masterCashless->getDeviceId().getValue() == envelope->getDevice()) {
		gotoStateSale();
		return;
	}
}

void SaleManagerMdbSlaveCore::unwaitedEventSessionBegin(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "unwaitedEventSessionBegin");
	MdbMasterCashlessInterface *cashless = masterCashlessStack->find(envelope->getDevice());
	if(cashless == NULL) {
		LOG_ERROR(LOG_SM, "Unknown cashless " << envelope->getDevice());
		context->incProtocolErrorCount();
		return;
	}
	cashless->closeSession();
}

