#include <lib/sale_manager/cci_t4/ErpOrderCashless.h>
#include "common/utils/include/Hex.h"
#include "common/logger/include/Logger.h"

ErpOrderCashless::ErpOrderCashless(
	ConfigModem *config,
	CodeScannerInterface *scanner,
	TcpIp *tcpConn,
	TimerEngine *timerEngine,
	EventEngineInterface *eventEngine
) :
	eventEngine(eventEngine),
	deviceId(eventEngine),
	state(State_Idle)
{
	erpOrderMaster = new ErpOrderMaster(config, scanner, tcpConn, timerEngine, eventEngine);
	eventEngine->subscribe(this, GlobalId_Order);
}

EventDeviceId ErpOrderCashless::getDeviceId() {
	return deviceId;
}

void ErpOrderCashless::reset() {
	erpOrderMaster->reset();
	gotoStateSale();
}

bool ErpOrderCashless::isRefundAble() {
	return false;
}

void ErpOrderCashless::disable() {
	erpOrderMaster->disable();
}

void ErpOrderCashless::enable() {
	erpOrderMaster->enable();
}

bool ErpOrderCashless::revalue(uint32_t credit) {
	return false;
}

bool ErpOrderCashless::sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) {
	this->productId = productId;
	this->productPrice = productPrice;
	erpOrderMaster->setProductId(productId);
	erpOrderMaster->enable();
	state = State_Approving;
	return true;
}

bool ErpOrderCashless::saleComplete() {
	if(state != State_Vending) {
		return false;
	}
	erpOrderMaster->saleComplete();
	state = State_Closing;
	return true;
}

bool ErpOrderCashless::saleFailed() {
	if(state != State_Vending) {
		return false;
	}
	erpOrderMaster->saleFailed();
	state = State_Closing;
	return true;
}

bool ErpOrderCashless::closeSession() {
	if(state == State_Approving) {
		gotoStateSale();
		return true;
	}
	return true;
}

void ErpOrderCashless::proc(EventEnvelope *envelope) {
	switch(state) {
	case State_Approving: stateApprovingEvent(envelope); break;
	case State_Closing: stateClosingEvent(envelope); break;
	default: LOG_DEBUG(LOG_SM, "Unwaited event " << state);
	}
}

void ErpOrderCashless::gotoStateSale() {
	LOG_DEBUG(LOG_SM, "gotoStateSale");
	state = State_Sale;
}

void ErpOrderCashless::stateApprovingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateApprovingEvent");
	switch(envelope->getType()) {
		case OrderInterface::Event_OrderRequest: stateApprovingEventOrderRequest(envelope); return;
		case OrderInterface::Event_OrderCancel: stateApprovingEventOrderCancel(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event: state=" << state << ", event=" << envelope->getType());
	}
}

void ErpOrderCashless::stateApprovingEventOrderRequest(EventEnvelope *envelope) {
	EventUint16Interface event(OrderInterface::Event_OrderRequest);
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
	eventEngine->transmit(&event2);
}

void ErpOrderCashless::stateApprovingEventOrderCancel() {
	LOG_INFO(LOG_SM, "stateApprovingEventOrderCancel");
	gotoStateSale();
	EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
	eventEngine->transmit(&event);
}

void ErpOrderCashless::stateClosingEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateClosingEvent");
	switch(envelope->getType()) {
		case OrderInterface::Event_OrderEnd: stateClosingEventOrderEnd(envelope); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << state << "," << envelope->getType());
	}
}

void ErpOrderCashless::stateClosingEventOrderEnd(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "stateClosingEventOrderEnd");
	gotoStateSale();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

