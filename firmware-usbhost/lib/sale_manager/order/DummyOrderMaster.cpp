#include <lib/sale_manager/order/DummyOrderMaster.h>
#include "common/logger/include/Logger.h"

DummyOrderMaster::DummyOrderMaster(
	EventEngineInterface *eventEngine
) :
	eventEngine(eventEngine),
	deviceId(eventEngine),
	order(NULL)
{

}

void DummyOrderMaster::setOrder(Order *order) {
	this->order = order;
}

void DummyOrderMaster::reset() {
	LOG_INFO(LOG_SM, "shutdown");
}

void DummyOrderMaster::connect() {
	LOG_INFO(LOG_SM, "connect");
	EventInterface request(deviceId, OrderMasterInterface::Event_Connected);
	eventEngine->transmit(&request);
}

void DummyOrderMaster::check() {
	LOG_INFO(LOG_SM, "check");
	if(order == NULL) {
		LOG_ERROR(LOG_SM, "order is NULL");
		return;
	}

	order->clear();
	order->add(1, 1);
	order->add(3, 2);
	order->add(4, 1);
	LOG_INFO(LOG_SM, "approved " << order->getQuantity());
	EventInterface request(deviceId, OrderMasterInterface::Event_Approved);
	eventEngine->transmit(&request);
}

void DummyOrderMaster::checkPinCode(const char *pincode) {
	LOG_INFO(LOG_SM, "checkPinCode");

}

void DummyOrderMaster::distribute(uint16_t cid) {
	LOG_INFO(LOG_SM, "distribute");
	EventInterface request(deviceId, OrderMasterInterface::Event_Distributed);
	eventEngine->transmit(&request);
}

void DummyOrderMaster::complete() {
	LOG_INFO(LOG_SM, "complete");

}
