#include "MdbSnifferBillValidator.h"

#include "common/mdb/MdbProtocolBillValidator.h"
#include "common/logger/include/Logger.h"

using namespace Mdb::BillValidator;

MdbSnifferBillValidator::MdbSnifferBillValidator(MdbBillValidatorContext *context, EventEngineInterface *eventEngine) :
	MdbSniffer(Mdb::Device_BillValidator, eventEngine),
	deviceId(eventEngine),
	context(context),
	state(State_Idle),
	enabled(false)
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
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
}

MdbSnifferBillValidator::~MdbSnifferBillValidator() {
	delete packetLayer;
}

void MdbSnifferBillValidator::reset() {
	LOG_INFO(LOG_MDBSBVS, "reset");
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	enabled = false;
	state = State_Sale;
}

bool MdbSnifferBillValidator::isEnable() {
	return enabled;
}

void MdbSnifferBillValidator::initSlave(MdbSlave::Sender *, MdbSlave::Receiver *receiver) {
	LOG_DEBUG(LOG_MDBSBVS, "initSlave");
	packetLayer->init(receiver);
}

void MdbSnifferBillValidator::recvCommand(const uint8_t command) {
	packetLayer->recvCommand(command);
}

void MdbSnifferBillValidator::recvSubcommand(const uint8_t subcommand) {
	packetLayer->recvSubcommand(subcommand);
}

void MdbSnifferBillValidator::recvRequest(const uint8_t *data, uint16_t len) {
	LOG_DEBUG(LOG_MDBSBVS, "recvRequest");
	LOG_DEBUG_HEX(LOG_MDBSBVS, data, len);
	packetLayer->recvRequest(data, len);
}

void MdbSnifferBillValidator::recvRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	(void)dataLen;
	LOG_DEBUG(LOG_MDBSBVS, "recvRequestPacket " << state << "," << commandId);
	switch(state) {
		case State_Idle: return;
		case State_Setup: stateSetupCommand(commandId, data); return;
		default: stateSaleCommand(commandId, data);
	}
}

void MdbSnifferBillValidator::recvConfirm(uint8_t control) {
	LOG_DEBUG(LOG_MDBSBVS, "recvConfirm " << state << "," << control);
}

void MdbSnifferBillValidator::procResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_DEBUG(LOG_MDBSBVS, "procResponse " << state);
	LOG_DEBUG_HEX(LOG_MDBSBVS, data, len);
	switch(state) {
		case State_Idle: return;
		case State_Sale: return;
		case State_Setup: stateSetupResponse(data, len, crc); return;
		case State_ExpansionIdentification: stateExpansionIdentificationResponse(data, len, crc); return;
		case State_Stacker: stateStackerResponse(data, len, crc); return;
		case State_Poll: statePollResponse(data, len, crc); return;
		case State_NotPoll: stateNotPollResponse(data, len, crc); return;
		default: LOG_ERROR(LOG_MDBSBVS, "Unwaited packet " << state << "," << crc);
	}
}

void MdbSnifferBillValidator::stateSaleCommand(const uint16_t commandId, const uint8_t *data) {
	LOG_DEBUG(LOG_MDBSBVS, "stateSaleCommand " << commandId);
	switch(commandId) {
		case CommandId_Reset: return;
		case CommandId_Poll: stateSaleCommandPoll(); return;
		case CommandId_Setup: stateSaleCommandSetup(); return;
		case CommandId_ExpansionIdentificationL1: stateSaleCommandExpansionIdentification(); return;
		case CommandId_BillType: stateSaleCommandBillType(data); return;
		case CommandId_Stacker: stateSaleCommandStacker(); return;
		default: {
			LOG_ERROR(LOG_MDBSBVS, "Unwaited packet " << state << "," << commandId);
			state = State_NotPoll;
		}
	}
}

void MdbSnifferBillValidator::stateSaleCommandPoll() {
	state = State_Poll;
}

void MdbSnifferBillValidator::stateSaleCommandSetup() {
	LOG_INFO(LOG_MDBSBVS, "stateSaleCommandSetupConfig");
	state = State_Setup;
}

void MdbSnifferBillValidator::stateSaleCommandExpansionIdentification() {
	LOG_INFO(LOG_MDBSBVS, "stateSaleExpansionIdentification");
	state = State_ExpansionIdentification;
}

void MdbSnifferBillValidator::stateSaleCommandBillType(const uint8_t *data) {
	LOG_INFO(LOG_MDBSBVS, "stateSaleCommandBillType");
	BillTypeRequest *req = (BillTypeRequest*)data;
	LOG_DEBUG_HEX(LOG_MDBSBVS, data, sizeof(*req));
	if(req->billEnable.get() == 0) {
		LOG_DEBUG(LOG_MDBSBVS, "Disable");
		context->setStatus(Mdb::DeviceContext::Status_Disabled);
		state = State_Sale;
		if(enabled == true) {
			enabled = false;
			EventInterface event(deviceId, Event_Disable);
			deliverEvent(&event);
		}
		return;
	} else {
		LOG_DEBUG(LOG_MDBSCC, "Enable");
		context->setStatus(Mdb::DeviceContext::Status_Enabled);
		state = State_Sale;
		if(enabled == false) {
			enabled = true;
			EventInterface event(deviceId, Event_Enable);
			deliverEvent(&event);
		}
		return;
	}
}

void MdbSnifferBillValidator::stateSaleCommandStacker() {
	LOG_DEBUG(LOG_MDBSBVS, "stateSaleCommandStacker");
	state = State_Stacker;
}

void MdbSnifferBillValidator::stateSetupCommand(const uint16_t commandId, const uint8_t *data) {
	LOG_DEBUG(LOG_MDBSBVS, "stateSetupCommand");
	switch(commandId) {
		case CommandId_Poll: return;
		default: stateSaleCommand(commandId, data);
	}
}

void MdbSnifferBillValidator::stateSetupResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_INFO(LOG_MDBSBVS, "stateSetupResponse " << crc);
	if(crc == false) {
		LOG_INFO(LOG_MDBSBVS, "Response in poll");
		return;
	}
	uint16_t expLen = sizeof(SetupResponse);
	if(len < expLen) {
		LOG_ERROR(LOG_MDBSBVS, "Wrong repsonse size");
		state = State_Sale;
		return;
	}

	SetupResponse *pkt = (SetupResponse*)data;
	uint16_t billNum = len - sizeof(SetupResponse);
	context->init(pkt->level, pkt->decimalPlaces, pkt->scalingFactor.get(), pkt->bills, billNum);
	context->setCurrency(pkt->currency.get());

	LOG_WARN(LOG_MDBSBVS, "level " << pkt->level);
	LOG_WARN(LOG_MDBSBVS, "currency " << pkt->currency.get());
	LOG_WARN(LOG_MDBSBVS, "scalingFactor " << pkt->scalingFactor.get());
	LOG_WARN(LOG_MDBSBVS, "decimalPlaces " << pkt->decimalPlaces);
	LOG_WARN(LOG_MDBSBVS, "stackerCapacity " << pkt->stackerCapacity.get());
	LOG_WARN(LOG_MDBSBVS, "securityLevel " << pkt->securityLevel.get());
	LOG_WARN(LOG_MDBSBVS, "escrow " << pkt->escrow);
	LOG_WARN_HEX(LOG_MDBSBVS, pkt->bills, billNum);
	for(uint16_t i = 0; i < 16; i++) {
		LOG_WARN(LOG_MDBSBVS, "Bill " << context->getBillNominal(i));
	}

	state = State_Sale;
}

void MdbSnifferBillValidator::stateExpansionIdentificationResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_INFO(LOG_MDBSBVS, "stateExpansionIdentificationResponse " << crc);
	if(crc == false) {
		LOG_ERROR(LOG_MDBSBVS, "Wrong response");
		state = State_Sale;
		return;
	}
	uint16_t expLen = sizeof(ExpansionIdentificationL1Response);
	if(len < expLen) {
		LOG_ERROR(LOG_MDBSBVS, "Wrong repsonse size");
		state = State_Sale;
		return;
	}

	ExpansionIdentificationL1Response *pkt = (ExpansionIdentificationL1Response*)data;
	LOG_WARN_HEX(LOG_MDBSBVS, pkt->manufacturerCode, sizeof(pkt->manufacturerCode));
	LOG_WARN_HEX(LOG_MDBSBVS, pkt->serialNumber, sizeof(pkt->serialNumber));
	LOG_WARN_HEX(LOG_MDBSBVS, pkt->model, sizeof(pkt->model));
	LOG_WARN(LOG_MDBSBVS, "softwareVersion " << pkt->softwareVersion.get());

	context->setManufacturer(pkt->manufacturerCode, sizeof(pkt->manufacturerCode));
	context->setModel(pkt->model, sizeof(pkt->model));
	context->setSerialNumber(pkt->serialNumber, sizeof(pkt->serialNumber));
	context->setSoftwareVersion(pkt->softwareVersion.get());
	state = State_Sale;
}

void MdbSnifferBillValidator::stateStackerResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_INFO(LOG_MDBSBVS, "stateStackerResponse " << crc);
	if(crc == false) {
		LOG_ERROR(LOG_MDBSBVS, "Wrong response");
		state = State_Sale;
		return;
	}
	uint16_t expLen = sizeof(StackerResponse);
	if(len < expLen) {
		LOG_ERROR(LOG_MDBSBVS, "Wrong repsonse size");
		state = State_Sale;
		return;
	}

	StackerResponse *pkt = (StackerResponse*)data;
	context->setBillInStacker(pkt->billNum.get() & BV_STACKER_NUM_MASK);
	LOG_INFO(LOG_MDBSBVS, "billNum " << context->getBillInStacker());
	state = State_Sale;
}

void MdbSnifferBillValidator::statePollResponse(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_DEBUG(LOG_MDBSBVS, "statePollResponse " << crc);
	if(crc == false) {
		state = State_Sale;
		return;
	}
	LOG_INFO(LOG_MDBSBVS, "stateSaleCommandPoll");
	LOG_INFO_HEX(LOG_MDBSBVS, data, dataLen);
	state = State_Sale;
	for(uint16_t i = 0; i < dataLen; i++) {
		uint8_t status = data[i];
		if(status & BillMask_Sign) {
			statePollResponseStatusDeposite(status);
			break;
		} else if(status == Status_InvalidEscrowRequest) {
			LOG_WARN(LOG_MDBMBV, "Recv InvalidEscrowRequest");
			break;
		} else if(status == Status_JustReset) {
			LOG_WARN(LOG_MDBMBV, "Recv JustReset");
			break;
		} else if(status == Status_ValidatorDisabled) {
			LOG_WARN(LOG_MDBMBV, "Poll ValidatorDisabled");
			break;
		} else {
			LOG_ERROR(LOG_MDBMBV, "Poll status " << status);
		}
	}
}

void MdbSnifferBillValidator::statePollResponseStatusDeposite(uint8_t status) {
	LOG_INFO(LOG_MDBSCCS, "DepositeCoin " << status);
	uint8_t billRoute = status & BillMask_Route;
	uint8_t billType = status & BillMask_Type;
	switch(billRoute) {
	case BillEvent_BillStacked:
	case BillEvent_BillToRecycler: {
		uint32_t nominal = context->getBillNominal(billType);
		LOG_WARN(LOG_MDBMBV, "Bill stacked " << nominal);
		context->registerLastBill(nominal);
		Mdb::EventDeposite event(deviceId, Event_DepositeBill, Route_Stacked, nominal);
		deliverEvent(&event);
		break;
	}
	case BillEvent_BillReturned: {
		uint32_t nominal = context->getBillNominal(billType);
		LOG_WARN(LOG_MDBMBV, "Bill Returned");
		Mdb::EventDeposite event(deviceId, Event_DepositeBill, Route_Rejected, nominal);
		deliverEvent(&event);
		break;
	}
	case BillEvent_EscrowPosition: {
		LOG_WARN(LOG_MDBMBV, "Detected bill");
		break;
	}
	}
}

void MdbSnifferBillValidator::stateNotPollResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_INFO(LOG_MDBSBVS, "stateNotPollResponse " << crc);
	LOG_INFO_HEX(LOG_MDBSBVS, data, len);
	state = State_Sale;
}
