#include <lib/sale_manager/mdb_no_cashless/include/SaleManagerMdbNoCashless.h>
#include "lib/sale_manager/cci_t3/include/SaleManagerCciT3.h"
#include "lib/sale_manager/cci_t4/ErpOrderCashless.h"
#include "lib/sale_manager/cci_t4/include/SaleManagerCciT4.h"
#include "lib/sale_manager/cci_t4/ScannerFree.h"
#include "logger/RemoteLogger.h"
#include "include/Modem.h"
#include "ModemLed.h"
#include "ConfigMaster.h"
#include "EventRegistrar.h"

#include "lib/sale_manager/mdb_slave/include/SaleManagerMdbSlave.h"
#include "lib/sale_manager/mdb_master/include/SaleManagerMdbMaster.h"
#include "lib/sale_manager/exe_slave/include/SaleManagerExeSlave.h"
#include "lib/sale_manager/exe_master/include/SaleManagerExeMaster.h"
#include "lib/sale_manager/cci_franke/include/SaleManagerCciFranke.h"
#include "lib/sale_manager/order/include/SaleManagerOrderCciT3.h"
#include "lib/sale_manager/order/include/SaleManagerOrderSpire.h"
#include "lib/client/ClientDeviceNeftemag.h"
#include "lib/network/include/Network.h"
#include "lib/network/NetworkUart.h"
#include "lib/network/NetworkConn.h"
#include "lib/ecp/EcpAgent.h"
#include "lib/erp/ErpAgent.h"
#include "lib/erp/ErpCommandProcessor.h"
#include "lib/utils/stm32/buttons.h"
#include "lib/utils/stm32/Beeper.h"
#include "lib/fiscal_register/orange_data/include/OrangeData.h"
#include "lib/fiscal_register/chekonline/include/ChekOnline.h"
#include "lib/fiscal_register/nanokassa/include/Nanokassa.h"
#include "lib/fiscal_register/ephor/include/Ephor.h"
#include "lib/rfid/RfidCashless.h"
#ifdef DEVICE_USB
#include "lib/usb/UsbUart.h"
#else
#ifdef DEVICE_USB2
#include "lib/usb/Usb2Engine.h"
#endif
#endif

#include "common/cardreader/inpas/include/Inpas.h"
#include "common/cardreader/sberbank/include/Sberbank.h"
#include "common/cardreader/vendotek/include/Vendotek.h"
#include "common/cardreader/twocan/include/Twocan.h"
#include "common/mdb/MdbBridge.h"
#include "common/sim900/include/GsmDriver.h"
#include "common/uart/stm32/include/uart.h"
#include "common/timer/include/TimerEngine.h"
#include "common/timer/stm32/include/RealTimeControl.h"
#include "common/event/include/EventEngine.h"
#include "common/beeper/include/Gramophone.h"
#include "common/fiscal_register/FiscalTest.h"
#include "common/fiscal_register/shtrih-m/include/ShtrihM.h"
#include "common/fiscal_register/atol/include/Atol.h"
#include "common/fiscal_register/rp_system_1fa/include/RpSystem1Fa.h"
#include "common/fiscal_register/terminal_fa/include/TerminalFa.h"
#include "common/utils/include/Utils.h"
#include "common/utils/stm32/include/Led.h"
#include "common/code_scanner/CodeScanner.h"
#include "common/code_scanner/unitex1/UnitexScanner.h"
#include "common/utils/include/Version.h"
#include "common/logger/include/Logger.h"

//+++ todo: Удалить
#include "common/mdb/master/coin_changer/MdbMasterCoinChanger.h"
//+++

Modem::Modem(
	ConfigModem *config,
	TimerEngine *timers,
	EventEngine *eventEngine,
	Gsm::Driver *gsm,
	Buttons *buttons,
	Uart *slaveUart,
	Uart *masterUart,
	Uart *dexUart,
	Uart *kktUart,
	Network *network,
	RealTimeInterface *realtime
) :
	timerEngine(timers),
	eventEngine(eventEngine),
	gsm(gsm),
	buttons(buttons),
	slaveUart(slaveUart),
	masterUart(masterUart),
	dexUart(dexUart),
	ext1Uart(kktUart),
	fiscalUart(NULL),
	fiscalConn(NULL),
	network(network),
	realtime(realtime),
	state(State_Idle),
	screen(NULL),
	loyalityScreen(NULL),
	rfid(NULL),
	fiscalRegister(NULL),
	fiscalManager(NULL),
	scanner(NULL),
	qrCodeStack(NULL),
	externCashless(NULL),
	verification(NULL),
	saleManager(NULL),
	eventRegistrar(NULL),
	ecpAgent(NULL),
	erpAgent(NULL),
	erpCommandProcessor(NULL),
	melodyButton1(NULL),
	melodyButton2(NULL),
	melodyButton3(NULL),
	gramophone(NULL),
	leds(NULL),
	powerAgent(NULL),
	relay(NULL)
{
	this->config = new ConfigMaster(config);
	this->client = new ClientContext;
	this->buttons->setObserver(this);
	this->qrCodeStack = new QrCodeStack;
	this->externCashless = new MdbMasterCashlessStack;
}

Modem::~Modem() {
	if(relay != NULL) { delete relay; }
	if(powerAgent != NULL) { delete powerAgent; }
	if(erpAgent != NULL) { delete erpAgent; }
	if(ecpAgent != NULL) { delete ecpAgent; }
	if(eventRegistrar != NULL) { delete eventRegistrar; }
	if(saleManager != NULL) { delete saleManager; }
	if(externCashless != NULL) { delete externCashless; }
	if(fiscalManager != NULL) { delete fiscalManager; }
	if(fiscalRegister != NULL) { delete fiscalRegister; }
	if(fiscalConn != NULL) { delete fiscalConn; }
	if(fiscalUart != NULL) { delete fiscalUart; }
	delete qrCodeStack;
	delete client;
	delete config;
}

void Modem::init(Reboot::Reason rebootReason) {
	this->rebootReason = rebootReason;
	initBattery();
	initRelay();
	initLeds();
	initExt1();
	initExt2();
	initUsb();
	initDex();
	initGsm();
	initErpAgent();
	initFiscalRegister();
	initEventRegistrar();
	initSaleManager();
	initFirmwareState();
	registerRebootReason();
}

void Modem::reset() {
	if(saleManager != NULL) { saleManager->reset(); }
	state = State_Work;
}

void Modem::proc(Event *event) {
	LOG_DEBUG(LOG_MODEM, "event");
	switch(state) {
		case State_Idle: return;
		case State_Work: stateWorkEvent(event); return;
		default: LOG_ERROR(LOG_MODEM, "Unwaited event " << state << "," << event->getType());
	}
}

void Modem::proc(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_MODEM, "proc");
	switch(state) {
		case State_Idle: return;
		case State_Work: stateWorkEnvelope(envelope); return;
		case State_Shutdown: stateShutdownEnvelope(envelope); return;
		default: LOG_ERROR(LOG_MODEM, "Unwaited envelope " << state << "," << envelope->getType());
	}
}

void Modem::initBattery() {
	powerAgent = new PowerAgent(config->getConfig(), timerEngine, eventEngine);
	powerAgent->reset();
	eventEngine->subscribe(this, GlobalId_Battery);
}

void Modem::initRelay() {
	if(config->getConfig()->getAutomat()->getPaymentBus() != ConfigAutomat::PaymentBus_CciT4) {
		relay = new Relay(powerAgent, timerEngine);
	}
}

void Modem::initLeds() {
	this->melodyButton1 = new MelodyButton1;
	this->melodyButton2 = new MelodyButton2;
	this->melodyButton3 = new MelodyButton3;
	this->gramophone = new Gramophone(Beeper::get(), timerEngine);
	this->leds = new ModemLed(Led::get(), timerEngine);
	this->leds->reset();
	LOG_INFO(LOG_MODEM, "Leds OK");
}

void Modem::initExt1() {
	ConfigAutomat *automat = config->getConfig()->getAutomat();
	uint16_t paymentBus = automat->getPaymentBus();
	if(paymentBus == ConfigAutomat::PaymentBus_OrderCciT3) {
		LOG_INFO(LOG_MODEM, "EXT1 DISABLED");
		return;
	}

	switch(automat->getExt1Device()) {
		case ConfigAutomat::Device_Scanner: {
			if(ext1Uart == NULL) {
				scanner = new CodeScannerFake;
				LOG_INFO(LOG_MODEM, "EXT1 fake scanner OK");
			} else {
				ext1Uart->setup(9600, Uart::Parity_None, 0);
				ext1Uart->setTransmitBufferSize(32);
				ext1Uart->setReceiveBufferSize(128);
				scanner = new CodeScanner(automat->getExt1CashlessContext(), ext1Uart, timerEngine);
				LOG_INFO(LOG_MODEM, "EXT1 scanner OK");
			}
#ifdef QRCODE_FREE
			if(automat->getQrType() & ConfigAutomat::QrType_Free) {
				ScannerFree *scannerFree = new ScannerFree(scanner, timerEngine, eventEngine, automat->getMaxCredit());
				externCashless->push(scannerFree);
				LOG_INFO(LOG_MODEM, "QrType Free OK");
			}
#endif
#ifdef QRCODE_UNITEX1
			if(automat->getQrType() & ConfigAutomat::QrType_Unitex1) {
				UnitexScanner *scannerUnitex = new UnitexScanner(scanner, timerEngine, eventEngine, automat->getMaxCredit());
				externCashless->push(scannerUnitex);
				LOG_INFO(LOG_MODEM, "QrType Unitex OK");
			}
#endif
#ifdef QRCODE_NEFTEMAG
			if(automat->getQrType() & ConfigAutomat::QrType_NefteMag) {
				new ClientDeviceNeftemag(client, scanner);
				LOG_INFO(LOG_MODEM, "QrType NefteMag OK");
			}
#endif
#ifdef QRCODE_ERPORDER
			if(automat->getQrType() & ConfigAutomat::QrType_EphorOrder) {
				ErpOrderCashless *scannerEphorOrder = new ErpOrderCashless(config->getConfig(), scanner, network->getTcpConnection4(), timerEngine, eventEngine);
				externCashless->push(scannerEphorOrder);
				LOG_INFO(LOG_MODEM, "QrType ErpOrder OK");
			}
#endif
			return;
		}
		case ConfigAutomat::Device_Inpas: {
			if(ext1Uart == NULL) {
				LOG_INFO(LOG_MODEM, "EXT1 DISABLED");
				return;
			}
			ext1Uart->setup(115200, Uart::Parity_None, 0);
			ext1Uart->setTransmitBufferSize(512);
			ext1Uart->setReceiveBufferSize(512);
			Inpas::Cashless *cashless = new Inpas::Cashless(automat->getExt1CashlessContext(), ext1Uart, network->getTcpConnection2(), timerEngine, eventEngine);
			externCashless->push(cashless);
			verification = cashless;
			if(automat->getFiscalPrinter() & ConfigAutomat::FiscalPrinter_Cashless) { qrCodeStack->push(cashless); }
			LOG_INFO(LOG_MODEM, "EXT1 cashless Inpas OK");
			return;
		}
		case ConfigAutomat::Device_Sberbank: {
			if(ext1Uart == NULL) {
				LOG_INFO(LOG_MODEM, "EXT1 DISABLED");
				return;
			}
			ext1Uart->setup(9600, Uart::Parity_None, 0);
			ext1Uart->setTransmitBufferSize(512);
			ext1Uart->setReceiveBufferSize(512);
			Sberbank::Cashless *cashless = new Sberbank::Cashless(automat->getExt1CashlessContext(), ext1Uart, network->getTcpConnection2(), timerEngine, eventEngine, automat->getMaxCredit());
			externCashless->push(cashless);
			verification = cashless;
			if(automat->getFiscalPrinter() & ConfigAutomat::FiscalPrinter_Cashless) { qrCodeStack->push(cashless); }
			LOG_INFO(LOG_MODEM, "EXT1 cashless Sberbank OK");
			return;
		}
		case ConfigAutomat::Device_Vendotek: {
			if(ext1Uart == NULL) {
				LOG_INFO(LOG_MODEM, "EXT1 DISABLED");
				return;
			}
			ext1Uart->setup(115200, Uart::Parity_None, 0);
			ext1Uart->setTransmitBufferSize(512);
			ext1Uart->setReceiveBufferSize(512);
			Vendotek::Cashless *cashless = new Vendotek::Cashless(automat->getExt1CashlessContext(), ext1Uart, network->getTcpConnection2(), timerEngine, eventEngine, realtime, automat->getMaxCredit());
			externCashless->push(cashless);
			verification = cashless;
			if(automat->getFiscalPrinter() & ConfigAutomat::FiscalPrinter_Cashless) { qrCodeStack->push(cashless); }
			LOG_INFO(LOG_MODEM, "EXT1 cashless Vendotek OK");
			return;
		}
		default: {
			LOG_INFO(LOG_MODEM, "EXT1 DISABLED");
			return;
		}
	}
}

void Modem::initExt2() {
	ConfigAutomat *automat = config->getConfig()->getAutomat();
	if(automat->getExt2Device() == ConfigAutomat::Device_Screen || automat->getFiscalPrinter() & ConfigAutomat::FiscalPrinter_Screen) {
#ifdef EXTERN_SCREEN
		screen = new Screen(automat->getScreenContext(), I2C::get(SCREEN_I2C), timerEngine, eventEngine);
		loyalityScreen = new LoyalityScreen(screen, timerEngine);
		loyalityScreen->reset();
		if(automat->getFiscalPrinter() & ConfigAutomat::FiscalPrinter_Screen) { qrCodeStack->push(loyalityScreen); }
		LOG_INFO(LOG_MODEM, "EXT2 screen OK");
#ifdef EXTERN_RFID
		rfid = new Rfid::Cashless(config->getConfig(), client, network->getTcpConnection3(), realtime, timerEngine, gramophone, loyalityScreen, eventEngine);
		externCashless->push(rfid);
		LOG_INFO(LOG_MODEM, "EXT2 Nfc OK");
#else
		QuickPay::Cashless *quickPay = new QuickPay::Cashless(config->getConfig(), network->getTcpConnection3(), loyalityScreen, timerEngine, eventEngine);
		externCashless->push(quickPay);
		LOG_INFO(LOG_MODEM, "EXT2 QuickPay OK");
#endif
#endif
	} else {
		LOG_INFO(LOG_MODEM, "EXT2 DISABLED");
		return;
	}
}

void Modem::initUsb() {
#ifdef DEVICE_USB
	UsbUart *usbUart = new UsbUart(USB::get());
	ConfigAutomat *automat = config->getConfig()->getAutomat();
	switch(automat->getUsb1Device()) {
		case ConfigAutomat::Device_Inpas: {
			Inpas::Cashless *cashless = new Inpas::Cashless(automat->getUsb1CashlessContext(), usbUart, network->getTcpConnection2(), timerEngine, eventEngine);
			externCashless->push(cashless);
			verification = cashless;
			if(automat->getFiscalPrinter() & ConfigAutomat::FiscalPrinter_Cashless) { qrCodeStack->push(cashless); }
			LOG_INFO(LOG_MODEM, "USB1 cashless Inpas OK");
			return;
		}
		case ConfigAutomat::Device_Sberbank: {
			Sberbank::Cashless *cashless = new Sberbank::Cashless(automat->getUsb1CashlessContext(), usbUart, network->getTcpConnection2(), timerEngine, eventEngine, automat->getMaxCredit());
			externCashless->push(cashless);
			verification = cashless;
			if(automat->getFiscalPrinter() & ConfigAutomat::FiscalPrinter_Cashless) { qrCodeStack->push(cashless); }
			LOG_INFO(LOG_MODEM, "USB1 cashless Sberbank OK");
			return;
		}
		case ConfigAutomat::Device_Twocan: {
			Twocan::Cashless *cashless = new Twocan::Cashless(automat->getUsb1CashlessContext(), usbUart, NULL, timerEngine, eventEngine, automat->getMaxCredit());
			externCashless->push(cashless);
			verification = cashless;
			if(automat->getFiscalPrinter() & ConfigAutomat::FiscalPrinter_Cashless) { qrCodeStack->push(cashless); }
			LOG_INFO(LOG_MODEM, "USB1 cashless Twocan OK");
			return;
		}
		default: {
			LOG_INFO(LOG_MODEM, "USB1 DISABLED");
			return;
		}
	}
#endif
}

void Modem::initDex() {
	uint16_t paymentBus = config->getConfig()->getAutomat()->getPaymentBus();
	if(this->dexUart == NULL || paymentBus == ConfigAutomat::PaymentBus_CciT3 || paymentBus == ConfigAutomat::PaymentBus_CciT4 || paymentBus == ConfigAutomat::PaymentBus_OrderCciT3) {
		LOG_INFO(LOG_MODEM, "DEX DISABLED");
		return;
	}

	dexUart->setup(9600, Uart::Parity_None, 0);
	dexUart->setTransmitBufferSize(256);
	dexUart->setReceiveBufferSize(256);
	ecpAgent = new EcpAgent(config, timerEngine, eventEngine, dexUart, realtime, screen);
	LOG_INFO(LOG_MODEM, "DEX OK");
}

void Modem::initGsm() {
#if 0
	if(this->gsm == NULL) {
		LOG_ERROR(LOG_MODEM, "GSM DISABLED");
		return;
	}

#ifdef GSM_STATIC_IP
	erpCommandProcessor = new ErpCommandProcessor(config->getConfig()->getAutomat()->getRemoteCashlessContext(), eventEngine);
	externCashless->push(erpCommandProcessor->getCashless());
#endif
	gsm->init(erpCommandProcessor);
	Gsm::SignalQuality *signalQuality = new Gsm::SignalQuality(gsm, eventEngine);
	erpAgent = new ErpAgent(config, timerEngine, eventEngine, network, signalQuality, ecpAgent, verification, realtime, relay, powerAgent, leds);
	externCashless->push(erpAgent->getCashless());
	erpAgent->reset();
	leds->setInternet(LedInterface::State_InProgress);
	eventEngine->subscribe(this, GlobalId_Sim900);
	LOG_INFO(LOG_MODEM, "GSM OK");
#else
	if(this->gsm == NULL) {
		LOG_ERROR(LOG_MODEM, "GSM DISABLED");
		return;
	}

#ifdef GSM_STATIC_IP
	erpCommandProcessor = new ErpCommandProcessor(config->getConfig()->getAutomat()->getCL4Context(), eventEngine);
	externCashless->push(erpCommandProcessor->getCashless());
#endif
	gsm->init(erpCommandProcessor);
	leds->setInternet(LedInterface::State_InProgress);
	eventEngine->subscribe(this, GlobalId_Sim900);
	LOG_INFO(LOG_MODEM, "GSM OK");
#endif
}

void Modem::initErpAgent() {
	Gsm::SignalQuality *signalQuality = (gsm == NULL) ? NULL : new Gsm::SignalQuality(gsm, eventEngine);
	erpAgent = new ErpAgent(config, timerEngine, eventEngine, network, signalQuality, ecpAgent, verification, realtime, relay, powerAgent, leds);
	externCashless->push(erpAgent->getCashless());
	erpAgent->reset();
	LOG_INFO(LOG_MODEM, "ERP OK");
}

void Modem::initFiscalRegister() {
	ConfigFiscal *fiscal = config->getConfig()->getFiscal();
	Fiscal::Context *context = config->getConfig()->getAutomat()->getFiscalContext();
	switch(fiscal->getKkt()) {
#ifdef DEVICE_ETHERNET
#ifdef FR_PAYKIOSK01FA
		case ConfigFiscal::Kkt_Paykiosk01FA: {
			fiscalUart = new NetworkUart(timerEngine, network, fiscal->getKktAddr(), fiscal->getKktPort());
			fiscalRegister = new ShtrihM(fiscalUart, timerEngine, eventEngine);
			LOG_INFO(LOG_MODEM, "Fiscal register Paykiosk01FA OK");
			break;
		}
#endif
#ifdef FR_ATOL
		case ConfigFiscal::Kkt_KaznachejFA: {
			fiscalConn = new NetworkConn(timerEngine);
			fiscalRegister = new Atol::FiscalRegister(context, fiscal->getKktAddr(), fiscal->getKktPort(), fiscalConn, timerEngine, eventEngine, leds);
			LOG_INFO(LOG_MODEM, "Fiscal register KaznachejFA OK");
			break;
		}
#endif
#ifdef FR_RPSYSTEMFA
		case ConfigFiscal::Kkt_RPSystem1FA: {
			fiscalUart = new NetworkUart(timerEngine, network, fiscal->getKktAddr(), fiscal->getKktPort());
			fiscalRegister = new RpSystem1Fa::FiscalRegister(fiscal->getKktInterface(), fiscalUart, timerEngine, eventEngine);
			LOG_INFO(LOG_MODEM, "Fiscal register RPSystem1F OK");
			break;
		}
#endif
#ifdef FR_TERMINALFA
		case ConfigFiscal::Kkt_TerminalFA: {
			fiscalUart = new NetworkUart(timerEngine, network, fiscal->getKktAddr(), fiscal->getKktPort());
			fiscalRegister = new TerminalFa::FiscalRegister(fiscalUart, timerEngine, eventEngine);
			LOG_INFO(LOG_MODEM, "Fiscal register TerminalFA OK");
			break;
		}
#endif
#endif
#ifdef FR_ORANGEDATA
		case ConfigFiscal::Kkt_OrangeData:
		case ConfigFiscal::Kkt_OrangeDataEphor: {
			fiscalRegister = new OrangeData::FiscalRegister(config->getConfig(), context, network->getTcpConnection4(), timerEngine, eventEngine, realtime, leds);
			LOG_INFO(LOG_MODEM, "Fiscal register OrangeData OK");
			break;
		}
#endif
#ifdef FR_CHEKONLINE
		case ConfigFiscal::Kkt_ChekOnline: {
			fiscalRegister = new ChekOnline::FiscalRegister(config->getConfig(), context, network->getTcpConnection4(), timerEngine, eventEngine, realtime, leds);
			LOG_INFO(LOG_MODEM, "Fiscal register ChekOnline OK");
			break;
		}
#endif
#ifdef FR_NANOKASSA
		case ConfigFiscal::Kkt_Nanokassa: {
			fiscalRegister = new Nanokassa::FiscalRegister(config->getConfig(), context, network->getTcpConnection4(), timerEngine, eventEngine, realtime, leds);
			LOG_INFO(LOG_MODEM, "Fiscal register Nanokassa OK");
			break;
		}
#endif
#ifdef FR_EPHOR
		case ConfigFiscal::Kkt_EphorOnline:
		case ConfigFiscal::Kkt_ServerOrangeData:
		case ConfigFiscal::Kkt_ServerOrangeDataEphor:
		case ConfigFiscal::Kkt_ServerOdf:
		case ConfigFiscal::Kkt_ServerNanokassa: {
			fiscalRegister = new Ephor::FiscalRegister(config->getConfig(), context, network->getTcpConnection4(), timerEngine, eventEngine, realtime, leds);
			LOG_INFO(LOG_MODEM, "Fiscal register EphorOnline OK");
			break;
		}
#endif
		default: {
			fiscalRegister = new Fiscal::DummyRegister(eventEngine);
			LOG_INFO(LOG_MODEM, "Fiscal register DISABLED");
			break;
		}
	}

	fiscalManager = new Fiscal::Manager(config->getConfig(), timerEngine, eventEngine, fiscalRegister, qrCodeStack);
#if 1
	fiscalTest = new FiscalTest(eventEngine, fiscalRegister);
#else
	fiscalTest = new FiscalTest(eventEngine, fiscalManager);
#endif
	LOG_INFO(LOG_MODEM, "Fiscal manager OK");
}

void Modem::initEventRegistrar() {
	eventRegistrar = new EventRegistrar(config->getConfig(), timerEngine, eventEngine, realtime);
	eventRegistrar->setObserver(this);
	eventRegistrar->reset();
	LOG_INFO(LOG_MODEM, "Event registrar enabled");
}

void Modem::initSaleManager() {
	ConfigAutomat *automat = config->getConfig()->getAutomat();
	switch(automat->getPaymentBus()) {
#ifdef SM_MDB_SLAVE
		case ConfigAutomat::PaymentBus_MdbSlave: {
			saleManager = new SaleManagerMdbSlave(config->getConfig(), timerEngine, eventEngine, slaveUart, masterUart, externCashless, fiscalManager, leds, eventRegistrar, rebootReason);
			LOG_INFO(LOG_MODEM, "MDB-SLAVE sale detector OK");
			return;
		}
#endif
#ifdef SM_MDB_MASTER
		case ConfigAutomat::PaymentBus_MdbMaster: {
			saleManager = new SaleManagerMdbMaster(config->getConfig(), timerEngine, eventEngine, slaveUart, masterUart, externCashless, fiscalManager, leds, eventRegistrar);
			LOG_INFO(LOG_MODEM, "MDB-MASTER sale detector OK");
			return;
		}
#endif
#ifdef SM_MDB_NOCASHLESS
		case ConfigAutomat::PaymentBus_MdbDumb: {
			saleManager = new SaleManagerMdbNoCashless(config->getConfig(), timerEngine, eventEngine, slaveUart, masterUart, externCashless, fiscalManager, leds, eventRegistrar, rebootReason);
			LOG_INFO(LOG_MODEM, "MDB-DUMB sale detector OK");
			return;
		}
#endif
#ifdef SM_EXE_SLAVE
		case ConfigAutomat::PaymentBus_ExeSlave: {
			saleManager = new SaleManagerExeSlave(config->getConfig(), timerEngine, eventEngine, slaveUart, masterUart, fiscalManager);
			LOG_INFO(LOG_MODEM, "EXE-SLAVE sale detector OK");
			return;
		}
#endif
#ifdef SM_EXE_MASTER
		case ConfigAutomat::PaymentBus_ExeMaster: {
			saleManager = new SaleManagerExeMaster(config->getConfig(), client, timerEngine, eventEngine, slaveUart, masterUart, externCashless, fiscalManager, leds, eventRegistrar);
			LOG_INFO(LOG_MODEM, "EXE-MASTER sale detector OK");
			return;
		}
#endif
#ifdef SM_CCI_T3
		case ConfigAutomat::PaymentBus_CciT3: {
			if(dexUart == NULL) { LOG_INFO(LOG_MODEM, "CCI/T3 sale detector need dexUart"); return; }
			saleManager = new SaleManagerCciT3(config->getConfig(), client, timerEngine, eventEngine, dexUart, scanner, network->getTcpConnection5(), externCashless, fiscalManager, leds, eventRegistrar);
			LOG_INFO(LOG_MODEM, "CCI/T3 sale detector OK");
			return;
		}
#endif
#ifdef SM_CCI_T4
		case ConfigAutomat::PaymentBus_CciT4: {
			if(dexUart == NULL) { LOG_INFO(LOG_MODEM, "CCI/T4 sale detector need dexUart"); return; }
			saleManager = new SaleManagerCciT4(config->getConfig(), client, timerEngine, eventEngine, dexUart, externCashless, fiscalManager);
			LOG_INFO(LOG_MODEM, "CCI/T4 sale detector OK");
			return;
		}
#endif
#ifdef SM_CCI_F1
		case ConfigAutomat::PaymentBus_CciF1: {
			if(dexUart == NULL) { LOG_INFO(LOG_MODEM, "CCI/F1 sale detector need dexUart"); return; }
			saleManager = new SaleManagerCciFranke(config->getConfig(), client, timerEngine, eventEngine, dexUart, externCashless, fiscalManager, leds, eventRegistrar);
			LOG_INFO(LOG_MODEM, "CCI/F1 sale detector OK");
			return;
		}
#endif
		case ConfigAutomat::PaymentBus_OrderCciT3: {
			if(dexUart == NULL) { LOG_INFO(LOG_MODEM, "ORDER/CCI/T3 sale detector need dexUart"); return; }
			saleManager = new SaleManagerOrderCciT3(config->getConfig(), client, timerEngine, eventEngine, dexUart, ext1Uart, network->getTcpConnection5(), fiscalManager, leds, eventRegistrar);
			LOG_INFO(LOG_MODEM, "ORDER CCI/T3 sale detector OK");
			return;
		}
		case ConfigAutomat::PaymentBus_OrderSpire: {
			saleManager = new SaleManagerOrderSpire(config->getConfig(), client, timerEngine, eventEngine, slaveUart, masterUart, ext1Uart, network->getTcpConnection5(), fiscalManager, leds, eventRegistrar);
			LOG_INFO(LOG_MODEM, "ORDER Spire sale detector OK");
			return;
		}
		default: {
			saleManager = new DummySaleManager(eventEngine);
			LOG_INFO(LOG_MODEM, "Sale detector DISABLED");
			return;
		}
	}
}

void Modem::initFirmwareState() {
	ConfigBoot *boot = config->getConfig()->getBoot();
	if(boot->getFirmwareState() != ConfigBoot::FirmwareState_None) {
		boot->setFirmwareState(ConfigBoot::FirmwareState_None);
		boot->save();
	}
}

void Modem::registerRebootReason() {
	eventEngine->subscribe(this, GlobalId_System);
	eventEngine->subscribe(this, GlobalId_SaleManager);
	if(rebootReason == Reboot::Reason_PowerDown) {
		config->getConfig()->getEvents()->add(ConfigEvent::Type_PowerUp);
		return;
	} else {
		StringBuilder value;
		value << rebootReason;
		config->getConfig()->getEvents()->add(ConfigEvent::Type_ModemReboot, value.getString());
		return;
	}
}

void Modem::stateWorkEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "stateWorkEvent");
	switch(event->getType()) {
		case EventRegistrar::Event_CriticalError: {
			LOG_INFO(LOG_MODEM, "Critical error detected");
			if(erpAgent != NULL) { erpAgent->syncEvents(); }
			return;
		}
		case Buttons::Event_Button1: {
			if(event->getUint8() == true) {
				gramophone->play(melodyButton1);
				if(erpAgent != NULL) { erpAgent->sendAudit(); }
			}
			return;
		}
		case Buttons::Event_Button2: {
			if(event->getUint8() == true) {
				gramophone->play(melodyButton2);
#ifdef PRODUCTION
				if(erpAgent != NULL) { erpAgent->syncEvents(); }
#else
				if(erpAgent != NULL) { erpAgent->syncEvents(); }
//				if(erpAgent != NULL) { erpAgent->ping(); }
//				externCashless->enable();
#endif
			}
			return;
		}
		case Buttons::Event_Button3: {
			if(event->getUint8() == true) {
				gramophone->play(melodyButton3);
#ifdef LOG2FIFO
				if(erpAgent != NULL) { erpAgent->sendDebug(); }
#endif
#ifdef REMOTE_LOGGING
				if(erpAgent != NULL) { erpAgent->sendDebug(); }
#else
#ifdef PRODUCTION
#ifdef EXTERN_RFID
				rfid->service();
#endif
#else
//				if(rfid != NULL) { rfid->service(); }
//				if(erpAgent != NULL) { erpAgent->loadConfig(); }
//				if(scanner != NULL) { LOG("SCANNER AZAZA"); scanner->test(); }
//				testFiscal();

				//USBH_StatusTypeDef  USBH_ReEnumerate   (USBH_HandleTypeDef *phost);

				// USB::get()->reload();

				testCashlessPayment();

//				testToken();
//				erpAgent->powerDown();
//				fiscalTest->test();
//				ecpAgent->loadAudit(NULL);
/*
				if(loyalityScreen == NULL) { LOG("loyalityScreen not inited"); }
				static int step = 0;
				if(step == 0) {	loyalityScreen->drawQrCode("Кассовый чек", "Продукт", "t=20170216T133600&s=780.00&fn=9999078900001327&i=91&fp=1697013557&n=1"); step = 1; }
				else if(step == 1) { loyalityScreen->drawImage(); step = 2; }
				else if(step == 2) { loyalityScreen->clear(); step = 3; }
				else if(step == 3) { loyalityScreen->drawText("Здравствуйте\n\nВасилий\n\nПетрович", 2, 0xFFFF, 0); step = 4; }
				else if(step > 3) { loyalityScreen->drawProgress("Проверка карты"); step = 0; }
*/
//				externCashless->closeSession();
//				while(DebugLogger::get()->isEmptyBuf() == false) {
//					*Logger::get() << DebugLogger::get()->readBuf();
//				}
//				if(neftmCashless != NULL) { neftmCashless->test(); }
#endif
#endif
			}
			return;
		}
	}
}

void Modem::testCashlessPayment() {
	externCashless->sale(16, 100, "TestPayment", 0);
}

void Modem::testFiscal() {
	Fiscal::Sale sale;
	sale.datetime.year = 19;
	sale.datetime.month = 4;
	sale.datetime.day = 20;
	sale.datetime.hour = 10;
	sale.datetime.minute = 20;
	sale.datetime.second = 30;
	sale.addProduct("123", 0, "Test", 2500, Fiscal::TaxRate_NDSNone, 1);
	sale.device.set("CA");
	sale.priceList = 0;
	sale.paymentType = Fiscal::Payment_Cash;
	sale.credit = 2500;
	sale.taxSystem = Fiscal::TaxSystem_ENVD;
	sale.fiscalRegister = 11111;
	sale.fiscalStorage = 22222;
	sale.fiscalDocument = 33333;
	sale.fiscalSign = 44444;
	fiscalManager->sale(&sale, 2);
}

void Modem::testQrCode() {
	qrCodeStack->drawQrCode("QR-code", "Тесточино1", "t=20190128T2004&s=600.00&fn=8710000101920854&i=3823&fp=3145911982&n=1");
}

void Modem::testToken() {
	MdbMasterCoinChanger::EventCoin eventCoin(0);
	eventCoin.set(MdbMasterCoinChanger::Event_DepositeToken, 0, 0, 0, 0);
	eventEngine->transmit(&eventCoin);
}

void Modem::stateWorkEnvelope(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_MODEM, "stateWorkEnvelope");
	switch(envelope->getType()) {
	case SystemEvent_Reboot: stateWorkEventReboot(); return;
	case Gsm::Driver::Event_NetworkUp: {
		LOG_INFO(LOG_MODEM, "NetworkUp");
		leds->setInternet(LedInterface::State_Success);
		return;
	}
	case Gsm::Driver::Event_NetworkDown: {
		LOG_INFO(LOG_MODEM, "NetworkDown");
		leds->setInternet(LedInterface::State_InProgress);
		return;
	}
	case PowerAgent::Event_PowerDown: {
		if(erpAgent != NULL) { erpAgent->powerDown(); }
		return;
	}
	}
}

void Modem::stateWorkEventReboot() {
	LOG_INFO(LOG_MODEM, "stateWorkEventReboot");
	saleManager->shutdown();
	state = State_Shutdown;
}

void Modem::stateShutdownEnvelope(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_MODEM, "stateShutdownEnvelope");
	switch(envelope->getType()) {
	case SaleManager::Event_Shutdown: stateShutdownEventSaleManagerShutdown(); return;
	}
}

void Modem::stateShutdownEventSaleManagerShutdown() {
	LOG_INFO(LOG_MODEM, "stateShutdownEventSaleManagerShutdown");
	Reboot::reboot();
}
