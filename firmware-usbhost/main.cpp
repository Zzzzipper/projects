#include <lib/utils/stm32/Reboot.h>
#include <logger/RemoteLogger.h>
#include "config.h"

#include "lib/modem/include/Modem.h"
#include "lib/adc/Adc.h"
#include "lib/utils/stm32/buttons.h"
#include "lib/utils/stm32/WatchDog.h"
#include "lib/network/include/Network.h"
#include "lib/sd/SD.h"
#include "lib/sd/LogTargetSD.h"
#include "lib/utils/stm32/HardwareUartForwardController.h"
#include "lib/adc/Adc.h"
#ifdef DEVICE_USB
#include "lib/usb/USB.h"
#else
#ifdef DEVICE_USB2
#include "lib/usb/Usb2Engine.h"
#endif
#endif

#include "common/common.h"
#include "common/platform/include/platform.h"
#include "common/utils/include/Version.h"
#include "common/sim900/include/GsmDriver.h"
#include "common/sim900/command/GsmCommandParser.h"
#include "common/uart/stm32/include/uart.h"
#include "common/timer/include/TimerEngine.h"
#include "common/timer/stm32/include/TimeMeter.h"
#include "common/event/include/EventEngine.h"
#include "common/ticker/include/Ticker.h"
#include "common/firmware/include/Firmware.h"
#include "common/i2c/I2C.h"
#include "common/memory/stm32/include/ExternalEeprom.h"
#include "common/memory/include/RamMemory.h"
#include "common/memory/include/MallocTest.h"
#include "common/timer/stm32/include/RealTimeControl.h"
#include "common/config/include/ConfigModem.h"
#include "common/config/include/ConfigRepairer.h"
#include "common/tcpip/include/TcpIpUtils.h"
#include "common/utils/stm32/include/Led.h"
#include "common/logger/include/LogTargetUart.h"
#include "common/logger/include/LogTargetList.h"
#include "common/logger/include/LogTargetEcho.h"
#include "common/logger/include/Logger.h"

#include "test/Tests.h"
#include "test/FastPsi.h"
#include "test/Psi.h"
#include "test/Terminal.h"
#include "defines.h"
#include "Hardware.h"

// Механизм для вывода данных через метод printf(). Смотри syscalls.c
extern "C" void _logPutChar(char c)
{
	(*Logger::get()) << c;
}

void InitRele() {
#if (HW_VERSION >= HW_3_0_2)
	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_OType = GPIO_OType_OD;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_Pin = RELE1_PIN;

	GPIO_Init(RELE1_PORT, &gpio);
	RELE1_OFF;
#endif
}

static uint8_t logTxBuffer[15192] __attribute__ ((section (".ccmram")));
static uint8_t logRxBuffer[256]  __attribute__ ((section (".ccmram")));
#ifdef LOG_ECHO
static uint8_t logEchoBuffer[768]  __attribute__ ((section (".ccmram")));
#endif
static uint8_t gsmTxBuffer[4096] __attribute__ ((section (".ccmram")));
static uint8_t gsmRxBuffer[4096] __attribute__ ((section (".ccmram")));
static uint8_t usbResetCounter[1] __attribute__ ((section (".ccmram")));

void productionMain(void) {
	Reboot::Reason lastReset = Reboot::getLastRebootReason();

	SystemCoreClockUpdate();

#ifndef DEBUG
	// Включаем WD
	WD_Init(7000);
#endif

#ifdef DEBUG_MIM
	MIM_START(5000);
#endif

	// Инициализация реле
	InitRele();

	// Инициализация аппаратного проброса данных UART.
	HardwareUartForwardController::init();

	// Часы реального времени
	RealTimeStm32 *realtime = new RealTimeStm32;
	realtime->init();

	// Инициализация отладочного UART
#ifdef LOGGING
	LogTargetList *logTargets = new LogTargetList;
	Logger::get()->registerTarget(logTargets);
	Logger::get()->registerRealTime(realtime);
#ifdef LOG2DEX
	Uart *debugUart = Uart::get(DEX_UART);
	debugUart->setup(115200, Uart::Parity_None, 0);
	debugUart->setTransmitBuffer(logTxBuffer, sizeof(logTxBuffer));
	debugUart->setReceiveBuffer(logRxBuffer, sizeof(logRxBuffer));
	debugUart->clear();
	logTargets->add(new LogTargetUart(debugUart));
#else
#ifndef EXTERN_CASHLESS
	Uart *debugUart = Uart::get(DEBUG_UART);
	debugUart->setup(256000, Uart::Parity_None, 0);
	debugUart->setTransmitBuffer(logTxBuffer, sizeof(logTxBuffer));
	debugUart->setReceiveBuffer(logRxBuffer, sizeof(logRxBuffer));
	debugUart->clear();
	logTargets->add(new LogTargetUart(debugUart));
#endif
#endif
#ifdef LOG2FIFO
	logTargets->add(RemoteLogger::get());
#endif
#ifdef LOG_ECHO
	LOG("===== LOG_ECHO START =====");
	LOG_STR(logEchoBuffer, sizeof(logEchoBuffer));
	LOG("===== LOG_ECHO END =====");
	logTargets->add(new LogTargetEcho(logEchoBuffer, sizeof(logEchoBuffer)));
#endif
#endif
#ifdef REMOTE_LOGGING
	RemoteLogger::get()->registerRealTime(realtime);
#endif
	LOG_MEMORY_USAGE("start");

	// Настраиваем движок таймеров на аппаратном таймере 4
	TimerEngine *timerEngine = new TimerEngine();
	Ticker *ticker = Ticker::get();
	ticker->registerConsumer(timerEngine);

	// Инициализация кнопок
	Buttons *buttons = new Buttons();
	buttons->init(timerEngine);

	// Инициализация модуля ADC
	Adc::get();

	// Инициализация USB
#ifdef DEVICE_USB
	USB *usb = USB::get();
	usb->init(timerEngine);
	usb->setResetCounterBuf(usbResetCounter);
#else
#ifdef DEVICE_USB2
	Usb2Engine *usbEngine = Usb2Engine::get();
	usbEngine->init(timerEngine);
#endif
#endif

	// Движок событий
	EventEngine *eventEngine = new EventEngine(EVENT_POOL_SIZE, EVENT_DATA_SIZE, EVENT_SUB_NUMBER);

	// Инициализация внешнего I2C для работы с SCREEN RFID
	I2C *i2c_screen = new I2C(SCREEN_I2C, I2C_SCREEN_SPEED);
	i2c_screen->setRetryPause(I2C_SCREEN_RETRY_PAUSE);
//	UNUSED(i2c_screen);

	// Загрузка конфигурации
	StatStorage *stat = new StatStorage;
	I2C *i2c = I2C::get(I2C_3);
	i2c->setStatStorage(stat);
	ExternalEeprom *memory = new ExternalEeprom(i2c, EE_ADDRESS, EE_MAX_SIZE, EE_PAGE_SIZE);
	ConfigModem *config = new ConfigModem(memory, realtime, stat);
#if 1
	MemoryResult result = config->load();
	if(result != MemoryResult_Ok) {
		if(result == MemoryResult_ReadError || result == MemoryResult_WriteError) {
			LOG_ERROR(LOG_MODEM, "I2C fatal error");
			while(1);
			return;
		} else {
			LOG_ERROR(LOG_MODEM, "Config load failed");
			ConfigRepairer repairer(config, memory);
			if(repairer.repair() != MemoryResult_Ok) {
				LOG_ERROR(LOG_MODEM, "Config repair failed");
				Reboot::reboot();
				return;
			}
		}
	}
#else
	config->init();
#endif

//+++++
//	config->getAutomat()->setPaymentBus(ConfigAutomat::PaymentBus_CciF1);
//	config->getAutomat()->setPaymentBus(ConfigAutomat::PaymentBus_OrderSpire);
//	config->getAutomat()->setPaymentBus(ConfigAutomat::PaymentBus_OrderCciT3);
//	config->getAutomat()->setPaymentBus(ConfigAutomat::PaymentBus_CciT4);
//	config->getAutomat()->setInternetDevice(ConfigAutomat::InternetDevice_Ethernet);
//	config->getAutomat()->setCashlessMaxLevel(Mdb::FeatureLevel_3);
#if 0
	config->getAutomat()->setEthMac(0x20, 0x89, 0x84, 0x6A, 0x96, 0x01);
	config->getAutomat()->setEthAddr(IPADDR4TO1(192,168,2,128));
	config->getAutomat()->setEthMask(IPADDR4TO1(255,255,255,0));
	config->getAutomat()->setEthGateway(IPADDR4TO1(192,168,2,1));
#elif 0
	config->getAutomat()->setEthMac(0x20, 0x89, 0x84, 0x6A, 0x96, 0x02);
	config->getAutomat()->setEthAddr(IPADDR4TO1(10,77,1,18));
	config->getAutomat()->setEthMask(IPADDR4TO1(255,255,255,248));
	config->getAutomat()->setEthGateway(IPADDR4TO1(10,77,1,17));
#elif 0
	config->getAutomat()->setEthMac(0x20, 0x89, 0x84, 0x6A, 0x96, 0x03);
	config->getAutomat()->setEthAddr(IPADDR4TO1(10,77,1,19));
	config->getAutomat()->setEthMask(IPADDR4TO1(255,255,255,248));
	config->getAutomat()->setEthGateway(IPADDR4TO1(10,77,1,17));
#elif 0 // spire1
	config->getAutomat()->setEthMac(0x20, 0x89, 0x84, 0x6A, 0x96, 0x04);
	config->getAutomat()->setEthAddr(IPADDR4TO1(10,77,1,20));
	config->getAutomat()->setEthMask(IPADDR4TO1(255,255,255,248));
	config->getAutomat()->setEthGateway(IPADDR4TO1(10,77,1,17));
#endif
//+++++
	config->getBoot()->setHardwareVersion((uint32_t)&HardwareVersion);
	config->getBoot()->setFirmwareVersion((uint32_t)&SoftwareVersion);
#ifdef ALWAYS_IDLE
	config->getAutomat()->setCashlessMaxLevel(Mdb::FeatureLevel_3);
#endif

#ifdef DEBUG
	#ifdef ERP_SSL_OFF
		config->getBoot()->setServerDomain("dev5.ephor.online");
		config->getBoot()->setServerPort(80);
	#else
		config->getBoot()->setServerDomain("dev1.ephor.online");
		config->getBoot()->setServerPort(443);
	#endif // ERP_SSL_OFF
//	config->getAutomat()->setInternetDevice(ConfigAutomat::InternetDevice_Gsm);
//	config->getAutomat()->setInternetDevice(ConfigAutomat::InternetDevice_Ethernet);
//	config->getBoot()->setGprsApn("FixedIP.nw");
//	config->getAutomat()->setCategoryMoney(true);
//	config->getAutomat()->setPaymentBus(ConfigAutomat::PaymentBus_MdbSlave);
//	config->getAutomat()->setPaymentBus(ConfigAutomat::PaymentBus_MdbMaster);
//	config->getAutomat()->setPaymentBus(ConfigAutomat::PaymentBus_ExeMaster);
//	config->getAutomat()->setPaymentBus(ConfigAutomat::PaymentBus_ExeSlave);
//	config->getAutomat()->setPaymentBus(ConfigAutomat::PaymentBus_CciT3);
//	config->getAutomat()->setPaymentBus(ConfigAutomat::PaymentBus_CciT4);
//	config->getAutomat()->setPaymentBus(ConfigAutomat::PaymentBus_CciF1);
//	config->getAutomat()->setPaymentBus(ConfigAutomat::PaymentBus_OrderCciT3);
//	config->getAutomat()->setPaymentBus(ConfigAutomat::PaymentBus_MdbDumb);
//	config->getAutomat()->setEvadts(true);
//	config->getAutomat()->setCashlessNumber(1);
//	config->getAutomat()->setEvadts(true);
//	config->getAutomat()->setPriceHolding(true);
//	config->getAutomat()->setCreditHolding(false);
//	config->getAutomat()->setMultiVend(false);
//	config->getAutomat()->setMaxCredit(25000);
//	config->getAutomat()->setExt1Device(ConfigAutomat::Device_Inpas);
//	config->getAutomat()->setExt1Device(ConfigAutomat::Device_Sberbank);
//	config->getAutomat()->setExt1Device(ConfigAutomat::Device_Vendotek);
//	config->getAutomat()->setExt1Device(ConfigAutomat::Device_Scanner);
	config->getAutomat()->setUsb1Device(ConfigAutomat::Device_Twocan);
//	config->getAutomat()->setQrType(ConfigAutomat::QrType_Unitex1);
//	config->getAutomat()->setExt2Device(ConfigAutomat::Device_Screen);
#else // DEBUG
	#ifdef DEBUG2
		#ifdef ERP_SSL_OFF
			config->getBoot()->setServerDomain("dev2.ephor.online");
			config->getBoot()->setServerPort(80);
		#else
			config->getBoot()->setServerDomain("dev2.ephor.online");
			config->getBoot()->setServerPort(443);
		#endif // ERP_SSL_OFF
	#elif defined(DEBUG3) // DEBUG2
		#ifdef ERP_SSL_OFF
			config->getBoot()->setServerDomain("dev3.ephor.online");
			config->getBoot()->setServerPort(80);
		#else
			config->getBoot()->setServerDomain("dev3.ephor.online");
			config->getBoot()->setServerPort(443);
		#endif // ERP_SSL_OFF
	#elif defined(DEBUG4) // DEBUG2
		#ifdef ERP_SSL_OFF
			config->getBoot()->setServerDomain("dev4.ephor.online");
			config->getBoot()->setServerPort(80);
		#else
			config->getBoot()->setServerDomain("dev4.ephor.online");
			config->getBoot()->setServerPort(443);
		#endif // ERP_SSL_OFF
	#elif defined(DEBUG5) // DEBUG2
		#ifdef ERP_SSL_OFF
			config->getBoot()->setServerDomain("dev5.ephor.online");
			config->getBoot()->setServerPort(80);
		#else
			config->getBoot()->setServerDomain("dev5.ephor.online");
			config->getBoot()->setServerPort(443);
		#endif // ERP_SSL_OFF
	#elif defined(DEBUG6) // DEBUG2
		#ifdef ERP_SSL_OFF
			config->getBoot()->setServerDomain("dev6.ephor.online");
			config->getBoot()->setServerPort(80);
		#else
			config->getBoot()->setServerDomain("dev6.ephor.online");
			config->getBoot()->setServerPort(443);
		#endif // ERP_SSL_OFF
	#else
		#ifdef ERP_SSL_OFF
			config->getBoot()->setServerDomain("api.ephor.online");
			config->getBoot()->setServerPort(80);
		#else
			config->getBoot()->setServerDomain("erp.ephor.online");
			config->getBoot()->setServerPort(443);
		#endif // ERP_SSL_OFF
	#endif // DEBUG2

	config->getAutomat()->setUsb1Device(ConfigAutomat::Device_Twocan);

#endif // DEBUG

	// Инициализация модема
	Gsm::Driver *gsm = NULL;
#ifdef DEVICE_GSM
	Uart *gsmUart = Uart::get(SIM900_UART);
	if(config->getAutomat()->getInternetDevice() == ConfigAutomat::InternetDevice_Gsm) {
		Gsm::Hardware *hardware = new Gsm::Hardware;
		gsmUart->setup(115200, Uart::Parity_None, 0);
		gsmUart->setTransmitBuffer(gsmTxBuffer, sizeof(gsmTxBuffer));
		gsmUart->setReceiveBuffer(gsmRxBuffer, sizeof(gsmRxBuffer));
		Gsm::CommandParser *gsmParser = new Gsm::CommandParser(gsmUart, timerEngine);
		gsm = new Gsm::Driver(config->getBoot(), hardware, gsmParser, timerEngine, eventEngine, stat);
	}
#endif

	// Порты
	Uart *slaveUart = Uart::get(MDB_UART);
	Uart *masterUart = Uart::get(MDB_MASTER_UART);
#ifdef EXTERN_CASHLESS
#ifdef LOG2DEX
	Uart *dexUart = NULL;
	Uart *ext1Uart = Uart::get(DEBUG_UART);
#else
	Uart *dexUart = Uart::get(DEX_UART);
	Uart *ext1Uart = Uart::get(DEBUG_UART);
#endif
#else
#ifdef LOG2DEX
	Uart *dexUart = NULL;
	Uart *ext1Uart = NULL;
#else
	Uart *dexUart = Uart::get(DEX_UART);
	Uart *ext1Uart = NULL;
#endif
#endif

	// Сеть
	Network *network = new Network(config, gsm, timerEngine);

	// Инициализация главного автомата
	Modem *modem = new Modem(config, timerEngine, eventEngine, gsm, buttons, slaveUart, masterUart, dexUart, ext1Uart, network, realtime);
	modem->init(lastReset);

	__enable_irq();
#if 0 // SD-карта отключена, так как блокированная запись вносит слишком большие задержки
	logTargets->add(new LogTargetSD);
#endif
	*(Logger::get()) << "Firmware: hardware " << LOG_VERSION((uint32_t)&HardwareVersion) << ", software " << LOG_VERSION((uint32_t)&SoftwareVersion) << Logger::endl;
	if(network != NULL) { network->init(); } // todo: переработать, чтобы была возможность использовать до включения прерываний
	modem->reset();

	LOG_MEMORY_USAGE("loop");
#ifdef DEBUG_MEMORY
	MallocPrintInfo();
#endif
#ifdef DEBUG_MIM
	MIM_CHECK();
#endif
	while(1) {
#ifdef DEBUG_MIM
		MIM_START(5000);
#endif
		IWDG_ReloadCounter();
		timerEngine->execute();
		eventEngine->execute();
#ifdef DEVICE_GSM
		gsmUart->execute();
#endif
		slaveUart->execute();
		masterUart->execute();
#ifdef EXTERN_CASHLESS
#ifdef LOG2DEX
		debugUart->execute();
		ext1Uart->execute();
#else
		dexUart->execute();
		ext1Uart->execute();
#endif
#else
#ifdef LOG2DEX
		debugUart->execute();
#else
		debugUart->execute();
		dexUart->execute();
#endif
#endif
#ifdef DEVICE_ETHERNET
		network->execute();
#endif
#ifdef DEVICE_USB
		usb->execute();
#else
#ifdef DEVICE_USB
		usbEngine->execute();
#endif
#endif
#ifdef DEBUG_MIM
		MIM_CHECK();
#endif
	}
}

int main(void) {
	{
		// Отключаем все прерывания, которые остались включенные после работы загрузчика.
		NVIC->ICER[0] = -1;
		NVIC->ICER[1] = -1;
		NVIC->ICER[2] = -1;
	}

	// Настраиваем работу регистров атомарного блока.
	{
		ATOMIC_BLOCK_CUSTOM_CLEAR_ALL_IRQ();
		// ATOMIC_BLOCK_CUSTOM_REGISTER_EXCEPT_IRQ(...);
	}

	GPIO_DeInit(GPIOA);
	GPIO_DeInit(GPIOB);
	GPIO_DeInit(GPIOC);

	NVIC_PriorityGroupConfig(IRQ_NVIC_PRIORITY_GROUP);

	Logger::get()->setRamOffset(0x20000000);
	Logger::get()->setRamSize(0x00020000);

	// Включаем CRC
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);

#if defined(PSI)
	#if defined(PSI_FAST_MODE)
		fastPsiMain();
	#else
		psiMain();
	#endif

#elif defined(TEST)
	testMain();
#elif defined(TERMINAL)
	terminalMain();
#else
	productionMain();
#endif
}

#ifdef  USE_FULL_ASSERT
extern "C" void assert_failed_handler(uint8_t* file, uint32_t line) {
	String str;
	str << "Неправильные параметры\t(" << (char *) file << ":" << (int) line << ")" << UartLogger::endl();
	Uart::get(DEBUG_UART)->send(str);
	while(1) {}
}
#endif

static int delay_value;

void Error_Handler(int index)
{
	int cnt = 16800000;
	while(1)
	{
		Led::get()->setLed(index, 1, 0);

		for (int i = 0; i < cnt; i++)
			delay_value += i;

		Led::get()->setLed(index, 0, 0);

		for (int i = 0; i < cnt; i++)
			delay_value += i;
	}
}

extern "C" void HardFault_Handler(void)
{
	Error_Handler(0);
}

extern "C" void MemManage_Handler(void)
{
	Error_Handler(1);
}

extern "C" void BusFault_Handler(void)
{
	Error_Handler(2);
}

extern "C" void UsageFault_Handler(void)
{
	Error_Handler(3);
}



