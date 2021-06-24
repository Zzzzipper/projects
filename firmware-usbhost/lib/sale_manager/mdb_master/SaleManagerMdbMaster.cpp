#include <lib/sale_manager/mdb_master/SaleManagerMdbMasterCore.h>
#include "include/SaleManagerMdbMaster.h"
#include "common/mdb/slave/include/MdbSlaveEngine.h"
#include "common/mdb/slave/cashless/MdbSlaveCashless3.h"
#include "common/mdb/master/include/MdbMasterEngine.h"
#include "common/mdb/master/cashless/MdbMasterCashless.h"
#include "common/uart/stm32/include/uart.h"
#include "common/logger/include/Logger.h"

SaleManagerMdbMaster::SaleManagerMdbMaster(
	ConfigModem *config,
	TimerEngine *timerEngine,
	EventEngine *eventEngine,
	Uart *slaveUart,
	Uart *masterUart,
	MdbMasterCashlessStack *masterCashlessStack,
	Fiscal::Register *fiscalRegister,
	LedInterface *leds,
	EventRegistrar *chronicler
) {
	SaleManagerMdbMasterParams params;
	params.config = config;
	params.timers = timerEngine;
	params.eventEngine = eventEngine;
	params.fiscalRegister = fiscalRegister;
	params.chronicler = chronicler;
	params.leds = leds;

	initMdbSlave(config, timerEngine, eventEngine, slaveUart, &params);
	initMdbMaster(config, timerEngine, eventEngine, masterUart, masterCashlessStack, &params);

	core = new SaleManagerMdbMasterCore(&params);
}

SaleManagerMdbMaster::~SaleManagerMdbMaster() {
	delete core;
}

void SaleManagerMdbMaster::reset() {
	core->reset();
	slaveEngine->reset();
	masterEngine->reset();
}

void SaleManagerMdbMaster::service() {
	core->service();
}

void SaleManagerMdbMaster::shutdown() {
	core->shutdown();
}

void SaleManagerMdbMaster::testCredit() {
	core->testCredit();
}

void SaleManagerMdbMaster::initMdbSlave(ConfigModem *config, TimerEngine *timerEngine, EventEngine *eventEngine, Uart *uart, SaleManagerMdbMasterParams *params) {
	uart->setup(9600, Uart::Parity_None, 0, Uart::Mode_9Bit, GPIO_OType_OD);
	uart->setTransmitBufferSize(80);
	uart->setReceiveBufferSize(80);
	uart->clear();

	ConfigAutomat *configAutomat = config->getAutomat();
	MdbSlaveCashless3 *slaveCashless1 = NULL;
	if(configAutomat->getCashlessNumber() == 1) {
		slaveCashless1 = new MdbSlaveCashless3(Mdb::Device_CashlessDevice1, configAutomat->getCashlessMaxLevel(), configAutomat->getSMContext(), timerEngine, eventEngine, config->getStat());
	} else {
		slaveCashless1 = new MdbSlaveCashless3(Mdb::Device_CashlessDevice2, configAutomat->getCashlessMaxLevel(), configAutomat->getSMContext(), timerEngine, eventEngine, config->getStat());
	}
	MdbSlaveCoinChanger *slaveCoinChanger = new MdbSlaveCoinChanger(configAutomat->getCCContext(), eventEngine, config->getStat());
	MdbSlaveBillValidator *slaveBillValidator = new MdbSlaveBillValidator(configAutomat->getBVContext(), eventEngine);

	slaveEngine = new MdbSlaveEngine();
	slaveEngine->init(uart);
	slaveEngine->add(slaveCashless1);
	slaveEngine->add(slaveCoinChanger);
	slaveEngine->add(slaveBillValidator);

	params->slaveCashless1 = slaveCashless1;
	params->slaveCoinChanger = slaveCoinChanger;
	params->slaveBillValidator = slaveBillValidator;
	LOG_INFO(LOG_MODEM, "UartSlave MdbSlave OK");
}

void SaleManagerMdbMaster::initMdbMaster(ConfigModem *config, TimerEngine *timers, EventEngine *eventEngine, Uart *uart, MdbMasterCashlessStack *masterCashlessStack, SaleManagerMdbMasterParams *params) {
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
