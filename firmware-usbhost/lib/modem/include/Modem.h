#ifndef LIB_MODEM_MODEM_H_
#define LIB_MODEM_MODEM_H_

#include <lib/sale_manager/cci_t3/ErpOrderMaster.h>
#include "lib/utils/stm32/Reboot.h"
#include "lib/fiscal_manager/include/FiscalManager.h"
#include "lib/screen/LoyalityScreen.h"
#include "lib/battery/Battery.h"
#include "lib/modem/Relay.h"
#include "lib/client/ClientContext.h"

#include "common/config/include/ConfigModem.h"
#include "common/timer/include/Timer.h"
#include "common/mdb/master/cashless/MdbMasterCashlessStack.h"
#include "common/fiscal_register/include/FiscalRegister.h"
#include "common/fiscal_register/include/QrCodeStack.h"
#include "common/utils/include/Event.h"

class ConfigMaster;
class AbstractUart;
class Uart;
class Network;
class NetworkUart;
class RealTimeInterface;
class StringBuilder;
class Buffer;
class TimerEngine;
class Buttons;
class ModemLed;
class Melody;
class GramophoneInterface;
class SaleManager;
class EventRegistrar;
class EcpAgent;
class ErpAgent;
class ErpCommandProcessor;
class EventEngine;
class TcpIp;
class FiscalTest;
class CodeScannerInterface;
namespace Gsm { class Driver; }
namespace Rfid { class Cashless; }

class Modem : public EventObserver, public EventSubscriber {
public:
	Modem(ConfigModem *config, TimerEngine *timers, EventEngine *events, Gsm::Driver *gsm, Buttons *buttons, Uart *slaveUart, Uart *masterUart, Uart *dexUart, Uart *kktUart, Network *network, RealTimeInterface *realtime);
	~Modem();
	void init(Reboot::Reason rebootReason);
	void reset();

	virtual void proc(Event *event);
	virtual void proc(EventEnvelope *envelope);

private:
	enum State {
		State_Idle = 0,
		State_Work,
		State_Shutdown,
	};
	ConfigMaster *config;
	ClientContext *client;
	TimerEngine *timerEngine;
	EventEngine *eventEngine;
	Gsm::Driver *gsm;
	Buttons *buttons;
	Uart *slaveUart;
	Uart *masterUart;
	Uart *dexUart;
	Uart *ext1Uart;
	AbstractUart *fiscalUart;
	TcpIp *fiscalConn;
	Network *network;
	RealTimeInterface *realtime;
	State state;
	Screen *screen;
	LoyalityScreen *loyalityScreen;
	Rfid::Cashless *rfid;
	Fiscal::Register *fiscalRegister;
	Fiscal::Manager *fiscalManager;
	CodeScannerInterface *scanner;
	QrCodeStack *qrCodeStack;
	MdbMasterCashlessInterface *cashless;
	MdbMasterCashlessStack *externCashless;
	VerificationInterface *verification;
	SaleManager *saleManager;
	EventRegistrar *eventRegistrar;
	EcpAgent *ecpAgent;
	ErpAgent *erpAgent;
	ErpCommandProcessor *erpCommandProcessor;
	Melody *melodyButton1;
	Melody *melodyButton2;
	Melody *melodyButton3;
	GramophoneInterface *gramophone;
	ModemLed *leds;
	FiscalTest *fiscalTest;
	Reboot::Reason rebootReason;
	PowerAgent *powerAgent;
	Relay *relay;

	void initBattery();
	void initRelay();
	void initLeds();
	void initDex();
	void initGsm();
	void initExt1();
	void initExt2();
	void initUsb();
	void initErpAgent();
	void initFiscalRegister();
	void initSaleManager();
	void initEventRegistrar();
	void initFirmwareState();
	void registerRebootReason();

	void stateWorkEvent(Event *event);
	void stateWorkEnvelope(EventEnvelope *envelope);
	void stateWorkEventReboot();

	void stateShutdownEnvelope(EventEnvelope *envelope);
	void stateShutdownEventSaleManagerShutdown();

	void testCashlessPayment();
	void testFiscal();
	void testQrCode();
	void testToken();
};

#endif
