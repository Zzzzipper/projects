/*
 * Reciver.cpp
 *
 * Created: 09.06.2014 20:02:34
 *  Author: Vladimir
 */

#include "DexReceiver.h"
#include "DexProtocol.h"
#include "DexCrc.h"
#include "include/DexServer.h"

#include "logger/include/Logger.h"

namespace Dex {

Receiver::Receiver(Customer *customer) : customer(customer), state(State_Idle), buf(DEX_PACKET_MAX_SIZE) {}

Receiver::~Receiver() {}

void Receiver::setConnection(AbstractUart *uart) {
	this->uart = uart;
	this->uart->setReceiveHandler(this);
}

void Receiver::reset() {
	state = State_Wait;
}

void Receiver::handle() {
	while(uart->isEmptyReceiveBuffer() == false) {
		uint8_t b1 = uart->receive();
		LOG_TRACE_HEX(LOG_DEX, b1);
		if(state == State_Wait) {
			if(b1 == DexControl_ENQ) {
				customer->procControl(DexControl_ENQ);
			} else if(b1 == DexControl_EOT) {
				customer->procControl(DexControl_EOT);
			} else if(b1 == DexControl_NAK) {
				customer->procControl(DexControl_NAK);
			} else if(b1 == DexControl_DLE) {
				state = State_DLE;
			}
		} else if(state == State_DLE) {
			if(b1 == DexControl_SOH || b1 == DexControl_STX) {
				buf.clear();
				LOG_TRACE(LOG_DEX, "SOH");
				state = State_Data;
			} else {
				state = State_Wait;
				customer->procConfirm(b1);
			}
		} else if(state == State_Data) {
			if(b1 == DexControl_DLE) {
				LOG_TRACE(LOG_DEX, "DLE");
				state = State_Flag;
			} else {
				buf.addUint8(b1);
			}
		} else if(state == State_Flag) {
			flag = b1;
			LOG_TRACE(LOG_DEX, "Flag");
			state = State_Crc0;
		} else if(state == State_Crc0) {
			crc0 = b1;
			LOG_TRACE(LOG_DEX, "CRC0");
			state = State_Crc1;
		} else if(state == State_Crc1) {
			crc1 = b1;
			state = State_Wait;
			LOG_TRACE(LOG_DEX, "CRC1");
			if(checkDataCrc() == false) {
				LOG_ERROR(LOG_DEX, "Wrong data crc.");
				customer->procWrongCrc();
				return;
			}
			customer->procData(buf.getData(), buf.getLen(), flag == DexControl_ETX);
		}
	}
}

bool Receiver::checkDataCrc() {
	Crc crc;
	crc.start();
	crc.add(buf.getData(), buf.getLen());
	crc.addUint8(flag);
	return (crc.getLowByte() == crc0 && crc.getHighByte() == crc1);
}

}
