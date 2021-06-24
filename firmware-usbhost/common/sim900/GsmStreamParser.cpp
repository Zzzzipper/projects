#include "GsmStreamParser.h"
#include "sim900/command/GsmCommand.h"

#include "logger/include/Logger.h"

namespace Gsm {

StreamParser::StreamParser(AbstractUart *uart) :
	uart(uart),
	state(State_Idle),
	crFlag(false),
	buf(AT_RESPONSE_MAX_SIZE)
{
	this->uart->setReceiveHandler(this);
}

void StreamParser::reset() {
	LOG_DEBUG(LOG_SIM, "reset");
	crFlag = false;
	state = State_Line;
}

void StreamParser::sendLine(const char *data) {
	for(int i = 0; data[i] != '\0'; i++) {
		uart->send(data[i]);
	}
	uart->send('\r');
	uart->send('\n');
}

void StreamParser::sendData(const uint8_t *data, uint16_t dataLen) {
	for(int i = 0; i < dataLen; i++) {
		uart->send(data[i]);
	}
}

void StreamParser::readLine(Customer *customer) {
	LOG_DEBUG(LOG_SIM, "readLine");
	this->customer = customer;
	state = State_Line;
}

void StreamParser::waitSymbol(uint8_t symbol, Customer *customer) {
	LOG_DEBUG(LOG_SIM, "waitSymbol");
	this->customer = customer;
	this->symbol = symbol;
	state = State_WaitSymbol;
}

void StreamParser::readData(Customer *customer) {
	LOG_DEBUG(LOG_SIM, "readData");
	this->customer = customer;
	state = State_Data;
}

void StreamParser::handle() {
	LOG_TRACE(LOG_SIM, "handle " << state);
	while(uart->isEmptyReceiveBuffer() == false) {
		uint8_t b = uart->receive();
		LOG_TRACE_HEX(LOG_SIM, b);
		LOG_TRACE_STR(LOG_SIMDATA, b);
		switch(state) {
		case State_Idle: break;
		case State_Line: stateLineByte(b); break;
		case State_WaitSymbol: stateWaitSymbolByte(b); break;
		case State_Data: stateDataByte(b); break;
		default: LOG_ERROR(LOG_SIM, "Unwaited data: " << state << "," << b);
		};
	}
}

void StreamParser::stateLineByte(uint8_t b) {
	LOG_TRACE(LOG_SIM, "procLine");
	if(b == '\r') {
		crFlag = true;
	} else {
		if(crFlag == true && b == '\n') {
			deliverLine();
			buf.clear();
			crFlag = false;
			return;
		} else {
			buf.addUint8(b);
			crFlag = false;
		}
	}
}

void StreamParser::stateWaitSymbolByte(uint8_t b) {
	LOG_TRACE(LOG_SIM, "procLine");
	if(b == '\r') {
		crFlag = true;
	} else {
		if(crFlag == true && b == '\n') {
			deliverLine();
			buf.clear();
			crFlag = false;
			return;
		} else {
			if(buf.getLen() == 0) {
				if(b == symbol) {
					customer->procData(b);
				} else {
					crFlag = false;
					buf.addUint8(b);
				}
			} else {
				buf.addUint8(b);
			}
		}
	}
}

void StreamParser::stateDataByte(uint8_t b) {
	LOG_TRACE(LOG_SIM, "procData");
	if(customer->procData(b) == false) {
		state = State_Line;
		return;
	}
}

void StreamParser::deliverLine() {
	uint16_t len = buf.getLen();
	if(len == 0) {
		return;
	}
	if(len >= buf.getSize()) {
		len = buf.getSize() - 1;
	}
	buf[len] = '\0';
	LOG_DEBUG_STR(LOG_SIMDATA, buf.getData(), len);
	customer->procLine((char*)buf.getData(), len);
}

}
