#include "include/ExeSniffer.h"
#include "ExeProtocol.h"
#include "logger/include/Logger.h"

using namespace Exe;

ExeSniffer::ExeSniffer(EventEngineInterface *eventEngine, StatStorage *stat) :
	eventEngine(eventEngine),
	deviceId(eventEngine),
	state(State_NotReady),
	packetLayer(this, stat)
{
}

ExeSniffer::~ExeSniffer() {
}

void ExeSniffer::init(AbstractUart *slaveUart, AbstractUart *masterUart) {
	LOG_INFO(LOG_EXE, "init");
	packetLayer.init(slaveUart, masterUart);
}

void ExeSniffer::reset() {
	ATOMIC {
		LOG_INFO(LOG_EXE, "reset");
		this->commandByte = 0;
		this->state = State_NotReady;
	}
}

void ExeSniffer::recvByte(uint8_t command, uint8_t result) {
	switch (state) {
	case State_NotReady: stateNotReadyCommand(command, result); break;
	case State_Ready: stateReadyCommand(command, result); break;
	case State_Approving: stateApprovingCommand(command, result); break;
	default: LOG_ERROR(LOG_EXE, "Wrong state " << state);
	}
}

void ExeSniffer::recvData(uint8_t *data, uint16_t dataLen) {

}

void ExeSniffer::stateNotReadyCommand(uint8_t command, uint8_t result) {
	if(command == Command_Status) {
		if((result & CommandStatus_VendingInhibited) == 0) {
			LOG_DEBUG(LOG_EXE, "Goto State_Ready");
			state = State_Ready;
			return;
		}
	}
}

void ExeSniffer::stateReadyCommand(uint8_t command, uint8_t result) {
	if(command == Command_Status) {
		if(result & CommandStatus_VendingInhibited) {
			LOG_DEBUG(LOG_EXE, "Goto State_NotReady");
			state = State_NotReady;
			return;
		}
	} else if(command == Command_Credit) {
		if(result == CommandCredit_NoVendRequest) {
			LOG_TRACE(LOG_EXE, "No vend request");
			return;
		}
		LOG_INFO(LOG_EXE, "Sale request " << result);
		productId = result;
		state = State_Approving;
		return;
	}
}

void ExeSniffer::stateApprovingCommand(uint8_t command, uint8_t result) {
	if(command == Command_Vend) {
		if(command & CommandVend_ResultFlag) {
			LOG_DEBUG(LOG_EXE, "Vending " << productId << " failed");
			state = State_Ready;
			return;
		}
		LOG_INFO(LOG_EXE, "Vend complete " << productId);
		state = State_Ready;
		EventUint8Interface event(deviceId, Event_Sale, productId);
		eventEngine->transmit(&event);
		return;
	} else if(command == Command_Credit) {
		if(result == CommandCredit_NoVendRequest) {
			LOG_TRACE(LOG_EXE, "No vend request");
			return;
		}
		LOG_INFO(LOG_EXE, "Sale request " << result);
		productId = result;
		state = State_Approving;
		return;
	}
}
