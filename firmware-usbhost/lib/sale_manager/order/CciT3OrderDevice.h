#ifndef LIB_ORDER_CCIT3ORDERDEVICE_H_
#define LIB_ORDER_CCIT3ORDERDEVICE_H_

#include "PinCodeDevice.h"

#include "common/ccicsi/CciT3Driver.h"

namespace Cci {
namespace T3 {

class OrderDevice : public OrderDeviceInterface, public EventSubscriber {
public:
	OrderDevice(AbstractUart *deviceUart, AbstractUart *pincodeUart, TimerEngine *timers, EventEngineInterface *eventEngine);
	~OrderDevice();

	void setOrder(Order *order) override;
	void reset() override;
	void disable() override;
	void enable() override;
	void approveVend() override;
	void requestPinCode() override;
	void denyVend() override;

	void proc(EventEnvelope *envelope) override;

private:
	Driver *driver;
	PinCodeInterface *pincode;
};

}
}

#endif
