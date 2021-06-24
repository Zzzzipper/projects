#ifndef COMMON_CCIFRANKE_DRIVER_H
#define COMMON_CCIFRANKE_DRIVER_H

#include <ccicsi/CciT3Driver.h>
#include "mdb/slave/cashless/MdbSlaveCashless3.h"
#include "uart/include/interface.h"

namespace CciCsi { class PacketLayer; }

namespace Cci {
namespace Franke {

class CommandLayer;

class Cashless : public MdbSlaveCashlessInterface /* public Csi::DriverInterface */ {
public:
	Cashless(AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine);
	virtual ~Cashless();
#if 1
	void reset() override;
	bool isInited() override;
	bool isEnable() override;
	void setCredit(uint32_t credit) override;
	void approveVend(uint32_t productPrice) override;
	void denyVend(bool close) override;
	void cancelVend() override;
#else
	void reset() override;
	void disableProducts() override;
	void enableProducts() override;
	void approveVend(uint16_t productId) override;
	void denyVend() override;
#endif

private:
	CciCsi::PacketLayer *packetLayer;
	CommandLayer *commandLayer;
};

}
}

#endif
