#include "config.h"

#if defined(TERMINAL)
#ifndef TEST_TERMINAL_H
#define TEST_TERMINAL_H

#include "test/EepromResourceTest.h"

#include "lib/screen/Screen.h"

#include "common/uart/include/interface.h"
#include "common/uart/include/UartStream.h"
#include "common/cardreader/inpas/include/Inpas.h"
#include "common/cardreader/sberbank/include/Sberbank.h"

class Uart;

class TerminalReceiveHandler : public UartReceiveHandler {
public:
	TerminalReceiveHandler(Uart *uart, TimerEngine *timerEngine);
	void handle();
	void execute();
	void testMonitor();

private:
	Uart *uart;
	UartStream stream;
	bool sim900Gateway;
	bool echo;
	bool screenI2C;
	Screen *screen;
	MdbMasterCashlessInterface *cashless;
	EepromResourceTest eepromTest;

	void runMemoryTest();
	void runModuleTests();
	void pressSimPowerButton();
	void getSimPowerStatus();
	void controlModemConsole();
	void controlScreenConsole();
	void runADCTest();
	void setRealTimeClock();
	void getRealTimeClock();
	void runEepromStressTest();
	void runScreenTest(uint8_t key);

	bool checkReadingEepromData(const uint8_t *source, uint32_t startAddr, uint32_t len);

	void createTimerThread();
	void destroyTimerThread();


};

void terminalMain();

#endif
#endif
