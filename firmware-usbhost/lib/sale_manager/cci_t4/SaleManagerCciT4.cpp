#include <lib/sale_manager/cci_t4/include/SaleManagerCciT4.h>
#include <lib/sale_manager/cci_t4/SaleManagerCciT4Core.h>
#include "common/uart/stm32/include/uart.h"
#include "common/logger/include/Logger.h"

#include <string.h>

SaleManagerCciT4::SaleManagerCciT4(ConfigModem *config, ClientContext *client, TimerEngine *timerEngine, EventEngine *eventEngine, Uart *uart, MdbMasterCashlessStack *masterCashlessStack, Fiscal::Register *fiscalRegister) {
	initSlaveCashless(timerEngine, eventEngine, uart);
	core = new SaleManagerCciT4Core(config, client, timerEngine, eventEngine, slaveCashless, masterCashlessStack, fiscalRegister);
}

SaleManagerCciT4::~SaleManagerCciT4() {
	delete core;
	delete slaveCashless;
}

void SaleManagerCciT4::reset() {
	core->reset();
}

void SaleManagerCciT4::shutdown() {
	core->shutdown();
}

void SaleManagerCciT4::initSlaveCashless(TimerEngine *timerEngine, EventEngine *eventEngine, Uart *uart) {
	uart->setup(9600, Uart::Parity_None, 0);
	uart->setTransmitBufferSize(256);
	uart->setReceiveBufferSize(256);
	slaveCashless = new CciCsi::Cashless(uart, timerEngine, eventEngine);
	LOG_INFO(LOG_SM, "CCI OK");
}
