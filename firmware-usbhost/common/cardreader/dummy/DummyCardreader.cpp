#include "include/DummyCardreader.h"

namespace Dummy {

Cashless::Cashless(
	Mdb::DeviceContext *context,
	AbstractUart *uart,
	TcpIp *tcpIp,
	TimerEngine *timerEngine,
	EventEngineInterface *eventEngine
) :
	context(context),
	timerEngine(timerEngine),
	eventEngine(eventEngine),
	deviceId(eventEngine)
{
	(void)uart;
	(void)tcpIp;
	this->timer = timerEngine->addTimer<Cashless, &Cashless::procTimer>(this);
}

Cashless::~Cashless() {
	timerEngine->deleteTimer(this->timer);
}

EventDeviceId Cashless::getDeviceId() {
	return deviceId;
}

void Cashless::reset() {
}

bool Cashless::isRefundAble() {
	return false;
}

void Cashless::disable() {
}

void Cashless::enable() {
}

bool Cashless::revalue(uint32_t credit) {
	(void)credit;
	return false;
}

bool Cashless::sale(
	uint16_t productId,
	uint32_t productPrice,
	const char *productName,
	uint32_t wareId
) {
	(void)productId;
	(void)productName;
	(void)wareId;
	this->productPrice = productPrice;
	this->timer->start(2000);
	return true;
}

bool Cashless::saleComplete() {
	return true;
}

bool Cashless::saleFailed() {
	return true;
}

bool Cashless::closeSession() {
	return true;
}

bool Cashless::drawQrCode(
	const char *header,
	const char *footer,
	const char *text
) {
	(void)header;
	(void)footer;
	(void)text;
	return true;
}

bool Cashless::verification() {
	return true;
}

void Cashless::procTimer() {
	MdbMasterCashlessInterface::EventApproved event(deviceId, Fiscal::Payment_Cashless, productPrice);
	eventEngine->transmit(&event);
}

}
