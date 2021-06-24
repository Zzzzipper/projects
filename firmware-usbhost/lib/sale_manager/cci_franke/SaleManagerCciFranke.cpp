#include <lib/sale_manager/cci_t3/SaleManagerCciT3Core.h>
#include <lib/sale_manager/cci_t4/SaleManagerCciT4Core.h>
#include "include/SaleManagerCciFranke.h"
#include "SaleManagerCciFrankeCore.h"
#include "common/uart/stm32/include/uart.h"
#include "common/logger/include/Logger.h"

#include <string.h>

SaleManagerCciFranke::SaleManagerCciFranke(
	ConfigModem *config,
	ClientContext *client,
	TimerEngine *timerEngine,
	EventEngine *eventEngine,
	Uart *uart,
	MdbMasterCashlessStack *masterCashlessStack,
	Fiscal::Register *fiscalRegister,
	LedInterface *leds,
	EventRegistrar *chronicler
) {
	initSlaveCashless(timerEngine, eventEngine, uart);
#if 0
	core = new SaleManagerCciFrankeCore(config, client, timerEngine, eventEngine, slaveCashless, masterCashlessStack, fiscalRegister);
#else
	core = new SaleManagerCciT4Core(config, client, timerEngine, eventEngine, slaveCashless, masterCashlessStack, fiscalRegister);
#endif
}

SaleManagerCciFranke::~SaleManagerCciFranke() {
	delete core;
	delete slaveCashless;
}

void SaleManagerCciFranke::reset() {
	core->reset();
}

void SaleManagerCciFranke::shutdown() {
	core->shutdown();
}

void SaleManagerCciFranke::initSlaveCashless(TimerEngine *timerEngine, EventEngine *eventEngine, Uart *uart) {
	uart->setup(9600, Uart::Parity_None, 0);
	uart->setTransmitBufferSize(256);
	uart->setReceiveBufferSize(256);
	slaveCashless = new Cci::Franke::Cashless(uart, timerEngine, eventEngine);
	LOG_INFO(LOG_SM, "CCI FRANKE OK");
}
