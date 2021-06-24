#include <logger/RemoteLogger.h>
#include "ExeMasterPacketLayer.h"
#include "common/executive/ExeProtocol.h"
#include "common/mdb/MdbProtocol.h"
#include "common/logger/include/Logger.h"

using namespace Exe;

#ifndef TEST
#define UART_9BIT
#endif

ExeMasterPacketLayer::ExeMasterPacketLayer(AbstractUart *uart, TimerEngine *timers, Customer *customer, StatStorage *stat) :
	uart(uart),
	timers(timers),
	customer(customer),
	state(State_Idle),
	data(EXE_DATA_SIZE)
{
	this->countCrcError = stat->add(Mdb::DeviceContext::Info_ExeM_CrcErrorCount, 0);
	this->countResponse = stat->add(Mdb::DeviceContext::Info_ExeM_ResponseCount, 0);
	this->countTimeout = stat->add(Mdb::DeviceContext::Info_ExeM_TimeoutCount, 0);
	this->timerRecv = timers->addTimer<ExeMasterPacketLayer, &ExeMasterPacketLayer::procRecvTimer>(this, TimerEngine::ProcInTick);
	this->uart->setReceiveHandler(this);
}

ExeMasterPacketLayer::~ExeMasterPacketLayer() {
	timers->deleteTimer(timerRecv);
}

void ExeMasterPacketLayer::reset() {
	LOG_INFO(LOG_EXEP, "reset");
	timerRecv->stop();
	state = State_Delay;
}

void ExeMasterPacketLayer::sendCommand(uint8_t command, uint32_t timeout) {
	LOG_DEBUG(LOG_EXEP, "sendCommand " << state);
#ifdef UART_9BIT
	sendParityByte(Device_VMC | Flag_Command | command);
#else
	uart->send(Device_VMC | Flag_Command | command);
#endif
	timerRecv->start(timeout);
	state = State_Command;
}

void ExeMasterPacketLayer::sendData(uint16_t baseUnit, uint8_t scalingFactor, uint8_t decimalPoint, bool changeFlag, uint32_t timeout) {
	LOG_DEBUG(LOG_EXEP, "sendData " << state);
	data.clear();
	data.addUint8(Device_VMC | Flag_Command | Command_AcceptData);
	data.addUint8(Device_VMC | (0x0F &  baseUnit));
	data.addUint8(Device_VMC | (0x0F & (baseUnit >> 4)));
	data.addUint8(Device_VMC | (0x0F & (baseUnit >> 8)));
	data.addUint8(Device_VMC | (0x0F & (baseUnit >> 12)));
	data.addUint8(Device_VMC | (0x0F &  scalingFactor));
	data.addUint8(Device_VMC | (0x0F & (scalingFactor >> 4)));
	data.addUint8(Device_VMC | (0x0F &  decimalPointToDataFormat(decimalPoint)));
	data.addUint8(Device_VMC | (0x0F & (changeFlag == true ? 0 : Flag_ExactChange)));
	data.addUint8(Device_VMC | Flag_Command | Command_DataSync);

	this->dataCount = 0;
	this->tryCount = 0;
	this->timeout = timeout;
#ifdef UART_9BIT
	sendParityByte(data[dataCount]);
#else
	uart->send(data[dataCount]);
#endif
	LOG_DEBUG(LOG_EXEP, "sendData " << state << "," << data[dataCount]);
	timerRecv->start(timeout);
	state = State_Data;
}

void ExeMasterPacketLayer::handle() {
	LOG_TRACE(LOG_EXEP, "handle");
	while(uart->isEmptyReceiveBuffer() == false) {
#ifdef UART_9BIT
		uint8_t b1 = uart->receive();
		uint8_t b2 = uart->receive();
		LOG_TRACE(LOG_EXEP, "recv: b1=" << b1 << ",b2=" << b2);
		REMOTE_LOG_IN(RLOG_EXE, b2);
		if(calcParity(b2) != b1) {
			LOG_ERROR(LOG_EXEP, "Wrong parity");
			countCrcError->inc();
#ifndef YARVEND_EXE
			continue;
#endif
		}
#else
		uint8_t b2 = uart->receive();
		LOG_TRACE(LOG_EXEP, "recv: b2=" << b2);
#endif
		countResponse->inc();
		switch(state) {
		case State_Command: stateCommandRecv(b2); break;
		case State_Data: stateDataRecv(b2); break;
		default: LOG_ERROR(LOG_EXEP, "Unwaited byte " << state << "," << b2);
		}
	}
}

bool ExeMasterPacketLayer::isInterruptMode() {
	return true;
}

void ExeMasterPacketLayer::procRecvTimer() {
	LOG_DEBUG(LOG_EXEP, "procRecvTimer " << state);
	countTimeout->inc();
	switch(state) {
	case State_Command: stateCommandRecvTimeout(); break;
	case State_Data: stateDataRecvTimeout(); break;
	default: LOG_ERROR(LOG_EXEP, "Unwaited timeout " << state);
	}
}

uint8_t ExeMasterPacketLayer::calcParity(uint8_t byte) {
	uint8_t p = 0;
	for(uint8_t i = 0; i < 8; i++) {
		p += (byte >> i) & 0x01;
	}
	return (p & 0x01);
}

void ExeMasterPacketLayer::sendParityByte(uint8_t b) {
	uint8_t p = calcParity(b);
	LOG_TRACE(LOG_EXEP, "sendParityByte1 " << p);
	uart->sendAsync(p);
	LOG_TRACE(LOG_EXEP, "sendParityByte2 " << b);
	uart->sendAsync(b);
	REMOTE_LOG_OUT(RLOG_EXE, b);
}

void ExeMasterPacketLayer::gotoStateDelay() {
	LOG_DEBUG(LOG_EXEP, "gotoStateDelay");
#if 0
	timerSend->start(EXE_POLL_TIMEOUT);
#endif
	state = State_Delay;
}

void ExeMasterPacketLayer::stateCommandRecv(uint8_t b) {
	LOG_DEBUG(LOG_EXEP, "stateCommandRecv");
	gotoStateDelay();
	timerRecv->stop();
	customer->recvByte(b);
}

void ExeMasterPacketLayer::stateCommandRecvTimeout() {
	LOG_DEBUG(LOG_EXEP, "stateCommandRecvTimeout");
	gotoStateDelay();
	customer->recvTimeout();
}

void ExeMasterPacketLayer::stateDataRecv(uint8_t b) {
	LOG_DEBUG(LOG_EXEP, "stateDataRecv " << dataCount << "/" << data.getLen());
#if 0
	if(b == Control_PNAK) {
		timerSend->start(EXE_SEND_TIMEOUT);
		return;
	}
#endif

	dataCount++;
	if(dataCount < data.getLen()) {
#if 0
		timerSend->start(EXE_SEND_TIMEOUT);
#else
#ifdef UART_9BIT
		sendParityByte(data[dataCount]);
#else
		uart->send(data[dataCount]);
#endif
#endif
		return;
	} else {
		gotoStateDelay();
		timerRecv->stop();
		customer->recvByte(Control_ACK);
		return;
	}
}

void ExeMasterPacketLayer::stateDataRecvTimeout() {
	LOG_DEBUG(LOG_EXEP, "stateDataRecvTimeout");
	tryCount++;
	if(tryCount < 10) {
		dataCount = 0;
#ifdef UART_9BIT
		sendParityByte(data[dataCount]);
#else
		uart->send(data[dataCount]);
#endif
		LOG_DEBUG(LOG_EXEP, "sendData " << state << "," << data[dataCount]);
		REMOTE_LOG(RLOG_EXE, "RETRY");
		timerRecv->start(timeout);
		countCrcError->inc();
	} else {
		gotoStateDelay();
		customer->recvTimeout();
	}
}

uint8_t ExeMasterPacketLayer::decimalPointToDataFormat(uint8_t decimalPoint) {
	switch(decimalPoint) {
	case 0: return DecimalPoint_0;
	case 1: return DecimalPoint_1;
	case 2: return DecimalPoint_2;
	case 3: return DecimalPoint_3;
	default: LOG_ERROR(LOG_EXEP, "Wrong decimal point " << decimalPoint); return DecimalPoint_3;
	}
}
