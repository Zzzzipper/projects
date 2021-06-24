#include <stdio.h>

#include "config.h"
#include "cmsis_boot/stm32f4xx_conf.h"

#if defined(TERMINAL)
#include "Terminal.h"
#include "Tests.h"
#include "Hardware.h"

#include "extern/qrcodegen/qrcodegen.h"

#include "lib/utils/stm32/buttons.h"
#include "lib/utils/stm32/WatchDog.h"
#include "lib/utils/stm32/Beeper.h"
#include "lib/adc/Adc.h"
#include "lib/mma7660/MMA7660.h"
#include "lib/sd/SD.h"
#include "lib/rfid/mfrc532/rfid.h"
#include "lib/utils/stm32/HardwareUartForwardController.h"
#include "lib/usb/USB.h"
#include "lib/usb/UsbUart.h"
#include "lib/battery/Battery.h"
#include "lib/utils/stm32/Random.h"

#include "common/sim900/GsmHardware.h"
#include "common/timer/stm32/include/SystemTimer.h"
#include "common/timer/stm32/include/RealTimeControl.h"
#include "common/timer/include/TimerEngine.h"
#include "common/ticker/include/Ticker.h"
#include "common/logger/include/Logger.h"
#include "common/logger/include/LogTargetUart.h"
#include "common/utils/include/StringBuilder.h"
#include "common/utils/include/Version.h"
#include "common/logger/arm/rtt/RttLogTarget.h"
#include "common/memory/stm32/include/ExternalEeprom.h"
#include "common/i2c/I2C.h"
#include "common/utils/stm32/include/Led.h"
#include "common/beeper/include/Gramophone.h"
#include "common/platform/include/platform.h"
#include "common/config/include/StatStorage.h"
#include "common/config/v1/boot/Config1Boot.h"

#define TIM5_PERIOD_MKS	5

extern void InitRele();
extern void InitBattery();

class FakeTcpIp : public TcpIp {
public:
	void setObserver(EventObserver *observer) {}
	bool connect(const char *domainname, uint16_t port, Mode mode) { return false; }
	bool hasRecvData() { return false; }
	bool send(const uint8_t *data, uint32_t len) { return false; }
	bool recv(uint8_t *buf, uint32_t bufSize) { return false; }
	void close() {}
};

Gsm::Hardware hardware;
FakeTcpIp fakeTcpIp;
RealTimeStm32 *realtime;
TimerEngine *timerEngine;
EventEngine *eventEngine;
UsbUart *usbUart;
Melody *melody;
Gramophone *gramophone;
StatStorage *statStorage;

volatile int monitorCounter;

TerminalReceiveHandler::TerminalReceiveHandler(Uart *uart, TimerEngine *timerEngine) :
	UartReceiveHandler(1),
	uart(uart),
	stream(uart),
	sim900Gateway(false),
	echo(false),
	screenI2C(false),
	eepromTest(timerEngine)
{
	screen = NULL;
	cashless = NULL;
}


void TerminalReceiveHandler::testMonitor()
{
	if (monitorCounter)  monitorCounter++;
	ATOMIC_BLOCK_CUSTOM_CLEAR_ALL_IRQ();
	ATOMIC_BLOCK_CUSTOM_REGISTER_EXCEPT_IRQ(EXTI9_5_IRQn);

	ATOMIC
	{
	monitorCounter++;
	monitorCounter = monitorCounter * 2;
	if (monitorCounter > 0)
	{
		monitorCounter++;
		uart->send('b');
	}
	}
	monitorCounter--;
}

void TerminalReceiveHandler::handle()
{
	uint8_t key = uart->receive();

	if(echo == true)
	{
		LOG(">>" << key);
	}

	if (sim900Gateway && key != '$')
	{
		Uart::get(SIM900_UART)->send(key);
		return;
	}

	if (screenI2C && key != 'l')
	{
		runScreenTest(key);
		return;
	}

	switch(key)
	{
		case '~': echo = !echo; break;
		case '$': controlModemConsole(); break;
		case 'l': controlScreenConsole(); break;

		case '`': runMemoryTest(); break;
		case '@': runModuleTests(); break;
		case '#': pressSimPowerButton(); break;
		case 'm': getSimPowerStatus(); break;
//		case '\\': runADCTest(); break;
		case 's': setRealTimeClock(); break;
		case 't': getRealTimeClock(); break;
		case 'r': stream.send("Начинаем перезагрузку через Watch Dog ..."); while(1); break;
		case 'a': MMA7660::get()->syncPrint(uart); break;
		case 'z': MMA7660::get()->asyncPrint(uart); break;
		case 'c': SD::get()->test(); break;
		case '1': Led::get()->setLed1(255, 0)->setLed2(255, 0)->setLed3(255, 0)->setLed4(255, 0); break;
		case '2': Led::get()->setLed1(0, 255)->setLed2(0, 255)->setLed3(0, 255)->setLed4(0, 255); break;
		case '3': Led::get()->setLed1(0, 0)->setLed2(0, 0)->setLed3(0, 0)->setLed4(0, 0); break;
		case '4': Led::get()->setLed1(255, 255)->setLed2(255, 255)->setLed3(255, 255)->setLed4(255, 255)->setPower(255); break;
		case '9': RELE1_ON; break;
		case '0': RELE1_OFF; break;
		case '5': Uart::get(MDB_UART)->send(0x55); break;
		case '6':
			{
				while(!Uart::get(MDB_UART)->isEmptyReceiveBuffer()) {
					char c = Uart::get(MDB_UART)->receive();
					uart->send(c);
				}
			}
		break;

		case '7': Uart::get(MDB_MASTER_UART)->send(0x44); break;

		case '8':
			{
				while(!Uart::get(MDB_MASTER_UART)->isEmptyReceiveBuffer()) {
					char c = Uart::get(MDB_MASTER_UART)->receive();
					uart->send(c);
				}
			}
		break;

		case 'y':
			HardwareUartForwardController::start();
		break;

		case 'u':
			HardwareUartForwardController::stop();
		break;

		case 'x': BATTERY_DISABLE; break;

		case 'g':
		{
			String str;
			uint32_t value = Random::get().getValue();
			str << "Random value: " << value << "\r\n";
			stream.send(str.getString());
		}
		break;

		// Подготовка модуля к чтению карты
//		case 'k':
//		{
//			RFID *rfid = RFID::get();
//			if (!rfid) rfid = new RFID(I2C::get(I2C_1));
//
//			uint32_t versiondata;
//			int timer = 10;
//			do {
//				SystemTimer::get()->delay_ms(100);
//				versiondata = rfid->getFirmwareVersion();
//			} while (!versiondata && timer--);
//
//
//			String str;
//			if (!versiondata) {
//				str = "Chip not found !";
//			} else {
//				str << "Found chip PN5" << ((versiondata>>24) & 0xFF) << "\r\n";
//				str << "Firmware ver. " << ((versiondata>>16) & 0xFF) << "." << ((versiondata>>8) & 0xFF) << "\r\n";
//				str << "Passive retried: " << rfid->setPassiveActivationRetries(0xff);
//				str << "Config: " << rfid->SAMConfig() << "\r\n";
//				str << "Prepare for read card: " << rfid->prepareReadPassiveTargetID(PN532_MIFARE_ISO14443A);
//			}
//			str << "\r\n";
//			stream.send(str.getString());
//		}
//		break;

		// Спрашиваем у модуля, о наличии карты и ее данных. После получения данных карты необходимо заново подготовить модуль к прочтению карты.
//		case 'l': {
//			uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
//			uint8_t uidLength;
//
//			bool success = RFID::get()->preparedReadPassiveTargetID(uid, &uidLength);
//
//			String str;
//			if (success) {
//				str = "Found an ISO14443A card\r\n";
//				str << "  UID Length: " << uidLength << " bytes\r\n";
//				str << "  UID Value: ";
//				for (int i = 0; i < uidLength; i++)
//					str << uid[i];
//				} else {
//					str = "Not found a card";
//				}
//
//			str << "\r\n";
//			stream.send(str.getString());
//		}
//		break;

//		case ';':
//		{
//			String str;
//			uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
//			uint8_t uidLength;
//
//			bool success;
//
//			success = RFID::get()->readPassiveTargetID(0, uid, &uidLength, 1000);
//
//			str << "readPassiveTargetID: "<< success;
//
//			if (success)
//			{
//
//			} else
//			{
//				str << " Not found a card";
//			}
//
//			str << "\r\n";
//
//			stream.send(str.getString());
//		}
//		break;

//		case 'p':
//		{
//			String str;
//			SD *sd = SD::get();
//
//			if (!sd->hasCard()) {
//				stream.send("SD Card not found");
//				return;
//			}
//
//			sd->test();
//		}
//		break;

		case 'b':
		{
			testMonitor();
		}
		break;

		case 'e':
		{
			runEepromStressTest();
		}
		break;

		case 'w': {
			eepromTest.test();
			break;
		}

		case 'h':
		{
/*
1. LOG_ECL установить на LOG_LEVEL_DEBUG
2. собрать и прошить
3. нажать h
4. нажать зеленый круг на терминале
5. нажать q
6. приложить к терминалу карту или вставить карту и ввести PIN (терминал тестовый и списывать не будет)
7. отладить ошибку.
 */
			stream.send("USB start");
			USB *usb = USB::get();
			Mdb::DeviceContext *context = new Mdb::DeviceContext(2, realtime);
			if (!cashless)
			{
//				cashless = new Inpas::Cashless(context, usbUart, timerEngine, eventEngine);
				cashless = new Sberbank::Cashless(context, usbUart, &fakeTcpIp, timerEngine, eventEngine, 25000);
			}
			cashless->reset();
			cashless->enable();
			usb->init(timerEngine);
		}
		break;

		case 'q':
		{
//			USB::get()->test();
			// начало оплаты
			stream.send("Cashless sale");
			cashless->sale(1, 100, "test sale", 1234);
		}
			break;

		case 'i':
		{
			// отмена оплаты
//			USB::get()->test2();
			stream.send("Cashless cancel");
			cashless->closeSession();
		}
			break;

		case 'j':
		{
			USB *usb = USB::get();
			usb->deinit();
		}
		break;

//		case '+':
//			createTimerThread();
//		break;
//
//		case '-':
//			destroyTimerThread();
//		break;

//		case 'l':
//		{
//			uint32_t i;
//			uint8_t txData[4];
//			uint8_t rxData[4];
//			txData[0] = 0x11;
//			txData[1] = 0x12;
//			txData[2] = 0x13;
//			txData[3] = 0x14;
//
//			I2C *i2c = I2C::get(SCREEN_I2C);
//			if (!i2c->syncWriteData(224, 10, 2, txData, 4, 100))
//			{
//				stream.sendln("Error i2c write data");
//				return;
//			}
//
////			SystemTimer::get()->delay_ms(250);
//
//			if (!i2c->syncReadData(224, 10, 2, rxData, 4, 100))
//			{
//				stream.sendln("Error i2c read data");
//				return;
//			}
//
//			for (i = 0; i < sizeof(rxData); i++)
//			{
//				if (txData[i] != rxData[i])
//				{
//					String str;
//					str << "Error compare received i2c data, index: " << i;
//					stream.sendln(str.getString());
//					return;
//				}
//			}
//
//			stream.sendln("I2C test successfull !!!");
//		}

	}
}

void TerminalReceiveHandler::runScreenTest(uint8_t key)
{
	if (!screen)
	{
		Mdb::DeviceContext *context = new Mdb::DeviceContext(2, realtime);
		screen = new Screen(context, I2C::get(SCREEN_I2C), timerEngine, eventEngine);
		screen->init();
	}

	switch(key)
	{
	case '1':
		screen->drawQR("t=20190128T2004&s=600.00&fn=8710000101920854&i=3823&fp=3145911982&n=1");
	break;

	case '2':
		screen->drawText("Пробный длинный\nтекст на\nкириллице,\nкоторый, судя \nпо всему, отображаться\nне будет, кроме\nзапятых и знаков\nвопросов!\nА вот и будет теперь!",
				0, 0, 1, 0xffff, 0x0000);
	break;

	case '3':
		screen->drawText("Hello world !!! Very long text showing to Screen without problem ? !!!",
				0, 50, 2, 0x00ff, 0xff00);
	break;

	case '4':
		screen->drawTextOnCenter("Hello world, line 1\r\nline 2\r\nВсе строки по центру !", 2, Screen::Green, Screen::Red);
	break;

	case '5':
		screen->drawText("1234567890\nQWERTYUIOPASDFGHJKLZXCVBNM\nqwertyuiopasdfghjklzxcvbnm", 0, 10, 2, 0x0000, 0x7f7f);
	break;

	case '6':
		screen->clearDisplay();
		screen->drawText("Hello world, line 1\r\nline 2\r\nline 3", 0, 10, 2, 0x0000, 0x7f7f);
	break;

	case '7':
		screen->drawText("1234567890\nQWERTYUIOPqwertyuiop\nАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩ\nабвгдеёжзийклмнопрстуфхчщ\n!!!", 0, 10, 2, 0x7f7f, 0x0000);
	break;

	case '8':
		screen->setBacklightBrightness(25);
	break;

	case '9':
		screen->setBacklightBrightness(50);
	break;

	case '0':
		screen->setBacklightBrightness(100);
	break;

	case '-':
		screen->clearDisplay();
	break;

	case 'i':
		screen->showDefaultImage();
	break;

	case 'h':
		screen->setRotation(Screen::Rotation::Horizontal2);
	break;

	case 'v':
		screen->setRotation(Screen::Rotation::Vertical2);
	break;

	case 'r':
		screen->restart();
	break;

	case 's':
	{
		uint32_t softwareVersion;
		Screen::Reg24 stage;
		Screen::Reg25 status;
		String str;

		bool result1 = screen->getSoftwareVersion(softwareVersion);
		bool result2 = screen->getApplicationStage(stage);
		bool result3 = screen->getSoftwareStatus(status);

		str << "Software version: ";
		if (result1) str << LOG_VERSION(softwareVersion);
		else str << "Error";

		str << "\r\nApplication stage: ";
		if (result2) str.addHex(static_cast<uint8_t>(stage));
		else str << "Error";


		str << "\r\nSoftware status: ";
		if (result3) str.addHex(static_cast<uint8_t>(status));
		else str << "Error";

		stream.sendln(str.getString());
	}
	break;

	case 'e':
		screen->eraseExtData();
	break;

	case 'z':
		screen->eraseFirmware();
	break;

	case 'w':
	{
		uint8_t data[256];
		for(uint32_t i = 0; i < arraysize(data); i++)
			data[i] = i;

		int max = 128; //1024;

		for (uint32_t i = 0; i < 128*max/arraysize(data); i++)
			screen->writeExtData(data, arraysize(data));

		for(uint32_t i = 0; i < arraysize(data); i++)
			data[i] = i+50;

		for (uint32_t i = 0; i < 128*max/arraysize(data); i++)
			screen->writeExtData(data, arraysize(data));

		for(uint32_t i = 0; i < arraysize(data); i++)
			data[i] = i+100;

		for (uint32_t i = 0; i < 128*max/arraysize(data); i++)
			screen->writeExtData(data, arraysize(data));

	}
	break;

	case 'b':
		screen->beep(800, 100);
	break;

	case 'g':
	{
		String str = "Touch Screen Event Counter: ";

		uint8_t value;
		if (screen->getTouchScreenEventCounter(value))
			str << value;
		else
			str << "Error";

		stream.sendln(str.getString());
	}
	break;

	default:
	{
		char str[32];
		sprintf(str, "Action for '%c' not implemented !", key);
		stream.sendln(str);
	}
	break;

	}
}

void TerminalReceiveHandler::runMemoryTest()
{
	LOG_MEMORY_USAGE("");
}

void TerminalReceiveHandler::runModuleTests()
{
// FIXME: Починить !
//	Tests tests(uart);
//	tests.start();
}


void TerminalReceiveHandler::pressSimPowerButton() {
	stream.sendln("Нажимаем кнопку питания модема");
	hardware.pressPowerButton();
	SystemTimer::get()->delay_ms(1200);
	hardware.releasePowerButton();
}

void TerminalReceiveHandler::getSimPowerStatus()
{
	stream.send("Состояние модуля SIM: ");
	if(hardware.isStatusUp()) {
		stream.sendln(" Включен");
	} else {
		stream.sendln(" Выключен");
	}
}

void TerminalReceiveHandler::controlModemConsole()
{
	sim900Gateway = !sim900Gateway;
	if(sim900Gateway)
		stream.send("-> Включена");
	else
		stream.send("-> Выключена");

	stream.sendln(" переадресация консоли в модуль Sim900 и обратно");
}

void TerminalReceiveHandler::controlScreenConsole()
{
	screenI2C = !screenI2C;
	if(screenI2C)
		stream.send("-> Включено");
	else
		stream.send("-> Выключено");

	stream.sendln(" тестирование SCREEN I2C");
}

void TerminalReceiveHandler::runADCTest()
{
	Adc *adc = Adc::get();

	StringBuilder str;
	str << "ADC, VCC_3: " << adc->read(Adc::VCC_3) << ", ";
	str << "VCC_5: " << adc->read(Adc::VCC_5) << ", ";
	str << "VCC_24: " << adc->read(Adc::VCC_24) << ", ";
	str << "VCC_BAT1: " << adc->read(Adc::VCC_BAT1) << ", ";
	str << "TEMP_SENSOR: " << (int) adc->read(Adc::TEMP_SENSOR) << ", ";
	str << "VCC_BAT2: " << adc->read(Adc::VCC_BAT2) << "\r\n";
	stream.send(str);
}

void TerminalReceiveHandler::runEepromStressTest()
{
	stream.sendln("Run eeprom stress test");

	I2C *i2c = I2C::get(I2C_3);
	ExternalEeprom mem(i2c, EE_ADDRESS, EE_MAX_SIZE, EE_PAGE_SIZE);

	StringBuilder str;
	uint8_t *source = new uint8_t[mem.getPageSize()];

	for (uint32_t rwLen = 1; rwLen < 8; rwLen++)
	{
		str.clear();
		str << "Проверяем запись/чтение 1 страницы, кол-во байт: " << rwLen << "\r\n";
		stream.send(str);

		for (uint32_t i = 0; i < mem.getPageSize(); i++)
		{
			IWDG_ReloadCounter();
			source[0] = (uint8_t) (i + 5);

			mem.setAddress(i);
			if (!mem.write(source, rwLen))
			{
				str.clear();
				str << "Ошибка записи, index: " << i << ", len: " << rwLen <<  "\r\n";
				stream.send(str);

				delete []source;
				return;
			}
			source[0]++;

			mem.setAddress(i);
			if (!mem.read(source, rwLen))
			{
				str.clear();
				str << "Ошибка чтения, index: " << i<< ", len: " << rwLen << "\r\n";
				stream.send(str);

				delete []source;
				return;
			}

			if (source[0] != (uint8_t) (i + 5))
			{
				str.clear();
				str << "Ошибка верификации, index: " << i << ", len: " << rwLen << ", actual: " << (uint8_t) (i+5) << ", get: " << source[0] <<  "\r\n";
				stream.send(str);

				delete []source;
				return;
			}
		}

		stream.sendln("Успешно !");
	}

	stream.sendln("Заполняем весь объем тестовыми данными");

	for (uint32_t i = 0; i < mem.getPageSize(); i++)
		source[i] = (uint8_t) (i+1);


	for (uint32_t page = 0; page < mem.getMaxSize()/mem.getPageSize(); page++)
	{
		IWDG_ReloadCounter();
		mem.setAddress(page * mem.getPageSize());

		if (!mem.write(source, mem.getPageSize()))
		{
			stream.sendln("Ошибка !");

			delete []source;
			return;
		}
	}

	stream.sendln("Успешно !");

	const uint32_t maxReadSize = mem.getPageSize() + mem.getPageSize()/2;

	str << "maxReadSize: " << maxReadSize << "\r\n";
	stream.send(str);

	source = new uint8_t[maxReadSize];

	stream.sendln("Проводим циклы чтения и верификации всей памяти");


	for (uint32_t startAddr = 0; startAddr < mem.getMaxSize() - maxReadSize; startAddr++)
	{
		str.clear();
		str << "Чтение, начало: " << startAddr << ", размер: 1.." << maxReadSize << "\r\n";
		stream.send(str);

		for (uint32_t readLen = 1; readLen < maxReadSize; readLen++)
		{
			IWDG_ReloadCounter();

			mem.setAddress(startAddr);

			if (!mem.read(source, readLen) || !checkReadingEepromData(source, startAddr, readLen))
			{
				str.clear();
				str << "Ошибка чтения! Адрес начала: " << startAddr << ", размер: " << readLen << "\r\n";
				stream.send(str);

				delete[] source;
				return;
			}
		}
	}

	stream.sendln("Успешно !");


	stream.sendln("Exit eeprom stress test");

	delete []source;
}

bool TerminalReceiveHandler::checkReadingEepromData(const uint8_t *source, uint32_t startAddr, uint32_t len)
{
	for (uint32_t i = 0; i < len; i++)
	{
		if (source[i] != (uint8_t) (startAddr + i + 1))
		{
			StringBuilder str;
			str << "Ошибка соответствия! Адрес: " << i << ", ожидается: " << (uint8_t) (i + 1) << ", получено: " << source[i] << "\r\n";
			stream.send(str);
			return false;
		}
	}

	return true;
}

void TerminalReceiveHandler::setRealTimeClock()
{
	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;

	RTC_TimeStructInit(&time);
	time.RTC_Hours = 16;
	time.RTC_Minutes = 8;
	time.RTC_Seconds = 0;

	RTC_DateStructInit(&date);
	date.RTC_Date = 14;
	date.RTC_Month = 5;
	date.RTC_Year = 17;

	RealTimeControl *rtc = RealTimeControl::get();

	bool b = rtc->setTime(&time) && rtc->setDate(&date);
	if(b) {
		stream.sendln("Новое время установлено");
	} else {
		stream.sendln("Новое время не установлено. Ошибка!");
	}
}

void TerminalReceiveHandler::getRealTimeClock() {
	RealTimeStm32 realTime;
	uint32_t totalSeconds = realTime.getUnixTimestamp();
	DateTime datetime;
	realTime.getDateTime(&datetime);

	StringBuilder str;
	str << "Seconds: " << totalSeconds << ", datetime: " << LOG_DATETIME(datetime);
	stream.send(str);
}

void TerminalReceiveHandler::execute()
{
	// В этом режиме все данные с sim900 попадают в консоль
	if(sim900Gateway) {
		Uart *simUart = Uart::get(SIM900_UART);
#ifdef LOG2DEX
		Uart *debugUart = Uart::get(DEX_UART);
#else
		Uart *debugUart = Uart::get(DEBUG_UART);
#endif
		while (!simUart->isEmptyReceiveBuffer()) {
			char b = simUart->receive();
			debugUart->send(b);
		}
	}
}

void TerminalReceiveHandler::createTimerThread()
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
	TIM_TimeBaseInitTypeDef timerStruct;

	int period = TIM5_PERIOD_MKS; // мкс
	int prescaler = APB_SPEED;
	while (period >= 0xffff)
	{
		period /= 10;
		prescaler *= 10;
	}

	timerStruct.TIM_Period = period - 1;
	timerStruct.TIM_Prescaler = prescaler - 1;
	timerStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	timerStruct.TIM_CounterMode = TIM_CounterMode_Up;
	timerStruct.TIM_RepetitionCounter = 0x0000;

	TIM_TimeBaseInit(TIM5, &timerStruct);

	TIM_ITConfig(TIM5, TIM_DIER_UIE, ENABLE);

	// TODO: NVIC, TIM5
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	StringBuilder str;
	str << "Запускаем таймер 5, период: " << TIM5_PERIOD_MKS << " мкс\r\n";
	stream.send(str);

	TIM_Cmd(TIM5, ENABLE);
}

void TerminalReceiveHandler::destroyTimerThread()
{
	TIM_Cmd(TIM5, DISABLE);
	stream.send("Останавливаем таймер 5\r\n");
}

uint32_t timerThreadCounter;
extern "C" void TIM5_IRQHandler(void)
{
	TIM_ClearITPendingBit(TIM5, TIM_IT_Update);
	if ((timerThreadCounter++ % (1000000/TIM5_PERIOD_MKS)) == 0)
	{
		UartStream stream(Uart::get(DEBUG_UART));
		stream.send("T5 ");
	}
}

class MdbReceiveHandler : public UartReceiveHandler {
public:
	MdbReceiveHandler(Uart *uart) : UartReceiveHandler(2), uart(uart) {}
	void handle() {
		uint8_t s = uart->receive();
		uint8_t b = uart->receive();

		String str = "Mdb:";
		str << (s ? "(адрес)" : "");
		str.addHex(b);
		str << "\r\n";
		UartStream debugStream(Uart::get(DEBUG_UART));
		debugStream.send(str);
	}

private:
	Uart *uart;
};

void terminalMain() {
	SystemCoreClockUpdate();

	// Включаем WD
//	WD_Init(7000);

	// Инициализация реле
	InitRele();

	// Инициализация аппаратного проброса данных UART.
	HardwareUartForwardController::init();

	// Часы реального времени
	realtime = new RealTimeStm32;

	// Настраиваем движок таймеров на аппаратном таймере 4
	timerEngine = new TimerEngine();
	Ticker *ticker = Ticker::get();
	ticker->registerConsumer(timerEngine);

	// Движок событий
	eventEngine = new EventEngine(EVENT_POOL_SIZE, EVENT_DATA_SIZE, EVENT_SUB_NUMBER);

	melody = new MelodyElochka;
	gramophone = new Gramophone(Beeper::get(), timerEngine);

	// Подключение АКБ
	Battery::init();
	BATTERY_ENABLE;

	// Инициализация кнопок
	Buttons *buttons = new Buttons();
	buttons->init(timerEngine);

	// Инициализация отладочного UART
#ifdef LOG2DEX
	Uart *debugUart = Uart::get(DEX_UART);
#else
	Uart *debugUart = Uart::get(DEBUG_UART);
#endif
	debugUart->setup(256000, Uart::Parity_None, 0);
	debugUart->setTransmitBufferSize(2048);
	debugUart->setReceiveBufferSize(2048);
	TerminalReceiveHandler *terminalReceiveHandler = new TerminalReceiveHandler(debugUart, timerEngine);
	debugUart->setReceiveHandler(terminalReceiveHandler);
	Logger::get()->registerRealTime(realtime);
	Logger::get()->registerTarget(new LogTargetUart(debugUart));
//	Logger::get()->registerTarget(new RttLogTarget(debugUart));

	// Инициализация модема
	Uart *simUart = Uart::get(SIM900_UART);
	simUart->setup(115200, Uart::Parity_None, 0);
	simUart->setTransmitBufferSize(1024);
	simUart->setReceiveBufferSize(1024);

	// Инициализация MDB-порта
	Uart *mdbUart = Uart::get(MDB_UART);
	mdbUart->setup(9600, Uart::Parity_None, 0, Uart::Mode_9Bit, GPIO_OType_OD);
	mdbUart->setReceiveHandler(new MdbReceiveHandler(mdbUart));

	Uart *mdbUart2 = Uart::get(MDB_MASTER_UART);
	mdbUart2->setup(9600, Uart::Parity_None, 0, Uart::Mode_9Bit, GPIO_OType_OD);

	// Инициализация DEX-порта
#ifndef LOG2DEX
	Uart *dexUart = Uart::get(DEX_UART);
	dexUart->setup(9600, Uart::Parity_None, 0);
	dexUart->setTransmitBufferSize(255);
	dexUart->setReceiveBufferSize(255);
#endif

	statStorage = new StatStorage();

	// USB
	USB *usb = USB::get();
	usbUart = new UsbUart(usb);

	I2C *i2c_screen = new I2C(SCREEN_I2C, I2C_SCREEN_SPEED);
	i2c_screen->setRetryPause(I2C_SCREEN_RETRY_PAUSE);
	UNUSED(i2c_screen);

#if 0
	// Сброс флага обновления
	I2C *i2c = I2C::get(I2C_3);
	i2c->setStatStorage(statStorage);
	ExternalEeprom *memory = new ExternalEeprom(i2c, EE_ADDRESS, EE_MAX_SIZE, EE_PAGE_SIZE, statStorage);
	ConfigBoot *boot = new ConfigBoot;
	boot->load(memory);
	if(boot->getFirmwareState() != ConfigBoot::FirmwareState_None) {
		boot->setFirmwareState(ConfigBoot::FirmwareState_None);
		boot->save();
	}
#endif
	__enable_irq();

	// Инициализация RFID/NFC
	RFID *rfid = new RFID(I2C::get(I2C_1));

	hardware.init();

	uint32_t hw = Hardware::get()->getVersion();
	*(Logger::get()) << "Firmware: hardware " << LOG_VERSION((uint32_t)&HardwareVersion) << ", software " << LOG_VERSION((uint32_t)&SoftwareVersion) << Logger::endl;
	*(Logger::get()) << "Manual testing starting..."  << Logger::endl;
	*(Logger::get()) << "Автоопределение версии платы: " << LOBYTE(HIWORD(hw)) << "." << HIBYTE(LOWORD(hw)) << "." << LOBYTE(LOWORD(hw)) << Logger::endl;

#ifdef TEST
	SystemTimer::get()->delay_ms(250);
//	PROBE_INIT(A, 0, GPIO_OType_PP);
 	RealTimeControl::get()->print(debugUart);
#endif

	while(1) {
		IWDG_ReloadCounter();
		timerEngine->execute();
		eventEngine->execute();
		debugUart->execute();
#ifndef LOG2DEX
		dexUart->execute();
#endif
		simUart->execute();
		terminalReceiveHandler->execute();
		usb->execute();
	}
}
#endif
