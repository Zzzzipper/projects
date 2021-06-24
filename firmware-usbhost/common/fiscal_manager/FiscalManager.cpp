#include "include/FiscalManager.h"
#include "logger/include/Logger.h"

#include <string.h>

#define FISCAL_POOL_SIZE 3

namespace Fiscal {

Queue::Queue() {
	pool = new Fifo<Fiscal::Sale*>(FISCAL_POOL_SIZE);
	for(uint16_t i = 0; i < FISCAL_POOL_SIZE; i++) {
		Fiscal::Sale *sale = new Fiscal::Sale;
		pool->push(sale);
	}
	fifo = new Fifo<Fiscal::Sale*>(FISCAL_POOL_SIZE);
}

Queue::~Queue() {
	delete fifo;
	delete pool;
}

bool Queue::isFull() {
	return pool->isEmpty();
}

bool Queue::push(Fiscal::Sale *saleData) {
	Fiscal::Sale *entry = pool->pop();
	if(entry == NULL) {
		return false;
	}
	entry->set(saleData);
	fifo->push(entry);
	return true;
}

bool Queue::pop(Fiscal::Sale *saleData) {
	Fiscal::Sale *entry = fifo->pop();
	if(entry == NULL) {
		return false;
	}
	saleData->set(entry);
	pool->push(entry);
	return true;
}

Manager::Manager(
	ConfigModem *config,
	TimerEngine *timerEngine,
	EventEngineInterface *eventEngine,
	Fiscal::Register *fr1,
	QrCodeInterface *qrCodePrinter
) :
	config(config),
	timerEngine(timerEngine),
	eventEngine(eventEngine),
	fr1(fr1),
	qrCodePrinter(qrCodePrinter),
	state(State_Idle),
	deviceId(eventEngine),
	converter(config->getAutomat()->getSMContext()->getMasterDecimalPoint()),
	qrCodeHeader(QR_HEADER_SIZE, QR_HEADER_SIZE),
	qrCodeFooter(QR_FOOTER_SIZE, QR_FOOTER_SIZE),
	qrCodeData(QR_TEXT_SIZE, QR_TEXT_SIZE)
{
	converter.setDeviceDecimalPoint(2);
	eventEngine->subscribe(this, GlobalId_FiscalRegister, fr1->getDeviceId());
}

Manager::~Manager() {
}

EventDeviceId Manager::getDeviceId() {
	return deviceId;
}

void Manager::reset() {
	fr1->reset();
	state = State_Idle;
}

void Manager::sale(Fiscal::Sale *newData, uint32_t decimalPoint) {
	LOG_DEBUG(LOG_SM, "sale " << state << "," << newData->getProductNum() << "," << newData->getPrice() << "," << newData->paymentType);
	config->getRealTime()->getDateTime(&newData->datetime);
	if(newData->getPrice() > 0 && (newData->paymentType == Fiscal::Payment_Cash || newData->paymentType == Fiscal::Payment_Cashless)) {
		this->decimalPoint = decimalPoint;
		switch(state) {
			case State_Idle: stateIdleSale(newData); return;
			case State_Registration: stateRegistrationSale(newData); return;
			default: LOG_ERROR(LOG_SM, "Unwaited event " << state);
		}
	} else {
		procFreeVend(newData);
		return;
	}
}

void Manager::proc(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "proc " << state << "," << envelope->getType() << "," << envelope->getDevice());
	switch(state) {
		case State_Registration: stateRegistrationEvent(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << state << "," << envelope->getType());
	}
}

void Manager::procFreeVend(Fiscal::Sale *newData) {
	LOG_DEBUG(LOG_SM, "procFreeVend");
	newData->taxSystem = Fiscal::TaxSystem_None;
	newData->fiscalRegister = 0;
	newData->fiscalStorage = Fiscal::Status_None;
	newData->fiscalDocument = 0;
	newData->fiscalSign = 0;
	registerSaleEvent(newData);
	EventInterface event(deviceId, Fiscal::Register::Event_CommandOK);
	eventEngine->transmit(&event);
}

void Manager::stateIdleSale(Fiscal::Sale *newData) {
	LOG_DEBUG(LOG_SM, "stateIdleSale");
	saleData.set(newData);
	gotoStateRegistration();
#ifndef FR_WAIT
	EventInterface event(deviceId, Fiscal::Register::Event_CommandOK);
	eventEngine->transmit(&event);
#endif
}

void Manager::gotoStateRegistration() {
	LOG_DEBUG(LOG_SM, "gotoStateRegistration");
	fr1->sale(&saleData, decimalPoint);
	state = State_Registration;
}

void Manager::stateRegistrationSale(Fiscal::Sale *newData) {
	LOG_DEBUG(LOG_SM, "stateRegistrationSale");
	if(fifo.isFull() == false) {
		LOG_DEBUG(LOG_SM, "Added to queue");
		fifo.push(newData);
	} else {
		LOG_DEBUG(LOG_SM, "Queue is full");
		newData->fiscalRegister = 0;
		newData->fiscalStorage = Fiscal::Status_Overflow;
		newData->fiscalDocument = 0;
		newData->fiscalSign = 0;
		registerSaleEvent(newData);
	}

	EventInterface event(deviceId, Fiscal::Register::Event_CommandOK);
	eventEngine->transmit(&event);
}

void Manager::stateRegistrationEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateRegistrationEvent");
	switch(envelope->getType()) {
		case Fiscal::Register::Event_CommandOK: stateRegistrationEventOK(envelope); return;
		case Fiscal::Register::Event_CommandError: stateRegistrationEventError(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << state << "," << envelope->getType());
	}
}

void Manager::stateRegistrationEventOK(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateRegistrationEventOK " << saleData.fiscalStorage);
	if(saleData.fiscalStorage > Fiscal::Status_Overflow) {
		showQrCode();
	}
	if(fr1->isRemoteFiscal() == false) {
		registerSaleEvent(&saleData);
	}

#ifdef FR_WAIT
	envelope->setDevice(deviceId.getValue());
	eventEngine->transmit(envelope);
#endif

	if(fifo.pop(&saleData) == false) {
		LOG_DEBUG(LOG_SM, "Check queue is empty");
		state = State_Idle;
		return;
	} else {
		LOG_DEBUG(LOG_SM, "Next check");
		gotoStateRegistration();
		return;
	}
}

void Manager::stateRegistrationEventError(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateRegistrationEventError");
	registerFiscalErrorEvent(&saleData.datetime, envelope);
	registerSaleEvent(&saleData);

#ifdef FR_WAIT
	envelope->setDevice(deviceId.getValue());
	eventEngine->transmit(envelope);
#endif

	if(fifo.pop(&saleData) == false) {
		LOG_DEBUG(LOG_SM, "Check queue is empty");
		state = State_Idle;
		return;
	} else {
		LOG_DEBUG(LOG_SM, "Next check");
		gotoStateRegistration();
		return;
	}
}

void Manager::registerSaleEvent(Fiscal::Sale *saleData) {
	config->getEvents()->add(&(saleData->datetime), saleData);
}

void Manager::registerFiscalErrorEvent(DateTime *datetime, EventEnvelope *envelope) {
	Fiscal::EventError event;
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event open failed");
		return;
	}

	config->getEvents()->add(datetime, (ConfigEvent::Code)event.code, event.data.getString());
}

void Manager::showQrCode() {
	if(qrCodePrinter == NULL) {
		LOG_INFO(LOG_SM, "qrCodePrinter is null");
		return;
	}

	qrCodeHeader.clear();
	qrCodeHeader << "Кассовый чек";
	LOG_INFO(LOG_SM, "qrCodeHeader " << qrCodeHeader.getString() << qrCodeHeader.getLen());

	Fiscal::Product *product = saleData.getProduct(0);
	qrCodeFooter.clear();
	qrCodeFooter << product->name.get();
	LOG_INFO(LOG_SM, "qrCodeFooter " << qrCodeFooter.getString() << qrCodeFooter.getLen());

	qrCodeData.clear();
	qrCodeData << "t=";
	datetime2fiscal(&(saleData.fiscalDatetime), &qrCodeData);
	uint32_t price1 = converter.convertMasterToDevice(saleData.getPrice());
	qrCodeData << "&s=" << price1/100 << ".";
	uint32_t price2 = price1 % 100;
	if(price2 < 10) { qrCodeData << "0"; } qrCodeData << price2;
	qrCodeData << "&fn=" << saleData.fiscalStorage;
	qrCodeData << "&i=" << saleData.fiscalDocument;
	qrCodeData << "&fp=" << saleData.fiscalSign;
	qrCodeData << "&n=1";
	LOG_INFO(LOG_SM, "qrCodeData " << qrCodeData.getString() << ", " << qrCodeData.getLen());

	qrCodePrinter->drawQrCode(qrCodeHeader.getString(), qrCodeFooter.getString(), qrCodeData.getString());
}

}
