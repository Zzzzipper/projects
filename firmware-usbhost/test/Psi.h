#include "config.h"

#ifdef PSI
#ifndef __UTILS_PSI
#define __UTILS_PSI

#include "common/uart/stm32/include/uart.h"
#include "common/logger/include/LogTargetUart.h"
#include "common/ticker/include/Ticker.h"
#include "common/timer/include/TimerEngine.h"
#include "common/config/include/StatStorage.h"
#include "common/utils/stm32/include/Led.h"
#include "common/sim900/GsmHardware.h"

/*
 * FIXME: Доделать вывод итогов теста, списком ошибок.
 */

class Buttons;

class Psi {
public:
	Psi(bool fastMode = false);
	void start();

protected:
	StatStorage *stat;
	TimerEngine *timerEngine;
	Uart *debugUart;
	Buttons *buttons;
	Led *led;
	Gsm::HardwareInterface *hardware;

	bool fastMode;

	int hwErrors;

	void initDebugUart();
	void initTimers();

	void printHeader(const char *head);
	void printResult(bool result, const char *info = NULL);
	void printDetails(const char *info);
	void printMessage(const char *info);
	bool printDialogAndWaitAnswer(const char *info, const char *waitString);
	bool sendToAndWaitAnswer(Uart *uart, const char *sendString, const char *waitString, uint32_t timeoutMls, bool printAnswer);

	// Тесты
	void testInit();

	bool testExternalEeprom();
	bool testRTC();
	void testButtons();
	void waitingPressButton(uint16_t buttonId);
	void testLeds();
	bool testSim900();
	void testBeeper();
	void testRfid();
	void testMMA7660();
	bool testEthernet();
	void testSD();
	void testRele();

	void testUarts();
	bool testUart_MDB();
	bool testUart_MDB(int speed);
	bool testUart_DEX();

};

void psiMain();

#endif
#endif
