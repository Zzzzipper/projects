#include "MdbSnifferCoinChanger.h"

#include "common/mdb/MdbProtocolCoinChanger.h"
#include "common/logger/RemoteLogger.h"
#include "common/logger/include/Logger.h"

using namespace Mdb::CoinChanger;

MdbSnifferCoinChanger::MdbSnifferCoinChanger(MdbCoinChangerContext *context, EventEngineInterface *eventEngine) :
	MdbSniffer(Mdb::Device_CoinChanger, eventEngine),
	deviceId(eventEngine),
	context(context),
	state(State_Idle),
	enabled(false)
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
		{ Command_Expansion, Subcommand_ExpansionDiagnostics, sizeof(ExpansionFeatureEnableRequest), sizeof(ExpansionFeatureEnableRequest) },
	};
	packetLayer = new MdbSlavePacketReceiver(getType(), this, packets, sizeof(packets)/sizeof(packets[0]));
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
}

MdbSnifferCoinChanger::~MdbSnifferCoinChanger() {
	delete packetLayer;
}

void MdbSnifferCoinChanger::reset() {
	LOG_INFO(LOG_MDBSCCS, "reset");
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	enabled = false;
	state = State_Sale;
}

bool MdbSnifferCoinChanger::isEnable() {
	return enabled;
}

void MdbSnifferCoinChanger::initSlave(MdbSlave::Sender *, MdbSlave::Receiver *receiver) {
	LOG_DEBUG(LOG_MDBSCCS, "initSlave");
	packetLayer->init(receiver);
}

void MdbSnifferCoinChanger::recvCommand(const uint8_t command) {
	packetLayer->recvCommand(command);
}

void MdbSnifferCoinChanger::recvSubcommand(const uint8_t subcommand) {
	packetLayer->recvSubcommand(subcommand);
}

void MdbSnifferCoinChanger::recvRequest(const uint8_t *data, uint16_t len) {
	LOG_DEBUG(LOG_MDBSCCS, "recvRequest");
	LOG_DEBUG_HEX(LOG_MDBSCCS, data, len);
	packetLayer->recvRequest(data, len);
}

void MdbSnifferCoinChanger::recvRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_MDBSCCS, "recvRequestPacket " << state << "," << commandId);
	REMOTE_LOG_IN(RLOG_MDBSCC, data, dataLen);
	switch(state) {
		case State_Idle: return;
		case State_Setup: stateSetupCommand(commandId, data); return;
		default: stateSaleCommand(commandId, data);
	}
}

void MdbSnifferCoinChanger::recvConfirm(uint8_t control) {
	LOG_DEBUG(LOG_MDBSCCS, "recvConfirm " << state << "," << control);
	REMOTE_LOG_IN(RLOG_MDBSCC, control);
}

void MdbSnifferCoinChanger::procResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_DEBUG(LOG_MDBSCCS, "procResponse " << state);
	LOG_DEBUG_HEX(LOG_MDBSCCS, data, len);
	REMOTE_LOG_OUT(RLOG_MDBSCC, data, len);
	switch(state) {
		case State_Idle: return;
		case State_Sale: return;
		case State_Setup: stateSetupResponse(data, len, crc); return;
		case State_ExpansionIdentification: stateExpansionIdentificationResponse(data, len, crc); return;
		case State_TubeStatus: stateTubeStatusResponse(data, len, crc); return;
		case State_Poll: statePollResponse(data, len, crc); return;
		case State_NotPoll: stateNotPollResponse(data, len, crc); return;
		default: LOG_ERROR(LOG_MDBSCCS, "Unwaited packet " << state << "," << crc);
	}
}

void MdbSnifferCoinChanger::stateSaleCommand(const uint16_t commandId, const uint8_t *data) {
	LOG_DEBUG(LOG_MDBSCCS, "stateSaleCommand " << commandId);
	switch(commandId) {
		case CommandId_Reset: return;
		case CommandId_Poll: stateSaleCommandPoll(); return;
		case CommandId_Setup: stateSaleCommandSetup(); return;
		case CommandId_ExpansionIdentification: stateSaleExpansionIdentification(); return;
		case CommandId_ExpansionFeatureEnable: return;
		case CommandId_ExpansionPayout: stateSaleCommandExpansionPayout(data); return;
		case CommandId_ExpansionPayoutStatus: return;
		case CommandId_ExpansionPayoutValuePoll: return;
		case CommandId_CoinType: stateSaleCommandCoinType(data); return;
		case CommandId_Dispense: stateSaleCommandDispense(data); return;
		case CommandId_TubeStatus: stateSaleCommandTubeStatus(); return;
		default: {
			LOG_ERROR(LOG_MDBSCCS, "Unwaited packet " << state << "," << commandId);
			state = State_NotPoll;
		}
	}
}

void MdbSnifferCoinChanger::stateSaleCommandPoll() {
	state = State_Poll;
}

void MdbSnifferCoinChanger::stateSaleCommandSetup() {
	LOG_INFO(LOG_MDBSCCS, "stateSaleCommandSetupConfig");
	state = State_Setup;
}

void MdbSnifferCoinChanger::stateSaleCommandTubeStatus() {
	LOG_INFO(LOG_MDBSCCS, "stateSaleCommandTubeStatus");
	state = State_TubeStatus;
}

void MdbSnifferCoinChanger::stateSaleExpansionIdentification() {
	LOG_INFO(LOG_MDBSCCS, "stateSaleExpansionIdentification");
	state = State_ExpansionIdentification;
}

void MdbSnifferCoinChanger::stateSaleCommandExpansionPayout(const uint8_t *data) {
	LOG_INFO(LOG_MDBSCCS, "stateSaleCommandExpansionPayout");
	ExpansionPayoutRequest *req = (ExpansionPayoutRequest*)data;
	LOG_DEBUG_HEX(LOG_MDBSCCS, data, sizeof(*req));
	uint32_t dispenseSum = context->value2money(req->sum);
	EventUint32Interface event(deviceId, Event_DispenseCoin, dispenseSum);
	deliverEvent(&event);
}

void MdbSnifferCoinChanger::stateSaleCommandCoinType(const uint8_t *data) {
	LOG_INFO(LOG_MDBSCCS, "stateSaleCommandCoinType");
	CoinTypeRequest *req = (CoinTypeRequest*)data;
	LOG_DEBUG_HEX(LOG_MDBSCCS, data, sizeof(*req));
	if(req->coinEnable.get() == 0) {
		LOG_DEBUG(LOG_MDBSCCS, "Disable");
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

void MdbSnifferCoinChanger::stateSaleCommandDispense(const uint8_t *data) {
	LOG_INFO(LOG_MDBSCCS, "stateSaleCommandDispense");
	uint8_t b1 = data[1];
	uint8_t coinIndex = b1 & 0x0F;
	uint32_t coinNumber = (b1 & 0xF0) >> 4;
	MdbCoin *coin = context->get(coinIndex);
	if(coin == NULL) {
		LOG_ERROR(LOG_MDBSCCS, "Coin " << coinIndex << " not found");
		return;
	}
	uint32_t sum = coin->getNominal() * coinNumber;
	EventUint32Interface event(deviceId, Event_DispenseCoin, sum);
	deliverEvent(&event);
}

void MdbSnifferCoinChanger::stateSetupCommand(const uint16_t commandId, const uint8_t *data) {
	LOG_DEBUG(LOG_MDBSCCS, "stateSetupCommand");
	switch(commandId) {
		case CommandId_Poll: return;
		default: stateSaleCommand(commandId, data);
	}
}

void MdbSnifferCoinChanger::stateSetupResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_INFO(LOG_MDBSCCS, "stateSetupResponse " << crc);
	if(crc == false) {
		LOG_INFO(LOG_MDBSCCS, "Response in poll");
		return;
	}
	uint16_t expLen = sizeof(SetupResponse);
	if(len < expLen) {
		LOG_ERROR(LOG_MDBSCCS, "Wrong repsonse size");
		state = State_Sale;
		return;
	}

	SetupResponse *pkt = (SetupResponse*)data;
	uint16_t coinNum = len - sizeof(SetupResponse);
	context->init(pkt->level, pkt->decimalPlaces, pkt->scalingFactor, pkt->coins, coinNum, pkt->coinTypeRouting.get());
	context->setCurrency(pkt->currency.get());

	LOG_WARN(LOG_MDBSCCS, "level " << pkt->level);
	LOG_WARN(LOG_MDBSCCS, "currency " << pkt->currency.get());
	LOG_WARN(LOG_MDBSCCS, "scalingFactor " << pkt->scalingFactor);
	LOG_WARN(LOG_MDBSCCS, "decimalPlaces " << pkt->decimalPlaces);
	LOG_WARN(LOG_MDBSCCS, "coinTypeRouting " << pkt->coinTypeRouting.get());
	LOG_WARN_HEX(LOG_MDBSCCS, pkt->coins, coinNum);
	for(uint16_t i = 0; i < 16; i++) {
		LOG_WARN(LOG_MDBSCCS, "Coin " << context->get(i)->getNominal());
	}

	context->setStatus(Mdb::DeviceContext::Status_Init);
	state = State_Sale;
}

void MdbSnifferCoinChanger::stateExpansionIdentificationResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_INFO(LOG_MDBSCCS, "stateExpansionIdentificationResponse " << crc);
	if(crc == false) {
		LOG_ERROR(LOG_MDBSCCS, "Wrong response");
		state = State_Sale;
		return;
	}
	uint16_t expLen = sizeof(ExpansionIdentificationResponse);
	if(len < expLen) {
		LOG_ERROR(LOG_MDBSCCS, "Wrong repsonse size");
		state = State_Sale;
		return;
	}

	ExpansionIdentificationResponse *pkt = (ExpansionIdentificationResponse*)data;
	LOG_WARN_HEX(LOG_MDBSCCS, pkt->manufacturerCode, sizeof(pkt->manufacturerCode));
	LOG_WARN_HEX(LOG_MDBSCCS, pkt->serialNumber, sizeof(pkt->serialNumber));
	LOG_WARN_HEX(LOG_MDBSCCS, pkt->model, sizeof(pkt->model));
	LOG_WARN(LOG_MDBSCCS, "softwareVersion " << pkt->softwareVersion.get());
	LOG_WARN(LOG_MDBSCCS, "features " << pkt->features.get());

	context->setManufacturer(pkt->manufacturerCode, sizeof(pkt->manufacturerCode));
	context->setModel(pkt->model, sizeof(pkt->model));
	context->setSerialNumber(pkt->serialNumber, sizeof(pkt->serialNumber));
	context->setSoftwareVersion(pkt->softwareVersion.get());
	state = State_Sale;
}

void MdbSnifferCoinChanger::stateTubeStatusResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_DEBUG(LOG_MDBSCCS, "stateTubeStatusResponse " << crc);
	if(crc == false) {
		LOG_ERROR(LOG_MDBSCCS, "Wrong response");
		state = State_Sale;
		return;
	}
	uint16_t expLen = sizeof(TubesResponse);
	if(len < expLen) {
		LOG_ERROR(LOG_MDBSCCS, "Wrong repsonse size");
		state = State_Sale;
		return;
	}

	TubesResponse *pkt = (TubesResponse*)data;
	uint16_t coinNum = len - sizeof(TubesResponse);
	context->update(pkt->tubeStatus.get(), pkt->tubeVolume, coinNum);

	LOG_DEBUG(LOG_MDBSCCS, "tubeStatus " << pkt->tubeStatus.get());
	LOG_DEBUG_HEX(LOG_MDBSCCS, pkt->tubeVolume, coinNum);
	LOG_INFO(LOG_MDBSCCS, "inTubes=" << context->getInTubeValue());
	state = State_Sale;
#if 0
	EventInterface event(deviceId, Event_TubeStatus);
	deliverEvent(&event);
#endif
}

void MdbSnifferCoinChanger::statePollResponse(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_DEBUG(LOG_MDBSCCS, "statePollResponse " << crc);
	if(crc == false) {
		state = State_Sale;
		return;
	}
	LOG_INFO(LOG_MDBSCCS, "stateSaleCommandPoll");
	LOG_INFO_HEX(LOG_MDBSCCS, data, dataLen);
	for(uint16_t i = 0; i < dataLen; i++) {
		uint8_t status = data[i];
		if(status & CoinMask_DepositeSign) {
			i++; if(i >= dataLen) { break; }
			statePollRecvDeposite(status, data[i]);
			break;
		} else if(status & CoinMask_DispenseSign) {
			i++; if(i >= dataLen) { break; }
//			statePollRecvDispense(status, data[i]);
			break;
		} else if(status == Status_EscrowRequest) {
			LOG_INFO(LOG_MDBSCCS, "Recv EscrowRequest");
			break;
		} else if(status == Status_ChangerWasReset) {
			LOG_INFO(LOG_MDBSCCS, "Recv JustReset");
			break;
		} else {
			LOG_WARN(LOG_MDBSCCS, "Poll status " << status);
		}
	}
	state = State_Sale;
}

void MdbSnifferCoinChanger::statePollRecvDeposite(uint8_t b1, uint8_t b2) {
	LOG_INFO(LOG_MDBSCCS, "DepositeCoin " << b1 << "," << b2);
	uint16_t coinIndex = b1 & CoinMask_DepositeCoin;
	MdbCoin *coin = context->get(coinIndex);
	if(coin == NULL) {
		LOG_ERROR(LOG_MDBSCCS, "Wrong coin index " << coinIndex);
		return;
	}

	uint32_t coinNominal = context->get(coinIndex)->getNominal();
	uint8_t coinRoute = Route_None;
	switch(b1 & CoinRoute_Mask) {
	case CoinRoute_CashBox: {
		coinRoute = Route_Cashbox;
		break;
	}
	case CoinRoute_Tubes: {
		coinRoute = Route_Tube;
		context->updateTube(coinIndex, b2);
		break;
	}
	default: {
		LOG_WARN(LOG_MDBSCCS, "Reject coin " << coinNominal);
		return;
	}
	}

	LOG_WARN(LOG_MDBSCCS, "Deposited coin " << coinNominal << " " << coinRoute << " " << b1 << " " << b2);
	if(coinNominal == MDB_CC_TOKEN) {
//		eventCoin.set(Event_DepositeToken, 0, coinRoute, b1, b2);
//		deliverEvent(&eventCoin);
		return;
	} else {
		context->registerLastCoin(coinNominal);
		Mdb::EventDeposite event(deviceId, Event_DepositeCoin, coinRoute, coinNominal);
		deliverEvent(&event);
		return;
	}
}

void MdbSnifferCoinChanger::stateNotPollResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_INFO(LOG_MDBSCCS, "stateNotPollResponse " << crc);
	LOG_INFO_HEX(LOG_MDBSCCS, data, len);
	state = State_Sale;
}
