#include "config.h"

#ifdef PSI
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "cmsis_boot/stm32f4xx_conf.h"

#include "FastPsi.h"
#include "lib/adc/adc.h"
#include "lib/utils/stm32/buttons.h"
#include "lib/utils/stm32/Beeper.h"
#include "lib/mma7660/MMA7660.h"
#include "lib/network/Enc28j60.h"
#include "lib/sd/SD.h"
#include "lib/utils/stm32/HardwareUartForwardController.h"

#include "common.h"
#include "common/memory/stm32/include/ExternalEeprom.h"
#include "common/utils/include/Version.h"
#include "common/sim900/GsmHardware.h"
#include "common/timer/stm32/include/SystemTimer.h"
#include "common/timer/stm32/include/RealTimeControl.h"
#include "common/utils/include/StringBuilder.h"
#include "common/utils/stm32/include/Led.h"
#include "common/memory/stm32/include/ExternalEeprom.h"
#include "common/uart/include/UartStream.h"
#include "common/i2c/I2C.h"
#include "common/beeper/include/Gramophone.h"

void fastPsiMain()
{
	SystemCoreClockUpdate();

	FastPsi psi;
	psi.start();

	while(1)
	{
		IWDG_ReloadCounter();
	}
}

FastPsi::FastPsi(): Psi(true)
{
}

void FastPsi::start()
{
	*(Logger::get()) << "Firmware: hardware " << LOG_VERSION((uint32_t)&HardwareVersion) << ", software " << LOG_VERSION((uint32_t)&SoftwareVersion) << Logger::endl;
	LOG("Fast PSI init...");

	testInit();

	Led::get()->setLed1(0,0)->setLed2(0,0)->setLed3(0,0)->setLed4(0,0);
	Led::get()->setPower(64);

	// Выключаем модуль SIM.
	Gsm::Hardware *hardware = new Gsm::Hardware;
	if(hardware->isStatusUp())
	{
		printDetails("Выключаем модуль SIM");
		hardware->pressPowerButton();
		SystemTimer::get()->delay_ms(1500);
		hardware->releasePowerButton();
	}

	testLeds();
	testBeeper();

	bool result = testRTC() && testExternalEeprom() && testEthernet();

	led->setLed1(!result, result);

	IWDG_ReloadCounter();

	result = testUart_MDB();

	led->setLed2(!result, result);

	IWDG_ReloadCounter();

	result = testUart_DEX() && testUart_Debug();

	led->setLed3(!result, result);

	IWDG_ReloadCounter();

	result = testSim900();

	led->setLed4(!result, result);
}

void FastPsi::testLeds()
{
	const int delay = 150;

	// Тушим все светодиоды
	led->setLed1(0, 0); SystemTimer::get()->delay_ms(delay);
	led->setLed2(0, 0); SystemTimer::get()->delay_ms(delay);
	led->setLed3(0, 0); SystemTimer::get()->delay_ms(delay);
	led->setLed4(0, 0); SystemTimer::get()->delay_ms(delay);

	waitingPressButton(Buttons::Event_Button1);

	// Зажигаем красные светодиоды
	led->setLed1(1, 0); SystemTimer::get()->delay_ms(delay);
	led->setLed2(1, 0); SystemTimer::get()->delay_ms(delay);
	led->setLed3(1, 0); SystemTimer::get()->delay_ms(delay);
	led->setLed4(1, 0); SystemTimer::get()->delay_ms(delay);

	waitingPressButton(Buttons::Event_Button2);

	// Зажигаем зеленые светодиоды
	led->setLed1(0, 1); SystemTimer::get()->delay_ms(delay);
	led->setLed2(0, 1); SystemTimer::get()->delay_ms(delay);
	led->setLed3(0, 1); SystemTimer::get()->delay_ms(delay);
	led->setLed4(0, 1); SystemTimer::get()->delay_ms(delay);

	waitingPressButton(Buttons::Event_Button3);

	// Тушим все светодиоды
	led->setLed1(0, 0); SystemTimer::get()->delay_ms(delay);
	led->setLed2(0, 0); SystemTimer::get()->delay_ms(delay);
	led->setLed3(0, 0); SystemTimer::get()->delay_ms(delay);
	led->setLed4(0, 0); SystemTimer::get()->delay_ms(delay);
}

bool FastPsi::testUart_Debug()
{
	Uart *uart = Uart::get(DEBUG_UART);

	uart->clear();

	SystemTimer::get()->delay_ms(250);

	uart->clear();

	for (uint8_t i = '0'; i <= '9'; i++)
		uart->send(i);

	SystemTimer::get()->delay_ms(250);

	for (uint8_t i = '0'; i <= '9'; i++)
	{
		uint8_t r = uart->receive();
		if (r != i) return false;
	}

	return true;
}

#endif
