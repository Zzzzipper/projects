
#if 0
#include <lib/sale_manager/mdb_no_cashless/SaleManagerMdbNoCashlessCore.h>

#include "common/logger/include/Logger.h"

#include "lib/sale_manager/include/SaleManager.h"
#include "lib/utils/stm32/HardwareUartForwardController.h"

//#define DISABLE_BRIDGE
#define SMMD_VENDING_TIMEOUT 120000
#define SMMD_CHANGE_TIMEOUT 1000

SaleManagerMdbNoCashlessCore::SaleManagerMdbNoCashlessCore(SaleManagerMdbNoCashlessParams *params) :
	config(params->config),
	timers(params->timers),
	eventEngine(params->eventEngine),
	leds(params->leds),
	chronicler(params->chronicler),
	slaveCoinChanger(params->slaveCoinChanger),
	slaveBillValidator(params->slaveBillValidator),
	masterCoinChanger(params->masterCoinChanger),
	masterBillValidator(params->masterBillValidator),
	fiscalRegister(params->fiscalRegister),
	rebootReason(params->rebootReason),
	deviceId(params->eventEngine),
	enabled(false)
{
	this->context = config->getAutomat()->getSMContext();
	this->context->setStatus(Mdb::DeviceContext::Status_NotFound);
	this->context->setState(State_Idle);

	this->timer = timers->addTimer<SaleManagerMdbNoCashlessCore, &SaleManagerMdbNoCashlessCore::procTimer>(this);

	eventEngine->subscribe(this, GlobalId_SnifferCoinChanger);
	eventEngine->subscribe(this, GlobalId_SnifferBillValidator);

	product = config->getAutomat()->createIterator();
}

SaleManagerMdbNoCashlessCore::~SaleManagerMdbNoCashlessCore() {
	delete product;
}

void SaleManagerMdbNoCashlessCore::reset() {
	LOG_INFO(LOG_SM, "reset");
	slaveCoinChanger->reset();
	slaveBillValidator->reset();
	if(fiscalRegister != NULL) { fiscalRegister->reset(); }
	leds->setPayment(LedInterface::State_InProgress);
	context->setStatus(Mdb::DeviceContext::Status_Init);
	HardwareUartForwardController::stop();
	gotoStateWait();
}

void SaleManagerMdbNoCashlessCore::service() {
	LOG_INFO(LOG_SM, "service");
}

void SaleManagerMdbNoCashlessCore::shutdown() {
	LOG_INFO(LOG_SM, "shutdown");
	context->setState(State_Idle);
	EventInterface event(SaleManager::Event_Shutdown);
	eventEngine->transmit(&event);
}

void SaleManagerMdbNoCashlessCore::procTimer() {
	LOG_DEBUG(LOG_SM, "procTimer " << context->getState());
	switch(context->getState()) {
	case State_CashChange: stateCashChangeTimeout(); break;
	default: LOG_ERROR(LOG_SM, "Unwaited timeout " << context->getState());
	}
}

void SaleManagerMdbNoCashlessCore::proc(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "proc " << context->getState() << "," << envelope->getType());
	switch(context->getState()) {
		case State_Idle: LOG_DEBUG(LOG_SM, "Ignore event " << context->getState() << "," << envelope->getType()); return;
		case State_Wait: stateWaitEvent(envelope); return;
		case State_CashCredit: stateCashCreditEvent(envelope); return;
		case State_CashVending: stateCashVendingEvent(envelope); return;
		case State_CashChange: stateCashChangeEvent(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbNoCashlessCore::procDeviceStateChange() {
	LOG_DEBUG(LOG_SM, "procDeviceStateChange");
	if(slaveBillValidator->isEnable() == true || slaveCoinChanger->isEnable() == true) {
		if(enabled == false) {
			enabled = true;
			leds->setPayment(LedInterface::State_Success);
			context->setStatus(Mdb::DeviceContext::Status_Enabled);
			EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, true);
			eventEngine->transmit(&event);
		}
	} else {
		if(enabled == true) {
			enabled = false;
			leds->setPayment(LedInterface::State_Success);
			context->setStatus(Mdb::DeviceContext::Status_Disabled);
			EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, false);
			eventEngine->transmit(&event);
		}
	}
}

bool SaleManagerMdbNoCashlessCore::procDepositeCoin(EventEnvelope *envelope) {
	MdbMasterCoinChanger::EventCoin event(MdbMasterCoinChanger::Event_Deposite);
	if(event.open(envelope) == false) {
		return false;
	}

	if(event.getRoute() == Mdb::CoinChanger::Route_Cashbox || event.getRoute() == Mdb::CoinChanger::Route_Tube) {
		uint32_t incomingCredit = event.getNominal();
		chronicler->registerCoinInEvent(incomingCredit, event.getRoute());
		LOG_INFO(LOG_SM, "procDepositeCoin " << incomingCredit);
		cashCredit += incomingCredit;
		return true;
	}

	return false;
}

bool SaleManagerMdbNoCashlessCore::procDepositeBill(EventEnvelope *envelope) {
	Mdb::EventDeposite event(MdbMasterBillValidator::Event_Deposite);
	if(event.open(envelope) == false) {
		return false;
	}

	if(event.getRoute() == Mdb::BillValidator::Route_Stacked) {
		uint32_t incomingCredit = event.getNominal();
		LOG_INFO(LOG_SM, "procDepositeBill " << incomingCredit);
		chronicler->registerBillInEvent(incomingCredit, event.getRoute());
		cashCredit += incomingCredit;
		return true;
	}

	return false;
}

void SaleManagerMdbNoCashlessCore::gotoStateWait() {
	LOG_DEBUG(LOG_SM, "gotoStateSale");
	cashCredit = 0;
	context->setState(State_Wait);
}

void SaleManagerMdbNoCashlessCore::stateWaitEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateWaitEvent");
	switch(envelope->getType()) {
		case MdbSlaveCoinChanger::Event_Enable: procDeviceStateChange(); return;
		case MdbSlaveCoinChanger::Event_Disable: procDeviceStateChange(); return;
		case MdbMasterCoinChanger::Event_Deposite: stateWaitEventDepositeCoin(envelope); return;
		case MdbSlaveBillValidator::Event_Enable: procDeviceStateChange(); return;
		case MdbSlaveBillValidator::Event_Disable: procDeviceStateChange(); return;
		case MdbMasterBillValidator::Event_Deposite: stateWaitEventDepositeBill(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbNoCashlessCore::stateWaitEventDepositeCoin(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateWaitEventDepositeCoin");
	if(procDepositeCoin(envelope) == true) {
		gotoStateCashCredit();
	}
}

void SaleManagerMdbNoCashlessCore::stateWaitEventDepositeBill(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateWaitEventDepositeBill");
	if(procDepositeBill(envelope) == true) {
		gotoStateCashCredit();
	}
}

void SaleManagerMdbNoCashlessCore::gotoStateCashCredit() {
	LOG_DEBUG(LOG_SM, "gotoStateCashCredit");
	context->setState(State_CashCredit);
}

void SaleManagerMdbNoCashlessCore::stateCashCreditEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashCreditEvent");
	switch(envelope->getType()) {
		case MdbSlaveCoinChanger::Event_Enable: procDeviceStateChange(); return;
		case MdbSlaveCoinChanger::Event_Disable: stateCashCreditEventDisable(); return;
		case MdbSlaveCoinChanger::Event_DispenseCoin: stateCashCreditEventDispenseCoin(envelope); return;
		case MdbMasterCoinChanger::Event_Deposite: procDepositeCoin(envelope); return;
		case MdbSlaveBillValidator::Event_Enable: procDeviceStateChange(); return;
		case MdbSlaveBillValidator::Event_Disable: procDeviceStateChange(); return;
		case MdbMasterBillValidator::Event_Deposite: procDepositeBill(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbNoCashlessCore::stateCashCreditEventDispenseCoin(EventEnvelope *envelope) {
#if 0
	EventUint8Interface event(MdbSlaveCoinChanger::Event_DispenseCoin);
	if(event.open(envelope) == false) {
		return;
	}

	uint32_t outcomingCredit = event.getValue();
	LOG_INFO(LOG_SM, "stateCashCreditEventDispenseCoin " << outcomingCredit);
	if(cashCredit <= outcomingCredit) {
		cashCredit = 0;
		gotoStateWait();
		return;
	}
	cashCredit -= outcomingCredit;
#else
	EventUint8Interface event(MdbSlaveCoinChanger::Event_DispenseCoin);
	if(event.open(envelope) == false) {
		return;
	}
/*
	uint32_t outcomingCredit = event.getValue();
	LOG_INFO(LOG_SM, "stateCashCreditEventDispenseCoin " << outcomingCredit);
	if(cashCredit <= outcomingCredit) {
		cashCredit = 0;
		gotoStateWait();
		return;
	}
	cashCredit -= outcomingCredit;*/
#endif
}

void SaleManagerMdbNoCashlessCore::stateCashCreditEventDisable() {
	LOG_INFO(LOG_SM, "stateCashCreditEventDisable");
	timer->start(SMMD_VENDING_TIMEOUT);
	context->setState(State_CashVending);
}

void SaleManagerMdbNoCashlessCore::stateCashVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashVendingEvent");
	switch(envelope->getType()) {
		case MdbMasterCoinChanger::Event_Deposite: procDepositeCoin(envelope); return;
		case MdbSlaveCoinChanger::Event_DispenseCoin: stateCashVendingEventDispenseCoin(envelope); return;
		case MdbSlaveCoinChanger::Event_Enable: stateCashVendingEventEnable(); return;
		case MdbSlaveCoinChanger::Event_Disable: procDeviceStateChange(); return;
		case MdbMasterBillValidator::Event_Deposite: procDepositeBill(envelope); return;
		case MdbSlaveBillValidator::Event_Enable: procDeviceStateChange(); return;
		case MdbSlaveBillValidator::Event_Disable: procDeviceStateChange(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbNoCashlessCore::stateCashVendingEventDispenseCoin(EventEnvelope *envelope) {
#if 0
	EventUint8Interface event(MdbSnifferCoinChanger::Event_DispenseCoin);
	if(event.open(envelope) == false) {
		return;
	}

	uint32_t outcomingCredit = event.getValue();
	LOG_INFO(LOG_SM, "stateCashVendingEventDispenseCoin " << outcomingCredit);
	if(cashCredit < outcomingCredit) {
		cashCredit = 0;
		gotoStateWait();
		return;
	}
	cashCredit -= outcomingCredit;
#else
	EventUint8Interface event(MdbSlaveCoinChanger::Event_DispenseCoin);
	if(event.open(envelope) == false) {
		return;
	}

	uint32_t outcomingCredit = event.getValue();
	LOG_INFO(LOG_SM, "stateCashVendingEventDispenseCoin " << outcomingCredit);
	if(cashCredit < outcomingCredit) {
		cashCredit = 0;
		gotoStateWait();
		return;
	}
	cashCredit -= outcomingCredit;
#endif
}

void SaleManagerMdbNoCashlessCore::stateCashVendingEventEnable() {
	LOG_INFO(LOG_SM, "stateCashVendingEventEnable");
	timer->start(SMMD_CHANGE_TIMEOUT);
	context->setState(State_CashChange);
}

void SaleManagerMdbNoCashlessCore::stateCashChangeEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashChangeEvent");
	switch(envelope->getType()) {
		case MdbSlaveCoinChanger::Event_Enable: procDeviceStateChange(); return;
		case MdbSlaveCoinChanger::Event_Disable: procDeviceStateChange(); return;
		case MdbSlaveCoinChanger::Event_DispenseCoin: stateCashChangeEventDispenseCoin(envelope); return;
		case MdbSlaveBillValidator::Event_Enable: procDeviceStateChange(); return;
		case MdbSlaveBillValidator::Event_Disable: procDeviceStateChange(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbNoCashlessCore::stateCashChangeEventDispenseCoin(EventEnvelope *envelope) {
#if 0
	EventUint32Interface event(MdbSnifferCoinChanger::Event_DispenseCoin);
	if(event.open(envelope) == false) {
		return;
	}

	uint32_t outcomingCredit = event.getValue();
	LOG_INFO(LOG_SM, "stateCashChangeEventDispenseCoin " << outcomingCredit);
	if(cashCredit < outcomingCredit) {
		cashCredit = 0;
		gotoStateWait();
		return;
	}
	cashCredit -= outcomingCredit;
#else
	EventUint8Interface event(MdbSlaveCoinChanger::Event_DispenseCoin);
	if(event.open(envelope) == false) {
		return;
	}
/*
	uint32_t outcomingCredit = event.getValue();
	LOG_INFO(LOG_SM, "stateCashChangeEventDispenseCoin " << outcomingCredit);
	if(cashCredit < outcomingCredit) {
		cashCredit = 0;
		gotoStateWait();
		return;
	}
	cashCredit -= outcomingCredit;*/
#endif
}

void SaleManagerMdbNoCashlessCore::stateCashChangeTimeout() {
	LOG_INFO(LOG_SM, "stateCashChangeTimeout " << cashCredit);
	price = cashCredit;
	if(product->findByPrice("CA", 0, price) == false) {
		LOG_ERROR(LOG_SM, "Product with price <" << price << "> not found");
		chronicler->registerCashlessIdNotFoundEvent(price, context);
		gotoStateWait();
		return;
	}

	saleData.selectId.set(product->getId());
	saleData.wareId = product->getWareId();
	saleData.name.set(product->getName());
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = price;
	saleData.price = price;
	saleData.taxRate = product->getTaxRate();
	saleData.taxSystem = config->getAutomat()->getTaxSystem();

	cashCredit -= price;
	config->getAutomat()->sale(saleData.selectId.get(), saleData.device.get(), saleData.priceList, saleData.price);
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
	gotoStateWait();
}
#else
#include <lib/sale_manager/mdb_no_cashless/SaleManagerMdbNoCashlessCore.h>

#include "lib/sale_manager/include/SaleManager.h"
#include "lib/utils/stm32/HardwareUartForwardController.h"

#include "common/logger/RemoteLogger.h"
#include "common/logger/include/Logger.h"

//#define DISABLE_BRIDGE
#define SMMD_VENDING_TIMEOUT 120000
#define SMMD_CHANGE_TIMEOUT 10000

SaleManagerMdbNoCashlessCore::SaleManagerMdbNoCashlessCore(SaleManagerMdbNoCashlessParams *params) :
	config(params->config),
	timers(params->timers),
	eventEngine(params->eventEngine),
	leds(params->leds),
	chronicler(params->chronicler),
	slaveCoinChanger(params->slaveCoinChanger),
	slaveBillValidator(params->slaveBillValidator),
	fiscalRegister(params->fiscalRegister),
	rebootReason(params->rebootReason),
	deviceId(params->eventEngine),
	enabled(false)
{
	this->context = config->getAutomat()->getSMContext();
	this->context->setStatus(Mdb::DeviceContext::Status_NotFound);
	this->context->setState(State_Idle);

	this->timer = timers->addTimer<SaleManagerMdbNoCashlessCore, &SaleManagerMdbNoCashlessCore::procTimer>(this);

	eventEngine->subscribe(this, GlobalId_SnifferCoinChanger);
	eventEngine->subscribe(this, GlobalId_SnifferBillValidator);

	product = config->getAutomat()->createIterator();
}

SaleManagerMdbNoCashlessCore::~SaleManagerMdbNoCashlessCore() {
	delete product;
}

void SaleManagerMdbNoCashlessCore::reset() {
	LOG_INFO(LOG_SM, "reset");
	slaveCoinChanger->reset();
	slaveBillValidator->reset();
	if(fiscalRegister != NULL) { fiscalRegister->reset(); }
	leds->setPayment(LedInterface::State_InProgress);
	context->setStatus(Mdb::DeviceContext::Status_Init);
	if(rebootReason == Reboot::Reason_PowerDown) {
		LOG_INFO(LOG_SM, "Start");
		HardwareUartForwardController::start();
		gotoStateWait();
		return;
	} else {
		gotoStateDelay();
		return;
	}
}

void SaleManagerMdbNoCashlessCore::service() {
	LOG_INFO(LOG_SM, "service");
}

void SaleManagerMdbNoCashlessCore::shutdown() {
	LOG_INFO(LOG_SM, "shutdown");
	HardwareUartForwardController::stop();
	context->setState(State_Idle);
	EventInterface event(SaleManager::Event_Shutdown);
	eventEngine->transmit(&event);
}

void SaleManagerMdbNoCashlessCore::procTimer() {
	LOG_DEBUG(LOG_SM, "procTimer " << context->getState());
	switch(context->getState()) {
	case State_Delay: stateDelayTimeout(); break;
	case State_CashChange: stateCashChangeTimeout(); break;
	default: LOG_ERROR(LOG_SM, "Unwaited timeout " << context->getState());
	}
}

void SaleManagerMdbNoCashlessCore::proc(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "proc " << context->getState() << "," << envelope->getType());
	switch(context->getState()) {
		case State_Idle: LOG_DEBUG(LOG_SM, "Ignore event " << context->getState() << "," << envelope->getType()); return;
		case State_Wait: stateWaitEvent(envelope); return;
		case State_CashCredit: stateCashCreditEvent(envelope); return;
		case State_CashVending: stateCashVendingEvent(envelope); return;
		case State_CashChange: stateCashChangeEvent(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbNoCashlessCore::gotoStateDelay() {
	LOG_DEBUG(LOG_SM, "gotoStateDelay");
	REMOTE_LOG(RLOG_MDBSCC, "2delay");
	HardwareUartForwardController::stop();
	timer->start(15000);
	context->setState(State_Delay);
}

void SaleManagerMdbNoCashlessCore::stateDelayTimeout() {
	LOG_DEBUG(LOG_SM, "stateDelayTimeout");
	HardwareUartForwardController::start();
	gotoStateWait();
}

void SaleManagerMdbNoCashlessCore::procDeviceStateChange() {
	LOG_DEBUG(LOG_SM, "procDeviceStateChange");
	if(slaveBillValidator->isEnable() == true || slaveCoinChanger->isEnable() == true) {
		if(enabled == false) {
			enabled = true;
			leds->setPayment(LedInterface::State_Success);
			context->setStatus(Mdb::DeviceContext::Status_Enabled);
			EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, true);
			eventEngine->transmit(&event);
		}
	} else {
		if(enabled == true) {
			enabled = false;
			leds->setPayment(LedInterface::State_Success);
			context->setStatus(Mdb::DeviceContext::Status_Disabled);
			EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, false);
			eventEngine->transmit(&event);
		}
	}
}

bool SaleManagerMdbNoCashlessCore::procDepositeCoin(EventEnvelope *envelope) {
	Mdb::EventDeposite event(MdbSnifferCoinChanger::Event_DepositeCoin);
	if(event.open(envelope) == false) {
		return false;
	}

	if(event.getRoute() == Mdb::CoinChanger::Route_Cashbox || event.getRoute() == Mdb::CoinChanger::Route_Tube) {
		uint32_t incomingCredit = event.getNominal();
		chronicler->registerCoinInEvent(incomingCredit, event.getRoute());
		LOG_INFO(LOG_SM, "procDepositeCoin " << incomingCredit);
		cashCredit += incomingCredit;
		return true;
	}

	return false;
}

bool SaleManagerMdbNoCashlessCore::procDepositeBill(EventEnvelope *envelope) {
	Mdb::EventDeposite event(MdbSnifferBillValidator::Event_DepositeBill);
	if(event.open(envelope) == false) {
		return false;
	}

	if(event.getRoute() == Mdb::BillValidator::Route_Stacked) {
		uint32_t incomingCredit = event.getNominal();
		LOG_INFO(LOG_SM, "procDepositeBill " << incomingCredit);
		chronicler->registerBillInEvent(incomingCredit, event.getRoute());
		cashCredit += incomingCredit;
		return true;
	}

	return false;
}

void SaleManagerMdbNoCashlessCore::procCashSale() {
	LOG_INFO(LOG_SM, "procCashSale " << cashCredit);
	if(cashCredit == 0) {
		LOG_DEBUG(LOG_SM, "All cash returned");
		return;
	}

	price = cashCredit;
	if(product->findByPrice("CA", 0, price) == false) {
		LOG_ERROR(LOG_SM, "Product with price <" << price << "> not found");
		chronicler->registerCashlessIdNotFoundEvent(price, context);
		return;
	}

	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), price, product->getTaxRate(), 1);
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = price;
	saleData.taxSystem = config->getAutomat()->getTaxSystem();

	cashCredit -= price;
	config->getAutomat()->sale(&saleData);
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
	return;
}

void SaleManagerMdbNoCashlessCore::gotoStateWait() {
	LOG_DEBUG(LOG_SM, "gotoStateSale");
	REMOTE_LOG(RLOG_MDBSCC, "2wait");
	cashCredit = 0;
	timer->stop();
	context->setState(State_Wait);
}

void SaleManagerMdbNoCashlessCore::stateWaitEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateWaitEvent");
	switch(envelope->getType()) {
		case MdbSnifferCoinChanger::Event_Enable: procDeviceStateChange(); return;
		case MdbSnifferCoinChanger::Event_Disable: procDeviceStateChange(); return;
		case MdbSnifferCoinChanger::Event_DepositeCoin: stateWaitEventDepositeCoin(envelope); return;
		case MdbSnifferBillValidator::Event_DepositeBill: stateWaitEventDepositeBill(envelope); return;
		case MdbSnifferBillValidator::Event_Enable: procDeviceStateChange(); return;
		case MdbSnifferBillValidator::Event_Disable: procDeviceStateChange(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbNoCashlessCore::stateWaitEventDepositeCoin(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateWaitEventDepositeCoin");
	if(procDepositeCoin(envelope) == true) {
		gotoStateCashCredit();
	}
}

void SaleManagerMdbNoCashlessCore::stateWaitEventDepositeBill(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateWaitEventDepositeBill");
	if(procDepositeBill(envelope) == true) {
		gotoStateCashCredit();
	}
}

void SaleManagerMdbNoCashlessCore::gotoStateCashCredit() {
	LOG_DEBUG(LOG_SM, "gotoStateCashCredit");
	REMOTE_LOG(RLOG_MDBSCC, "2credit");
	timer->stop();
	context->setState(State_CashCredit);
}

void SaleManagerMdbNoCashlessCore::stateCashCreditEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashCreditEvent");
	switch(envelope->getType()) {
		case MdbSnifferCoinChanger::Event_Enable: procDeviceStateChange(); return;
		case MdbSnifferCoinChanger::Event_Disable: stateCashCreditEventDisable(); return;
		case MdbSnifferCoinChanger::Event_DepositeCoin: procDepositeCoin(envelope); return;
		case MdbSnifferCoinChanger::Event_DispenseCoin: stateCashCreditEventDispenseCoin(envelope); return;
		case MdbSnifferBillValidator::Event_Enable: procDeviceStateChange(); return;
		case MdbSnifferBillValidator::Event_Disable: procDeviceStateChange(); return;
		case MdbSnifferBillValidator::Event_DepositeBill: procDepositeBill(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbNoCashlessCore::stateCashCreditEventDispenseCoin(EventEnvelope *envelope) {
	EventUint32Interface event(MdbSnifferCoinChanger::Event_DispenseCoin);
	if(event.open(envelope) == false) {
		return;
	}

	uint32_t outcomingCredit = event.getValue();
	LOG_INFO(LOG_SM, "stateCashCreditEventDispenseCoin " << outcomingCredit);
	if(cashCredit <= outcomingCredit) {
		cashCredit = 0;
		gotoStateWait();
		return;
	}
	cashCredit -= outcomingCredit;
}

void SaleManagerMdbNoCashlessCore::stateCashCreditEventDisable() {
	LOG_INFO(LOG_SM, "stateCashCreditEventDisable");
	gotoStateCashVending();
}

void SaleManagerMdbNoCashlessCore::gotoStateCashVending() {
	LOG_DEBUG(LOG_SM, "gotoStateCashVending");
	REMOTE_LOG(RLOG_MDBSCC, "2vend");
	timer->start(SMMD_VENDING_TIMEOUT);
	context->setState(State_CashVending);
}

void SaleManagerMdbNoCashlessCore::stateCashVendingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashVendingEvent");
	switch(envelope->getType()) {
		case MdbSnifferCoinChanger::Event_DepositeCoin: procDepositeCoin(envelope); return;
		case MdbSnifferCoinChanger::Event_DispenseCoin: stateCashVendingEventDispenseCoin(envelope); return;
		case MdbSnifferCoinChanger::Event_Enable: stateCashVendingEventEnable(); return;
		case MdbSnifferCoinChanger::Event_Disable: procDeviceStateChange(); return;
		case MdbSnifferBillValidator::Event_DepositeBill: procDepositeBill(envelope); return;
		case MdbSnifferBillValidator::Event_Enable: procDeviceStateChange(); return;
		case MdbSnifferBillValidator::Event_Disable: procDeviceStateChange(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbNoCashlessCore::stateCashVendingEventDispenseCoin(EventEnvelope *envelope) {
	EventUint32Interface event(MdbSnifferCoinChanger::Event_DispenseCoin);
	if(event.open(envelope) == false) {
		return;
	}

	uint32_t outcomingCredit = event.getValue();
	LOG_INFO(LOG_SM, "stateCashVendingEventDispenseCoin " << outcomingCredit);
	if(cashCredit < outcomingCredit) {
		cashCredit = 0;
		gotoStateWait();
		return;
	}
	cashCredit -= outcomingCredit;
}

void SaleManagerMdbNoCashlessCore::stateCashVendingEventEnable() {
	LOG_INFO(LOG_SM, "stateCashVendingEventEnable");
	gotoStateCashChange();
}

void SaleManagerMdbNoCashlessCore::gotoStateCashChange() {
	LOG_DEBUG(LOG_SM, "gotoStateCashChange");
	REMOTE_LOG(RLOG_MDBSCC, "2change");
	timer->start(SMMD_CHANGE_TIMEOUT);
	context->setState(State_CashChange);
}

void SaleManagerMdbNoCashlessCore::stateCashChangeEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashChangeEvent");
	switch(envelope->getType()) {
		case MdbSnifferCoinChanger::Event_Enable: procDeviceStateChange(); return;
		case MdbSnifferCoinChanger::Event_Disable: procDeviceStateChange(); return;
		case MdbSnifferCoinChanger::Event_DepositeCoin: stateCashChangeEventDepositeCoin(envelope); return;
		case MdbSnifferCoinChanger::Event_DispenseCoin: stateCashChangeEventDispenseCoin(envelope); return;
		case MdbSnifferBillValidator::Event_Enable: procDeviceStateChange(); return;
		case MdbSnifferBillValidator::Event_Disable: procDeviceStateChange(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void SaleManagerMdbNoCashlessCore::stateCashChangeEventDepositeCoin(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateCashChangeEventDepositeCoin");
	procCashSale();
	if(procDepositeCoin(envelope) == false) {
		gotoStateWait();
		return;
	}
	gotoStateCashCredit();
}

void SaleManagerMdbNoCashlessCore::stateCashChangeEventDispenseCoin(EventEnvelope *envelope) {
	EventUint32Interface event(MdbSnifferCoinChanger::Event_DispenseCoin);
	if(event.open(envelope) == false) {
		return;
	}

	uint32_t outcomingCredit = event.getValue();
	LOG_INFO(LOG_SM, "stateCashChangeEventDispenseCoin " << outcomingCredit);
	if(cashCredit < outcomingCredit) {
		cashCredit = 0;
		gotoStateWait();
		return;
	}
	cashCredit -= outcomingCredit;
	timer->start(SMMD_CHANGE_TIMEOUT);
}

void SaleManagerMdbNoCashlessCore::stateCashChangeTimeout() {
	LOG_INFO(LOG_SM, "stateCashChangeTimeout");
	procCashSale();
	gotoStateWait();
}
#endif
