#if 1
#include "config.h"
#include "ExeMaster.h"
#include "common/executive/ExeProtocol.h"
#include "common/mdb/MdbProtocol.h"
#include "common/logger/include/Logger.h"

using namespace Exe;

ExeMaster::ExeMaster(AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine, StatStorage *stat) :
	eventEngine(eventEngine),
	timers(timers),
	deviceId(eventEngine),
	command(Command_None),
	decimalPoint(2),
	scalingFactor(1),
	change(false),
	credit(0),
	price(0)
{
	this->state = stat->add(Mdb::DeviceContext::Info_ExeM_State, State_Idle);
	this->timer = timers->addTimer<ExeMaster, &ExeMaster::procTimer>(this, TimerEngine::ProcInTick);
	this->packetLayer = new ExeMasterPacketLayer(uart, timers, this, stat);
}

ExeMaster::~ExeMaster() {
	delete packetLayer;
	timers->deleteTimer(timer);
}

void ExeMaster::reset() {
	ATOMIC {
		command = Command_None;
		gotoStateNotFound();
		packetLayer->reset();
	}
}

void ExeMaster::setDicimalPoint(uint8_t decimalPoint) {
	this->decimalPoint = decimalPoint;
}

void ExeMaster::setScalingFactor(uint8_t scalingFactor) {
	this->scalingFactor = scalingFactor;
}

void ExeMaster::setChange(bool change) {
	this->change = change;
}

void ExeMaster::setCredit(uint32_t credit) {
	LOG_INFO(LOG_EXE, "setCredit " << state->get() << "," << credit);
	ATOMIC {
		this->credit = credit;
		this->command = Command_ChangeCredit;
	}
}

void ExeMaster::setPrice(uint32_t price) {
	LOG_INFO(LOG_EXE, "setPrice " << state->get() << "," << price);
	ATOMIC {
		this->price = price;
		this->command = Command_VendPrice;
	}
}

void ExeMaster::approveVend(uint32_t credit) {
	LOG_INFO(LOG_EXE, "approveVend " << state->get() << "," << credit);
	ATOMIC {
		if(state->get() == State_Approve) {
			gotoStateVending();
		} else {
			this->credit = credit;
			this->command = Command_VendApprove;
		}
	}
}

void ExeMaster::denyVend(uint32_t price) {
	LOG_INFO(LOG_EXE, "denyVend " << state->get() << "," << price);
	ATOMIC {
		if(state->get() == State_Approve) {
			gotoStatePriceDelay();
		} else {
			this->price = price;
			this->command = Command_VendDeny;
		}
	}
}

bool ExeMaster::isEnabled() {
	bool result = false;
	ATOMIC {
		result = (state->get() != State_Idle && state->get() != State_NotFound && state->get() != State_NotReady);
	}
	return result;
}

void ExeMaster::recvByte(uint8_t byte) {
	LOG_TRACE(LOG_EXE, "recvByte " << state->get());
	switch(state->get()) {
	case State_NotFound: stateNotFoundRecv(byte); break;
	case State_NotReady: stateNotReadyRecv(byte); break;
	case State_ReadyStatus: stateReadyStatusRecv(byte); break;
	case State_ReadyCredit: stateReadyCreditRecv(byte); break;
	case State_CreditShow: stateCreditShowRecv(); break;
	case State_CreditDelay: stateCreditDelayRecv(byte); break;
	case State_PriceShow: statePriceShowRecv(); break;
	case State_Vending: stateVendingRecv(byte); break;
	case State_PriceDelay: statePriceDelayRecv(byte); break;
	default: LOG_ERROR(LOG_EXE, "Unwaited byte " << state->get() << "," << byte);
	}
}

void ExeMaster::recvTimeout() {
	LOG_DEBUG(LOG_EXE, "recvTimeout " << state->get());
	switch(state->get()) {
	case State_NotFound: gotoStateNotFound(); break;
	case State_NotReady: gotoStateNotReady(); break;
	case State_ReadyStatus: gotoStateReadyStatus(); break;
	case State_ReadyCredit: gotoStateReadyCredit(); break;
	case State_CreditShow: gotoStateReadyStatus(); break;
	case State_CreditDelay: gotoStateReadyStatus(); break;
	case State_PriceShow: statePriceShowTimeout(); break;
	case State_Vending: stateVendingRecvTimeout(); break;
	case State_PriceDelay: gotoStateReadyStatus(); break;
	default: LOG_ERROR(LOG_EXE, "Unwaited timeout " << state->get());
	}
}

void ExeMaster::procTimer() {
	LOG_DEBUG(LOG_EXE, "procTimer");
	switch(state->get()) {
	case State_NotFound: stateNotFoundSend(); break;
	case State_NotReady: stateNotReadySend(); break;
	case State_ReadyStatus: stateReadyStatusSend(); break;
	case State_ReadyCredit: stateReadyCreditSend(); break;
	case State_CreditShow: stateCreditShowSend(); break;
	case State_CreditDelay: stateCreditDelaySend(); break;
	case State_PriceShow: statePriceShowSend(); break;
	case State_Approve: stateApproveTimeout(); break;
	case State_PriceDelay: statePriceDelaySend(); break;
	default: LOG_ERROR(LOG_EXE, "Unwaited timeout " << state->get());
	}
}

void ExeMaster::gotoStateNotFound() {
	LOG_DEBUG(LOG_EXE, "gotoStateNotFound");
	timer->start(EXE_POLL_TIMEOUT);
	state->set(State_NotFound);
}

void ExeMaster::stateNotFoundSend() {
	LOG_INFO(LOG_EXE, "stateNotFoundSend");
	packetLayer->sendCommand(Command_Status, EXE_COMMAND_TIMEOUT);
}

void ExeMaster::stateNotFoundRecv(uint8_t b) {
	LOG_INFO(LOG_EXE, "stateNotFoundRecv " << b);
	if((b & CommandStatus_VendingInhibited) == 0) {
		gotoStateReadyStatus();
		EventInterface event(deviceId, Event_Ready);
		eventEngine->transmit(&event);
		return;
	}
	gotoStateNotReady();
}

void ExeMaster::gotoStateNotReady() {
	LOG_DEBUG(LOG_EXE, "gotoStateNotReady");
	timer->start(EXE_POLL_TIMEOUT);
	state->set(State_NotReady);
}

void ExeMaster::stateNotReadySend() {
	LOG_INFO(LOG_EXE, "stateNotReadySend");
	packetLayer->sendCommand(Command_Status, EXE_COMMAND_TIMEOUT);
}

void ExeMaster::stateNotReadyRecv(uint8_t b) {
	LOG_INFO(LOG_EXE, "stateNotReadyRecv " << b);
	if((b & CommandStatus_VendingInhibited) == 0) {
		LOG_WARN(LOG_EXE, "Status enabled " << b);
		gotoStateReadyStatus();
		EventInterface event(deviceId, Event_Ready);
		eventEngine->transmit(&event);
		return;
	}
	gotoStateNotReady();
}

void ExeMaster::gotoStateReadyStatus() {
	LOG_DEBUG(LOG_EXE, "gotoStateReadyStatus");
	timer->start(EXE_POLL_TIMEOUT);
	state->set(State_ReadyStatus);
}

void ExeMaster::stateReadyStatusSend() {
	LOG_INFO(LOG_EXE, "stateReadyStatusSend");
	if(checkReadyCommand() == true) {
		return;
	}
	packetLayer->sendCommand(Command_Status, EXE_COMMAND_TIMEOUT);
}

void ExeMaster::stateReadyStatusRecv(uint8_t b) {
	LOG_INFO(LOG_EXE, "stateReadyStatusRecv " << b);
	if((b & CommandStatus_VendingInhibited) > 0) {
		LOG_WARN(LOG_EXE, "Status disabled " << b);
		gotoStateNotReady();
		EventInterface event(deviceId, Event_NotReady);
		eventEngine->transmit(&event);
		return;
	}
	gotoStateReadyCredit();
}

void ExeMaster::gotoStateReadyCredit() {
	LOG_DEBUG(LOG_EXE, "gotoStateReadyCredit");
	timer->start(EXE_POLL_TIMEOUT);
	state->set(State_ReadyCredit);
}

void ExeMaster::stateReadyCreditSend() {
	LOG_INFO(LOG_EXE, "stateReadyCreditSend");
	if(checkReadyCommand() == true) {
		return;
	}
	packetLayer->sendCommand(Command_Credit, EXE_COMMAND_TIMEOUT);
}

void ExeMaster::stateReadyCreditRecv(uint8_t b) {
	LOG_INFO(LOG_EXE, "stateReadyCreditRecv " << b);
	if(b >= CommandCredit_NoVendRequest) {
		gotoStateReadyStatus();
		return;
	} else {
		gotoStateReadyStatus();
		EventUint8Interface event(deviceId, Event_VendRequest, b);
		eventEngine->transmit(&event);
		return;
	}
}

bool ExeMaster::checkReadyCommand() {
	LOG_DEBUG(LOG_EXE, "checkReadyCommand " << command);
	switch(command) {
	case Command_ChangeCredit: {
		command = Command_None;
		gotoStateCreditShow();
		return true;
	}
	case Command_VendPrice: {
		command = Command_None;
		gotoStatePriceShow();
		return true;
	}
	case Command_VendApprove: {
		command = Command_None;
		gotoStateVending();
		return true;
	}
	case Command_VendDeny: {
		command = Command_None;
		gotoStateReadyStatus();
		return true;
	}
	case Command_None: return false;
	default: {
		LOG_ERROR(LOG_EXE, "Unwaited command " << state->get() << "," << command);
		return false;
	}
	}
}

void ExeMaster::gotoStateCreditShow() {
	LOG_DEBUG(LOG_EXE, "gotoStateCreditShow");
	timer->stop();
	state->set(State_CreditShow);
	stateCreditShowSend();
}

void ExeMaster::stateCreditShowSend() {
	uint16_t baseUnit = credit / scalingFactor;
	LOG_INFO(LOG_EXE, "stateCreditShowSend " << baseUnit << "," << change);
	packetLayer->sendData(baseUnit, scalingFactor, decimalPoint, change, EXE_DATA_TIMEOUT);
}

void ExeMaster::stateCreditShowRecv() {
	LOG_INFO(LOG_EXE, "stateCreditShowRecv");
	gotoStateCreditDelay();
}

void ExeMaster::gotoStateCreditDelay() {
	LOG_DEBUG(LOG_EXE, "gotoStateCreditDelay");
	timer->start(EXE_POLL_TIMEOUT);
	repeatCount = 0;
	state->set(State_CreditDelay);
}

void ExeMaster::stateCreditDelaySend() {
	LOG_INFO(LOG_EXE, "stateCreditDelaySend");
	if(checkReadyCommand() == true) {
		return;
	}
	packetLayer->sendCommand(Command_Status, EXE_COMMAND_TIMEOUT);
}

void ExeMaster::stateCreditDelayRecv(uint8_t b) {
	LOG_INFO(LOG_EXE, "stateCreditDelayRecv " << b);
	if((b & CommandStatus_VendingInhibited) > 0) {
		LOG_WARN(LOG_EXE, "Status disabled " << b);
		gotoStateNotReady();
		EventInterface event(deviceId, Event_NotReady);
		eventEngine->transmit(&event);
		return;
	}
	repeatCount++;
	if(repeatCount >= EXE_SHOW_DELAY_COUNT) {
		gotoStateReadyCredit();
		return;
	}
	timer->start(EXE_POLL_TIMEOUT);
}

void ExeMaster::gotoStatePriceShow() {
	LOG_DEBUG(LOG_EXE, "gotoStatePriceShow");
	timer->stop();
	state->set(State_PriceShow);
	statePriceShowSend();
}

void ExeMaster::statePriceShowSend() {
	uint16_t baseUnit = price / scalingFactor;
	LOG_INFO(LOG_EXE, "statePriceShowSend " << baseUnit << "," << change);
	packetLayer->sendData(baseUnit, scalingFactor, decimalPoint, change, EXE_DATA_TIMEOUT);
}

void ExeMaster::statePriceShowRecv() {
	LOG_INFO(LOG_EXE, "statePriceShowRecv");
	gotoStateApprove();
	EventInterface event(deviceId, Event_VendPrice);
	eventEngine->transmit(&event);
}

void ExeMaster::statePriceShowTimeout() {
	LOG_INFO(LOG_EXE, "statePriceShowTimeout");
	gotoStateApprove();
	EventInterface event(deviceId, Event_VendPrice);
	eventEngine->transmit(&event);
}

void ExeMaster::gotoStateApprove() {
	LOG_DEBUG(LOG_EXE, "gotoStateApprove");
	timer->start(EXE_APPROVE_TIMEOUT);
	state->set(State_Approve);
}

void ExeMaster::stateApproveTimeout() {
	LOG_INFO(LOG_EXE, "stateApproveTimeout");
	gotoStateReadyStatus();
}

void ExeMaster::gotoStateVending() {
	LOG_INFO(LOG_EXE, "gotoStateVending");
	timer->stop();
	stateVendingSend();
	state->set(State_Vending);
}

void ExeMaster::stateVendingSend() {
	LOG_INFO(LOG_EXE, "stateVendingSend");
	packetLayer->sendCommand(Command_Vend, EXE_VEND_TIMEOUT);
}

void ExeMaster::stateVendingRecv(uint8_t byte) {
#ifdef YARVEND_EXE
	LOG_INFO(LOG_EXE, "stateVendingRecv " << byte << "," << (byte & CommandVend_ResultFlag));
	gotoStateReadyStatus();
	EventInterface event(deviceId, Event_VendComplete);
	eventEngine->transmit(&event);
#else
	LOG_INFO(LOG_EXE, "stateVendingRecv " << byte);
	if((byte & CommandVend_ResultFlag) == 0) {
		LOG_INFO(LOG_EXE, "Vending succeed");
		gotoStateReadyStatus();
		EventInterface event(deviceId, Event_VendComplete);
		eventEngine->transmit(&event);
		return;
	} else {
		LOG_INFO(LOG_EXE, "Vending failed");
		gotoStateReadyStatus();
		EventInterface event(deviceId, Event_VendFailed);
		eventEngine->transmit(&event);
		return;
	}
#endif
}

void ExeMaster::stateVendingRecvTimeout() {
	LOG_INFO(LOG_EXE, "stateVendingRecvTimeout");
	gotoStateReadyStatus();
	EventInterface event(deviceId, Event_VendFailed);
	eventEngine->transmit(&event);
}

void ExeMaster::gotoStatePriceDelay() {
	LOG_DEBUG(LOG_EXE, "gotoStatePriceDelay");
	timer->start(EXE_POLL_TIMEOUT);
	repeatCount = 0;
	state->set(State_PriceDelay);
}

void ExeMaster::statePriceDelaySend() {
	LOG_INFO(LOG_EXE, "statePriceDelaySend");
	if(checkReadyCommand() == true) {
		return;
	}
	packetLayer->sendCommand(Command_Status, EXE_COMMAND_TIMEOUT);
}

void ExeMaster::statePriceDelayRecv(uint8_t b) {
	LOG_INFO(LOG_EXE, "statePriceDelayRecv " << b);
	if((b & CommandStatus_VendingInhibited) > 0) {
		LOG_WARN(LOG_EXE, "Status disabled " << b);
		gotoStateNotReady();
		EventInterface event(deviceId, Event_NotReady);
		eventEngine->transmit(&event);
		return;
	}
	repeatCount++;
	if(repeatCount >= EXE_SHOW_DELAY_COUNT) {
		gotoStateCreditShow();
		return;
	}
	timer->start(EXE_POLL_TIMEOUT);
}
#else
#include "config.h"
#include "ExeMaster.h"
#include "common/executive/ExeProtocol.h"
#include "common/logger/include/Logger.h"

using namespace Exe;

ExeMaster::ExeMaster(AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine) :
	eventEngine(eventEngine),
	timers(timers),
	deviceId(eventEngine),
	state(State_Idle),
	command(Command_None),
	decimalPoint(2),
	scalingFactor(1),
	change(false),
	credit(0),
	price(0)
{
	this->timer = timers->addTimer<ExeMaster, &ExeMaster::procTimer>(this, TimerEngine::ProcInTick);
	this->packetLayer = new ExeMasterPacketLayer(uart, timers, this);
}

ExeMaster::~ExeMaster() {
	delete packetLayer;
	timers->deleteTimer(timer);
}

void ExeMaster::reset() {
	ATOMIC {
		command = Command_None;
		state = State_NotReady;
		packetLayer->reset();
	}
}

void ExeMaster::setDicimalPoint(uint8_t decimalPoint) {
	this->decimalPoint = decimalPoint;
}

void ExeMaster::setScalingFactor(uint8_t scalingFactor) {
	this->scalingFactor = scalingFactor;
}

void ExeMaster::setChange(bool change) {
	this->change = change;
}

void ExeMaster::setCredit(uint32_t credit) {
	LOG_INFO(LOG_EXE, "setCredit " << state << "," << credit);
	ATOMIC {
		this->credit = credit;
		this->command = Command_ChangeCredit;
	}
}

void ExeMaster::setPrice(uint32_t price) {
	LOG_INFO(LOG_EXE, "setPrice " << state << "," << price);
	ATOMIC {
		this->credit = price;
		this->command = Command_VendPrice;
	}
}

void ExeMaster::approveVend(uint32_t credit) {
	LOG_INFO(LOG_EXE, "approveVend " << state << "," << credit);
	ATOMIC {
		this->credit = credit;
		this->command = Command_VendApprove;
	}
}

void ExeMaster::denyVend(uint32_t price) {
	LOG_INFO(LOG_EXE, "denyVend " << state << "," << price);
	ATOMIC {
		this->price = price;
		this->command = Command_VendDeny;
	}
}

bool ExeMaster::isEnabled() {
	bool result = false;
	ATOMIC {
		result = (state != State_Idle && state != State_NotFound && state != State_NotReady);
	}
	return result;
}

void ExeMaster::send() {
	LOG_TRACE(LOG_EXE, "send " << state);
	switch(state) {
	case State_NotReady: stateNotReadySend(); break;
	case State_ReadyStatus: stateReadyStatusSend(); break;
	case State_ReadyCredit: stateReadyCreditSend(); break;
	case State_CreditShow: stateCreditShowSend(); break;
	case State_CreditDelay: stateCreditDelaySend(); break;
	case State_PriceShow: statePriceShowSend(); break;
	case State_PriceDelay: statePriceDelaySend(); break;
	case State_Vending: stateVendingSend(); break;
	default: LOG_ERROR(LOG_EXE, "Unwaited send " << state);
	}
}

void ExeMaster::recvByte(uint8_t byte) {
	LOG_TRACE(LOG_EXE, "recvByte " << state);
	switch(state) {
	case State_NotReady: stateNotReadyRecv(byte); break;
	case State_ReadyStatus: stateReadyStatusRecv(byte); break;
	case State_ReadyCredit: stateReadyCreditRecv(byte); break;
	case State_CreditShow: stateCreditShowRecv(); break;
	case State_CreditDelay: stateCreditDelayRecv(byte); break;
	case State_PriceShow: statePriceShowRecv(); break;
	case State_PriceDelay: statePriceDelayRecv(byte); break;
	case State_VendCredit: stateVendCreditRecv(); break;
	case State_Vending: stateVendingRecv(byte); break;
	default: LOG_ERROR(LOG_EXE, "Unwaited byte " << state << "," << byte);
	}
}

void ExeMaster::recvTimeout() {
	LOG_DEBUG(LOG_EXE, "recvTimeout " << state);
	switch(state) {
	case State_Vending: stateVendingRecvTimeout(); break;
	default: LOG_ERROR(LOG_EXE, "Unwaited timeout " << state);
	}
}

void ExeMaster::procTimer() {
	LOG_DEBUG(LOG_EXE, "procTimer");
	switch(state) {
	case State_ReadyStatus: stateReadyTimeout(); break;
	case State_ReadyCredit: stateReadyTimeout(); break;
	case State_CreditDelay: stateCreditDelayTimeout(); break;
	case State_PriceDelay: statePriceDelayTimeout(); break;
	default: LOG_ERROR(LOG_EXE, "Unwaited timeout " << state);
	}
}

void ExeMaster::stateNotReadySend() {
	LOG_DEBUG(LOG_EXE, "stateNotReadySend");
	packetLayer->sendCommand(Command_Status, EXE_COMMAND_TIMEOUT);
}

void ExeMaster::stateNotReadyRecv(uint8_t b) {
	LOG_DEBUG(LOG_EXE, "stateNotReadyRecv " << b);
	if((b & CommandStatus_VendingInhibited) == 0) {
		state = State_ReadyStatus;
		EventInterface event(deviceId, Event_Ready);
		eventEngine->transmit(&event);
		return;
	}
}

void ExeMaster::stateReadyStatusSend() {
	LOG_DEBUG(LOG_EXE, "stateReadyStatusSend");
	if(checkReadyCommand() == true) {
		return;
	}
	packetLayer->sendCommand(Command_Status, EXE_COMMAND_TIMEOUT);
}

void ExeMaster::stateReadyStatusRecv(uint8_t b) {
	LOG_DEBUG(LOG_EXE, "stateReadyStatusRecv " << b);
	if((b & CommandStatus_VendingInhibited) > 0) {
		state = State_NotReady;
		EventInterface event(deviceId, Event_NotReady);
		eventEngine->transmit(&event);
		return;
	}
	state = State_ReadyCredit;
}

void ExeMaster::stateReadyCreditSend() {
	LOG_DEBUG(LOG_EXE, "stateReadyCreditSend");
	if(checkReadyCommand() == true) {
		return;
	}
	packetLayer->sendCommand(Command_Credit, EXE_COMMAND_TIMEOUT);
}

void ExeMaster::stateReadyCreditRecv(uint8_t b) {
	LOG_DEBUG(LOG_EXE, "stateReadyCreditRecv " << b);
	if(b >= CommandCredit_NoVendRequest) {
		state = State_ReadyStatus;
		return;
	} else {
		state = State_ReadyStatus;
		EventUint8Interface event(deviceId, Event_VendRequest, b);
		eventEngine->transmit(&event);
		return;
	}
}

void ExeMaster::stateReadyTimeout() {
	LOG_DEBUG(LOG_EXE, "stateReadyTimeout");
	if(command == Command_None) {
		command = Command_ChangeCredit;
	}
}

bool ExeMaster::checkReadyCommand() {
	LOG_DEBUG(LOG_EXE, "checkReadyCommand " << command);
	switch(command) {
	case Command_ChangeCredit: {
		command = Command_None;
		gotoStateCreditShow();
		return true;
	}
	case Command_VendApprove: {
		command = Command_None;
		gotoStateVendCredit();
		return true;
	}
	case Command_VendDeny: {
		command = Command_None;
		gotoStatePriceShow();
		return true;
	}
	case Command_None: return false;
	default: {
		LOG_ERROR(LOG_EXE, "Unwaited command " << state << "," << command);
		return false;
	}
	}
}

void ExeMaster::gotoStateCreditShow() {
	LOG_DEBUG(LOG_EXE, "gotoStateCreditShow");
	timer->stop();
	state = State_CreditShow;
	stateCreditShowSend();
}

void ExeMaster::stateCreditShowSend() {
	uint16_t baseUnit = credit / scalingFactor;
	LOG_DEBUG(LOG_EXE, "stateCreditShowSend " << baseUnit);
	packetLayer->sendData(baseUnit, scalingFactor, decimalPoint, change, EXE_COMMAND_TIMEOUT);
}

void ExeMaster::stateCreditShowRecv() {
	LOG_DEBUG(LOG_EXE, "stateCreditShowRecv");
	timer->start(EXE_SHOW_TIMEOUT);
	state = State_CreditDelay;
}

void ExeMaster::stateCreditDelaySend() {
	LOG_DEBUG(LOG_EXE, "stateCreditDelaySend");
	if(checkReadyCommand() == true) {
		return;
	}
	packetLayer->sendCommand(Command_Status, EXE_COMMAND_TIMEOUT);
}

void ExeMaster::stateCreditDelayRecv(uint8_t b) {
	LOG_DEBUG(LOG_EXE, "stateCreditDelayRecv " << b);
	if((b & CommandStatus_VendingInhibited) > 0) {
		state = State_NotReady;
		EventInterface event(deviceId, Event_NotReady);
		eventEngine->transmit(&event);
		return;
	}
}

void ExeMaster::stateCreditDelayTimeout() {
	LOG_DEBUG(LOG_EXE, "stateCreditDelayTimeout");
	state = State_ReadyCredit;
}

void ExeMaster::gotoStatePriceShow() {
	LOG_DEBUG(LOG_EXE, "gotoStatePriceShow");
	timer->stop();
	state = State_PriceShow;
	statePriceShowSend();
}

void ExeMaster::statePriceShowSend() {
	uint16_t baseUnit = price / scalingFactor;
	LOG_DEBUG(LOG_EXE, "statePriceShowSend " << baseUnit);
	packetLayer->sendData(baseUnit, scalingFactor, decimalPoint, change, EXE_COMMAND_TIMEOUT);
}

void ExeMaster::statePriceShowRecv() {
	LOG_DEBUG(LOG_EXE, "statePriceShowRecv");
	timer->start(EXE_SHOW_TIMEOUT);
	state = State_PriceDelay;
}

void ExeMaster::statePriceDelaySend() {
	LOG_DEBUG(LOG_EXE, "statePriceDelaySend");
	if(checkReadyCommand() == true) {
		return;
	}
	packetLayer->sendCommand(Command_Status, EXE_COMMAND_TIMEOUT);
}

void ExeMaster::statePriceDelayRecv(uint8_t b) {
	LOG_DEBUG(LOG_EXE, "statePriceDelayRecv " << b);
	if((b & CommandStatus_VendingInhibited) > 0) {
		state = State_NotReady;
		EventInterface event(deviceId, Event_NotReady);
		eventEngine->transmit(&event);
		return;
	}
}

void ExeMaster::statePriceDelayTimeout() {
	LOG_DEBUG(LOG_EXE, "statePriceDelayTimeout");
	if(command == Command_None) {
		command = Command_ChangeCredit;
	}
	state = State_ReadyStatus;
}

void ExeMaster::gotoStateVendCredit() {
	LOG_DEBUG(LOG_EXE, "gotoStateVendCredit");
	timer->stop();
	state = State_VendCredit;
	stateVendCreditSend();
}

void ExeMaster::stateVendCreditSend() {
	uint16_t baseUnit = credit / scalingFactor;
	LOG_INFO(LOG_EXE, "stateVendCreditSend " << baseUnit);
	packetLayer->sendData(baseUnit, scalingFactor, decimalPoint, change, EXE_COMMAND_TIMEOUT);
}

void ExeMaster::stateVendCreditRecv() {
	LOG_DEBUG(LOG_EXE, "stateVendCreditRecv");
	state = State_Vending;
}

void ExeMaster::stateVendingSend() {
	LOG_INFO(LOG_EXE, "stateVendingSend");
	packetLayer->sendCommand(Command_Vend, EXE_VEND_TIMEOUT);
}

void ExeMaster::stateVendingRecv(uint8_t byte) {
	LOG_INFO(LOG_EXE, "stateVendingRecv " << byte);
	if((byte & CommandVend_ResultFlag) == 0) {
		LOG_INFO(LOG_EXE, "Vending succeed");
		state = State_ReadyStatus;
		EventInterface event(deviceId, Event_VendComplete);
		eventEngine->transmit(&event);
		return;
	} else {
		LOG_INFO(LOG_EXE, "Vending failed");
		state = State_ReadyStatus;
		EventInterface event(deviceId, Event_VendFailed);
		eventEngine->transmit(&event);
		return;
	}
}

void ExeMaster::stateVendingRecvTimeout() {
	LOG_DEBUG(LOG_EXE, "stateVendingRecvTimeout");
	state = State_ReadyStatus;
	EventInterface event(deviceId, Event_VendFailed);
	eventEngine->transmit(&event);
}
#endif
