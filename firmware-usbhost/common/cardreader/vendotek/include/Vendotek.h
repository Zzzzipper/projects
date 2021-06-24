#ifndef COMMON_VENDOTEK_H
#define COMMON_VENDOTEK_H

#include "mdb/MdbProtocol.h"
#include "mdb/master/cashless/MdbMasterCashlessInterface.h"
#include "uart/include/interface.h"
#include "tcpip/include/TcpIp.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "timer/include/RealTime.h"
#include "fiscal_register/include/QrCodeStack.h"

namespace Vendotek {

class CommandLayer;
class PacketLayer;

class Cashless : public MdbMasterCashlessInterface, public VerificationInterface, public QrCodeInterface {
public:
	Cashless(Mdb::DeviceContext *context, AbstractUart *uart, TcpIp *tcpIp, TimerEngine *timers, EventEngineInterface *eventEngine, RealTimeInterface *realtime, uint32_t credit);
	virtual ~Cashless();
	virtual EventDeviceId getDeviceId();
	virtual void reset() override;
	virtual bool isRefundAble() override;
	virtual void disable() override;
	virtual void enable() override;
	virtual bool revalue(uint32_t credit) override;
	virtual bool sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) override;
	virtual bool saleComplete() override;
	virtual bool saleFailed() override;
	virtual bool closeSession() override;

	virtual bool drawQrCode(const char *header, const char *footer, const char *text) override;

	bool verification();

private:
	PacketLayer *packetLayer;
	CommandLayer *commandLayer;
};

}

#endif
