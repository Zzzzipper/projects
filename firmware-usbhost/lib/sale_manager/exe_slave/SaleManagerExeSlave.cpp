#include "include/SaleManagerExeSlave.h"
#include "SaleManagerExeSlaveCore.h"

#include "common/logger/include/Logger.h"

#include <string.h>

SaleManagerExeSlave::SaleManagerExeSlave(ConfigModem *config, TimerEngine *timers, EventEngine *eventEngine, Uart *slaveUart, Uart *masterUart, Fiscal::Register *fiscalRegister) {
	initSniffer(eventEngine, slaveUart, masterUart, config->getStat());
	core = new SaleManagerExeSlaveCore(config, timers, eventEngine, exeSniffer, fiscalRegister);
}

SaleManagerExeSlave::~SaleManagerExeSlave() {
	delete core;
	delete exeSniffer;
}

void SaleManagerExeSlave::reset() {
	core->reset();
}

void SaleManagerExeSlave::shutdown() {
	core->shutdown();
}

void SaleManagerExeSlave::initSniffer(EventEngine *eventEngine, Uart *slaveUart, Uart *masterUart, StatStorage *stat) {
	slaveUart->setup(9600, Uart::Parity_None, 0, Uart::Mode_Normal, GPIO_OType_OD);
	slaveUart->setTransmitBufferSize(80);
	slaveUart->setReceiveBufferSize(80);
	slaveUart->clear();
	LOG_INFO(LOG_SM, "UartSlave Executive OK");

	masterUart->setup(9600, Uart::Parity_None, 0, Uart::Mode_Normal, GPIO_OType_OD);
	masterUart->setTransmitBufferSize(80);
	masterUart->setReceiveBufferSize(80);
	masterUart->clear();
	LOG_INFO(LOG_SM, "UartMaster Executive OK");

	exeSniffer = new ExeSniffer(eventEngine, stat);
	exeSniffer->init(slaveUart, masterUart);
	exeSniffer->reset();
}
