#include <logger/RemoteLogger.h>
#include "MdbSlaveCoinChanger.h"

#include "mdb/MdbProtocolCoinChanger.h"
#include "utils/include/Version.h"
#include "logger/include/Logger.h"

#include <string.h>

using namespace Mdb::CoinChanger;

MdbSlaveCoinChanger::MdbSlaveCoinChanger(MdbCoinChangerContext *context, EventEngineInterface *eventEngine, StatStorage *stat) :
	MdbSlave(Mdb::Device_CoinChanger, eventEngine),
	deviceId(eventEngine),
	context(context),
	pollData(MDB_POLL_DATA_SIZE)
{
	static MdbSlavePacketReceiver::Packet packets[] = {
		{ Command_Reset, MDB_SUBCOMMAND_NONE, 0xFF, 0xFF },
		{ Command_Setup, MDB_SUBCOMMAND_NONE, 0xFF, 0xFF },
		{ Command_TubeStatus, MDB_SUBCOMMAND_NONE, 0xFF, 0xFF },
		{ Command_Poll, MDB_SUBCOMMAND_NONE, 0xFF, 0xFF },
		{ Command_CoinType, MDB_SUBCOMMAND_NONE, sizeof(CoinTypeRequest), sizeof(CoinTypeRequest) },
		{ Command_Dispense, MDB_SUBCOMMAND_NONE, sizeof(DispenseRequest), sizeof(DispenseRequest) },
		{ Command_Expansion, Subcommand_ExpansionIdentification, 0xFF, 0xFF },
		{ Command_Expansion, Subcommand_ExpansionFeatureEnable, sizeof(ExpansionFeatureEnableRequest), sizeof(ExpansionFeatureEnableRequest) },
		{ Command_Expansion, Subcommand_ExpansionPayout, sizeof(ExpansionPayoutRequest), sizeof(ExpansionPayoutRequest) },
		{ Command_Expansion, Subcommand_ExpansionPayoutStatus, 0xFF, 0xFF },
		{ Command_Expansion, Subcommand_ExpansionPayoutValuePoll, 0xFF, 0xFF },
		{ Command_Expansion, Subcommand_ExpansionDiagnostics, 0xFF, 0xFF },
	};
	state = stat->add(Mdb::DeviceContext::Info_MdbS_CC_State, State_Idle);
	countPoll = stat->add(Mdb::DeviceContext::Info_MdbS_CC_PollCount, 0);
	countDisable = stat->add(Mdb::DeviceContext::Info_MdbS_CC_DisableCount, 0);
	countEnable = stat->add(Mdb::DeviceContext::Info_MdbS_CC_EnableCount, 0);
	packetLayer = new MdbSlavePacketReceiver(getType(), this, packets, sizeof(packets)/sizeof(packets[0]));
}

EventDeviceId MdbSlaveCoinChanger::getDeviceId() {
	return deviceId;
}

void MdbSlaveCoinChanger::reset() {
	ATOMIC {
		LOG_ERROR(LOG_MDBSCC, "reset");
		command = Command_None;
		state->set(State_Reset);
	}
}

bool MdbSlaveCoinChanger::isReseted() {
	bool result = false;
	ATOMIC {
		result = (state->get() != State_Idle);
	}
	return result;
}

bool MdbSlaveCoinChanger::isEnable() {
	bool result = false;
	ATOMIC {
		result = (state->get() != State_Idle && state->get() != State_Reset && state->get() != State_Disabled);
	}
	return result;
}

void MdbSlaveCoinChanger::deposite(uint8_t b1, uint8_t b2) {
	ATOMIC {
		LOG_INFO(LOG_MDBSCC, "deposite " << b1 << "," << b2);
		this->command = Command_Deposite;
		this->b1 = b1;
		this->b2 = b2;
	}
}

void MdbSlaveCoinChanger::dispenseComplete() {
	ATOMIC {
		LOG_INFO(LOG_MDBSCC, "dispenseComplete");
		state->set(State_Disabled);
	}
}

void MdbSlaveCoinChanger::escrowRequest() {
	ATOMIC {
		LOG_INFO(LOG_MDBSCC, "escrowRequest");
		this->command = Command_EscrowRequest;
	}
}

void MdbSlaveCoinChanger::initSlave(MdbSlave::Sender *sender, MdbSlave::Receiver *receiver) {
	slaveReceiver = receiver;
	slaveSender = sender;
	packetLayer->init(receiver);
}

void MdbSlaveCoinChanger::recvCommand(const uint8_t command) {
	packetLayer->recvCommand(command);
}

void MdbSlaveCoinChanger::recvSubcommand(const uint8_t subcommand) {
	packetLayer->recvSubcommand(subcommand);
}

void MdbSlaveCoinChanger::recvRequest(const uint8_t *data, uint16_t len) {
	packetLayer->recvRequest(data, len);
}

void MdbSlaveCoinChanger::recvRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_MDBSCC, "recvRequestPacket " << state->get() << "," << commandId);
	REMOTE_LOG_IN(RLOG_MDBSCC, data, dataLen);
	this->commandId = commandId;
	switch(state->get()) {
		case State_Idle: return;
		case State_Reset: stateResetRequestPacket(commandId, data, dataLen); return;
		case State_Disabled: stateDisabledRequestPacket(commandId, data, dataLen); return;
		case State_Enabled: stateEnabledRequestPacket(commandId, data, dataLen); return;
		case State_Dispense: stateDispenseRequestPacket(commandId, data, dataLen); return;
		default: LOG_ERROR(LOG_MDBSCC, "Unsupported state " << state->get());
	}
}

void MdbSlaveCoinChanger::recvUnsupportedPacket(const uint16_t commandId) {
#ifndef DEBUG_PROTOCOL
	(void)commandId;
#else
	procUnwaitedPacket(commandId, NULL, 0);
#endif
}

void MdbSlaveCoinChanger::recvConfirm(uint8_t control) {
	LOG_DEBUG(LOG_MDBSCC, "recvConfirm " << control);
	switch (commandId) {
		case Mdb::CoinChanger::CommandId_Poll: procCommandPollConfirm(control); return;
		default: LOG_DEBUG(LOG_MDBSCC, "Unwaited confirm state=" << state->get() << ", commandId=" << commandId << ", control=" << control);
	}
}

void MdbSlaveCoinChanger::stateResetRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_Setup: procCommandSetup(); return;
		case CommandId_ExpansionIdentification: procCommandExpansionIdentification(); return;
		case CommandId_ExpansionFeatureEnable: procCommandExpansionFeatureEnable(); return;
		case CommandId_ExpansionPayout: procCommandExpansionPayout(); return;
		case CommandId_ExpansionPayoutStatus: procCommandExpansionPayoutStatus(); return;
		case CommandId_ExpansionPayoutValuePoll: procCommandExpansionPayoutValuePoll(); return;
		case CommandId_ExpansionDiagnostics: procCommandExpansionDiagnostics(); return;
		case CommandId_TubeStatus: procCommandTubeStatus(); return;
		case CommandId_CoinType: procCommandCoinType(data); return;
		case CommandId_Dispense: procCommandDispense(data); return;
		case CommandId_Poll: stateResetCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen);	return;
	}
}

void MdbSlaveCoinChanger::stateResetCommandPoll() {
	LOG_INFO(LOG_MDBSCC, "stateResetCommandPoll " << state->get());
	countPoll->inc();
	pollData.clear();
	pollData.addUint8(Status_ChangerWasReset);
	REMOTE_LOG_OUT(RLOG_MDBSCC, pollData.getData(), pollData.getLen());
	slaveSender->sendData(pollData.getData(), pollData.getLen());
	slaveReceiver->recvConfirm();
	state->set(State_Disabled);
}

void MdbSlaveCoinChanger::procCommandReset() {
	LOG_INFO(LOG_MDBSCC, "procCommandReset");
	pollData.clear();
	pollData.addUint8(Status_ChangerWasReset);
	REMOTE_LOG_OUT(RLOG_MDBSCC, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	state->set(State_Disabled);
}

void MdbSlaveCoinChanger::procCommandPoll() {
	LOG_DEBUG(LOG_MDBSCC, "procCommandPoll state=" << state->get());
	if(pollData.getLen() > 0) {
		LOG_INFO(LOG_MDBSCC, "poll");
		LOG_INFO_HEX(LOG_MDBSCC, pollData.getData(), pollData.getLen());
		REMOTE_LOG_OUT(RLOG_MDBSCC, pollData.getData(), pollData.getLen());
		slaveSender->sendData(pollData.getData(), pollData.getLen());
		slaveReceiver->recvConfirm();
		return;
	} else {
		REMOTE_LOG_OUT(RLOG_MDBSCC, Mdb::Control_ACK);
		slaveSender->sendAnswer(Mdb::Control_ACK);
		return;
	}
}

void MdbSlaveCoinChanger::procCommandPollConfirm(uint8_t control) {
	LOG_DEBUG(LOG_MDBSCC, "procCommandPollConfirm " << control);
	if(control != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBSCC, "Wrong confirm " << control);
		return;
	}
	pollData.clear();
}

void MdbSlaveCoinChanger::stateDisabledRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_Setup: procCommandSetup(); return;
		case CommandId_ExpansionIdentification: procCommandExpansionIdentification(); return;
		case CommandId_ExpansionFeatureEnable: procCommandExpansionFeatureEnable(); return;
		case CommandId_ExpansionPayout: procCommandExpansionPayout(); return;
		case CommandId_ExpansionPayoutStatus: procCommandExpansionPayoutStatus(); return;
		case CommandId_ExpansionPayoutValuePoll: procCommandExpansionPayoutValuePoll(); return;
		case CommandId_ExpansionDiagnostics: procCommandExpansionDiagnostics(); return;
		case CommandId_TubeStatus: procCommandTubeStatus(); return;
		case CommandId_CoinType: procCommandCoinType(data); return;
		case CommandId_Dispense: procCommandDispense(data); return;
		case CommandId_Poll: stateDisabledCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveCoinChanger::stateDisabledCommandPoll() {
	LOG_DEBUG(LOG_MDBSCC, "stateDisabledCommandPoll");
	countPoll->inc();
	if(command == Command_None) {
		procCommandPoll();
		return;
	} else if(command == Command_Deposite) {
		command = Command_None;
		procCommandDeposite();
		return;
	} else if(command == Command_EscrowRequest) {
		command = Command_None;
		procCommandEscrowRequest();
		return;
	} else {
		LOG_ERROR(LOG_MDBSCC, "Unwaited command " << command);
		command = Command_None;
		return;
	}
}

void MdbSlaveCoinChanger::procCommandDeposite() {
	LOG_DEBUG(LOG_MDBSCC, "procCommandDeposite");
	pollData.addUint8(b1);
	pollData.addUint8(b2);
	procCommandPoll();
}

void MdbSlaveCoinChanger::procCommandEscrowRequest() {
	LOG_DEBUG(LOG_MDBSCC, "procCommandDeposite");
	pollData.addUint8(Status_EscrowRequest);
	procCommandPoll();
}

void MdbSlaveCoinChanger::procCommandSetup() {
	LOG_INFO(LOG_MDBSCC, "procCommandSetup");
	SetupResponse resp;
	resp.level = Mdb::FeatureLevel_3;
	resp.currency.set(context->getCurrency());
	resp.scalingFactor = context->getScalingFactor();
	resp.decimalPlaces = context->getDecimalPoint();
	resp.coinTypeRouting.set(context->getInTubeMask());

	slaveSender->startData();
	slaveSender->addData((const uint8_t*)&resp, sizeof(resp));
	LOG_INFO(LOG_MDBSCC, "setup:" << Mdb::FeatureLevel_3 << "," << context->getScalingFactor() << "," << context->getDecimalPoint());
	LOG_INFO(LOG_MDBSCC, "coins:" << context->getSize());
	for(uint16_t i = 0; i < context->getSize(); i++) {
		MdbCoin *coin = context->get(i);
		LOG_INFO(LOG_MDBSCC, "coin" << context->money2value(coin->getNominal()));
		slaveSender->addUint8(context->money2value(coin->getNominal()));
	}
	REMOTE_LOG_OUT(RLOG_MDBSCC, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
}

void MdbSlaveCoinChanger::procCommandExpansionIdentification() {
	LOG_DEBUG(LOG_MDBSCC, "procCommandExpansionIdentification");

	ExpansionIdentificationResponse resp;
	strncpy((char*)resp.manufacturerCode, context->getManufacturer(), context->getManufacturerSize());
	strncpy((char*)resp.serialNumber, context->getSerialNumber(), context->getSerialNumberSize());
	strncpy((char*)resp.model, context->getModel(), context->getModelSize());
	resp.softwareVersion.set(100);
	resp.features.set(Feature_AlternativePayout);

	REMOTE_LOG_OUT(RLOG_MDBSCC, (const uint8_t*)&resp, sizeof(resp));
	slaveSender->sendData((const uint8_t*)&resp, sizeof(resp));
}

void MdbSlaveCoinChanger::procCommandExpansionFeatureEnable() {
	LOG_DEBUG(LOG_MDBSCC, "procCommandExpansionFeatureEnable");
	REMOTE_LOG_OUT(RLOG_MDBSCC, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveCoinChanger::procCommandExpansionPayout() {
	LOG_DEBUG(LOG_MDBSCC, "procCommandExpansionPayout");
	REMOTE_LOG_OUT(RLOG_MDBSCC, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveCoinChanger::procCommandExpansionPayoutStatus() {
	LOG_DEBUG(LOG_MDBSCC, "procCommandExpansionPayoutStatus");
	slaveSender->startData();
	slaveSender->addUint8(0x00);
	REMOTE_LOG_OUT(RLOG_MDBSCC, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
	slaveReceiver->recvConfirm();
}

void MdbSlaveCoinChanger::procCommandExpansionPayoutValuePoll() {
	LOG_DEBUG(LOG_MDBSCC, "procCommandExpansionPayoutValuePoll");
	REMOTE_LOG_OUT(RLOG_MDBSCC, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveCoinChanger::procCommandExpansionDiagnostics() {
	LOG_DEBUG(LOG_MDBSCC, "procCommandExpansionDiagnostics");
	slaveSender->startData();
	slaveSender->addUint8(0x03); // Changer fully operational and ready to accept coins
	slaveSender->addUint8(0x00);
	REMOTE_LOG_OUT(RLOG_MDBSCC, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
	slaveReceiver->recvConfirm();
}

void MdbSlaveCoinChanger::procCommandTubeStatus() {
	LOG_DEBUG(LOG_MDBSCC, "procCommandTubeStatus");
	slaveSender->startData();
	slaveSender->addUint16(context->getFullMask());
	for(uint16_t i = 0; i < context->getSize(); i++) {
		MdbCoin *coin = context->get(i);
		slaveSender->addUint8(coin->getNumber());
	}
	REMOTE_LOG_OUT(RLOG_MDBSCC, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
	slaveReceiver->recvConfirm();
}

void MdbSlaveCoinChanger::procCommandCoinType(const uint8_t *data) {
	LOG_DEBUG(LOG_MDBSCC, "procCommandCoinType " << state->get());
	CoinTypeRequest *req = (CoinTypeRequest*)data;
	LOG_DEBUG_HEX(LOG_MDBSCC, data, sizeof(*req));
	REMOTE_LOG_OUT(RLOG_MDBSCC, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	if(req->coinEnable.get() == 0) {
		LOG_DEBUG(LOG_MDBSCC, "Disable " << state->get());
		if(state->get() != State_Disabled) {
			LOG_INFO(LOG_MDBSCC, "Disabled");
			state->set(State_Disabled);
			countDisable->inc();
			EventInterface event(deviceId, Event_Disable);
			deliverEvent(&event);
		}
	} else {
		LOG_DEBUG(LOG_MDBSCC, "Enable " << state->get());
		if(state->get() != State_Enabled) {
			LOG_INFO(LOG_MDBSCC, "Enabled");
			state->set(State_Enabled);
			countEnable->inc();
			EventInterface event(deviceId, Event_Enable);
			deliverEvent(&event);
		}
	}
}

void MdbSlaveCoinChanger::procCommandDispense(const uint8_t *data) {
	LOG_INFO(LOG_MDBSCC, "procCommandDispense " << state->get());
	DispenseRequest *req = (DispenseRequest*)data;
	LOG_DEBUG_HEX(LOG_MDBSCC, data, sizeof(*req));
	REMOTE_LOG_OUT(RLOG_MDBSCC, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	state->set(State_Dispense);
	EventUint8Interface event(deviceId, Event_DispenseCoin, req->coin);
	deliverEvent(&event);
}

void MdbSlaveCoinChanger::stateEnabledRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_Setup: procCommandSetup(); return;
		case CommandId_ExpansionIdentification: procCommandExpansionIdentification(); return;
		case CommandId_ExpansionFeatureEnable: procCommandExpansionFeatureEnable(); return;
		case CommandId_ExpansionPayout: procCommandExpansionPayout(); return;
		case CommandId_ExpansionPayoutStatus: procCommandExpansionPayoutStatus(); return;
		case CommandId_ExpansionPayoutValuePoll: procCommandExpansionPayoutValuePoll(); return;
		case CommandId_ExpansionDiagnostics: procCommandExpansionDiagnostics(); return;
		case CommandId_TubeStatus: procCommandTubeStatus(); return;
		case CommandId_CoinType: procCommandCoinType(data); return;
		case CommandId_Poll: stateDisabledCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen);	return;
	}
}

void MdbSlaveCoinChanger::stateDispenseRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_Poll: stateDispenseCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen);	return;
	}
}

void MdbSlaveCoinChanger::stateDispenseCommandPoll() {
	LOG_DEBUG(LOG_MDBSCC, "stateDispenseCommandPoll state=" << state->get());
	slaveSender->startData();
	slaveSender->addUint8(Status_ChangerPayoutBusy);
	REMOTE_LOG_OUT(RLOG_MDBSCC, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
}

void MdbSlaveCoinChanger::procUnwaitedPacket(uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	LOG_ERROR(LOG_MDBSCC, "procUnwaitedPacket " << state->get() << "," << commandId);
#ifndef DEBUG_PROTOCOL
	(void)data;
	(void)dataLen;
#else
	Mdb::EventError event(deviceId, MdbSlaveCoinChanger::Event_Error);
	event.code = ConfigEvent::Type_CoinUnwaitedPacket;
	event.data.clear();
	event.data << "scc" << state->get() << "*" << getType() << "*";
	event.data.addHex(commandId >> 8);
	event.data.addHex(commandId);
	event.data << "*";
	for(uint16_t i = 0; i < dataLen; i++) {
		event.data.addHex(data[i]);
	}
	deliverEvent(&event);
#endif
}
