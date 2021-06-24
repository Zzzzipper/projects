#include "SaleManagerExeSlaveCore.h"

#include "lib/sale_manager/include/SaleManager.h"
#include "lib/utils/stm32/HardwareUartForwardController.h"

#include "common/logger/include/Logger.h"

SaleManagerExeSlaveCore::SaleManagerExeSlaveCore(ConfigModem *config, TimerEngine *timers, EventEngine *eventEngine, ExeSnifferInterface *exeSniffer, Fiscal::Register *fiscalRegister) :
	config(config),
	timers(timers),
	eventEngine(eventEngine),
	exeSniffer(exeSniffer),
	fiscalRegister(fiscalRegister),
	deviceId(eventEngine)
{
	this->context = config->getAutomat()->getSMContext();
	this->context->setStatus(Mdb::DeviceContext::Status_Init);
	this->context->setState(State_Idle);
	this->product = config->getAutomat()->createIterator();
	this->timer = timers->addTimer<SaleManagerExeSlaveCore, &SaleManagerExeSlaveCore::procTimer>(this);

	eventEngine->subscribe(this, GlobalId_Executive);
	eventEngine->subscribe(this, GlobalId_FiscalRegister);
	HardwareUartForwardController::init();
}

SaleManagerExeSlaveCore::~SaleManagerExeSlaveCore() {
	timers->deleteTimer(timer);
	delete this->product;
}

void SaleManagerExeSlaveCore::reset() {
	LOG_DEBUG(LOG_SM, "reset");
	gotoStateWait();
	exeSniffer->reset();
	if(fiscalRegister != NULL) { fiscalRegister->reset(); }
	HardwareUartForwardController::start();
	context->setStatus(Mdb::DeviceContext::Status_Work);
	EventUint8Interface event(deviceId, SaleManager::Event_AutomatState, true);
	eventEngine->transmit(&event);
}

void SaleManagerExeSlaveCore::shutdown() {
	LOG_DEBUG(LOG_SM, "shutdown");
	EventInterface event(deviceId, SaleManager::Event_Shutdown);
	eventEngine->transmit(&event);
	context->setState(State_Idle);
}

void SaleManagerExeSlaveCore::procTimer() {
	switch(context->getState()) {
	default:;
	}
}

void SaleManagerExeSlaveCore::proc(EventEnvelope *envelope) {
	switch(context->getState()) {
	case State_Wait: stateWaitEvent(envelope); break;
	case State_PrintCheck: statePrintCheckEvent(envelope); break;
	default: LOG_DEBUG(LOG_SM, "Unwaited event " << context->getState());
	}
}

void SaleManagerExeSlaveCore::gotoStateWait() {
	LOG_DEBUG(LOG_SM, "gotoStateWait");
	context->setState(State_Wait);
}

void SaleManagerExeSlaveCore::stateWaitEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateWaitEvent");
	switch(envelope->getType()) {
		case ExeSniffer::Event_Sale: stateWaitEventSale(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << context->getState() << ", event=" << envelope->getType());
	}
}

void SaleManagerExeSlaveCore::stateWaitEventSale(EventEnvelope *envelope) {
	EventUint8Interface event(ExeSniffer::Event_Sale);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Envelope open failed");
		return;
	}

	uint16_t productId = event.getValue();
	LOG_INFO(LOG_SM, "stateWaitEventSale " << productId);
	if(product->findByCashlessId(productId) == false) {
		LOG_ERROR(LOG_SM, "Product " << productId << " not found");
		gotoStateWait();
		return;
	}

	price = product->getPrice("CA", 0); //todo: определение цены из обмена по протоколу EXECUTIVE
	if(price == NULL) {
		LOG_ERROR(LOG_SM, "Undefined price " << productId);
		gotoStateWait();
		return;
	}

	saleData.setProduct(product->getId(), product->getWareId(), product->getName(), price->getPrice(), product->getTaxRate(), 1);
	saleData.device.set("CA");
	saleData.priceList = 0;
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = price->getPrice();
	saleData.taxSystem = config->getAutomat()->getTaxSystem();

	LOG_INFO(LOG_SM, ">>>>>>>>>>>>>>>>TICKET " << saleData.getProductNum() << "," << saleData.getPrice() << ":" << config->getAutomat()->getDecimalPoint());
	config->getAutomat()->sale(&saleData);
	gotoStatePrintCheck();
}

void SaleManagerExeSlaveCore::gotoStatePrintCheck() {
	LOG_INFO(LOG_SM, "gotoStatePrintCheck");
	fiscalRegister->sale(&saleData, config->getAutomat()->getDecimalPoint());
	context->setState(State_PrintCheck);
}

void SaleManagerExeSlaveCore::statePrintCheckEvent(EventEnvelope *envelope) {
	switch(envelope->getType()) {
		case Fiscal::Register::Event_CommandOK: gotoStateWait(); return;
		case Fiscal::Register::Event_CommandError: gotoStateWait(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << context->getState() << ", event=" << envelope->getType());
	}
}
