#include "CciT3OrderDevice.h"
#include "PinCodeDevice.h"

#include "common/logger/include/Logger.h"

namespace Cci {
namespace T3 {

OrderDevice::OrderDevice(AbstractUart *deviceUart, AbstractUart *pincodeUart, TimerEngine *timerEngine, EventEngineInterface *eventEngine) {
	driver = new Cci::T3::Driver(deviceUart, timerEngine, eventEngine);
	if(pincodeUart == NULL) {
		pincode = new DummyPinCodeDevice(eventEngine);
	} else {
		pincode = new PinCodeDevice(timerEngine, eventEngine, pincodeUart);
	}
	eventEngine->subscribe(this, GlobalId_OrderDevice, driver->getDeviceId());
}

OrderDevice::~OrderDevice() {
	delete this->driver;
	delete this->pincode;
}

void OrderDevice::setOrder(Order *order) {
	driver->setOrder(order);
}

void OrderDevice::reset() {
	driver->reset();
	pincode->pageStart();
}

void OrderDevice::disable() {
	driver->disable();
	pincode->pageStart();
}

void OrderDevice::enable() {
	driver->enable();
	pincode->pageStart();
}

void OrderDevice::approveVend() {
	driver->approveVend();
	pincode->pageSale();
}

void OrderDevice::requestPinCode() {
	pincode->pagePinCode();
}

void OrderDevice::denyVend() {
	driver->denyVend();
	pincode->pageStart();
}

void OrderDevice::proc(EventEnvelope *envelope) {
	switch(envelope->getType()) {
		case OrderDeviceInterface::Event_VendRequest: pincode->pageProgress(); return;
		case OrderDeviceInterface::Event_VendCompleted: pincode->pageStart(); return;
		case OrderDeviceInterface::Event_VendCancelled: pincode->pageStart(); return;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << envelope->getType());
	}
}

}
}
