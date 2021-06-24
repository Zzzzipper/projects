#include "MdbSlaveComGateway.h"

#include "utils/include/Version.h"
#include "logger/RemoteLogger.h"
#include "logger/include/Logger.h"

#include <string.h>
#include <tgmath.h>

using namespace Mdb::ComGateway;

bool MdbSlaveComGateway::ReportTransaction::open(EventEnvelope *envelope) {
	if(EventInterface::open(envelope) == false) { return false; }
	if(envelope->getParam((uint8_t*)&data, sizeof(data)) == false) { return false; }
	return true;
}

bool MdbSlaveComGateway::ReportTransaction::pack(EventEnvelope *envelope) {
	if(EventInterface::pack(envelope) == false) { return false; }
	if(envelope->setParam((uint8_t*)&data, sizeof(data)) == false) { return false; }
	return true;
}

bool MdbSlaveComGateway::ReportEvent::open(EventEnvelope *envelope) {
	if(EventInterface::open(envelope) == false) { return false; }
	if(envelope->getParam((uint8_t*)&data, sizeof(data)) == false) { return false; }
	return true;
}

bool MdbSlaveComGateway::ReportEvent::pack(EventEnvelope *envelope) {
	if(EventInterface::pack(envelope) == false) { return false; }
	if(envelope->setParam((uint8_t*)&data, sizeof(data)) == false) { return false; }
	return true;
}

MdbSlaveComGateway::MdbSlaveComGateway(Mdb::DeviceContext *automat, EventEngineInterface *events) :
	MdbSlave(Mdb::Device_ComGateway, events),
	deviceId(events),
	automat(automat),
	converter(automat->getMasterDecimalPoint()),
	state(State_Idle),
	pollData(MDB_POLL_DATA_SIZE)
{
	static MdbSlavePacketReceiver::Packet packets[] = {
		{ Command_Reset, MDB_SUBCOMMAND_NONE, 0xFF, 0xFF },
		{ Command_Setup, MDB_SUBCOMMAND_NONE, sizeof(SetupRequest), sizeof(SetupRequest) },
		{ Command_Poll, MDB_SUBCOMMAND_NONE, 0xFF, 0xFF },
		{ Command_Report, Subcommand_ReportTransaction, sizeof(ReportTransactionRequest), sizeof(ReportTransactionRequest) },
		{ Command_Report, Subcommand_ReportDtsEvent, sizeof(ReportDtsEventRequest), sizeof(ReportDtsEventRequest) },
		{ Command_Report, Subcommand_ReportAssetId, 0xFF, 0xFF },
		{ Command_Report, Subcommand_ReportCurrencyId, 0xFF, 0xFF },
		{ Command_Report, Subcommand_ReportProductId, 0xFF, 0xFF },
		{ Command_Control, Subcommand_ControlDisabled, 0xFF, 0xFF },
		{ Command_Control, Subcommand_ControlEnabled, 0xFF, 0xFF },
		{ Command_Control, Subcommand_ControlTransmit, 0xFF, 0xFF },
		{ Command_Expansion, Subcommand_ExpansionIdentification, 0xFF, 0xFF },
		{ Command_Expansion, Subcommand_ExpansionFeatureEnable, sizeof(ExpanstionFeatureEnbaleRequest), sizeof(ExpanstionFeatureEnbaleRequest) },
	};
	packetLayer = new MdbSlavePacketReceiver(getType(), this, packets, sizeof(packets)/sizeof(packets[0]));
}

EventDeviceId MdbSlaveComGateway::getDeviceId() {
	return deviceId;
}

void MdbSlaveComGateway::reset() {
	ATOMIC {
		LOG_ERROR(LOG_MDBSCG, "reset");
		command = Command_None;
		state = State_Reset;
	}
}

bool MdbSlaveComGateway::isInited() {
	bool result = false;
	ATOMIC {
		result = (state != State_Idle && state != State_Reset);
	}
	return result;
}

bool MdbSlaveComGateway::isEnable() {
	bool result = false;
	ATOMIC {
		result = (state != State_Idle && state != State_Reset && state != State_Disabled);
	}
	return result;
}

void MdbSlaveComGateway::initSlave(MdbSlave::Sender *sender, MdbSlave::Receiver *receiver) {
	slaveReceiver = receiver;
	slaveSender = sender;
	packetLayer->init(receiver);
}

void MdbSlaveComGateway::recvCommand(const uint8_t command) {
	packetLayer->recvCommand(command);
}

void MdbSlaveComGateway::recvSubcommand(const uint8_t subcommand) {
	packetLayer->recvSubcommand(subcommand);
}

void MdbSlaveComGateway::recvRequest(const uint8_t *data, uint16_t len) {
	packetLayer->recvRequest(data, len);
}

void MdbSlaveComGateway::recvRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_MDBSCG, "recvRequestPacket " << state << "," << commandId);
	REMOTE_LOG_IN(RLOG_MDBSCG, data, dataLen);
	this->commandId = commandId;
	switch(state) {
		case State_Idle: return;
		case State_Reset: stateResetCommand(commandId, data, dataLen); return;
		case State_Disabled: stateDisabledCommand(commandId, data, dataLen); return;
		default: LOG_ERROR(LOG_MDBSCG, "Unsupported state " << state);
	}
}

void MdbSlaveComGateway::recvUnsupportedPacket(const uint16_t commandId) {
#ifdef DEBUG_PROTOCOL
	procUnwaitedPacket(commandId, NULL, 0);
#endif
}

void MdbSlaveComGateway::recvConfirm(uint8_t control) {
	LOG_DEBUG(LOG_MDBSCG, "recvConfirm " << control);
	REMOTE_LOG_IN(RLOG_MDBSCL, control);
	switch (commandId) {
		case Mdb::ComGateway::CommandId_Poll: procCommandPollConfirm(control); return;
		default: LOG_DEBUG(LOG_MDBSCG, "Unwaited confirm state=" << state << ", commandId=" << commandId << ", control=" << control);
	}
}

void MdbSlaveComGateway::stateResetCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBSCG, "stateResetCommand");
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_Poll: stateResetCommandPoll(); return;
		case CommandId_Setup: procCommandSetup(data); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveComGateway::stateResetCommandPoll() {
	LOG_INFO(LOG_MDBSCC, "stateResetCommandPoll state=" << state);
	pollData.clear();
	pollData.addUint8(Status_JustReset);
	slaveSender->sendData(pollData.getData(), pollData.getLen());
	slaveReceiver->recvConfirm();
	state = State_Disabled;
	EventInterface event(deviceId, Event_Reset);
	deliverEvent(&event);
}

void MdbSlaveComGateway::procCommandReset() {
	LOG_INFO(LOG_MDBSCG, "procCommandReset");
	pollData.clear();
	pollData.addUint8(Status_JustReset);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	state = State_Disabled;
	EventInterface event(deviceId, Event_Reset);
	deliverEvent(&event);
}

void MdbSlaveComGateway::procCommandPoll() {
	LOG_DEBUG(LOG_MDBSCG, "procCommandPoll state=" << state);
	if(pollData.getLen() > 0) {
		slaveSender->sendData(pollData.getData(), pollData.getLen());
		slaveReceiver->recvConfirm();
		return;
	} else {
		slaveSender->sendAnswer(Mdb::Control_ACK);
		return;
	}
}

void MdbSlaveComGateway::procCommandPollConfirm(uint8_t control) {
	LOG_DEBUG(LOG_MDBSCG, "procCommandPollConfirm " << control);
	if(control != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBSCG, "Wrong confirm " << control);
		return;
	}
	pollData.clear();
}

void MdbSlaveComGateway::stateDisabledCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_Setup: procCommandSetup(data); return;
		case CommandId_ExpansionIdentification: procCommandExpansionIdentification(); return;
		case CommandId_ExpansionFeatureEnable: procCommandExpansionFeatureEnable(data); return;
		case CommandId_ControlDisabled: procCommandControlDisabled(); return;
		case CommandId_ControlEnabled: procCommandControlEnabled(); return;
		case CommandId_Poll: stateDisabledCommandPoll(); return;
		case CommandId_ReportTransaction: procReportTransaction(data); return;
		case CommandId_ReportDtsEvent: procReportDtsEvent(data); return;
		case CommandId_ReportAssetId: procReportAssetId(data); return;
		case CommandId_ReportCurrencyId: procReportCurrencyId(data); return;
		case CommandId_ReportProductId: procReportProductId(data); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveComGateway::stateDisabledCommandPoll() {
	LOG_DEBUG(LOG_MDBSCG, "stateDisabledCommandPoll");
	procCommandPoll();
}

void MdbSlaveComGateway::procCommandSetup(const uint8_t *data) {
	LOG_DEBUG(LOG_MDBSCG, "procCommandSetup");
	SetupRequest *req = (SetupRequest*)data;
	converter.setDeviceDecimalPoint(req->decimalPoint);
	converter.setScalingFactor(req->scaleFactor);
	LOG_INFO(LOG_MDBSCG, "Setup: " << req->featureLevel << "," << req->scaleFactor << "," << req->decimalPoint);

	SetupResponse resp;
	resp.command = 0x01;
	resp.featureLevel = Mdb::FeatureLevel_1;
	resp.maxResponseTime.set(5);

	LOG_INFO_HEX(LOG_MDBSCG, (const uint8_t*)&resp, sizeof(resp));
	slaveSender->sendData((const uint8_t*)&resp, sizeof(resp));
	state = State_Disabled;
}

void MdbSlaveComGateway::procCommandExpansionIdentification() {
	LOG_INFO(LOG_MDBSCG, "procCommandExpansionIdentification");

	ExpansionIdentificationResponse resp;
	resp.perepheralId = ComGatewayPerepheralId;
	strncpy((char*)resp.manufacturerCode, MDB_MANUFACTURER_CODE, sizeof(resp.manufacturerCode));
	strncpy((char*)resp.serialNumber, "0123456789AB", sizeof(resp.serialNumber));
	strncpy((char*)resp.modelNumber, "0123456789AB", sizeof(resp.modelNumber));
	resp.softwareVersion.set(100);
	resp.featureBits.set(ExpansionOption_VerboseMode);

	slaveSender->sendData((const uint8_t*)&resp, sizeof(resp));
}

void MdbSlaveComGateway::procCommandExpansionFeatureEnable(const uint8_t *data) {
	LOG_INFO(LOG_MDBSCG, "procCommandExpansionFeatureEnable");
	ExpanstionFeatureEnbaleRequest *req = (ExpanstionFeatureEnbaleRequest*)data;
	LOG_INFO(LOG_MDBSCG, "FeatureEnbale: " << req->featureBits.get());

	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveComGateway::procCommandControlEnabled() {
	LOG_INFO(LOG_MDBSCG, "procCommandControlEnabled");
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveComGateway::procCommandControlDisabled() {
	LOG_INFO(LOG_MDBSCG, "procCommandControlDisabled");
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveComGateway::procReportTransaction(const uint8_t *data) {
	ReportTransactionRequest *req = (ReportTransactionRequest*)data;
	uint32_t itemPrice = converter.convertDeviceToMaster(req->price.get());
	LOG_INFO(LOG_MDBSCG, "transactionType " << req->transactionType);
	LOG_INFO(LOG_MDBSCG, "procReportTransaction " << req->itemNumber.get() << "," << req->price.get() << "/" << itemPrice);
	LOG_DEBUG_HEX(LOG_MDBSCG, data, sizeof(ReportTransactionRequest));
	LOG_INFO(LOG_MDBSCG, "cashInCoinTubes " << req->cashInCoinTubes.get());
	LOG_INFO(LOG_MDBSCG, "cashInCashbox " << req->cashInCashbox.get());
	LOG_INFO(LOG_MDBSCG, "cashInBills " << req->cashInBills.get());
	LOG_INFO(LOG_MDBSCG, "valueInCashless1 " << req->valueInCashless1.get());
	LOG_INFO(LOG_MDBSCG, "valueInCashless2 " << req->valueInCashless2.get());
	LOG_INFO(LOG_MDBSCG, "revalueInCashless1 " << req->revalueInCashless1.get());
	LOG_INFO(LOG_MDBSCG, "revalueInCashless2 " << req->revalueInCashless2.get());
	LOG_INFO(LOG_MDBSCG, "cashOut " << req->cashOut.get());
	LOG_INFO(LOG_MDBSCG, "discountAmount " << req->discountAmount.get());
	LOG_INFO(LOG_MDBSCG, "surchargeAmount " << req->surchargeAmount.get());
	LOG_INFO(LOG_MDBSCG, "userGroup " << req->userGroup);
	LOG_INFO(LOG_MDBSCG, "priceList " << req->priceList);
	LOG_INFO(LOG_MDBSCG, "date " << req->date[0] << "/" << req->date[1] << "/" << req->date[2] << "/" << req->date[3]);
	LOG_INFO(LOG_MDBSCG, "time " << req->date[0] << "/" << req->date[1]);

	slaveSender->sendAnswer(Mdb::Control_ACK);

	ReportTransaction event(deviceId);
	event.data.transactionType = req->transactionType;
	event.data.itemNumber = req->itemNumber.get();
	event.data.price = itemPrice;
	event.data.cashInCoinTubes = converter.convertDeviceToMaster(req->cashInCoinTubes.get());
	event.data.cashInCashbox = converter.convertDeviceToMaster(req->cashInCashbox.get());
	event.data.cashInBills = converter.convertDeviceToMaster(req->cashInBills.get());
	event.data.valueInCashless1 = converter.convertDeviceToMaster(req->valueInCashless1.get());
	event.data.valueInCashless2 = converter.convertDeviceToMaster(req->valueInCashless2.get());
	event.data.revalueInCashless1 = converter.convertDeviceToMaster(req->revalueInCashless1.get());
	event.data.revalueInCashless2 = converter.convertDeviceToMaster(req->revalueInCashless2.get());
	event.data.cashOut = converter.convertDeviceToMaster(req->cashOut.get());
	event.data.discountAmount = converter.convertDeviceToMaster(req->discountAmount.get());
	event.data.surchargeAmount = converter.convertDeviceToMaster(req->surchargeAmount.get());
	event.data.userGroup = req->userGroup;
	event.data.priceList = req->priceList;
	deliverEvent(&event);
}

void MdbSlaveComGateway::procReportDtsEvent(const uint8_t *data) {
	ReportDtsEventRequest *req = (ReportDtsEventRequest*)data;
	LOG_INFO(LOG_MDBSCG, "procReportDtsEvent");
	LOG_DEBUG_HEX(LOG_MDBSCG, data, sizeof(ReportDtsEventRequest));
	LOG_INFO_STR(LOG_MDBSCG, req->code, sizeof(req->code));
	LOG_INFO(LOG_MDBSCG, "date " << req->year.get() << "/" << req->month.get() << "/" << req->day.get());
	LOG_INFO(LOG_MDBSCG, "time " << req->hour.get() << "/" << req->minute.get());
	LOG_INFO(LOG_MDBSCG, "duration " << req->duration.get());
	LOG_INFO(LOG_MDBSCG, "activity " << req->activity);

	slaveSender->sendAnswer(Mdb::Control_ACK);

	ReportEvent event(deviceId);
	event.data.code.set((char*)req->code, sizeof(req->code));
	event.data.datetime.year = req->year.get() < 2000 ? 0 : req->year.get() - 2000;
	event.data.datetime.month = req->month.get();
	event.data.datetime.day = req->day.get();
	event.data.datetime.hour = req->hour.get();
	event.data.datetime.minute = req->minute.get();
	event.data.datetime.second = 0;
	event.data.duration = req->duration.get();
	event.data.activity = req->activity;
	deliverEvent(&event);
}

void MdbSlaveComGateway::procReportAssetId(const uint8_t *data) {
	(void)data;
	LOG_INFO(LOG_MDBSCG, "procReportAssetId");
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveComGateway::procReportCurrencyId(const uint8_t *data) {
	(void)data;
	LOG_INFO(LOG_MDBSCG, "procReportCurrencyId");
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveComGateway::procReportProductId(const uint8_t *data) {
	(void)data;
	LOG_INFO(LOG_MDBSCG, "procReportProductId");
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveComGateway::procUnwaitedPacket(uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	LOG_ERROR(LOG_MDBSCG, "procUnwaitedPacket " << state << "," << commandId);
#ifdef DEBUG_PROTOCOL
	Mdb::EventError event(deviceId, Event_Error);
	event.code = ConfigEvent::Type_CoinUnwaitedPacket;
	event.data.clear();
	event.data << "scg" << state << "*" << getType() << "*";
	event.data.addHex(commandId >> 8);
	event.data.addHex(commandId);
	event.data << "*";
	for(uint16_t i = 0; i < dataLen; i++) {
		event.data.addHex(data[i]);
	}
	deliverEvent(&event);
#endif
}
