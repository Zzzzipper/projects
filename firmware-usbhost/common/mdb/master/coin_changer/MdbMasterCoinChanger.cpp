#include "MdbMasterCoinChanger.h"

#include "mdb/MdbProtocolCoinChanger.h"
#include "mdb/MdbCoinChangerPollParser.h"
#include "logger/RemoteLogger.h"
#include "logger/include/Logger.h"

using namespace Mdb::CoinChanger;

enum EventCoinParam {
	EventCoinParam_Nominal = 0,
	EventCoinParam_Route,
	EventCoinParam_B1,
	EventCoinParam_B2
};

MdbMasterCoinChanger::EventCoin::EventCoin(EventDeviceId deviceId, uint16_t type, uint32_t nominal, uint8_t route, uint8_t b1, uint8_t b2) :
	EventInterface(deviceId, type),
	nominal(nominal),
	route(route),
	b1(b1),
	b2(b2)
{

}

void MdbMasterCoinChanger::EventCoin::set(uint16_t type, uint32_t nominal, uint8_t route, uint8_t b1, uint8_t b2) {
	this->type = type;
	this->nominal = nominal;
	this->route = route;
	this->b1 = b1;
	this->b2 = b2;
}

bool MdbMasterCoinChanger::EventCoin::open(EventEnvelope *envelope) {
	if(EventInterface::open(envelope) == false) { return false; }
	if(envelope->getUint32(EventCoinParam_Nominal, &nominal) == false) { return false; }
	if(envelope->getUint8(EventCoinParam_Route, &route) == false) { return false; }
	if(envelope->getUint8(EventCoinParam_B1, &b1) == false) { return false; }
	if(envelope->getUint8(EventCoinParam_B2, &b2) == false) { return false; }
	return true;
}

bool MdbMasterCoinChanger::EventCoin::pack(EventEnvelope *envelope) {
	if(EventInterface::pack(envelope) == false) { return false; }
	if(envelope->addUint32(EventCoinParam_Nominal, nominal) == false) { return false; }
	if(envelope->addUint8(EventCoinParam_Route, route) == false) { return false; }
	if(envelope->addUint8(EventCoinParam_B1, b1) == false) { return false; }
	if(envelope->addUint8(EventCoinParam_B2, b2) == false) { return false; }
	return true;
}

MdbMasterCoinChanger::MdbMasterCoinChanger(MdbCoinChangerContext *coins, EventEngineInterface *eventEngine) :
	MdbMaster(Mdb::Device_CoinChanger, eventEngine),
	sender(NULL),
	deviceId(eventEngine),
	diagnostic(false),
	enabling(false),
	enabled(false),
	context(coins),
	eventCoin(deviceId)
{
	context->setState(State_Idle);
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
}

EventDeviceId MdbMasterCoinChanger::getDeviceId() {
	return deviceId;
}

void MdbMasterCoinChanger::reset() {
	ATOMIC {
		LOG_ERROR(LOG_MDBMCC, "reset");
		gotoStateReset();
	}
}

// Обязательно должен быть получен баланс туб
bool MdbMasterCoinChanger::isInited() {
	bool result = false;
	ATOMIC {
		result = (context->getState() > State_TubeStatus);
	}
	return result;
}

bool MdbMasterCoinChanger::hasChange() {
	bool result = false;
	ATOMIC {
		if(context->getState() < State_Poll) { continue; }
		result = context->hasChange();
	}
	return result;
}

void  MdbMasterCoinChanger::dispense(const uint32_t sum) {
	ATOMIC {
		LOG_WARN(LOG_MDBMCC, "Payout " << context->getState() << "," << sum);
		this->command = CommandType_Dispense;
		this->dispenseSum = sum;
	}
}

void MdbMasterCoinChanger::dispenseCoin(uint8_t data) {
	ATOMIC {
		LOG_WARN(LOG_MDBMCC, "Dispance coin " << context->getState() << "," << data);
		this->command = CommandType_DispenseCoin;
		this->coinData = data;
	}
}

void MdbMasterCoinChanger::startTubeFilling() {
	ATOMIC {
		LOG_WARN(LOG_MDBMCC, "Tube filling start" << context->getState());
		this->command = CommandType_TubeFillingStart;
	}
}

void MdbMasterCoinChanger::stopTubeFilling() {
	ATOMIC {
		LOG_WARN(LOG_MDBMCC, "Tube filling stop" << context->getState());
		this->command = CommandType_TubeFillingStop;
	}
}

void MdbMasterCoinChanger::disable() {
	ATOMIC {
		LOG_WARN(LOG_MDBMCC, "Disable " << context->getState());
		this->enabling = false;
	}
}

void MdbMasterCoinChanger::enable() {
	ATOMIC {
		LOG_WARN(LOG_MDBMCC, "Enable " << context->getState());
		this->enabling = true;
	}
}

void MdbMasterCoinChanger::initMaster(MdbMasterSender *sender) {
	this->sender = sender;
}

void MdbMasterCoinChanger::sendRequest() {
	LOG_DEBUG(LOG_MDBMCC, "sendRequest " << context->getState());
	switch(context->getState()) {
		case State_Reset: sendReset(); break;
		case State_ResetWait: sendResetWait(); break;
		case State_Setup: sendSetup(); break;
		case State_ExpansionIdentification: sendExpansionIdentification(); break;
		case State_ExpansionFeatureEnable: sendExpansionFeatureEnable(); break;
		case State_TubeStatus: stateTubeStatusSend(); break;
		case State_CoinType: sendCoinType(); break;
		case State_Poll: statePollSend(); break;
		case State_PollTubeStatus: statePollTubeStatusSend(); break;
		case State_PollDiagnostic: statePollDiagnosticSend(); break;
		case State_Disable: stateDisableSend(); break;
		case State_Enable: stateEnableSend(); break;
		case State_ExpansionPayout: stateExpansionPayoutSend(); break;
		case State_ExpansionPayoutWait: stateExpansionPayoutWaitSend(); break;
		case State_ExpansionPayoutStatus: stateExpansionPayoutStatusSend(); break;
		case State_DispenseCoin: stateDispenseCoinSend(); break;
		case State_DispenseWait: stateDispenseWaitSend(); break;
		case State_DispenseTubeStatus: stateDispenseTubeStatusSend(); break;
		case State_TubeFillingInit: stateTubeFillingInitSend(); break;
		case State_TubeFilling: stateTubeFillingSend(); break;
		case State_TubeFillingUpdate: stateTubeFillingUpdateSend(); break;
		default: LOG_ERROR(LOG_MDBMCC, "Unsupported state " << context->getState());
	}
}

void MdbMasterCoinChanger::recvResponse(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_DEBUG(LOG_MDBMCC, "recvResponse " << context->getState());
	LOG_TRACE_HEX(LOG_MDBMCC, data, dataLen);
	REMOTE_LOG_IN(RLOG_MDBMCC, data, dataLen);
	this->tryCount = 0;
	switch(context->getState()) {
		case State_Reset: recvReset(data, dataLen, crc); break;
		case State_ResetWait: recvResetWait(data, dataLen, crc); break;
		case State_Setup: recvSetup(data, dataLen, crc); break;
		case State_ExpansionIdentification: recvExpansionIdentification(data, dataLen, crc); break;
		case State_ExpansionFeatureEnable: recvExpansionFeatureEnable(data, dataLen, crc); break;
		case State_TubeStatus: stateTubeStatusRecv(data, dataLen, crc); break;
		case State_CoinType: recvCoinType(data, dataLen, crc); break;
		case State_Poll: statePollRecv(data, dataLen, crc); break;
		case State_PollTubeStatus: statePollTubeStatusRecv(data, dataLen, crc); break;
		case State_PollDiagnostic: statePollDiagnosticRecv(data, dataLen, crc); break;
		case State_Disable: stateDisableRecv(data, dataLen, crc); break;
		case State_Enable: stateEnableRecv(data, dataLen, crc); break;
		case State_ExpansionPayout: stateExpansionPayoutRecv(data, dataLen, crc); break;
		case State_ExpansionPayoutWait: stateExpansionPayoutWaitRecv(data, dataLen, crc); break;
		case State_ExpansionPayoutStatus: stateExpansionPayoutStatusRecv(data, dataLen, crc); break;
		case State_DispenseCoin: stateDispenseCoinRecv(data, dataLen, crc); break;
		case State_DispenseWait: stateDispenseWaitRecv(data, dataLen, crc); break;
		case State_DispenseTubeStatus: stateDispenseTubeStatusRecv(data, dataLen, crc); break;
		case State_TubeFillingInit: stateTubeFillingInitRecv(data, dataLen, crc); break;
		case State_TubeFilling: stateTubeFillingRecv(data, dataLen, crc); break;
		case State_TubeFillingUpdate: stateTubeFillingUpdateRecv(data, dataLen, crc); break;
		default: LOG_ERROR(LOG_MDBMCC, "Unsupported state " << context->getState());
	}
}

void MdbMasterCoinChanger::timeoutResponse() {
	LOG_DEBUG(LOG_MDBMCC, "timeoutResponse " << context->getState());
	if(context->getState() != State_Idle && context->getState() != State_Reset) {
		this->tryCount += 1;
		if(this->tryCount >= MDB_TRY_NUMBER) {
			LOG_ERROR(LOG_MDBMCC, "Too many tries. Reset.");
			reset();
			EventUint16Interface event(deviceId, Event_Error, Error_NotResponsible);
			deliverEvent(&event);
		}
	}
}

void MdbMasterCoinChanger::gotoStateReset() {
	tryCount = 0;
	command = CommandType_None;
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	context->incResetCount();
	context->setState(State_Reset);
}

void MdbMasterCoinChanger::sendReset() {
	sender->startRequest();
	sender->addUint8(getType() | Command_Reset);
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCoinChanger::recvReset(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "Recv Reset");
	LOG_DEBUG_HEX(LOG_MDBMCC, data, dataLen);
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMCC, "Wrong answer");
		return;
	}

	LOG_INFO(LOG_MDBMCC, "Recv ACK");
	gotoStateResetWait();
}

void MdbMasterCoinChanger::gotoStateResetWait() {
	repeatCount = 0;
	context->setState(State_ResetWait);
}

void MdbMasterCoinChanger::sendResetWait() {
	sender->startRequest();
	sender->addUint8(getType() | Command_Poll);
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCoinChanger::recvResetWait(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "recvResetWait");
	LOG_DEBUG_HEX(LOG_MDBMCC, data, dataLen);
	if(crc == false) {
		LOG_DEBUG(LOG_MDBMCC, "No answer");
		repeatCount++;
		if(repeatCount >= MDB_JUST_RESET_COUNT) {
			LOG_ERROR(LOG_MDBMCC, "JustReset not found");
			gotoStateReset();
			return;
		}
		return;
	}

	sender->sendConfirm(Mdb::Control_ACK);
	for(uint16_t i = 0; i < dataLen; i++) {
		if(data[i] == Status_ChangerWasReset) {
			LOG_INFO(LOG_MDBMCC, "Recv JustReset");
			gotoStateSetup();
			return;
		}
	}
}

void MdbMasterCoinChanger::gotoStateSetup() {
	LOG_INFO(LOG_MDBMCC, "sendSetup");
	enabled = false;
	context->setStatus(Mdb::DeviceContext::Status_Init);
	context->setState(State_Setup);
}

void MdbMasterCoinChanger::sendSetup() {
	LOG_INFO(LOG_MDBMCC, "sendSetup");
	sender->startRequest();
	sender->addUint8(getType() | Command_Setup);
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCoinChanger::recvSetup(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "recvSetup");
	if(crc == false || dataLen < sizeof(SetupResponse)) {
		LOG_ERROR(LOG_MDBMCC, "Wrong response");
		return;
	}
	sender->sendConfirm(Mdb::Control_ACK);

	SetupResponse *pkt = (SetupResponse*)data;
	uint16_t coinNum = dataLen - sizeof(SetupResponse);
	context->init(pkt->level, pkt->decimalPlaces, pkt->scalingFactor, pkt->coins, coinNum, pkt->coinTypeRouting.get());
	context->setCurrency(pkt->currency.get());
	dispenseStep = context->value2money(200);

	LOG_INFO(LOG_MDBMCC, "level " << pkt->level);
	LOG_INFO(LOG_MDBMCC, "currency " << pkt->currency.get());
	LOG_INFO(LOG_MDBMCC, "scalingFactor " << pkt->scalingFactor);
	LOG_INFO(LOG_MDBMCC, "decimalPlaces " << pkt->decimalPlaces);
	LOG_INFO(LOG_MDBMCC, "coinTypeRouting " << pkt->coinTypeRouting.get());
	LOG_INFO_HEX(LOG_MDBMCC, pkt->coins, coinNum);
	for(uint16_t i = 0; i < 16; i++) {
		LOG_INFO(LOG_MDBMCC, "Coin " << context->get(i)->getNominal());
	}

	if(pkt->level >= 3) {
		context->setState(State_ExpansionIdentification);
	} else {
		context->setState(State_TubeStatus);
	}
}

void MdbMasterCoinChanger::sendExpansionIdentification() {
	LOG_INFO(LOG_MDBMCC, "sendExpansionIdentification");
	sender->startRequest();
	sender->addUint8(getType() | Command_Expansion);
	sender->addUint8(Subcommand_ExpansionIdentification);
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCoinChanger::recvExpansionIdentification(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "recvExpansionIdentification");
	if(crc == false || dataLen < sizeof(ExpansionIdentificationResponse)) {
		LOG_ERROR(LOG_MDBMCC, "Wrong response.");
		return;
	}
	sender->sendConfirm(Mdb::Control_ACK);

	ExpansionIdentificationResponse *pkt = (ExpansionIdentificationResponse*)data;
	LOG_INFO_HEX(LOG_MDBMCC, pkt->manufacturerCode, sizeof(pkt->manufacturerCode));
	LOG_INFO_HEX(LOG_MDBMCC, pkt->serialNumber, sizeof(pkt->serialNumber));
	LOG_INFO_HEX(LOG_MDBMCC, pkt->model, sizeof(pkt->model));
	LOG_INFO(LOG_MDBMBV, "softwareVersion " << pkt->softwareVersion.get());
	LOG_INFO(LOG_MDBMBV, "features " << pkt->features.get());

	context->setManufacturer(pkt->manufacturerCode, sizeof(pkt->manufacturerCode));
	context->setModel(pkt->model, sizeof(pkt->model));
	context->setSerialNumber(pkt->serialNumber, sizeof(pkt->serialNumber));
	context->setSoftwareVersion(pkt->softwareVersion.get());
	if(pkt->features.get() & Feature_ExtendedDiagnostic) { diagnostic = true; }
	context->setState(State_ExpansionFeatureEnable);
}

void MdbMasterCoinChanger::sendExpansionFeatureEnable() {
	LOG_INFO(LOG_MDBMCC, "sendExpansionFeatureEnable");
	sender->startRequest();
	sender->addUint8(getType() | Command_Expansion);
	sender->addUint8(Subcommand_ExpansionFeatureEnable);
	sender->addUint8(0x00);
	sender->addUint8(0x00);
	sender->addUint8(0x00);
	sender->addUint8(0x0F);
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCoinChanger::recvExpansionFeatureEnable(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "recvExpansionFeatureEnable");
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMCC, "Wrong response");
		return;
	}
	LOG_INFO(LOG_MDBMCC, "Recv ACK");
	context->setState(State_TubeStatus);
}

void MdbMasterCoinChanger::stateTubeStatusSend() {
	LOG_INFO(LOG_MDBMCC, "Send TubeStatus");
	sendTubeStatus();
}

void MdbMasterCoinChanger::stateTubeStatusRecv(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "stateTubeStatusRecv");
	if(recvTubeStatus(data, dataLen, crc) == false) { return; }
	context->setState(State_CoinType);
}

void MdbMasterCoinChanger::sendCoinType() {
	LOG_INFO(LOG_MDBMCC, "sendCoinType");
	sender->startRequest();
	sender->addUint8(getType() | Command_CoinType);
	sender->addUint16(0);
	sender->addUint16(0);
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCoinChanger::recvCoinType(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "Recv CoinType " << dataLen << " " << data[0]);
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMCC, "Wrong answer");
		return;
	}
	LOG_INFO(LOG_MDBMCC, "Recv ACK");
	gotoStatePoll();
	EventInterface event(deviceId, Event_Ready);
	deliverEvent(&event);
}

void MdbMasterCoinChanger::gotoStatePoll() {
	LOG_DEBUG(LOG_MDBMCL, "gotoStatePoll");
	repeatCount = 0;
	context->setState(State_Poll);
}

void MdbMasterCoinChanger::statePollSend() {
	if(enabling != enabled) {
		if(enabling == false) {
			LOG_INFO(LOG_MDBMCL, "Disabling");
			stateDisableSend();
			context->setState(State_Disable);
			return;
		} else {
			LOG_INFO(LOG_MDBMCL, "Enabling");
			stateEnableSend();
			context->setState(State_Enable);
			return;
		}
	}

	if(repeatCount == MDB_POLL_TUBE_STATUS_COUNT) {
		statePollTubeStatusSend();
		context->setState(State_PollTubeStatus);
		return;
	} else if(repeatCount == MDB_POLL_DIAGNOSTIC_COUNT) {
		if(diagnostic == true) {
			statePollDiagnosticSend();
			context->setState(State_PollDiagnostic);
			return;
		}
	} else if(repeatCount == MDB_POLL_ENABLE_COUNT) {
		if(enabled == false) {
			stateDisableSend();
			context->setState(State_Disable);
			return;
		} else {
			stateEnableSend();
			context->setState(State_Enable);
			return;
		}
	}

	switch(command) {
		case CommandType_Dispense: {
			command = CommandType_None;
			gotoStateExpansionPayout();
			return;
		}
		case CommandType_DispenseCoin: {
			command = CommandType_None;
			stateDispenseCoinSend();
			context->setState(State_DispenseCoin);
			return;
		}
		case CommandType_TubeFillingStart: {
			command = CommandType_None;
			stateTubeFillingInitSend();
			context->setState(State_TubeFillingInit);
			return;
		}
		default: sendPoll();
	}
}

void MdbMasterCoinChanger::statePollRecv(const uint8_t *data, uint16_t dataLen, bool crc) {
#if 0
	repeatCount++;
	if(crc == false) {
		return;
	}
	LOG_DEBUG(LOG_MDBMCC, "statePollRecv " << dataLen);
	LOG_DEBUG_HEX(LOG_MDBMCC, data, dataLen);
	sender->sendConfirm(Mdb::Control_ACK);

	for(uint16_t i = 0; i < dataLen; i++) {
		uint8_t status = data[i];
		if(status & CoinMask_DepositeSign) {
			i++; if(i >= dataLen) { break; }
			statePollRecvDeposite(status, data[i]);
			break;
		} else if(status & CoinMask_DispenseSign) {
			i++; if(i >= dataLen) { break; }
			statePollRecvDispense(status, data[i]);
			break;
		} else if(status == Status_EscrowRequest) {
			LOG_INFO(LOG_MDBMCC, "Recv EscrowRequest");
			statePollRecvEscrowRequest();
			break;
		} else if(status == Status_ChangerWasReset) {
			LOG_INFO(LOG_MDBMCC, "Recv JustReset");
			gotoStateSetup();
			break;
		} else {
			LOG_WARN(LOG_MDBMCC, "Poll status " << status);
		}
	}
#else
	repeatCount++;
	if(crc == false) {
		return;
	}
	LOG_DEBUG(LOG_MDBMCC, "statePollRecv " << dataLen);
	LOG_DEBUG_HEX(LOG_MDBMCC, data, dataLen);
	sender->sendConfirm(Mdb::Control_ACK);

	for(uint16_t i = 0; i < dataLen; i++) {
		uint8_t status = data[i];
		if(status & CoinMask_DispenseSign) {
			i++; if(i >= dataLen) { break; }
			statePollRecvDispense(status, data[i]);
			break;
		} else if(status & CoinMask_DepositeSign) {
			i++; if(i >= dataLen) { break; }
			statePollRecvDeposite(status, data[i]);
			break;
		} else if(status == Status_EscrowRequest) {
			LOG_INFO(LOG_MDBMCC, "Recv EscrowRequest");
			statePollRecvEscrowRequest();
			break;
		} else if(status == Status_ChangerWasReset) {
			LOG_INFO(LOG_MDBMCC, "Recv JustReset");
			gotoStateSetup();
			break;
		} else {
			LOG_WARN(LOG_MDBMCC, "Poll status " << status);
		}
	}
#endif
}

void MdbMasterCoinChanger::statePollRecvDeposite(uint8_t b1, uint8_t b2) {
	LOG_INFO(LOG_MDBMCC, "DepositeCoin " << b1 << "," << b2);
	uint16_t coinIndex = b1 & CoinMask_DepositeCoin;
	MdbCoin *coin = context->get(coinIndex);
	if(coin == NULL) {
		LOG_ERROR(LOG_MDBMCC, "Wrong coin index " << coinIndex);
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
		LOG_WARN(LOG_MDBMCC, "Reject coin " << coinNominal);
		return;
	}
	}

	LOG_WARN(LOG_MDBMCC, "Deposited coin " << coinNominal << " " << coinRoute << " " << b1 << " " << b2);
	if(coinNominal == MDB_CC_TOKEN) {
		eventCoin.set(Event_DepositeToken, 0, coinRoute, b1, b2);
		deliverEvent(&eventCoin);
		return;
	} else {
		context->registerLastCoin(coinNominal);
		eventCoin.set(Event_Deposite, coinNominal, coinRoute, b1, b2);
		deliverEvent(&eventCoin);
		return;
	}
}

void MdbMasterCoinChanger::statePollRecvDispense(uint8_t b1, uint8_t b2) {
	uint16_t coinIndex = b1 & CoinMask_DispenseCoin;
	MdbCoin *coin = context->get(coinIndex);
	if(coin == NULL) {
		LOG_ERROR(LOG_MDBMCC, "Wrong coinIndex " << coinIndex);
		return;
	}
	uint32_t coinNominal = coin->getNominal();
	uint32_t coinNumber = (b1 & CoinMask_DispenseNumber) >> 4;
	context->updateTube(coinIndex, b2);
	LOG_WARN(LOG_MDBMCC, "Manual dispensed " << coinNumber << " of coin " << coinNominal << " " << b2);
	eventCoin.set(Event_DispenseManual, coinNumber * coinNominal, 0, b1, b2);
	deliverEvent(&eventCoin);
}

void MdbMasterCoinChanger::statePollRecvEscrowRequest() {
	LOG_WARN(LOG_MDBMCC, "statePollRecvEscrowRequest");
	EventInterface event(deviceId, Event_EscrowRequest);
	deliverEvent(&event);
}

void MdbMasterCoinChanger::statePollTubeStatusSend() {
	LOG_INFO(LOG_MDBMCC, "statePollTubeStatusSend");
	sendTubeStatus();
}

void MdbMasterCoinChanger::statePollTubeStatusRecv(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "statePollTubeStatusRecv");
	repeatCount++;
	if(recvTubeStatus(data, dataLen, crc) == false) { return; }
	context->setState(State_Poll);
}

void MdbMasterCoinChanger::statePollDiagnosticSend() {
	LOG_INFO(LOG_MDBMCC, "statePollDiagnosticSend");
	sender->startRequest();
	sender->addUint8(getType() | Command_Expansion);
	sender->addUint8(Subcommand_ExpansionDiagnosticStatus);
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCoinChanger::statePollDiagnosticRecv(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "statePollDiagnosticRecv");
	LOG_INFO_HEX(LOG_MDBMCC, data, dataLen);
	repeatCount++;
	if(crc == false) {
		LOG_ERROR(LOG_MDBMCC, "Wrong answer");
		return;
	}
	sender->sendConfirm(Mdb::Control_ACK);
	context->setState(State_Poll);
}

void MdbMasterCoinChanger::stateDisableSend() {
	LOG_INFO(LOG_MDBMCC, "stateDisableSend");
	sender->startRequest();
	sender->addUint8(getType() | Command_CoinType);
	sender->addUint16(0); // coin disable
	sender->addUint16(0); // manual dispense enable
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCoinChanger::stateDisableRecv(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "stateDisableRecv " << crc << " " << data[0]);
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMCC, "Wrong answer");
		return;
	}
	LOG_DEBUG(LOG_MDBMCC, "Recv ACK");
	enabled = false;
	context->setStatus(Mdb::DeviceContext::Status_Disabled);
	gotoStatePoll();
}

void MdbMasterCoinChanger::stateEnableSend() {
	LOG_INFO(LOG_MDBMCC, "stateEnableSend " << context->getMask());
	sender->startRequest();
	sender->addUint8(getType() | Command_CoinType);
	sender->addUint16(context->getMask()); // coin enable
	sender->addUint16(context->getInTubeMask()); // manual dispense enable
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCoinChanger::stateEnableRecv(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "stateEnableRecv " << crc << " " << data[0]);
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMCC, "Wrong answer");
		return;
	}
	LOG_DEBUG(LOG_MDBMCC, "Recv ACK");
	enabled = true;
	context->setStatus(Mdb::DeviceContext::Status_Enabled);
	gotoStatePoll();
}

void MdbMasterCoinChanger::gotoStateExpansionPayout() {
	LOG_INFO(LOG_MDBMCC, "gotoStateExpansionPayout " << dispenseSum << "," << dispenseStep);
	dispenseCurrent = dispenseSum;
	if(dispenseCurrent > dispenseStep) {
		dispenseCurrent = dispenseStep;
	}
	dispenseSum = dispenseSum - dispenseCurrent;
	stateExpansionPayoutSend();
	context->setState(State_ExpansionPayout);
}

void MdbMasterCoinChanger::stateExpansionPayoutSend() {
	uint16_t value = context->money2value(dispenseCurrent);
	LOG_INFO(LOG_MDBMCC, "stateExpansionPayoutSend " << dispenseCurrent << "/" << dispenseSum << "," << value);
	sender->startRequest();
	sender->addUint8(getType() | Command_Expansion);
	sender->addUint8(Subcommand_ExpansionPayout);
	sender->addUint8(value);
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCoinChanger::stateExpansionPayoutRecv(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "stateExpansionPayoutRecv");
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMCC, "Wrong answer");
		return;
	}
	LOG_INFO(LOG_MDBMCC, "Recv ACK");
	dispenseCount = 0;
	context->setState(State_ExpansionPayoutWait);
}

void MdbMasterCoinChanger::stateExpansionPayoutWaitSend() {
	LOG_INFO(LOG_MDBMCC, "stateExpansionPayoutWaitSend");
	sender->startRequest();
	sender->addUint8(getType() | Command_Expansion);
	sender->addUint8(Subcommand_ExpansionPayoutValuePoll);
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCoinChanger::stateExpansionPayoutWaitRecv(const uint8_t *data, uint16_t dataLen, bool crc) {
	(void)dataLen;
	LOG_INFO(LOG_MDBMCC, "stateExpansionPayoutWaitRecv " << data[0] << " " << crc);
	if(crc == false && data[0] == Mdb::Control_ACK) {
		LOG_INFO(LOG_MDBMCC, "Dispense complete");
		context->setState(State_ExpansionPayoutStatus);
		return;
	}

	sender->sendConfirm(Mdb::Control_ACK);
	dispenseCount++;
	if(dispenseCount >= MDB_CC_EXP_PAYOUT_POLL_MAX) {
		gotoStateReset();
		eventCoin.set(Event_Dispense, 0, 0, 0, 0);
		deliverEvent(&eventCoin);
	}
}

void MdbMasterCoinChanger::stateExpansionPayoutStatusSend() {
	LOG_INFO(LOG_MDBMCC, "stateExpansionPayoutStatusSend");
	sender->startRequest();
	sender->addUint8(getType() | Command_Expansion);
	sender->addUint8(Subcommand_ExpansionPayoutStatus);
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCoinChanger::stateExpansionPayoutStatusRecv(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "stateExpansionPayoutStatusRecv");
	if(crc == false) {
		if(data[0] == Mdb::Control_ACK) {
			LOG_INFO(LOG_MDBMCC, "Payout is busy");
			return;
		} else {
			LOG_ERROR(LOG_MDBMCC, "Unwaited control " << data[0]);
			return;
		}
	}

	LOG_DEBUG_HEX(LOG_MDBMCC, data, dataLen);
	sender->sendConfirm(Mdb::Control_ACK);

	LOG_INFO(LOG_MDBMCC, "Dispense complete");
	if(dispenseSum > 0) {
		dispenseCurrent = dispenseSum;
		if(dispenseCurrent > dispenseStep) {
			dispenseCurrent = dispenseStep;
		}
		dispenseSum = dispenseSum - dispenseCurrent;
		context->setState(State_ExpansionPayout);
		return;
	} else {
		eventCoin.set(Event_Dispense, 0, 0, 0, 0);
		context->setState(State_DispenseTubeStatus);
		return;
	}
}

void MdbMasterCoinChanger::stateDispenseCoinSend() {
	LOG_INFO(LOG_MDBMCC, "stateDispenseCoinSend " << coinData);
	sender->startRequest();
	sender->addUint8(getType() | Command_Dispense);
	sender->addUint8(coinData);
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCoinChanger::stateDispenseCoinRecv(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "stateDispenseCoinRecv");
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMCC, "Wrong response");
		return;
	}
	LOG_INFO(LOG_MDBMCC, "Recv ACK");
	eventCoin.set(Event_DispenseCoin, 0, 0, coinData, 0);
	context->setState(State_DispenseWait);
}

void MdbMasterCoinChanger::stateDispenseWaitSend() {
	LOG_INFO(LOG_MDBMCC, "stateDispenseWaitSend");
	sender->startRequest();
	sender->addUint8(getType() | Command_Poll);
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCoinChanger::stateDispenseWaitRecv(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "stateDispenseWaitRecv " << data[0] << " " << crc);
	if(crc == false && data[0] == Mdb::Control_ACK) {
		LOG_INFO(LOG_MDBMCC, "Dispense complete");
		context->setState(State_DispenseTubeStatus);
		return;
	}
	sender->sendConfirm(Mdb::Control_ACK);
	for(uint16_t i = 0; i < dataLen; i++) {
		uint8_t status = data[i];
		if(status & CoinMask_DepositeSign) { i++; }
		else if(status & CoinMask_DispenseSign) { i++; }
		else if(status == Status_ChangerPayoutBusy) {
			LOG_INFO(LOG_MDBMCC, "Dispense in progress");
			return;
		} else {
			LOG_WARN(LOG_MDBMCC, "Poll status " << status);
		}
	}
	LOG_INFO(LOG_MDBMCC, "Dispense complete");
	context->setState(State_DispenseTubeStatus);
}

void MdbMasterCoinChanger::stateDispenseTubeStatusSend() {
	LOG_INFO(LOG_MDBMCC, "stateUpdateTubeStatusSend");
	sendTubeStatus();
}

void MdbMasterCoinChanger::stateDispenseTubeStatusRecv(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "stateUpdateTubeStatusRecv");
	if(recvTubeStatus(data, dataLen, crc) == false) {
		return;
	}
	gotoStatePoll();
	deliverEvent(&eventCoin);
}

void MdbMasterCoinChanger::stateTubeFillingInitSend() {
	LOG_INFO(LOG_MDBMCC, "stateTubeFillingInitSend");
	sender->startRequest();
	sender->addUint8(getType() | Command_CoinType);
	sender->addUint16(context->getNotFullMask()); // disable full tubes
	sender->addUint16(0); // manual dispense disable
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCoinChanger::stateTubeFillingInitRecv(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_INFO(LOG_MDBMCC, "Recv CoinType " << dataLen << " " << data[0]);
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMCC, "Wrong answer");
		return;
	}
	LOG_INFO(LOG_MDBMCC, "Recv ACK");
	context->setState(State_TubeFilling);
}

void MdbMasterCoinChanger::stateTubeFillingSend() {
	switch(command) {
		case CommandType_TubeFillingStop: {
			sendCoinType();
			context->setState(State_CoinType);
			break;
		}
		default: {
			sendPoll();
		}
	}
}

void MdbMasterCoinChanger::stateTubeFillingRecv(const uint8_t *data, uint16_t dataLen, bool crc) {
	if(crc == false) {
		return;
	}
	sender->sendConfirm(Mdb::Control_ACK);
	for(uint16_t i = 0; i < dataLen; i++) {
		uint8_t status = data[i];
		if(status & CoinMask_DepositeSign) {
			i++; if(i >= dataLen) { break; }
			statePollRecvDeposite(status, data[i]);
		} else if(status & CoinMask_DispenseSign) {
			i++; if(i >= dataLen) { break; }
			statePollRecvDispense(status, data[i]);
		} else {
			LOG_WARN(LOG_MDBMCC, "Poll status " << status);
		}
	}
}

void MdbMasterCoinChanger::stateTubeFillingUpdateSend() {
	sendTubeStatus();
}

void MdbMasterCoinChanger::stateTubeFillingUpdateRecv(const uint8_t *data, uint16_t dataLen, bool crc) {
	if(recvTubeStatus(data, dataLen, crc) == false) { return; }
	context->setState(State_TubeFillingInit);
	deliverEvent(&eventCoin);
}

void MdbMasterCoinChanger::sendTubeStatus() {
	sender->startRequest();
	sender->addUint8(getType() | Command_TubeStatus);
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

bool MdbMasterCoinChanger::recvTubeStatus(const uint8_t *data, uint16_t dataLen, bool crc) {
	if(crc == false || dataLen < sizeof(TubesResponse)) {
		LOG_ERROR(LOG_MDBMCC, "Wrong response.");
		LOG_ERROR_HEX(LOG_MDBMCC, data, dataLen);
		return false;
	}

	sender->sendConfirm(Mdb::Control_ACK);

	TubesResponse *pkt = (TubesResponse*)data;
	uint16_t coinNum = dataLen - sizeof(TubesResponse);
	context->update(pkt->tubeStatus.get(), pkt->tubeVolume, coinNum);

	LOG_INFO(LOG_MDBMCC, "tubeStatus " << pkt->tubeStatus.get());
	LOG_INFO_HEX(LOG_MDBMCC, pkt->tubeVolume, coinNum);
	return true;
}

void MdbMasterCoinChanger::sendPoll() {
	sender->startRequest();
	sender->addUint8(getType() | Command_Poll);
	REMOTE_LOG_OUT(RLOG_MDBMCC, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}
