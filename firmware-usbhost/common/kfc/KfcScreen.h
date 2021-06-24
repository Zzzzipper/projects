#ifndef COMMON_KFC_SCREEN_H
#define COMMON_KFC_SCREEN_H

#include "mdb/MdbProtocol.h"
#include "mdb/master/cashless/MdbMasterCashlessInterface.h"
#include "uart/include/interface.h"
#include "tcpip/include/TcpIp.h"
#include "timer/include/TimerEngine.h"
#include "event/include/EventEngine.h"
#include "fiscal_register/include/QrCodeStack.h"

namespace Kfc {

class Cashless : public MdbMasterCashlessInterface, public QrCodeInterface, public UartReceiveHandler {
public:
	Cashless(AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine);
	virtual ~Cashless();
	EventDeviceId getDeviceId() override;
	void reset() override;
	bool isRefundAble() override;
	void disable() override;
	void enable() override;
	bool revalue(uint32_t credit) override;
	bool sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) override;
	bool saleComplete() override;
	bool saleFailed() override;
	bool closeSession() override;

	bool drawQrCode(const char *header, const char *footer, const char *text) override;

	bool verification();

	void handle();

private:
	EventDeviceId deviceId;
	AbstractUart *uart;
};

}

#endif
