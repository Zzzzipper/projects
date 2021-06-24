#include "include/SaleManagerExeMaster.h"
#include "SaleManagerExeMasterCore.h"
#include "ExeMaster.h"

#include "common/timer/include/Timer.h"
#include "common/mdb/master/include/MdbMasterEngine.h"
#include "common/uart/stm32/include/uart.h"
#include "common/logger/include/Logger.h"

SaleManagerExeMaster::SaleManagerExeMaster(
	ConfigModem *config,
	ClientContext *client,
	TimerEngine *timers,
	EventEngine *eventEngine,
	Uart *slaveUart,
	Uart *masterUart,
	MdbMasterCashlessStack *masterCashlessStack,
	Fiscal::Register *fiscalRegister,
	LedInterface *leds,
	EventRegistrar *chronicler
) {
	SaleManagerExeMasterParams params;
	params.config = config;
	params.client = client;
	params.timers = timers;
	params.eventEngine = eventEngine;
	params.fiscalRegister = fiscalRegister;
	params.leds = leds;
	params.chronicler = chronicler;

	initExeMaster(timers, eventEngine, slaveUart, &params);
	initMdbMaster(config, timers, eventEngine, masterUart, masterCashlessStack, &params);

	this->core = new SaleManagerExeMasterCore(&params);
}

SaleManagerExeMaster::~SaleManagerExeMaster() {
	delete core;
	delete masterEngine;
	delete executive;
}

void SaleManagerExeMaster::reset() {
	core->reset();
	masterEngine->reset();
	executive->reset();
}

void SaleManagerExeMaster::shutdown() {
	core->shutdown();
}

void SaleManagerExeMaster::initExeMaster(TimerEngine *timers, EventEngine *eventEngine, Uart *uart, SaleManagerExeMasterParams *params) {
	uart->setup(9600, Uart::Parity_None, 0, Uart::Mode_9Bit, GPIO_OType_OD);
	uart->setTransmitBufferSize(80);
	uart->setReceiveBufferSize(80);
	uart->clear();

	executive = new ExeMaster(uart, timers, eventEngine, params->config->getStat());

	params->executive = executive;
	LOG_INFO(LOG_SM, "UartSlave Executive OK");
}

void SaleManagerExeMaster::initMdbMaster(ConfigModem *config, TimerEngine *timers, EventEngine *eventEngine, Uart *uart, MdbMasterCashlessStack *masterCashlessStack, SaleManagerExeMasterParams *params) {
	uart->setup(9600, Uart::Parity_None, 0, Uart::Mode_9Bit, GPIO_OType_OD);
	uart->setTransmitBufferSize(80);
	uart->setReceiveBufferSize(80);
	uart->clear();

	MdbMasterCoinChanger *masterCoinChanger = new MdbMasterCoinChanger(config->getAutomat()->getCCContext(), eventEngine);
	MdbMasterBillValidator *masterBillValidator = new MdbMasterBillValidator(config->getAutomat()->getBVContext(), eventEngine);
	MdbMasterCashless *masterCashless1 = new MdbMasterCashless(Mdb::Device_CashlessDevice1, config->getAutomat()->getMdb1CashlessContext(), timers, eventEngine);
	MdbMasterCashless *masterCashless2 = new MdbMasterCashless(Mdb::Device_CashlessDevice2, config->getAutomat()->getMdb2CashlessContext(), timers, eventEngine);

	masterEngine = new MdbMasterEngine(config->getStat());
	masterEngine->init(uart, timers);
	masterEngine->add(masterCoinChanger);
	masterEngine->add(masterBillValidator);
	masterEngine->add(masterCashless1);
	masterEngine->add(masterCashless2);

	params->masterCoinChanger = masterCoinChanger;
	params->masterBillValidator = masterBillValidator;
	params->masterCashlessStack = masterCashlessStack;
	params->masterCashlessStack->push(masterCashless1);
	params->masterCashlessStack->push(masterCashless2);
	LOG_INFO(LOG_SM, "UartMaster MdbMaster OK");
}
