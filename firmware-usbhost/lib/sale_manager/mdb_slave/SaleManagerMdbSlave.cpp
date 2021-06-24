#include "include/SaleManagerMdbSlave.h"
#include "SaleManagerMdbSlaveCore.h"
#include "MdbSnifferEngine.h"

#include "common/uart/stm32/include/uart.h"

SaleManagerMdbSlave::SaleManagerMdbSlave(
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
	MdbSnifferCashless *slaveCashless1 = NULL;
	MdbSlaveCashless3 *slaveCashless2 = NULL;
	if(configAutomat->getCashlessNumber() == 1) {
		slaveCashless1 = new MdbSnifferCashless(Mdb::Device_CashlessDevice2, configAutomat->getMdb1CashlessContext(), events);
		slaveCashless2 = new MdbSlaveCashless3(Mdb::Device_CashlessDevice1, configAutomat->getCashlessMaxLevel(), configAutomat->getSMContext(), timers, events, config->getStat());
	} else {
		slaveCashless1 = new MdbSnifferCashless(Mdb::Device_CashlessDevice1, configAutomat->getMdb1CashlessContext(), events);
		slaveCashless2 = new MdbSlaveCashless3(Mdb::Device_CashlessDevice2, configAutomat->getCashlessMaxLevel(), configAutomat->getSMContext(), timers, events, config->getStat());
	}
	MdbSlaveComGateway *slaveGateway = new MdbSlaveComGateway(configAutomat->getSMContext(), events);

	SaleManagerMdbSlaveParams params;
	params.config = config;
	params.timers = timers;
	params.eventEngine = events;
	params.fiscalRegister = fiscalRegister;
	params.leds = leds;
	params.chronicler = chronicler;
	params.rebootReason = rebootReason;
	params.externCashless = externCashless;
	params.slaveCoinChanger = new MdbSnifferCoinChanger(configAutomat->getCCContext(), events);
	params.slaveBillValidator = new MdbSnifferBillValidator(configAutomat->getBVContext(), events);
	params.slaveCashless1 = slaveCashless1;
	params.slaveCashless2 = slaveCashless2;
	params.slaveGateway = slaveGateway;

	engine = new MdbSnifferEngine(config->getStat());
	engine->init(slaveUart, masterUart);
	engine->addSniffer(params.slaveCoinChanger);
	engine->addSniffer(params.slaveBillValidator);
	engine->addSniffer(params.slaveCashless1);
	engine->addSlave(slaveGateway);
	engine->addSlave(slaveCashless2);

	this->core = new SaleManagerMdbSlaveCore(&params);
}

SaleManagerMdbSlave::~SaleManagerMdbSlave() {
	delete core;
	delete engine;
}

void SaleManagerMdbSlave::reset() {
	core->reset();
	engine->reset();
}

void SaleManagerMdbSlave::service() {
	core->service();
}

void SaleManagerMdbSlave::shutdown() {
	core->shutdown();
}
