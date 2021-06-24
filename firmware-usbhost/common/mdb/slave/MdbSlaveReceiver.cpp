#include "MdbSlaveReceiver.h"
#include "mdb/MdbProtocol.h"
#include "logger/include/Logger.h"

MdbSlaveReceiver::MdbSlaveReceiver(Customer *context) : context(context), uart(NULL), buf(MDB_PACKET_MAX_SIZE) {}

void MdbSlaveReceiver::setUart(AbstractUart *uart, AbstractUart *sendUart) {
	this->uart = uart;
	this->uart->setReceiveHandler(this);
	this->sendUart = sendUart;
}

void MdbSlaveReceiver::recvAddress() {
	this->setLen(1);
	this->state = State_Idle;
}

void MdbSlaveReceiver::recvSubcommand() {
	this->setLen(1);
	this->recvLen = sizeof(Mdb::Header);
	this->state = State_Subcommand;
}

void MdbSlaveReceiver::recvRequest(uint16_t len) {
	this->setLen(1);
	this->recvLen = len + 1;
	this->state = State_Request;
}

void MdbSlaveReceiver::recvConfirm() {
	this->setLen(1);
	this->state = State_Confirm;
}

void MdbSlaveReceiver::stop() {
	this->setLen(0);
}

void MdbSlaveReceiver::handle() {
	if(uart == NULL) {
		return;
	}
	while(uart->isEmptyReceiveBuffer() == false) {
		uint8_t b1 = uart->receive();
		uint8_t b2 = uart->receive();
		LOG_TRACE(LOG_MDBS, "req: b1=" << b1 << ",b2=" << b2);
		if(sendUart != NULL) {
			sendUart->sendAsync(b1);
			sendUart->sendAsync(b2);
		}
		if(b1 == 0x01) {
			buf.clear();
			buf.addUint8(b2);
			context->recvAddress(buf[0]);
		} else {
			if(state == State_Subcommand) {
				buf.addUint8(b2);
				if(buf.getLen() == recvLen) {
					state = State_Idle;
					context->recvSubcommand(buf[1]);
					return;
				}
			} else if(state == State_Request) {
				buf.addUint8(b2);
				if(buf.getLen() == recvLen) {
					state = State_Idle;
					uint16_t len = buf.getLen() - 1;
					uint8_t actCrc = Mdb::calcCrc(buf.getData(), len);
					uint8_t expCrc = buf[len];
					if(actCrc != expCrc) {
						LOG_ERROR(LOG_MDBS, "Wrong CRC " << actCrc << "<>" << expCrc);
						LOG_ERROR_HEX(LOG_MDBS, buf.getData(), buf.getLen());
						return;
					}
					context->recvRequest(buf.getData(), len);
					return;
				}
			} else if(state == State_Confirm) {
				state = State_Idle;
				context->recvConfirm(b2);
				return;
			}
		}
	}
}
