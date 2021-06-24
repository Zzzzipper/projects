#include "DexSender.h"
#include "DexProtocol.h"
#include "DexCrc.h"
#include "uart/include/interface.h"
#include "timer/include/TimerEngine.h"
#include "logger/include/Logger.h"

namespace Dex {

Sender::Sender() : timer(NULL), parity(0) {}

void Sender::init(AbstractUart *uart, TimerEngine *timers) {
	this->uart = uart;
	this->timer = timers->addTimer<Sender, &Sender::procTimer>(this);
}

void Sender::sendControl(uint8_t control) {
	messageType = Type_Control;
	control1 = control;
	timer->start(DEX_TIMEOUT_SENDING);
}

void Sender::sendConfirm() {
	messageType = Type_Confirm;
	timer->start(DEX_TIMEOUT_SENDING);
}

void Sender::repeatConfirm() {
    LOG_DEBUG(LOG_DEX, "repeatConfirm");
    LOG_TRACE_HEX(LOG_DEX, DexControl_DLE);
    uart->send(DexControl_DLE);
    if(parity == 1) {
        LOG_TRACE_HEX(LOG_DEX, '0');
        uart->send('0');
    } else {
        LOG_TRACE_HEX(LOG_DEX, '1');
        uart->send('1');
    }
}

void Sender::sendData(const void *data, const uint16_t len, const uint8_t control1, const uint8_t control2) {
	messageType = Type_Data;
	this->data = data;
	this->dataLen = len;
	this->control1 = control1;
	this->control2 = control2;
	timer->start(DEX_TIMEOUT_SENDING);
}

void Sender::procTimer() {
	LOG_TRACE(LOG_DEX, "procTimer");
	switch(messageType) {
		case Type_Control: sendControl2(); break;
		case Type_Confirm: sendConfirm2(); break;
		case Type_Data: sendData2(); break;
	}
}

void Sender::sendControl2() {
    LOG_DEBUG(LOG_DEX, "sendControl2");
    LOG_TRACE_HEX(LOG_DEX, control1);
    uart->send(control1);
}

void Sender::sendConfirm2() {
    LOG_DEBUG(LOG_DEX, "sendConfirm2");
    LOG_TRACE_HEX(LOG_DEX, DexControl_DLE);
    uart->send(DexControl_DLE);
	if(parity == 0) {
        LOG_TRACE_HEX(LOG_DEX, '0');
        uart->send('0');
		this->parity = 1;
	} else {
        LOG_TRACE_HEX(LOG_DEX, '1');
        uart->send('1');
		this->parity = 0;
	}
}

void Sender::sendData2() {
    LOG_DEBUG(LOG_DEX, "sendData2");
    uart->send(DexControl_DLE);
	LOG_TRACE_HEX(LOG_DEX, DexControl_DLE);
	uart->send(control1);
	LOG_TRACE_HEX(LOG_DEX, control1);

    uint8_t *d = (uint8_t*)data;
    for(uint16_t i = 0; i < dataLen; i++) {
        uart->send(d[i]);
		LOG_TRACE_HEX(LOG_DEX, d[i]);
	}

    uart->send(DexControl_DLE);
	LOG_TRACE_HEX(LOG_DEX, DexControl_DLE);
    uart->send(control2);
	LOG_TRACE_HEX(LOG_DEX, control2);

	Crc crc;
	crc.start();
    crc.add(data, dataLen);
	crc.addUint8(control2);
	uart->send(crc.getLowByte());
	LOG_TRACE_HEX(LOG_DEX, crc.getLowByte());
	uart->send(crc.getHighByte());
	LOG_TRACE_HEX(LOG_DEX, crc.getHighByte());
}

}
