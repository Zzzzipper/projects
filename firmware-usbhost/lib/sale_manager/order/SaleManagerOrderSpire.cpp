#include <lib/sale_manager/order/include/SaleManagerOrderSpire.h>
#include <lib/sale_manager/order/DummyOrderMaster.h>
#include <lib/sale_manager/order/FaceIdOrderMaster.h>
#include <lib/sale_manager/order/SaleManagerOrderCore.h>
#include "lib/sale_manager/order/CciT3OrderDevice.h"
#include "lib/sale_manager/order/MdbSlaveSpire.h"
#include "lib/sale_manager/mdb_slave/MdbSnifferEngine.h"

#include "common/ccicsi/CciT3Driver.h"
#include "common/uart/stm32/include/uart.h"
#include "common/logger/include/Logger.h"

#include <string.h>

SaleManagerOrderSpire::SaleManagerOrderSpire(
	ConfigModem *config,
	ClientContext *client,
	TimerEngine *timerEngine,
	EventEngine *eventEngine,
	Uart *slaveUart,
	Uart *masterUart,
	Uart *pincodeUart,
	TcpIp *tcpConn,
	Fiscal::Register *fiscalRegister,
	LedInterface *leds,
	EventRegistrar *chronicler
) {
	OrderMasterInterface *master = initMaster(config, timerEngine, eventEngine, tcpConn);
	OrderDeviceInterface *device = initDevice(config, timerEngine, eventEngine, slaveUart, masterUart);
	core = new SaleManagerOrderCore(config, client, timerEngine, eventEngine, master, device, fiscalRegister, leds, chronicler);
}

SaleManagerOrderSpire::~SaleManagerOrderSpire() {
	delete core;
}

void SaleManagerOrderSpire::reset() {
	core->reset();
	engine->reset();
}

void SaleManagerOrderSpire::shutdown() {
	core->shutdown();
}

OrderMasterInterface *SaleManagerOrderSpire::initMaster(ConfigModem *config, TimerEngine *timerEngine, EventEngine *eventEngine, TcpIp *tcpConn) {
#if 0
#if 0
	const char* faceIdDevice = "ephor1";
	const char* addr = "95.216.78.99";
	uint16_t port = 1883;
	const char* username = "ephor";
	const char* password = "2kDR8TMCu5dhiN2";
#elif 1
	const char* faceIdDevice = "spire1";
	const char* addr = "10.77.1.132";
	uint16_t port = 1883;
	const char* username = "app:beverages";
	const char* password = "maiFini9";
#elif 1
	const char* faceIdDevice = "spire2";
	const char* addr = "10.77.1.132";
	uint16_t port = 1883;
	const char* username = "app:beverages";
	const char* password = "maiFini9";
#endif
//	return new DummyOrderMaster(eventEngine);
	return new FaceIdOrderMaster(tcpConn, timerEngine, eventEngine, faceIdDevice, addr, port, username, password);
#else
	ConfigAutomat *automat = config->getAutomat();
	return new FaceIdOrderMaster(tcpConn, timerEngine, eventEngine, automat->getFidDevice(), automat->getFidAddr(), automat->getFidPort(), automat->getFidUsername(), automat->getFidPassword());
#endif
}

OrderDeviceInterface *SaleManagerOrderSpire::initDevice(ConfigModem *config, TimerEngine *timerEngine, EventEngine *eventEngine, Uart *slaveUart, Uart *masterUart) {
	slaveUart->setup(9600, Uart::Parity_None, 0, Uart::Mode_9Bit, GPIO_OType_OD);
	slaveUart->setTransmitBufferSize(80);
	slaveUart->setReceiveBufferSize(80);
	slaveUart->clear();

	masterUart->setup(9600, Uart::Parity_None, 0, Uart::Mode_9Bit, GPIO_OType_OD);
	masterUart->setTransmitBufferSize(80);
	masterUart->setReceiveBufferSize(80);
	masterUart->clear();

	ConfigAutomat *configAutomat = config->getAutomat();
	MdbSlaveSpire *slaveCashless1 = new MdbSlaveSpire(Mdb::Device_CashlessDevice1, Mdb::FeatureLevel_3, configAutomat->getSMContext(), timerEngine, eventEngine, config->getStat());

	engine = new MdbSnifferEngine(config->getStat());
	engine->init(slaveUart, masterUart);
	engine->addSlave(slaveCashless1);

	return slaveCashless1;
}
