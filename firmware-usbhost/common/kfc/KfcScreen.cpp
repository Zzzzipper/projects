#include "KfcScreen.h"
#include "logger/include/Logger.h"

namespace Kfc {

Cashless::Cashless(
	AbstractUart *uart,
	TimerEngine *timers,
	EventEngineInterface *eventEngine
) :
	deviceId(eventEngine),
	uart(uart)
{
	uart->setReceiveHandler(this);
}

Cashless::~Cashless() {
}

EventDeviceId Cashless::getDeviceId() {
	return deviceId;
}

void Cashless::reset() {
	LOG("reset");
	uart->send('p');
	uart->send('a');
	uart->send('g');
	uart->send('e');
	uart->send(' ');
	uart->send('0');
	uart->send(0xFF);
	uart->send(0xFF);
	uart->send(0xFF);
}

void Cashless::handle() {
	while(uart->isEmptyReceiveBuffer() == false) {
		uint8_t b = uart->receive();
		LOG_HEX(b);
	}
}

bool Cashless::isRefundAble() {
	return false;
}

void Cashless::disable() {
}

void Cashless::enable() {
}

bool Cashless::revalue(uint32_t credit) {
	return false;
}

bool Cashless::sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) {
	LOG("sale");
	uart->send('p');
	uart->send('a');
	uart->send('g');
	uart->send('e');
	uart->send(' ');
	uart->send('1');
	uart->send(0xFF);
	uart->send(0xFF);
	uart->send(0xFF);
	return false;
}

bool Cashless::saleComplete() {
	LOG("disable");
	uart->send('p');
	uart->send('a');
	uart->send('g');
	uart->send('e');
	uart->send(' ');
	uart->send('2');
	uart->send(0xFF);
	uart->send(0xFF);
	uart->send(0xFF);
	return false;
}

bool Cashless::saleFailed() {
	LOG("enable");
	uart->send('p');
	uart->send('a');
	uart->send('g');
	uart->send('e');
	uart->send(' ');
	uart->send('3');
	uart->send(0xFF);
	uart->send(0xFF);
	uart->send(0xFF);
	return false;
}

bool Cashless::closeSession() {
	return false;
}

bool Cashless::drawQrCode(const char *header, const char *footer, const char *text) {
	return false;
}

bool Cashless::verification() {
	return false;
}

}
