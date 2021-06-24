#include "MdbSlaveCashless3.h"

#include "mdb/MdbProtocolCashless.h"
#include "mdb/master/cashless/MdbMasterCashless.h"
#include "utils/include/Version.h"
#include "logger/RemoteLogger.h"
#include "logger/include/Logger.h"

#include <string.h>
#include <tgmath.h>

using namespace Mdb::Cashless;

enum EventVendRequestParam {
	EventVendRequestParam_productId = 0,
	EventVendRequestParam_price
};

bool MdbSlaveCashlessInterface::EventVendRequest::open(EventEnvelope *envelope) {
	if(EventInterface::open(envelope) == false) { return false; }
	if(envelope->getUint16(EventVendRequestParam_productId, &productId) == false) { return false; }
	if(envelope->getUint32(EventVendRequestParam_price, &price) == false) { return false; }
	return true;
}

bool MdbSlaveCashlessInterface::EventVendRequest::pack(EventEnvelope *envelope) {
	if(EventInterface::pack(envelope) == false) { return false; }
	if(envelope->addUint16(EventVendRequestParam_productId, productId) == false) { return false; }
	if(envelope->addUint32(EventVendRequestParam_price, price) == false) { return false; }
	return true;
}

MdbSlaveCashless3::MdbSlaveCashless3(
	Mdb::Device deviceType,
	uint8_t maxLevel,
	Mdb::DeviceContext *context,
	TimerEngine *timerEngine,
	EventEngineInterface *events,
	StatStorage *stat
) :
	MdbSlave(deviceType, events),
	deviceId(events),
	context(context),
	timerEngine(timerEngine),
	maxLevel(maxLevel),
	pollData(MDB_POLL_DATA_SIZE)
{
	static MdbSlavePacketReceiver::Packet packets[] = {
		{ Command_Reset, MDB_SUBCOMMAND_NONE, 0xFF, 0xFF },
		{ Command_Setup, Subcommand_SetupConfig, sizeof(SetupConfigRequest), sizeof(SetupConfigRequest) },
		{ Command_Setup, Subcommand_SetupPrices, sizeof(SetupPricesL1Request), sizeof(SetupPricesL3Request) },
		{ Command_Poll, MDB_SUBCOMMAND_NONE, 0xFF, 0xFF },
		{ Command_Vend, Subcommand_VendRequest, sizeof(VendRequestRequest), sizeof(VendRequestRequestL3) },
		{ Command_Vend, Subcommand_VendCancel, 0xFF, 0xFF },
		{ Command_Vend, Subcommand_VendSuccess, sizeof(VendSuccessRequest), sizeof(VendSuccessRequest) },
		{ Command_Vend, Subcommand_VendFailure, 0xFF, 0xFF },
		{ Command_Vend, Subcommand_SessionComplete, 0xFF, 0xFF },
		{ Command_Vend, Subcommand_CashSale, sizeof(VendCashSaleRequest), sizeof(VendCashSaleRequestEC) },
		{ Command_Reader, Subcommand_ReaderEnable, 0xFF, 0xFF },
		{ Command_Reader, Subcommand_ReaderDisable, 0xFF, 0xFF },
		{ Command_Reader, Subcommand_ReaderCancel, 0xFF, 0xFF },
		{ Command_Revalue, Subcommand_RevalueRequest, sizeof(RevalueRequestL2), sizeof(RevalueRequestEC) },
		{ Command_Revalue, Subcommand_RevalueLimitRequest, 0xFF, 0xFF },
		{ Command_Expansion, Subcommand_ExpansionRequestId, sizeof(ExpansionRequestIdRequest), sizeof(ExpansionRequestIdRequest) },
		{ Command_Expansion, Subcommand_ExpansionEnableOptions, sizeof(ExpansionEnableOptionsRequest), sizeof(ExpansionEnableOptionsRequest) },
	};
	state = stat->add(Mdb::DeviceContext::Info_MdbS_CL_State, State_Idle);
	countPoll = stat->add(Mdb::DeviceContext::Info_MdbS_CL_PollCount, 0);
	packetLayer = new MdbSlavePacketReceiver(getType(), this, packets, sizeof(packets)/sizeof(packets[0]));
	timer = timerEngine->addTimer<MdbSlaveCashless3, &MdbSlaveCashless3::procTimer>(this);
}

MdbSlaveCashless3::~MdbSlaveCashless3() {
	timerEngine->deleteTimer(timer);
}

EventDeviceId MdbSlaveCashless3::getDeviceId() {
	return deviceId;
}

void MdbSlaveCashless3::reset() {
	ATOMIC {
		LOG_ERROR(LOG_MDBSCL, "reset");
		level = Mdb::FeatureLevel_1;
		command = Command_None;
		enabled = false;
		packetLayer->setMode(Mdb::Mode_Normal);
		state->set(State_Reset);
	}
}

bool MdbSlaveCashless3::isInited() {
	bool result = false;
	ATOMIC {
		result = (state->get() != State_Idle && state->get() != State_Reset);
	}
	return result;
}

bool MdbSlaveCashless3::isEnable() {
	bool result = false;
	ATOMIC {
		result = enabled;
	}
	return result;
}

void MdbSlaveCashless3::setCredit(uint32_t credit) {
	ATOMIC {
		LOG_INFO(LOG_MDBSCL, "setCredit " << state->get() << "," << credit);
		this->command = Command_SetCredit;
		this->credit = credit;
	}
}

void MdbSlaveCashless3::approveVend(uint32_t productPrice) {
	ATOMIC {
		LOG_INFO(LOG_MDBSCL, "approveVend " << state->get() << "," << productPrice);
		this->command = Command_ApproveVend;
		this->productPrice = productPrice;
	}
}

void MdbSlaveCashless3::denyVend(bool close) {
	ATOMIC {
		LOG_INFO(LOG_MDBSCL, "denyVend");
		if(close == true) {
			this->command = Command_DenyVendAndClose;
		} else {
			this->command = Command_DenyVend;
		}
	}
}

void MdbSlaveCashless3::cancelVend() {
	ATOMIC {
		LOG_INFO(LOG_MDBSCL, "cancelVend");
		this->command = Command_None;
		switch(state->get()) {
		case State_SessionIdle:
		case State_VendApproving: {
			state->set(State_SessionCancel);
			continue;
		}
		default: LOG_ERROR(LOG_MDBSCL, "Unwaited command " << state->get());
		}
	}
}

void MdbSlaveCashless3::initSlave(MdbSlave::Sender *sender, MdbSlave::Receiver *receiver) {
	slaveReceiver = receiver;
	slaveSender = sender;
	packetLayer->init(receiver);
}

void MdbSlaveCashless3::recvCommand(const uint8_t command) {
	packetLayer->recvCommand(command);
}

void MdbSlaveCashless3::recvSubcommand(const uint8_t subcommand) {
	packetLayer->recvSubcommand(subcommand);
}

void MdbSlaveCashless3::recvRequest(const uint8_t *data, uint16_t len) {
	packetLayer->recvRequest(data, len);
}

void MdbSlaveCashless3::recvRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_MDBSCL, "recvRequestPacket " << state->get() << "," << commandId);
	REMOTE_LOG_IN(RLOG_MDBSCL, data, dataLen);
	if(state->get() == State_Idle) { return; }

	this->commandId = commandId;
	switch(commandId) {
		case CommandId_SetupConfig: procCommandSetupConfig(data); return;
		case CommandId_SetupPricesL1: procCommandSetupPricesL1(data); return;
		case CommandId_ExpansionRequestId: procCommandExpansionRequestId(data); return;
		case CommandId_ExpansionEnableOptions: procCommandExpansionEnableOptions(data); return;
		case CommandId_ExpansionDiagnostics: procCommandExpansionDiagnostics(); return;
		case CommandId_CashSale: procCommandVendCashSale(data); return;
	}

	switch(state->get()) {
		case State_Reset: stateResetCommand(commandId, data, dataLen); return;
		case State_Disabled: stateDisabledCommand(commandId, data, dataLen); return;
		case State_Enabled: stateEnabledCommand(commandId, data, dataLen); return;
		case State_SessionIdle: stateSessionIdleCommand(commandId, data, dataLen); return;
		case State_VendApproving: stateVendApprovingCommand(commandId, data, dataLen); return;
		case State_Vending: stateVendingCommand(commandId, data, dataLen); return;
		case State_SessionCancel: stateSessionCancelCommand(commandId, data, dataLen); return;
		case State_SessionEnd: stateSessionEndCommand(commandId, data, dataLen); return;
		case State_ReaderCancel: stateReaderCancelCommand(commandId, data, dataLen); return;
		default: LOG_ERROR(LOG_MDBSCL, "Unsupported state " << state->get());
	}
}

void MdbSlaveCashless3::recvUnsupportedPacket(const uint16_t commandId) {
#ifndef DEBUG_PROTOCOL
	(void)commandId;
#else
	procUnwaitedPacket(commandId, NULL, 0);
#endif
}

void MdbSlaveCashless3::recvConfirm(uint8_t control) {
	LOG_DEBUG(LOG_MDBSCL, "recvConfirm " << control);
	REMOTE_LOG_IN(RLOG_MDBSCL, control);
	switch (commandId) {
		case Mdb::Cashless::CommandId_Poll: procCommandPollConfirm(control); return;
		case Mdb::Cashless::CommandId_SessionComplete: procCommandSessionCompleteConfirm(control); return;
		default: LOG_DEBUG(LOG_MDBSCL, "Unwaited confirm state=" << state->get() << ", commandId=" << commandId << ", control=" << control);
	}
}

void MdbSlaveCashless3::procTimer() {
	LOG_ERROR(LOG_MDBMCL, "procTimer " << state->get());
	switch(state->get()) {
	case State_Vending: stateVendingTimeout(); return;
	default: LOG_ERROR(LOG_SM, "Unwaited timeout " << state->get());
	}
}

void MdbSlaveCashless3::procCommandSetupConfig(const uint8_t *data) {
	LOG_DEBUG(LOG_MDBSCL, "procCommandSetupConfig");
	SetupConfigRequest *req = (SetupConfigRequest*)data;
	LOG_INFO(LOG_MDBSCL, "SetupConfig: " << req->subcommand << "," << req->featureLevel << "," << req->columnsOnDisplay << "," << req->rowsOnDisplay << "," << req->displayInfo);
	if(req->featureLevel == Mdb::FeatureLevel_3) {
		level = (maxLevel >= Mdb::FeatureLevel_3) ? Mdb::FeatureLevel_3 : Mdb::FeatureLevel_1;
	} else {
		level = Mdb::FeatureLevel_1;
	}

	SetupConfigResponse resp;
	resp.configData = 0x01;
	resp.featureLevel = level;
	resp.currency.set(context->getCurrency());
	resp.scaleFactor = context->getScalingFactor();
	resp.decimalPlaces = context->getDecimalPoint();
	resp.maxRespTime = 120;
#ifdef PRODUCTION_SPENGLER
	resp.options = SetupConfigOption_CashSale | SetupConifgOption_RefundAble;
#else
	resp.options = SetupConfigOption_CashSale;
#endif

	LOG_INFO_HEX(LOG_MDBSCL, (const uint8_t*)&resp, sizeof(resp));
	REMOTE_LOG_OUT(RLOG_MDBSCL, (const uint8_t*)&resp, sizeof(resp));
	slaveSender->sendData((const uint8_t*)&resp, sizeof(resp));
}

void MdbSlaveCashless3::procCommandSetupPricesL1(const uint8_t *data) {
	LOG_INFO(LOG_MDBSCL, "procCommandSetupPricesL1");
	if(packetLayer->getMode() == Mdb::Mode_Normal) {
		SetupPricesL1Request *req = (SetupPricesL1Request*)data;
		LOG_INFO(LOG_MDBSCL, "SetupPricesL1: " << req->subcommand << "," << req->minimunPrice << "," << req->maximumPrice);

		REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
		slaveSender->sendAnswer(Mdb::Control_ACK);
	} else {
		SetupPricesL3Request *req = (SetupPricesL3Request*)data;
		LOG_INFO(LOG_MDBSCL, "SetupPricesEC: " << req->subcommand << "," << req->minimunPrice << "," << req->maximumPrice);

		REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
		slaveSender->sendAnswer(Mdb::Control_ACK);
	}
}

void MdbSlaveCashless3::procCommandExpansionRequestId(const uint8_t *data) {
	LOG_INFO(LOG_MDBSCL, "procCommandExpansionRequestId");
	ExpansionRequestIdRequest *req = (ExpansionRequestIdRequest*)data;
	LOG_INFO_HEX(LOG_MDBSCL, req->manufacturerCode, sizeof(req->manufacturerCode));
	LOG_INFO_HEX(LOG_MDBSCL, req->serialNumber, sizeof(req->serialNumber));
	LOG_INFO_HEX(LOG_MDBSCL, req->modelNumber, sizeof(req->modelNumber));
	LOG_INFO(LOG_MDBMBV, "softwareVersion " << req->softwareVersion.get());

	context->setManufacturer(req->manufacturerCode, sizeof(req->manufacturerCode));
	context->setModel(req->modelNumber, sizeof(req->modelNumber));
	context->setSerialNumber(req->serialNumber, sizeof(req->serialNumber));
	context->setSoftwareVersion(req->softwareVersion.get());

	if(level == Mdb::FeatureLevel_3) {
		ExpansionRequestIdResponseL3 resp;
		resp.perepheralId = PerepheralId;
		strncpy((char*)resp.manufacturerCode, MDB_MANUFACTURER_CODE, sizeof(resp.manufacturerCode));
		strncpy((char*)resp.serialNumber, "0123456789AB", sizeof(resp.serialNumber));
		strncpy((char*)resp.modelNumber, "0123456789AB", sizeof(resp.modelNumber));
		resp.softwareVersion.set(100);
		resp.featureBits.set(FeatureOption_AlwaysIdle);

		REMOTE_LOG_OUT(RLOG_MDBSCL, (const uint8_t*)&resp, sizeof(resp));
		slaveSender->sendData((const uint8_t*)&resp, sizeof(resp));
		return;
	} else {
		ExpansionRequestIdResponseL1 resp;
		resp.perepheralId = PerepheralId;
		strncpy((char*)resp.manufacturerCode, MDB_MANUFACTURER_CODE, sizeof(resp.manufacturerCode));
		strncpy((char*)resp.serialNumber, "0123456789AB", sizeof(resp.serialNumber));
		strncpy((char*)resp.modelNumber, "0123456789AB", sizeof(resp.modelNumber));
		resp.softwareVersion.set(100);

		REMOTE_LOG_OUT(RLOG_MDBSCL, (const uint8_t*)&resp, sizeof(resp));
		slaveSender->sendData((const uint8_t*)&resp, sizeof(resp));
		return;
	}
}

void MdbSlaveCashless3::procCommandExpansionEnableOptions(const uint8_t *data) {
	LOG_INFO(LOG_MDBSCL, "procCommandExpansionEnableOptions");
	ExpansionEnableOptionsRequest *req = (ExpansionEnableOptionsRequest*)data;
	LOG_INFO(LOG_MDBMBV, "softwareVersion " << req->featureBits.get());

	uint32_t featureBits = req->featureBits.get();
	if((featureBits & FeatureOption_32bitMonetaryFormat) || (featureBits & FeatureOption_MultiCurrency)) {
		packetLayer->setMode(Mdb::Mode_Expanded);
	}

	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveCashless3::procCommandExpansionDiagnostics() {
	LOG_INFO(LOG_MDBSCL, "procCommandExpansionDiagnostics");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveCashless3::procCommandVendCashSale(const uint8_t *data) {
	VendRequestRequest *req = (VendRequestRequest*)data;
	uint16_t itemNumber = req->itemNumber.get() & CashSaleNumber_Mask;
	uint32_t itemPrice = context->value2money(req->itemPrice.get());
	LOG_INFO(LOG_MDBSCL, "procCommandVendCashSale " << req->itemNumber.get() << "/" << itemNumber << "," << req->itemPrice.get() << "/" << itemPrice);

	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	MdbSlaveCashlessInterface::EventVendRequest event(deviceId, MdbSlaveCashlessInterface::Event_CashSale, itemNumber, itemPrice);
	deliverEvent(&event);
}

void MdbSlaveCashless3::stateResetCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBSCL, "stateResetCommand");
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_Poll: stateResetCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveCashless3::procCommandReset() {
	LOG_INFO(LOG_MDBSCL, "procCommandReset");
	pollData.clear();
	pollData.addUint8(Status_JustReset);

	level = Mdb::FeatureLevel_1;
	enabled = false;
	packetLayer->setMode(Mdb::Mode_Normal);

	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	state->set(State_Disabled);
	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_Reset);
	deliverEvent(&event);
}

void MdbSlaveCashless3::stateResetCommandPoll() {
	LOG_INFO(LOG_MDBSCL, "stateResetCommandPoll " << state->get());
	countPoll->inc();
	pollData.clear();
	pollData.addUint8(Status_JustReset);

	REMOTE_LOG_OUT(RLOG_MDBSCL, pollData.getData(), pollData.getLen());
	slaveSender->sendData(pollData.getData(), pollData.getLen());
	slaveReceiver->recvConfirm();
	state->set(State_Disabled);
	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_Reset);
	deliverEvent(&event);
}

void MdbSlaveCashless3::procCommandPoll() {
	LOG_DEBUG(LOG_MDBSCL, "procCommandPoll " << state->get() << ", " << pollData.getLen());
	countPoll->inc();
	if(pollData.getLen() > 0) {
		REMOTE_LOG_OUT(RLOG_MDBSCL, pollData.getData(), pollData.getLen());
		slaveSender->sendData(pollData.getData(), pollData.getLen());
		slaveReceiver->recvConfirm();
		return;
	} else {
		REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
		slaveSender->sendAnswer(Mdb::Control_ACK);
		return;
	}
}

void MdbSlaveCashless3::procCommandPollConfirm(uint8_t control) {
	LOG_DEBUG(LOG_MDBSCL, "procCommandPollConfirm " << control);
	if(control != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBSCL, "Wrong confirm " << control);
		return;
	}
	pollData.clear();
}

void MdbSlaveCashless3::stateDisabledCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_ReaderEnable: stateDisabledReaderEnable(); return;
		case CommandId_ReaderDisable: procCommandReaderDisable(); return;
		case CommandId_VendRequest: stateSessionIdleCommandVendRequest(data); return;
		case CommandId_Poll: stateDisabledCommandPoll(); return;
		case CommandId_ReaderCancel: procCommandReaderCancel(); return;
		case CommandId_SessionComplete: procCommandSessionComplete(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveCashless3::stateDisabledReaderEnable() {
	LOG_INFO(LOG_MDBSCL, "stateDisabledReaderEnable");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	enabled = true;
	gotoStateEnabled();
	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_Enable);
	deliverEvent(&event);
}

void MdbSlaveCashless3::stateDisabledCommandPoll() {
	LOG_DEBUG(LOG_MDBSCL, "stateDisabledCommandPoll");
	procCommandPoll();
}

void MdbSlaveCashless3::procCommandReaderEnable() {
	LOG_INFO(LOG_MDBSCL, "procCommandReaderEnable");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	enabled = true;
}

void MdbSlaveCashless3::procCommandReaderDisable() {
	LOG_INFO(LOG_MDBSCL, "procCommandReaderDisable");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	enabled = false;
}

void MdbSlaveCashless3::procCommandReaderCancel() {
	LOG_INFO(LOG_MDBSCL, "procCommandReaderCancel");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	state->set(State_ReaderCancel);
}

void MdbSlaveCashless3::gotoStateEnabled() {
	LOG_DEBUG(LOG_MDBSCL, "gotoStateEnabled");
	if(enabled == true) {
		state->set(State_Enabled);
	} else {
		state->set(State_Disabled);
	}
}

void MdbSlaveCashless3::stateEnabledCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_ReaderDisable: stateEnabledCommandReaderDisable(); return;
		case CommandId_ReaderEnable: procCommandReaderEnable(); return;
		case CommandId_VendRequest: stateSessionIdleCommandVendRequest(data); return;
		case CommandId_Poll: stateEnabledCommandPoll(); return;
		case CommandId_ReaderCancel: procCommandReaderCancel(); return;
		case CommandId_SessionComplete: procCommandSessionComplete(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveCashless3::stateEnabledCommandReaderDisable() {
	LOG_INFO(LOG_MDBSCL, "stateEnabledCommandReaderDisable");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	enabled = false;
	state->set(State_Disabled);
	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_Disable);
	deliverEvent(&event);
}

void MdbSlaveCashless3::stateEnabledCommandPoll() {
	LOG_DEBUG(LOG_MDBSCL, "stateEnabledCommandPoll");
	if(command == Command_SetCredit) {
		command = Command_None;
		procCommandPollSessionBegin();
		return;
	} else {
		procCommandPoll();
		return;
	}
}

void MdbSlaveCashless3::procCommandPollSessionBegin() {
	LOG_INFO(LOG_MDBSCL, "procCommandPollSessionBegin " << context->money2value(credit));
	if(level == Mdb::FeatureLevel_1) {
		PollSectionBeginSession section;
		section.request = Status_BeginSession;
		section.amount.set(context->money2value(credit));
		pollData.add(&section, sizeof(section));
	} else if(packetLayer->getMode() == Mdb::Mode_Normal) {
		PollSectionBeginSessionL3 section;
		section.request = Status_BeginSession;
		section.amount.set(context->money2value(credit));
		section.paymentId.set(0xFFFFFFFF);
		section.paymentType = 0;
		section.paymentData.set(0);
		pollData.add(&section, sizeof(section));
	} else {
		PollSectionBeginSessionEC section;
		section.request = Status_BeginSession;
		section.amount.set(context->money2value(credit));
		section.paymentId.set(0xFFFFFFFF);
		section.paymentType = 0;
		section.paymentData.set(0);
		strncpy((char*)section.language, "ru", sizeof(section.language));
		section.currency.set(context->getCurrency());
		section.cardOptions = 0;
		pollData.add(&section, sizeof(section));
	}

	REMOTE_LOG_OUT(RLOG_MDBSCL, pollData.getData(), pollData.getLen());
	slaveSender->sendData(pollData.getData(), pollData.getLen());
	slaveReceiver->recvConfirm();
	countPoll->inc();
	state->set(State_SessionIdle);
}

void MdbSlaveCashless3::stateSessionIdleCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: stateSessionIdleCommandReset(); return;
		case CommandId_ReaderDisable: procCommandReaderDisable(); return;
		case CommandId_ReaderEnable: procCommandReaderEnable(); return;
		case CommandId_SessionComplete: stateSessionIdleCommandSessionComplete(); return;
		case CommandId_VendRequest: stateSessionIdleCommandVendRequest(data); return;
		case CommandId_RevalueLimitRequest: stateSessionIdleCommandRevalueLimitRequest(data); return;
		case CommandId_RevalueRequest: stateSessionIdleCommandRevalueRequest(data); return;
		case CommandId_Poll: stateSessionIdleCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveCashless3::stateSessionIdleCommandReset() {
	LOG_INFO(LOG_MDBSCL, "stateSessionIdleCommandReset");
	procCommandReset();
	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_VendCancel);
	deliverEvent(&event);
}

void MdbSlaveCashless3::stateSessionIdleCommandSessionComplete() {
	LOG_INFO(LOG_MDBSCL, "stateSessionIdleCommandSessionComplete");
	procCommandSessionComplete();
	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_VendCancel);
	deliverEvent(&event);
}

void MdbSlaveCashless3::stateSessionIdleCommandVendRequest(const uint8_t *data) {
	VendRequestRequest *req = (VendRequestRequest*)data;
	LOG_INFO(LOG_MDBSCL, "stateSessionIdleCommandVendRequest " << req->itemNumber.get() << "," << req->itemPrice.get());
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	requestPrice = context->value2money(req->itemPrice.get());
	state->set(State_VendApproving);
	MdbSlaveCashlessInterface::EventVendRequest event(deviceId, MdbSlaveCashlessInterface::Event_VendRequest, req->itemNumber.get(), requestPrice);
	deliverEvent(&event);
}

// note: можно вернуть 0 или Status_RevalueDenied. Пока не понятно, что лучше.
void MdbSlaveCashless3::stateSessionIdleCommandRevalueLimitRequest(const uint8_t *) {
	LOG_INFO(LOG_MDBSCL, "stateSessionIdleCommandRevalueLimitRequest");
	slaveSender->startData();
	slaveSender->addUint8(0x0F);
	slaveSender->addUint16(0);
	REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
}

void MdbSlaveCashless3::stateSessionIdleCommandRevalueRequest(const uint8_t *) {
	LOG_INFO(LOG_MDBSCL, "stateSessionIdleCommandRevalueRequest");
	slaveSender->startData();
	slaveSender->addUint8(Mdb::Cashless::Status_RevalueDenied);
	REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
}

void MdbSlaveCashless3::stateSessionIdleCommandPoll() {
	LOG_DEBUG(LOG_MDBSCL, "stateSessionIdleCommandPoll");
	procCommandPoll();
	if(command == Command_SetCredit) {
		state->set(State_SessionCancel);
	}
}

void MdbSlaveCashless3::procCommandSessionComplete() {
	LOG_INFO(LOG_MDBSCL, "procCommandSessionComplete");
	slaveSender->startData();
	slaveSender->addUint8(Mdb::Cashless::Status_EndSession);
	REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
	slaveReceiver->recvConfirm();
	state->set(State_SessionEnd);
}

void MdbSlaveCashless3::procCommandSessionCompleteConfirm(uint8_t control) {
	LOG_INFO(LOG_MDBSCL, "procCommandSessionCompleteConfirm");
	if(control == Mdb::Control_ACK) {
		LOG_INFO(LOG_MDBSCL, "Recv ACK");
		gotoStateEnabled();

		return;
	} else {
		LOG_ERROR(LOG_MDBSCL, "Wrong confirm " << control);
		state->set(State_SessionEnd);
		return;
	}
}

void MdbSlaveCashless3::stateVendApprovingCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: stateSessionIdleCommandReset(); return;
		case CommandId_SessionComplete: stateSessionIdleCommandSessionComplete(); return;
		case CommandId_VendRequest: stateVendApprovingCommandVendRequest(data); return;
		case CommandId_VendCancel: stateVendApprovingCommandVendCancel(); return;
		case CommandId_Poll: stateVendApprovingCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveCashless3::stateVendApprovingCommandVendRequest(const uint8_t *data) {
	VendRequestRequest *req = (VendRequestRequest*)data;
	LOG_INFO(LOG_MDBSCL, "stateVendApprovingCommandVendRequest " << req->itemNumber.get() << "," << req->itemPrice.get());
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveCashless3::stateVendApprovingCommandVendCancel() {
	LOG_INFO(LOG_MDBSCL, "stateVendApprovingCommandVendCancel");
	slaveSender->startData();
	slaveSender->addUint8(Status_VendDenied);
	REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
	state->set(State_SessionCancel);
	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_VendCancel);
	deliverEvent(&event);
}

void MdbSlaveCashless3::stateVendApprovingCommandPoll() {
	LOG_DEBUG(LOG_MDBSCL, "stateVendApprovingCommandPoll");
	switch(command) {
		case Command_ApproveVend: {
			LOG_INFO(LOG_MDBSCL, "ApproveVend " << productPrice << "," << requestPrice);
			command = Command_None;
			slaveSender->startData();
			slaveSender->addUint8(Status_VendApproved);
			slaveSender->addUint16(context->money2value(requestPrice));
			REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
			slaveSender->sendData();
			gotoStateVending();
			return;
		}
		case Command_DenyVend: {
			LOG_INFO(LOG_MDBSCL, "DenyVend");
			command = Command_None;
			slaveSender->startData();
			slaveSender->addUint8(Status_VendDenied);
			REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
			slaveSender->sendData();
			state->set(State_SessionIdle);
			return;
		}
		case Command_DenyVendAndClose: {
			LOG_INFO(LOG_MDBSCL, "DenyVend");
			command = Command_None;
			slaveSender->startData();
			slaveSender->addUint8(Status_VendDenied);
			REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
			slaveSender->sendData();
			state->set(State_SessionCancel);
			return;
		}
		case Command_SetCredit: {
			LOG_INFO(LOG_MDBSCL, "DenyVend");
			slaveSender->startData();
			slaveSender->addUint8(Status_VendDenied);
			REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
			slaveSender->sendData();
			state->set(State_SessionCancel);
			return;
		}
		default: {
			REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
			slaveSender->sendAnswer(Mdb::Control_ACK);
			return;
		}
	}
}

void MdbSlaveCashless3::gotoStateVending() {
	timer->start(MDB_CL_VENDING_TIMEOUT);
	state->set(State_Vending);
}

void MdbSlaveCashless3::stateVendingCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: stateVendingCommandReset(); return;
		case CommandId_SessionComplete: stateVendingCommandSessionComplete(); return;
		case CommandId_VendSuccess: stateVendingCommandVendSuccess(data); return;
		case CommandId_VendFailure: stateVendingCommandVendFailure(); return;
		case CommandId_Poll: procCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

/**
 * MDB 4.2, 7.4.7 A reset between VEND APPROVED and VEND SUCCESS shall be interpreted as a VEND SUCCESS.
 */
void MdbSlaveCashless3::stateVendingCommandReset() {
	LOG_INFO(LOG_MDBSCL, "stateVendWaitCommandReset");
	timer->stop();
	procCommandReset();
	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_VendComplete);
	deliverEvent(&event);
}

void MdbSlaveCashless3::stateVendingCommandSessionComplete() {
	LOG_INFO(LOG_MDBSCL, "stateVendingCommandSessionComplete");
	timer->stop();
	procCommandSessionComplete();
	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_VendCancel);
	deliverEvent(&event);
}

void MdbSlaveCashless3::stateVendingCommandVendSuccess(const uint8_t *data) {
	VendSuccessRequest *req = (VendSuccessRequest*)data;
	LOG_INFO(LOG_MDBSCL, "stateVendWaitCommandVendSuccess " << req->itemNumber.get());
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	timer->stop();
	state->set(State_SessionCancel);
	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_VendComplete);
	deliverEvent(&event);
}

void MdbSlaveCashless3::stateVendingCommandVendFailure() {
	LOG_INFO(LOG_MDBSCL, "stateVendWaitCommandVendFailure");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	timer->stop();
	state->set(State_SessionCancel);
	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_VendCancel);
	deliverEvent(&event);
}

void MdbSlaveCashless3::stateVendingTimeout() {
	LOG_INFO(LOG_MDBSCL, "stateVendingTimeout");
	state->set(State_SessionCancel);
	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_VendCancel);
	deliverEvent(&event);
}

void MdbSlaveCashless3::stateSessionCancelCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_ReaderDisable: procCommandReaderDisable(); return;
		case CommandId_ReaderEnable: procCommandReaderEnable(); return;
		case CommandId_SessionComplete: procCommandSessionComplete(); return;
		case CommandId_Poll: stateSessionCancelCommandPoll(); return;
		case CommandId_RevalueLimitRequest: stateSessionCancelRevalueLimitRequest(); return;
		case CommandId_VendRequest: stateSessionCancelVendRequest(); return;
		case CommandId_VendSuccess: stateSessionCancelVendSuccess(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveCashless3::stateSessionCancelCommandPoll() {
	LOG_INFO(LOG_MDBSCL, "stateSessionCancelCommandPoll");
	slaveSender->startData();
	slaveSender->addUint8(Status_SessionCancelRequest);
	REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
}

void MdbSlaveCashless3::stateSessionCancelRevalueLimitRequest() {
	LOG_INFO(LOG_MDBSCL, "stateSessionCancelRevalueLimitRequest");
	slaveSender->startData();
	slaveSender->addUint8(0x0F);
	slaveSender->addUint16(0);
	REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
}

void MdbSlaveCashless3::stateSessionCancelVendRequest() {
	LOG_INFO(LOG_MDBSCL, "stateSessionCancelVendRequest");
	slaveSender->startData();
	slaveSender->addUint8(Status_VendDenied);
	REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
}

void MdbSlaveCashless3::stateSessionCancelVendSuccess() {
	LOG_INFO(LOG_MDBSCL, "stateSessionCancelVendSuccess");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveCashless3::stateSessionEndCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_ReaderDisable: procCommandReaderDisable(); return;
		case CommandId_ReaderEnable: procCommandReaderEnable(); return;
		case CommandId_SessionComplete: procCommandSessionComplete(); return;
		case CommandId_Poll: stateSessionEndCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveCashless3::stateSessionEndCommandPoll() {
	LOG_INFO(LOG_MDBSCL, "stateSessionEndCommandPoll");
	pollData.addUint8(Status_EndSession);
	REMOTE_LOG_OUT(RLOG_MDBSCL, pollData.getData(), pollData.getLen());
	slaveSender->sendData(pollData.getData(), pollData.getLen());
	slaveReceiver->recvConfirm();
	gotoStateEnabled();
	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_SessionClosed);
	deliverEvent(&event);
}

void MdbSlaveCashless3::stateReaderCancelCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_ReaderDisable: procCommandReaderDisable(); return;
		case CommandId_ReaderEnable: procCommandReaderEnable(); return;
		case CommandId_Poll: stateReaderCancelCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveCashless3::stateReaderCancelCommandPoll() {
	LOG_INFO(LOG_MDBSCL, "stateReaderCancelCommandPoll");
	pollData.addUint8(Status_Cancelled);
	REMOTE_LOG_OUT(RLOG_MDBSCL, pollData.getData(), pollData.getLen());
	slaveSender->sendData(pollData.getData(), pollData.getLen());
	slaveReceiver->recvConfirm();
	gotoStateEnabled();
}

void MdbSlaveCashless3::procUnwaitedPacket(uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	LOG_ERROR(LOG_MDBSCL, "procUnwaitedPacket " << state->get() << "," << commandId);
#ifndef DEBUG_PROTOCOL
	(void)data;
	(void)dataLen;
#else
	Mdb::EventError event(deviceId, MdbSlaveCashlessInterface::Event_Error);
	event.code = ConfigEvent::Type_CoinUnwaitedPacket;
	event.data.clear();
	event.data << "scl" << state->get() << "*" << getType() << "*";
	event.data.addHex(commandId >> 8);
	event.data.addHex(commandId);
	event.data << "*";
	for(uint16_t i = 0; i < dataLen; i++) {
		event.data.addHex(data[i]);
	}
	deliverEvent(&event);
#endif
}
