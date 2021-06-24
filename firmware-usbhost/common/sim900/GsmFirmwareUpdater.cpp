#include "GsmFirmwareUpdater.h"
#include "GsmHardware.h"

#include "timer/include/TimerEngine.h"
#include "logger/include/Logger.h"

namespace Gsm {

#define FIRMWARE_HEADER_SIZE 128

FirmwareUpdater::FirmwareUpdater(HardwareInterface *hardware, AbstractUart *uart, TimerEngine *timers) :
	hardware(hardware),
	uart(uart),
	timers(timers),
	state(State_Idle),
	data(128)
{
	this->timer = timers->addTimer<FirmwareUpdater, &FirmwareUpdater::procTimer>(this);
	this->uart->setReceiveHandler(this);
	this->hardware->init();
}

FirmwareUpdater::~FirmwareUpdater() {
	this->timers->deleteTimer(this->timer);
}

void FirmwareUpdater::setObserver(EventObserver *observer) {
	courier.setRecipient(observer);
}

Dex::DataParser::Result FirmwareUpdater::start(uint32_t dataSize) {
	LOG_DEBUG(LOG_SIM, "start " << dataSize);
	if(state != State_Idle) {
		LOG_ERROR(LOG_SIM, "Unwaited start " << state);
		return Result_Error;
	}
	gotoStatePowerButtonPress();
	return Result_Ok;
}

Dex::DataParser::Result FirmwareUpdater::procData(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_SIM, "procData");
	switch(state) {
	case State_PowerButtonPress: return Result_Busy;
	case State_PowerButtonDelay: return Result_Busy;
	case State_Sync: return Result_Busy;
	case State_HeaderRecv: stateHeaderRecvData(data, len); return Result_Ok;
	case State_HeaderSend: return Result_Busy;
	case State_DataRecv: stateDataRecvData(data, len); return Result_Async;
	case State_DataSend: return Result_Busy;
	case State_Error: return Result_Error;
	default: return Result_Async;
	}
}

Dex::DataParser::Result FirmwareUpdater::complete() {
	LOG_DEBUG(LOG_SIM, "complete");
	if(state != State_DataRecv) {
		LOG_ERROR(LOG_SIM, "Unwaited complete " << state);
		return Result_Error;
	}
	gotoStateEnd();
	return Result_Async;
}

void FirmwareUpdater::error() {
	LOG_DEBUG(LOG_SIM, "error");
	state = State_Idle;
}

/*
 * TODO: В случае ошибки обновления GSM-модуль начинает спамить сообщеним об ошибке.
 * Обработчик потока не справляется с входящими данными и застревает в этом цикле.
 * Требуется проверка этой гипотезы, так как производительности по идее должно хватать,
 * чтобы всё обработать и выйти из цикла.
 */
void FirmwareUpdater::handle() {
	LOG_TRACE(LOG_SIM, "handle");
	while(uart->isEmptyReceiveBuffer() == false) {
		LOG_TRACE(LOG_SIM, "have data");
		switch(state) {
		case State_Sync: stateSyncRecv(); break;
		case State_HeaderSend: stateHeaderSendRecv(); break;
		case State_PacketSize: statePacketSizeRecv(); break;
		case State_DataSend: stateDataSendRecv(); break;
		case State_End: stateEndRecv(); break;
		case State_Reset: stateResetRecv(); break;
		case State_Error: stateErrorRecv(); break;
		default: LOG_DEBUG(LOG_SIM, "Unwaited data state=" << state << ", byte=" << uart->receive());
		}
	}
}

void FirmwareUpdater::procTimer() {
	switch(state) {
	case State_PowerButtonPress: gotoStatePowerButtonDelay(); break;
	case State_PowerButtonDelay: gotoStateSync(); break;
	case State_Sync: stateSyncTimeout(); break;
	case State_Error: stateErrorTimeout(); break;
	default: LOG_ERROR(LOG_SIM, "Unwaited timeout: " << state); return;
	}
}

void FirmwareUpdater::gotoStatePowerButtonPress() {
#if 0
	LOG_INFO(LOG_SIM, "gotoStatePowerButtonPress");
	hardware->pressPowerButton();
	timer->start(POWER_BUTTON_PRESS);
	state = State_PowerButtonPress;
#else
	LOG_INFO(LOG_SIM, "gotoStatePowerButtonPress");
	hardware->pressPowerButton();
	gotoStateSync();
#endif
}

void FirmwareUpdater::gotoStatePowerButtonDelay() {
	LOG_INFO(LOG_SIM, "gotoStatePowerButtonDelay");
	hardware->releasePowerButton();
	timer->start(1);
	state = State_PowerButtonDelay;
}

void FirmwareUpdater::gotoStateSync() {
	LOG_INFO(LOG_SIM, "gotoStateSync");
	uart->send(Control_Sync);
	tries = SYNC_TRIES_MAX;
	timer->start(SYNC_TIMEOUT);
	state = State_Sync;
}

void FirmwareUpdater::stateSyncTimeout() {
	LOG_DEBUG(LOG_SIM, "stateSyncTimeout " << tries);
	if(tries <= 0) {
		LOG_ERROR(LOG_SIM, "Too many sync tries.");
		gotoStateError();
		return;
	}
	tries--;
	uart->send(Control_Sync);
	timer->start(SYNC_TIMEOUT);
}

void FirmwareUpdater::stateSyncRecv() {
	LOG_DEBUG(LOG_SIM, "stateSyncRecv");
	uint8_t b1 = uart->receive();
	LOG_DEBUG(LOG_SIM, "recv=" << b1 << "," << (char)b1);
	if(b1 == Control_SyncComplete) {
		gotoStateHeaderRecv();
		return;
	} else if(b1 == Control_EraseError) {
		gotoStateError();
		return;
	} else if(b1 == Control_ENQ) {
		gotoStateError();
		return;
	} else {
		LOG_ERROR(LOG_SIM, "Unwaited answer " << b1);
		return;
	}
}

void FirmwareUpdater::gotoStateHeaderRecv() {
	LOG_DEBUG(LOG_SIM, "gotoStateHeaderRecv");
	state = State_HeaderRecv;
}

void FirmwareUpdater::stateHeaderRecvData(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_SIM, "stateHeaderRecvData");
	if(len != FIRMWARE_HEADER_SIZE) {
		LOG_ERROR(LOG_SIM, "Wrong header size " << FIRMWARE_HEADER_SIZE << "<>" << len);
		return;
	}
	uart->send(Control_Erase);
	for(uint16_t i = 0; i < len; i++) {
		uart->send(data[i]);
	}
	count = len;
	state = State_HeaderSend;
}

void FirmwareUpdater::stateHeaderSendRecv() {
	LOG_DEBUG(LOG_SIM, "stateHeaderSendRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_Erasing) {
		LOG_INFO(LOG_SIM, "Erasing...");
		return;
	} else if(b1 == Control_EraseComplete) {
		LOG_INFO(LOG_SIM, "Erase complete");
		data.clear();
		state = State_PacketSize;
		return;
	} else if(b1 == Control_EraseError) {
		LOG_ERROR(LOG_SIM, "Erase failed");
		gotoStateError();
		return;
	} else {
		LOG_ERROR(LOG_SIM, "Unwaited answer " << b1);
		return;
	}
}

void FirmwareUpdater::statePacketSizeRecv() {
	LOG_DEBUG(LOG_SIM, "statePacketSizeRecv");
	uint8_t b1 = uart->receive();
	data.addUint8(b1);
	if(data.getLen() < 2) {
		return;
	} else {
		maxSize = ((uint16_t)data[1] << 8) + data[0];
		LOG_DEBUG(LOG_SIM, "Packet max size " << maxSize);
		state = State_DataRecv;
	}
}

void FirmwareUpdater::stateDataRecvData(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_SIM, "stateDataRecvData " << len << "/" << count);
	uart->send(Control_Data);
	uart->send(len & 0xFF);
	uart->send((len >> 8) & 0xFF);
	uart->send(0);
	uart->send(0);
	uint32_t crc = 0;
	for(uint16_t i = 0; i < len; i++) {
		uart->send(data[i]);
		crc += data[i];
	}
	uart->send(crc & 0xFF);
	uart->send((crc >>  8) & 0xFF);
	uart->send((crc >> 16) & 0xFF);
	uart->send((crc >> 24) & 0xFF);
	count += len;
	state = State_DataSend;
}

void FirmwareUpdater::stateDataSendRecv() {
	LOG_DEBUG(LOG_SIM, "stateDataSendRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_DataConfirm) {
		LOG_INFO(LOG_SIM, ".");
		state = State_DataRecv;
		courier.deliver(Event_AsyncOk);
		return;
	} else {
		LOG_ERROR(LOG_SIM, "Unwaited answer " << b1);
		return;
	}
}

void FirmwareUpdater::gotoStateEnd() {
	LOG_DEBUG(LOG_SIM, "gotoStateEnd");
	uart->send(Control_DataEnd);
	state = State_End;
}

void FirmwareUpdater::stateEndRecv() {
	LOG_DEBUG(LOG_SIM, "stateEndRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_DataEndConfirm) {
		gotoStateReset();
		return;
	} else {
		LOG_ERROR(LOG_SIM, "Unwaited answer " << b1);
		return;
	}
}

void FirmwareUpdater::gotoStateReset() {
	LOG_DEBUG(LOG_SIM, "gotoStateReset");
	uart->send(Control_Reset);
	state = State_Reset;
}

void FirmwareUpdater::stateResetRecv() {
	LOG_DEBUG(LOG_SIM, "stateEndRecv");
	uint8_t b1 = uart->receive();
	if(b1 == Control_ResetConfirm) {
		LOG_INFO(LOG_SIM, "SUCCEED");
		state = State_Idle;
		courier.deliver(Event_AsyncOk);
		return;
	} else {
		LOG_ERROR(LOG_SIM, "Unwaited answer " << b1);
		return;
	}
}

void FirmwareUpdater::gotoStateError() {
	LOG_INFO(LOG_SIM, "gotoStateError");
	timer->start(AT_COMMAND_DELAY);
	state = State_Error;
}

void FirmwareUpdater::stateErrorRecv() {
	LOG_INFO(LOG_SIM, "stateErrorRecv");
	uint8_t b1 = uart->receive();
	if(b1 != Control_ResetConfirm) {
		uart->send(Control_Reset);
	}
}

void FirmwareUpdater::stateErrorTimeout() {
	LOG_INFO(LOG_SIM, "stateErrorTimeout");
	uart->send(Control_Reset);
}

}
