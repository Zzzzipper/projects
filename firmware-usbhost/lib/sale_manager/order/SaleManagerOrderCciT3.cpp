#include <lib/sale_manager/order/include/SaleManagerOrderCciT3.h>
#include <lib/sale_manager/order/DummyOrderMaster.h>
#include <lib/sale_manager/order/FaceIdOrderMaster.h>
#include <lib/sale_manager/order/SaleManagerOrderCore.h>
#include "lib/sale_manager/order/CciT3OrderDevice.h"

#include "common/ccicsi/CciT3Driver.h"
#include "common/uart/stm32/include/uart.h"
#include "common/logger/include/Logger.h"

#include <string.h>

SaleManagerOrderCciT3::SaleManagerOrderCciT3(
	ConfigModem *config,
	ClientContext *client,
	TimerEngine *timerEngine,
	EventEngine *eventEngine,
	Uart *deviceUart,
	Uart *pincodeUart,
	TcpIp *tcpConn,
	Fiscal::Register *fiscalRegister,
	LedInterface *leds,
	EventRegistrar *chronicler
) {
	OrderMasterInterface *master = initMaster(config, timerEngine, eventEngine, tcpConn);
	OrderDeviceInterface *device = initDevice(timerEngine, eventEngine, deviceUart, pincodeUart);
	core = new SaleManagerOrderCore(config, client, timerEngine, eventEngine, master, device, fiscalRegister, leds, chronicler);
}

SaleManagerOrderCciT3::~SaleManagerOrderCciT3() {
	delete core;
}

void SaleManagerOrderCciT3::reset() {
	core->reset();
}

void SaleManagerOrderCciT3::shutdown() {
	core->shutdown();
}

OrderMasterInterface *SaleManagerOrderCciT3::initMaster(ConfigModem *config, TimerEngine *timerEngine, EventEngine *eventEngine, TcpIp *tcpConn) {
#if 0
#if 0
	const char* faceIdDevice = "ephor1";
	const char* addr = "95.216.78.99";
	uint16_t port = 1883;
	const char* username = "ephor";
	const char* password = "2kDR8TMCu5dhiN2";
#elif 0
	const char* faceIdDevice = "thermoplan1";
	const char* addr = "10.77.1.132";
	uint16_t port = 1883;
	const char* username = "app:beverages";
	const char* password = "maiFini9";
#elif 1
	const char* faceIdDevice = "thermoplan2";
	const char* addr = "10.77.1.132";
	uint16_t port = 1883;
	const char* username = "app:beverages";
	const char* password = "maiFini9";
#endif
	return new FaceIdOrderMaster(tcpConn, timerEngine, eventEngine, faceIdDevice, addr, port, username, password);
#else
	ConfigAutomat *automat = config->getAutomat();
	return new FaceIdOrderMaster(tcpConn, timerEngine, eventEngine, automat->getFidDevice(), automat->getFidAddr(), automat->getFidPort(), automat->getFidUsername(), automat->getFidPassword());
#endif
}

OrderDeviceInterface *SaleManagerOrderCciT3::initDevice(TimerEngine *timerEngine, EventEngine *eventEngine, Uart *deviceUart, Uart *pincodeUart) {
	deviceUart->setup(9600, Uart::Parity_None, 0);
	deviceUart->setTransmitBufferSize(256);
	deviceUart->setReceiveBufferSize(256);
	if(pincodeUart != NULL) {
		pincodeUart->setup(9600, Uart::Parity_None, 0);
		pincodeUart->setTransmitBufferSize(256);
		pincodeUart->setReceiveBufferSize(256);
	}
	return new Cci::T3::OrderDevice(deviceUart, pincodeUart, timerEngine, eventEngine);
}
