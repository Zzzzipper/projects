
#if 0
#include <lib/sale_manager/mdb_no_cashless/include/SaleManagerMdbNoCashless.h>
#include <lib/sale_manager/mdb_no_cashless/SaleManagerMdbNoCashlessCore.h>

#include "common/mdb/slave/include/MdbSlaveEngine.h"
#include "common/mdb/master/include/MdbMasterEngine.h"
#include "common/uart/stm32/include/uart.h"

SaleManagerMdbNoCashless::SaleManagerMdbNoCashless(
	ConfigModem *config,
	TimerEngine *timers,
	EventEngine *eventEngine,
	Uart *slaveUart,
	Uart *masterUart,
	MdbMasterCashlessStack *externCashless,
	Fiscal::Register *fiscalRegister,
	LedInterface *leds,
	EventRegistrar *chronicler,
	Reboot::Reason rebootReason
) {
	slaveUart->setup(9600, Uart::Parity_None, 0, Uart::Mode_9Bit, GPIO_OType_OD);
	slaveUart->setTransmitBufferSize(80);
	slaveUart->setReceiveBufferSize(80);
	slaveUart->clear();

	masterUart->setup(9600, Uart::Parity_None, 0, Uart::Mode_9Bit, GPIO_OType_OD);
	masterUart->setTransmitBufferSize(80);
	masterUart->setReceiveBufferSize(80);
	masterUart->clear();

	ConfigAutomat *configAutomat = config->getAutomat();

	SaleManagerMdbNoCashlessParams params;
	params.config = config;
	params.timers = timers;
	params.eventEngine = eventEngine;
	params.fiscalRegister = fiscalRegister;
	params.leds = leds;
	params.chronicler = chronicler;
	params.rebootReason = rebootReason;
	params.slaveCoinChanger = new MdbSlaveCoinChanger(configAutomat->getCCContext(), eventEngine, config->getStat());
	params.slaveBillValidator = new MdbSlaveBillValidator(configAutomat->getBVContext(), eventEngine);
	params.masterCoinChanger = new MdbMasterCoinChanger(config->getAutomat()->getCCContext(), eventEngine);
	params.masterBillValidator = new MdbMasterBillValidator(config->getAutomat()->getBVContext(), eventEngine);

	masterEngine = new MdbMasterEngine(config->getStat());
	masterEngine->init(masterUart, timers);
	masterEngine->add(params.masterCoinChanger);
	masterEngine->add(params.masterBillValidator);

	slaveEngine = new MdbSlaveEngine();
	slaveEngine->init(slaveUart);
	slaveEngine->add(params.slaveCoinChanger);
	slaveEngine->add(params.slaveBillValidator);

	this->core = new SaleManagerMdbNoCashlessCore(&params);
}

SaleManagerMdbNoCashless::~SaleManagerMdbNoCashless() {
	delete core;
	delete slaveEngine;
	delete masterEngine;
}

void SaleManagerMdbNoCashless::reset() {
	core->reset();
	slaveEngine->reset();
	masterEngine->reset();
}

void SaleManagerMdbNoCashless::service() {
	core->service();
}

void SaleManagerMdbNoCashless::shutdown() {
	core->shutdown();
}
#else
#include <lib/sale_manager/mdb_no_cashless/include/SaleManagerMdbNoCashless.h>
#include <lib/sale_manager/mdb_no_cashless/SaleManagerMdbNoCashlessCore.h>

#include "lib/sale_manager/mdb_slave/MdbSnifferEngine.h"

#include "common/uart/stm32/include/uart.h"

SaleManagerMdbNoCashless::SaleManagerMdbNoCashless(
	ConfigModem *config,
	TimerEngine *timers,
	EventEngine *events,
	Uart *slaveUart,
	Uart *masterUart,
	MdbMasterCashlessStack *externCashless,
	Fiscal::Register *fiscalRegister,
	LedInterface *leds,
	EventRegistrar *chronicler,
	Reboot::Reason rebootReason
) {
	slaveUart->setup(9600, Uart::Parity_None, 0, Uart::Mode_9Bit, GPIO_OType_OD);
	slaveUart->setTransmitBufferSize(80);
	slaveUart->setReceiveBufferSize(80);
	slaveUart->clear();

	masterUart->setup(9600, Uart::Parity_None, 0, Uart::Mode_9Bit, GPIO_OType_OD);
	masterUart->setTransmitBufferSize(80);
	masterUart->setReceiveBufferSize(80);
	masterUart->clear();

	ConfigAutomat *configAutomat = config->getAutomat();

	SaleManagerMdbNoCashlessParams params;
	params.config = config;
	params.timers = timers;
	params.eventEngine = events;
	params.fiscalRegister = fiscalRegister;
	params.leds = leds;
	params.chronicler = chronicler;
	params.rebootReason = rebootReason;
	params.slaveCoinChanger = new MdbSnifferCoinChanger(configAutomat->getCCContext(), events);
	params.slaveBillValidator = new MdbSnifferBillValidator(configAutomat->getBVContext(), events);

	engine = new MdbSnifferEngine(config->getStat());
	engine->init(slaveUart, masterUart);
	engine->addSniffer(params.slaveCoinChanger);
	engine->addSniffer(params.slaveBillValidator);

	this->core = new SaleManagerMdbNoCashlessCore(&params);
}

SaleManagerMdbNoCashless::~SaleManagerMdbNoCashless() {
	delete core;
	delete engine;
}

void SaleManagerMdbNoCashless::reset() {
	core->reset();
	engine->reset();
}

void SaleManagerMdbNoCashless::service() {
	core->service();
}

void SaleManagerMdbNoCashless::shutdown() {
	core->shutdown();
}
#endif
