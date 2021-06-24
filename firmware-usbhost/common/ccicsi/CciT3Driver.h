#ifndef COMMON_CCICSI_T3_DRIVER_H_
#define COMMON_CCICSI_T3_DRIVER_H_

#include "ccicsi/Order.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"

namespace CciCsi { class PacketLayer; }

namespace Cci {
namespace T3 {

class CommandLayer;

class Driver : public OrderDeviceInterface {
public:
	Driver(AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine);
	~Driver();
	EventDeviceId getDeviceId();
	void setOrder(Order *order) override;
	void reset() override;
	void disable() override;
	void enable() override;
	void approveVend() override;
	void requestPinCode() override;
	void denyVend() override;

private:
	CciCsi::PacketLayer *packetLayer;
	CommandLayer *commandLayer;
};

}
}

#endif
