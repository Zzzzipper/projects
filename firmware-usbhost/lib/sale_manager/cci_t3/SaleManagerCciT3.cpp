#include <lib/sale_manager/cci_t3/ErpOrderMaster.h>
#include <lib/sale_manager/cci_t3/include/SaleManagerCciT3.h>
#include <lib/sale_manager/cci_t3/SaleManagerCciT3Core.h>
#include "common/uart/stm32/include/uart.h"
#include "common/logger/include/Logger.h"

#include <string.h>

SaleManagerCciT3::SaleManagerCciT3(
	ConfigModem *config,
	ClientContext *client,
	TimerEngine *timerEngine,
	EventEngine *eventEngine,
	Uart *dexUart,
	CodeScannerInterface *scanner,
	TcpIp *tcpConn,
	MdbMasterCashlessStack *masterCashlessStack,
	Fiscal::Register *fiscalRegister,
	LedInterface *leds,
	EventRegistrar *chronicler
) {
	initCsiDriver(timerEngine, eventEngine, dexUart);
	ErpOrderMaster *orderMaster = (scanner == NULL) ? NULL : new ErpOrderMaster(config, scanner, tcpConn, timerEngine, eventEngine);
	core = new SaleManagerCciT3Core(config, client, timerEngine, eventEngine, driver, orderMaster, masterCashlessStack, fiscalRegister, leds, chronicler);
}

SaleManagerCciT3::~SaleManagerCciT3() {
	delete core;
	delete driver;
}

void SaleManagerCciT3::reset() {
	core->reset();
}

void SaleManagerCciT3::shutdown() {
	core->shutdown();
}

void SaleManagerCciT3::initCsiDriver(TimerEngine *timerEngine, EventEngine *eventEngine, Uart *uart) {
	uart->setup(9600, Uart::Parity_None, 0);
	uart->setTransmitBufferSize(256);
	uart->setReceiveBufferSize(256);
	driver = new Cci::T3::Driver(uart, timerEngine, eventEngine);
}
