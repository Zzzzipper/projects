#include "include/Sberbank.h"
#include "SberbankCommandLayer.h"
#include "SberbankPacketLayer.h"
#include "SberbankFrameLayer.h"
#include "logger/include/Logger.h"

namespace Sberbank {

Cashless::Cashless(
	Mdb::DeviceContext *context,
	AbstractUart *uart,
	TcpIp *conn,
	TimerEngine *timers,
	EventEngineInterface *eventEngine,
	uint32_t maxCredit
) {
	this->frameLayer = new FrameLayer(timers, uart);
	this->packetLayer = new PacketLayer(timers, frameLayer);
	this->commandLayer = new CommandLayer(context, packetLayer, conn, timers, eventEngine, maxCredit);
}

Cashless::~Cashless() {
	delete this->commandLayer;
	delete this->packetLayer;
	delete this->frameLayer;
}

EventDeviceId Cashless::getDeviceId() {
	return commandLayer->getDeviceId();
}

void Cashless::reset() {
	commandLayer->reset();
}

bool Cashless::isRefundAble() {
	return false;
}

void Cashless::disable() {
	commandLayer->disable();
}

void Cashless::enable() {
	commandLayer->enable();
}

bool Cashless::revalue(uint32_t credit) {
	(void)credit;
	return false;
}

bool Cashless::sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) {
	(void)productName;
	(void)wareId;
	return commandLayer->sale(productId, productPrice);
}

bool Cashless::saleComplete() {
	return commandLayer->saleComplete();
}

bool Cashless::saleFailed() {
	return commandLayer->saleFailed();
}

bool Cashless::closeSession() {
	return commandLayer->closeSession();
}

bool Cashless::drawQrCode(const char *header, const char *footer, const char *text) {
	return commandLayer->printQrCode(header, footer, text);
}

bool Cashless::verification() {
	return commandLayer->verification();
}

}
