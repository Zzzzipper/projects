#ifndef COMMON_CCICSI_H
#define COMMON_CCICSI_H

#include "mdb/slave/cashless/MdbSlaveCashless3.h"
#include "uart/include/interface.h"

namespace CciCsi {

class PacketLayer;
class CommandLayer;

class Cashless : public MdbSlaveCashlessInterface {
public:
	Cashless(AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine);
	virtual ~Cashless();

	void reset() override;
	bool isInited() override;
	bool isEnable() override;
	void setCredit(uint32_t credit) override;
	void approveVend(uint32_t productPrice) override;
	void denyVend(bool close) override;
	void cancelVend() override;

private:
	PacketLayer *packetLayer;
	CommandLayer *commandLayer;
};

}

#endif
