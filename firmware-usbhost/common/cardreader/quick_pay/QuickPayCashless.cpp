#include "cardreader/quick_pay/include/QuickPayCashless.h"
#include "utils/include/Hex.h"
#include "logger/include/Logger.h"

namespace QuickPay {

Cashless::Cashless(
	ConfigModem *config,
	TcpIp *tcpConn,
	ScreenInterface *screen,
	TimerEngine *timerEngine,
	EventEngineInterface *eventEngine
) :
	eventEngine(eventEngine),
	deviceId(eventEngine),
	state(State_Idle)
{
	commandLayer = new CommandLayer(config, tcpConn, timerEngine, eventEngine);
	eventEngine->subscribe(this, GlobalId_Order);
}

EventDeviceId Cashless::getDeviceId() {
	return deviceId;
}

void Cashless::reset() {
	commandLayer->reset();
	gotoStateSale();
}

bool Cashless::isRefundAble() {
	return false;
}

void Cashless::disable() {

}

void Cashless::enable() {

}

bool Cashless::revalue(uint32_t credit) {
	return false;
}

bool Cashless::sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) {
	this->productId = productId;
	this->productPrice = productPrice;
	this->commandLayer->sale(productPrice);
	state = State_Loading;
	return true;
}

bool Cashless::saleComplete() {
	if(state != State_Vending) {
		return false;
	}
	this->commandLayer->saleComplete();
	gotoStateSale();
	return true;
}

bool Cashless::saleFailed() {
	if(state != State_Vending) {
		return false;
	}
	this->commandLayer->saleFailed();
	gotoStateSale();
	return true;
}

bool Cashless::closeSession() {
	if(state == State_Approving) {
		gotoStateSale();
		return true;
	}
	return true;
}

void Cashless::proc(EventEnvelope *envelope) {
	switch(state) {
	case State_Loading: stateLoadingEvent(envelope); break;
	case State_Approving: stateApprovingEvent(envelope); break;
	default: LOG_DEBUG(LOG_SM, "Unwaited event " << state);
	}
}

void Cashless::gotoStateSale() {
	LOG_DEBUG(LOG_SM, "gotoStateSale");
	state = State_Sale;
}

void Cashless::stateLoadingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateLoadingEvent");
	switch(envelope->getType()) {
		case CommandLayer::Event_QrReceived: stateLoadingEventQrReceived(envelope); return;
		case CommandLayer::Event_QrError: stateLoadingEventQrError(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << state << ", event=" << envelope->getType());
	}
}

void Cashless::stateLoadingEventQrReceived(EventEnvelope *envelope) {
/*	EventUint16Interface event(OrderInterface::Event_OrderRequest);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	LOG_INFO(LOG_SM, "stateApprovingEventOrderRequest " << event.getDevice() << "," << event.getValue());
	uint16_t productId = event.getValue();
	if(this->productId != productId) {
		LOG_ERROR(LOG_SM, "ProductId not equeal " << this->productId << "<>" << productId);
	}

	state = State_Vending;
	MdbMasterCashlessInterface::EventApproved event2(deviceId, Fiscal::Payment_Ephor, 0);
	eventEngine->transmit(&event2);*/
}

void Cashless::stateLoadingEventQrError() {
	LOG_INFO(LOG_SM, "stateLoadingEventQrError");
/*	gotoStateSale();
	EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
	eventEngine->transmit(&event);*/
}

void Cashless::stateApprovingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateApprovingEvent");
	switch(envelope->getType()) {
		case CommandLayer::Event_QrApproved: stateApprovingEventQrApproved(envelope); return;
		case CommandLayer::Event_QrDenied:
		case CommandLayer::Event_QrError: stateApprovingEventQrDenied(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << state << ", event=" << envelope->getType());
	}
}

void Cashless::stateApprovingEventQrApproved(EventEnvelope *envelope) {
/*	EventUint16Interface event(OrderInterface::Event_OrderRequest);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_SM, "Event load failed");
		return;
	}

	LOG_INFO(LOG_SM, "stateApprovingEventQrApproved " << event.getDevice() << "," << event.getValue());
	uint16_t productId = event.getValue();
	if(this->productId != productId) {
		LOG_ERROR(LOG_SM, "ProductId not equeal " << this->productId << "<>" << productId);
	}

	state = State_Vending;
	MdbMasterCashlessInterface::EventApproved event2(deviceId, Fiscal::Payment_Ephor, 0);
	eventEngine->transmit(&event2);*/
}

void Cashless::stateApprovingEventQrDenied() {
	LOG_INFO(LOG_SM, "stateApprovingEventQrDenied");
/*	gotoStateSale();
	EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
	eventEngine->transmit(&event);*/
}

}
