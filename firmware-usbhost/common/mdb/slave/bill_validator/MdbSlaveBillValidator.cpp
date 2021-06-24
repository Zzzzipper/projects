#include "MdbSlaveBillValidator.h"

#include "mdb/MdbProtocolBillValidator.h"
#include "utils/include/Version.h"
#include "logger/include/Logger.h"

#include <string.h>

using namespace Mdb::BillValidator;

MdbSlaveBillValidator::MdbSlaveBillValidator(MdbBillValidatorContext *context, EventEngineInterface *eventEngine) :
	MdbSlave(Mdb::Device_BillValidator, eventEngine),
	deviceId(eventEngine),
	state(State_Idle),
	context(context),
	pollData(MDB_POLL_DATA_SIZE)
{
	static MdbSlavePacketReceiver::Packet packets[] = {
		{ Command_Reset, MDB_SUBCOMMAND_NONE, 0xFF, 0xFF },
		{ Command_Setup, MDB_SUBCOMMAND_NONE, 0xFF, 0xFF },
		{ Command_Security, MDB_SUBCOMMAND_NONE, sizeof(SecurityRequest), sizeof(SecurityRequest) },
		{ Command_Poll, MDB_SUBCOMMAND_NONE, 0xFF, 0xFF },
		{ Command_BillType, MDB_SUBCOMMAND_NONE, sizeof(BillTypeRequest), sizeof(BillTypeRequest) },
		{ Command_Escrow, MDB_SUBCOMMAND_NONE, sizeof(EscrowRequest), sizeof(EscrowRequest) },
		{ Command_Stacker, MDB_SUBCOMMAND_NONE, 0xFF, 0xFF },
		{ Command_Expansion, Subcommand_ExpansionIdentificationL1, 0xFF, 0xFF },
	};
	packetLayer = new MdbSlavePacketReceiver(getType(), this, packets, sizeof(packets)/sizeof(packets[0]));
}

void MdbSlaveBillValidator::reset() {
	ATOMIC {
		LOG_ERROR(LOG_MDBSBV, "reset");
		state = State_Reset;
		command = Command_None;
	}
}

bool MdbSlaveBillValidator::isReseted() {
	bool result = false;
	ATOMIC {
		result = (state != State_Idle);
	}
	return result;
}

bool MdbSlaveBillValidator::isEnable() {
	bool result = false;
	ATOMIC {
		result = (state != State_Idle && state != State_Reset && state != State_Disabled);
	}
	return result;
}

bool MdbSlaveBillValidator::deposite(uint8_t index) {
	bool result = false;
	ATOMIC {
		this->command = Command_Deposite;
		this->b1 = index;
	}
	return result;
}

void MdbSlaveBillValidator::initSlave(MdbSlave::Sender *sender, MdbSlave::Receiver *receiver) {
	slaveReceiver = receiver;
	slaveSender = sender;
	packetLayer->init(receiver);
}

void MdbSlaveBillValidator::recvCommand(const uint8_t command) {
	packetLayer->recvCommand(command);
}

void MdbSlaveBillValidator::recvSubcommand(const uint8_t subcommand) {
	packetLayer->recvSubcommand(subcommand);
}

void MdbSlaveBillValidator::recvRequest(const uint8_t *data, uint16_t len) {
	packetLayer->recvRequest(data, len);
}

void MdbSlaveBillValidator::recvRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_MDBSBV, "recvRequestPacket " << state << "," << commandId);
	this->commandId = commandId;
	switch(state) {
		case State_Idle: return;
		case State_Reset: stateResetRequestPacket(commandId, data, dataLen); return;
		case State_Disabled: stateDisabledRequestPacket(commandId, data, dataLen); return;
		case State_Enabled: stateEnabledRequestPacket(commandId, data, dataLen); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveBillValidator::recvUnsupportedPacket(const uint16_t commandId) {
#ifndef DEBUG_PROTOCOL
	(void)commandId;
#else
	procUnwaitedPacket(commandId, NULL, 0);
#endif
}

void MdbSlaveBillValidator::recvConfirm(uint8_t control) {
	LOG_DEBUG(LOG_MDBSBV, "recvConfirm " << control);
	switch (commandId) {
		case Mdb::BillValidator::CommandId_Poll: procCommandPollConfirm(control); return;
		default: LOG_DEBUG(LOG_MDBSBV, "Unwaited confirm state=" << state << ", commandId=" << commandId << ", control=" << control);
	}
}

void MdbSlaveBillValidator::stateResetRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_Poll: stateResetCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveBillValidator::stateResetCommandPoll() {
	LOG_INFO(LOG_MDBSBV, "stateResetCommandPoll " << state);
	pollData.clear();
	pollData.addUint8(Status_JustReset);
	slaveSender->sendData(pollData.getData(), pollData.getLen());
	slaveReceiver->recvConfirm();
	state = State_Disabled;
}

void MdbSlaveBillValidator::procCommandReset() {
	LOG_INFO(LOG_MDBSBV, "procCommandReset");
	pollData.clear();
	pollData.addUint8(Status_JustReset);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	state = State_Disabled;
}

void MdbSlaveBillValidator::procCommandPoll2() {
	LOG_DEBUG(LOG_MDBSBV, "procCommandPoll2 state=" << state);
	if(command == Command_None) {
		procCommandPoll();
		return;
	} else if(command == Command_Deposite) {
		command = Command_None;
		procCommandDeposite();
		return;
	} else {
		LOG_ERROR(LOG_MDBSBV, "Unwaited command " << command);
		command = Command_None;
		return;
	}
}

void MdbSlaveBillValidator::procCommandDeposite() {
	LOG_DEBUG(LOG_MDBSBV, "procCommandDeposite");
	pollData.addUint8(b1);
	procCommandPoll();
}

void MdbSlaveBillValidator::procCommandPoll() {
	LOG_DEBUG(LOG_MDBSBV, "procCommandPoll state=" << state);
	if(pollData.getLen() > 0) {
		slaveSender->sendData(pollData.getData(), pollData.getLen());
		slaveReceiver->recvConfirm();
		return;
	} else {
		slaveSender->sendAnswer(Mdb::Control_ACK);
		return;
	}
}

void MdbSlaveBillValidator::procCommandPollConfirm(uint8_t control) {
	LOG_DEBUG(LOG_MDBSBV, "procCommandPollConfirm " << control);
	if(control != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBSBV, "Wrong confirm " << control);
		return;
	}
	pollData.clear();
}

void MdbSlaveBillValidator::stateDisabledRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_Setup: procCommandSetup(); return;
		case CommandId_Security: procCommandSecurity(); return;
		case CommandId_ExpansionIdentificationL1: procCommandExpansionIdentificationL1(); return;
		case CommandId_Escrow: procCommandEscrow(); return;
		case CommandId_Stacker: procCommandStacker(); return;
		case CommandId_BillType: procCommandBillType(data); return;
		case CommandId_Poll: procCommandPoll2(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveBillValidator::procCommandSetup() {
	LOG_DEBUG(LOG_MDBSBV, "procCommandSetup");
	SetupResponse resp;
	resp.level = Mdb::FeatureLevel_1;
	resp.currency.set(context->getCurrency());
	resp.scalingFactor.set(context->getScalingFactor());
	resp.decimalPlaces = context->getDecimalPoint();
	resp.stackerCapacity.set(0x0000);
	resp.securityLevel.set(0xFFFF);
	resp.escrow = 0xFF;

	slaveSender->startData();
	slaveSender->addData((const uint8_t*)&resp, sizeof(resp));
	LOG_INFO(LOG_MDBSBV, "setup:" << Mdb::FeatureLevel_1 << "," << context->getScalingFactor() << "," << context->getDecimalPoint());
	LOG_INFO(LOG_MDBSBV, "bills:" << context->getBillNumber());
	for(uint16_t i = 0; i < context->getBillNumber(); i++) {
		uint8_t value = context->getBillValue(i);
		LOG_DEBUG(LOG_MDBSBV, "bill" << value);
		slaveSender->addUint8(value);
	}
	slaveSender->sendData();
}

void MdbSlaveBillValidator::procCommandSecurity() {
	LOG_INFO(LOG_MDBSBV, "procCommandSecurity");
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveBillValidator::procCommandExpansionIdentificationL1() {
	LOG_INFO(LOG_MDBSBV, "procCommandExpansionIdentificationL1");

	ExpansionIdentificationL1Response resp;
	strncpy((char*)resp.manufacturerCode, context->getManufacturer(), context->getManufacturerSize());
	strncpy((char*)resp.serialNumber, context->getSerialNumber(), context->getSerialNumberSize());
	strncpy((char*)resp.model, context->getModel(), context->getModelSize());
	resp.softwareVersion.set(100);

	slaveSender->sendData((const uint8_t*)&resp, sizeof(resp));
}

void MdbSlaveBillValidator::procCommandEscrow() {
	LOG_DEBUG(LOG_MDBSBV, "procCommandEscrow");
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveBillValidator::procCommandStacker() {
	LOG_DEBUG(LOG_MDBSBV, "procCommandStacker");
	slaveSender->startData();
	slaveSender->addUint16(0x0000);
	slaveSender->sendData();
}

void MdbSlaveBillValidator::procCommandBillType(const uint8_t *data) {
	LOG_DEBUG(LOG_MDBSBV, "procCommandBillType " << state);
	BillTypeRequest *req = (BillTypeRequest*)data;
	LOG_DEBUG_HEX(LOG_MDBSBV, data, sizeof(*req));
	slaveSender->sendAnswer(Mdb::Control_ACK);
	if(req->billEnable.get() == 0) {
		LOG_DEBUG(LOG_MDBSBV, "Disabled " << state);
		if(state != State_Disabled) {
			LOG_INFO(LOG_MDBSBV, "Disabled");
			state = State_Disabled;
			EventInterface event(deviceId, Event_Disable);
			deliverEvent(&event);
		}
	} else {
		LOG_DEBUG(LOG_MDBSBV, "Enabled " << state);
		if(state != State_Enabled) {
			LOG_INFO(LOG_MDBSBV, "Enabled");
			state = State_Enabled;
			EventInterface event(deviceId, Event_Enable);
			deliverEvent(&event);
		}
	}
}

void MdbSlaveBillValidator::stateEnabledRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_Setup: procCommandSetup(); return;
		case CommandId_Security: procCommandSecurity(); return;
		case CommandId_Escrow: procCommandEscrow(); return;
		case CommandId_Stacker: procCommandStacker(); return;
		case CommandId_BillType: procCommandBillType(data); return;
		case CommandId_Poll: procCommandPoll2(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveBillValidator::procUnwaitedPacket(uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	LOG_ERROR(LOG_MDBSBV, "procUnwaitedPacket " << state << "," << commandId);
#ifndef DEBUG_PROTOCOL
	(void)data;
	(void)dataLen;
#else
	Mdb::EventError event(deviceId, MdbSlaveBillValidator::Event_Error);
	event.code = ConfigEvent::Type_CoinUnwaitedPacket;
	event.data.clear();
	event.data << "sbv" << state << "*" << getType() << "*";
	event.data.addHex(commandId >> 8);
	event.data.addHex(commandId);
	event.data << "*";
	for(uint16_t i = 0; i < dataLen; i++) {
		event.data.addHex(data[i]);
	}
	deliverEvent(&event);
#endif
}
