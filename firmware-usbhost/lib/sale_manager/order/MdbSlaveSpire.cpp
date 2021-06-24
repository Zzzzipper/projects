#include "MdbSlaveSpire.h"

#include "mdb/MdbProtocolCashless.h"
#include "mdb/slave/MdbSlavePacketReceiver.h"
#include "mdb/master/cashless/MdbMasterCashless.h"
#include "logger/RemoteLogger.h"
#include "logger/include/Logger.h"

#include <string.h>

using namespace Mdb::Cashless;

MdbSlaveSpire::MdbSlaveSpire(
	Mdb::Device deviceType,
	uint8_t maxLevel,
	Mdb::DeviceContext *context,
	TimerEngine *timerEngine,
	EventEngineInterface *eventEngine,
	StatStorage *stat
) :
	MdbSlave(deviceType, eventEngine),
	deviceId(eventEngine),
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
		{ Command_Vend, Subcommand_OrderRequest, sizeof(OrderRequestRequest), sizeof(OrderRequestRequest) },
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
	timer = timerEngine->addTimer<MdbSlaveSpire, &MdbSlaveSpire::procTimer>(this);
}

MdbSlaveSpire::~MdbSlaveSpire() {
	timerEngine->deleteTimer(timer);
}

EventDeviceId MdbSlaveSpire::getDeviceId() {
	return deviceId;
}

void MdbSlaveSpire::setOrder(Order *order) {
	this->order = order;
}

void MdbSlaveSpire::reset() {
	ATOMIC {
		LOG_ERROR(LOG_MDBSCL, "reset");
		level = Mdb::FeatureLevel_1;
		command = Command_None;
		enabled = false;
		session = false;
		packetLayer->setMode(Mdb::Mode_Normal);
		pollData.clear();
		pollData.addUint8(Status_JustReset);
		state->set(State_Disabled);
	}
}

void MdbSlaveSpire::disable() {
	ATOMIC {
		LOG_INFO(LOG_MDBSCL, "enable");
		session = false;
	}
}

void MdbSlaveSpire::enable() {
	ATOMIC {
		LOG_INFO(LOG_MDBSCL, "enable");
		session = true;
		credit = 100; //todo:
	}
}

void MdbSlaveSpire::approveVend() {
	ATOMIC {
		LOG_INFO(LOG_MDBSCL, "approveVend " << order->getQuantity());
		orderCid = order->getFirstCid();
		if(orderCid == OrderCell::CidUndefined) {
			LOG_ERROR(LOG_MDBSCL, "Fatal error");
			this->command = Command_DenyVend;
			return;
		}
		this->command = Command_ApproveVend;
	}
}

void MdbSlaveSpire::requestPinCode() {
	ATOMIC {
		LOG_INFO(LOG_MDBSCL, "requestPinCode");
		this->command = Command_PinCode;
	}
}

void MdbSlaveSpire::denyVend() {
	ATOMIC {
		LOG_INFO(LOG_MDBSCL, "denyVend");
		this->command = Command_DenyVendAndClose;
	}
}

void MdbSlaveSpire::initSlave(MdbSlave::Sender *sender, MdbSlave::Receiver *receiver) {
	slaveReceiver = receiver;
	slaveSender = sender;
	packetLayer->init(receiver);
}

void MdbSlaveSpire::recvCommand(const uint8_t command) {
	packetLayer->recvCommand(command);
}

void MdbSlaveSpire::recvSubcommand(const uint8_t subcommand) {
	packetLayer->recvSubcommand(subcommand);
}

void MdbSlaveSpire::recvRequest(const uint8_t *data, uint16_t len) {
	packetLayer->recvRequest(data, len);
}

void MdbSlaveSpire::recvRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
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
		case CommandId_RevalueLimitRequest: procCommandRevalueLimitRequest(); return;
	}

	switch(state->get()) {
		case State_Disabled: stateDisabledCommand(commandId, data, dataLen); return;
		case State_Enabled: stateEnabledCommand(commandId, data, dataLen); return;
		case State_SessionIdle: stateSessionIdleCommand(commandId, data, dataLen); return;
		case State_PinCode: statePinCodeCommand(commandId, data, dataLen); return;
		case State_VendApproving: stateVendApprovingCommand(commandId, data, dataLen); return;
		case State_Vending: stateVendingCommand(commandId, data, dataLen); return;
		case State_NextRequest: stateNextRequestCommand(commandId, data, dataLen); return;
		case State_SessionCancel: stateSessionCancelCommand(commandId, data, dataLen); return;
		case State_SessionEnd: stateSessionEndCommand(commandId, data, dataLen); return;
		case State_ReaderCancel: stateReaderCancelCommand(commandId, data, dataLen); return;
		default: LOG_ERROR(LOG_MDBSCL, "Unsupported state " << state->get());
	}
}

void MdbSlaveSpire::recvUnsupportedPacket(const uint16_t commandId) {
#ifndef DEBUG_PROTOCOL
	(void)commandId;
#else
	procUnwaitedPacket(commandId, NULL, 0);
#endif
}

void MdbSlaveSpire::recvConfirm(uint8_t control) {
	LOG_DEBUG(LOG_MDBSCL, "recvConfirm " << control);
	REMOTE_LOG_IN(RLOG_MDBSCL, control);
	switch (commandId) {
		case Mdb::Cashless::CommandId_Poll: procCommandPollConfirm(control); return;
		case Mdb::Cashless::CommandId_SessionComplete: procCommandSessionCompleteConfirm(control); return;
		default: LOG_DEBUG(LOG_MDBSCL, "Unwaited confirm state=" << state->get() << ", commandId=" << commandId << ", control=" << control);
	}
}

void MdbSlaveSpire::procTimer() {
	LOG_ERROR(LOG_MDBMCL, "procTimer " << state->get());
	switch(state->get()) {
	case State_Vending: stateVendingTimeout(); return;
	default: LOG_ERROR(LOG_SM, "Unwaited timeout " << state->get());
	}
}

void MdbSlaveSpire::procCommandSetupConfig(const uint8_t *data) {
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
	resp.options = 0;

	LOG_INFO_HEX(LOG_MDBSCL, (const uint8_t*)&resp, sizeof(resp));
	REMOTE_LOG_OUT(RLOG_MDBSCL, (const uint8_t*)&resp, sizeof(resp));
	slaveSender->sendData((const uint8_t*)&resp, sizeof(resp));
}

void MdbSlaveSpire::procCommandSetupPricesL1(const uint8_t *data) {
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

void MdbSlaveSpire::procCommandExpansionRequestId(const uint8_t *data) {
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
		resp.featureBits.set(FeatureOption_Order);

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

void MdbSlaveSpire::procCommandExpansionEnableOptions(const uint8_t *data) {
	LOG_INFO(LOG_MDBSCL, "procCommandExpansionEnableOptions");
	ExpansionEnableOptionsRequest *req = (ExpansionEnableOptionsRequest*)data;
	LOG_INFO(LOG_MDBMBV, "softwareVersion " << req->featureBits.get());

	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveSpire::procCommandExpansionDiagnostics() {
	LOG_INFO(LOG_MDBSCL, "procCommandExpansionDiagnostics");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveSpire::procCommandVendCashSale(const uint8_t *data) {
	VendRequestRequest *req = (VendRequestRequest*)data;
	uint16_t itemNumber = req->itemNumber.get() & CashSaleNumber_Mask;
	uint32_t itemPrice = context->value2money(req->itemPrice.get());
	LOG_INFO(LOG_MDBSCL, "procCommandVendCashSale " << req->itemNumber.get() << "/" << itemNumber << "," << req->itemPrice.get() << "/" << itemPrice);

	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveSpire::procCommandReset() {
	LOG_INFO(LOG_MDBSCL, "procCommandReset");
	pollData.clear();
	pollData.addUint8(Status_JustReset);

	level = Mdb::FeatureLevel_1;
	enabled = false;
	packetLayer->setMode(Mdb::Mode_Normal);

	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	state->set(State_Disabled);
//	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_Reset);
//	deliverEvent(&event);
}

void MdbSlaveSpire::procCommandPoll() {
	LOG_DEBUG(LOG_MDBSCL, "procCommandPoll " << state->get());
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

void MdbSlaveSpire::procCommandPollConfirm(uint8_t control) {
	LOG_DEBUG(LOG_MDBSCL, "procCommandPollConfirm " << control);
	if(control != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBSCL, "Wrong confirm " << control);
		return;
	}
	pollData.clear();
}

void MdbSlaveSpire::procCommandRevalueLimitRequest() {
	LOG_INFO(LOG_MDBSCL, "procCommandRevalueLimitRequest");
	slaveSender->startData();
	slaveSender->addUint8(0x0F);
	slaveSender->addUint16(0);
	REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
}

void MdbSlaveSpire::stateDisabledCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_ReaderEnable: stateDisabledReaderEnable(); return;
		case CommandId_ReaderDisable: procCommandReaderDisable(); return;
		case CommandId_VendRequest: stateSessionIdleCommandOrderRequest(data); return;
		case CommandId_Poll: stateDisabledCommandPoll(); return;
		case CommandId_ReaderCancel: procCommandReaderCancel(); return;
		case CommandId_SessionComplete: procCommandSessionComplete(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveSpire::stateDisabledReaderEnable() {
	LOG_INFO(LOG_MDBSCL, "stateDisabledReaderEnable");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	enabled = true;
	gotoStateEnabled();
//	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_Enable);
//	deliverEvent(&event);
}

void MdbSlaveSpire::stateDisabledCommandPoll() {
	LOG_DEBUG(LOG_MDBSCL, "stateDisabledCommandPoll");
	procCommandPoll();
}

void MdbSlaveSpire::procCommandReaderEnable() {
	LOG_INFO(LOG_MDBSCL, "procCommandReaderEnable");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	enabled = true;
}

void MdbSlaveSpire::procCommandReaderDisable() {
	LOG_INFO(LOG_MDBSCL, "procCommandReaderDisable");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	enabled = false;
}

void MdbSlaveSpire::procCommandReaderCancel() {
	LOG_INFO(LOG_MDBSCL, "procCommandReaderCancel");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	state->set(State_ReaderCancel);
}

void MdbSlaveSpire::gotoStateEnabled() {
	LOG_DEBUG(LOG_MDBSCL, "gotoStateEnabled");
	if(enabled == true) {
		state->set(State_Enabled);
	} else {
		state->set(State_Disabled);
	}
}

void MdbSlaveSpire::stateEnabledCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_ReaderDisable: stateEnabledCommandReaderDisable(); return;
		case CommandId_ReaderEnable: procCommandReaderEnable(); return;
		case CommandId_OrderRequest: stateSessionIdleCommandOrderRequest(data); return;
		case CommandId_Poll: stateEnabledCommandPoll(); return;
		case CommandId_ReaderCancel: procCommandReaderCancel(); return;
		case CommandId_SessionComplete: procCommandSessionComplete(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveSpire::stateEnabledCommandReaderDisable() {
	LOG_INFO(LOG_MDBSCL, "stateEnabledCommandReaderDisable");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	enabled = false;
	state->set(State_Disabled);
}

void MdbSlaveSpire::stateEnabledCommandPoll() {
	LOG_DEBUG(LOG_MDBSCL, "stateEnabledCommandPoll");
	if(session == true) {
		procCommandPollSessionBegin();
		return;
	} else {
		procCommandPoll();
		return;
	}
}

void MdbSlaveSpire::procCommandPollSessionBegin() {
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

void MdbSlaveSpire::stateSessionIdleCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: stateSessionIdleCommandReset(); return;
		case CommandId_ReaderDisable: procCommandReaderDisable(); return;
		case CommandId_ReaderEnable: procCommandReaderEnable(); return;
		case CommandId_SessionComplete: stateSessionIdleCommandSessionComplete(); return;
		case CommandId_OrderRequest: stateSessionIdleCommandOrderRequest(data); return;
		case CommandId_Poll: stateSessionIdleCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveSpire::stateSessionIdleCommandReset() {
	LOG_INFO(LOG_MDBSCL, "stateSessionIdleCommandReset");
	procCommandReset();
}

void MdbSlaveSpire::stateSessionIdleCommandSessionComplete() {
	LOG_INFO(LOG_MDBSCL, "stateSessionIdleCommandSessionComplete");
	procCommandSessionComplete();
}

void MdbSlaveSpire::stateSessionIdleCommandOrderRequest(const uint8_t *data) {
	OrderRequestRequest *req = (OrderRequestRequest*)data;
	LOG_INFO(LOG_MDBSCL, "stateSessionIdleCommandOrderRequest " << req->pincodeLen);
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	command = Command_None;
	state->set(State_VendApproving);
	EventUint16Interface event(deviceId, OrderDeviceInterface::Event_VendRequest, 0);
	deliverEvent(&event);
}

void MdbSlaveSpire::stateSessionIdleCommandPoll() {
	LOG_DEBUG(LOG_MDBSCL, "stateSessionIdleCommandPoll");
	procCommandPoll();
	if(command == Command_SetCredit) {
		command = Command_None;
		state->set(State_SessionCancel);
	}
}

void MdbSlaveSpire::procCommandSessionComplete() {
	LOG_INFO(LOG_MDBSCL, "procCommandSessionComplete");
	slaveSender->startData();
	slaveSender->addUint8(Mdb::Cashless::Status_EndSession);
	REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
	slaveReceiver->recvConfirm();
	state->set(State_SessionEnd);
}

void MdbSlaveSpire::procCommandSessionCompleteConfirm(uint8_t control) {
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

void MdbSlaveSpire::gotoStatePinCode() {
	LOG_INFO(LOG_MDBSCL, "gotoStatePinCode");
	state->set(State_PinCode);
}

void MdbSlaveSpire::statePinCodeCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: statePinCodeCommandReset(); return;
		case CommandId_ReaderDisable: procCommandReaderDisable(); return;
		case CommandId_ReaderEnable: procCommandReaderEnable(); return;
		case CommandId_SessionComplete: statePinCodeCommandSessionComplete(); return;
		case CommandId_OrderRequest: statePinCodeCommandOrderRequest(data); return;
		case CommandId_Poll: statePinCodeCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveSpire::statePinCodeCommandReset() {
	LOG_INFO(LOG_MDBSCL, "statePinCodeCommandReset");
	procCommandReset();
	EventInterface event(deviceId, OrderDeviceInterface::Event_PinCodeCancelled);
	deliverEvent(&event);
}

void MdbSlaveSpire::statePinCodeCommandSessionComplete() {
	LOG_INFO(LOG_MDBSCL, "statePinCodeCommandSessionComplete");
	procCommandSessionComplete();
	EventInterface event(deviceId, OrderDeviceInterface::Event_PinCodeCancelled);
	deliverEvent(&event);
}

void MdbSlaveSpire::statePinCodeCommandOrderRequest(const uint8_t *data) {
	OrderRequestRequest *req = (OrderRequestRequest*)data;
	LOG_INFO(LOG_MDBSCL, "statePinCodeCommandOrderRequest " << req->pincodeLen);
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	state->set(State_VendApproving);
	OrderDeviceInterface::EventPinCodeCompleted event(deviceId, (const char*)req->pincode);
	deliverEvent(&event);
}

void MdbSlaveSpire::statePinCodeCommandPoll() {
	LOG_DEBUG(LOG_MDBSCL, "statePinCodeCommandPoll");
	procCommandPoll();
	if(command == Command_SetCredit) {
		command = Command_None;
		state->set(State_SessionCancel);
		EventInterface event(deviceId, OrderDeviceInterface::Event_PinCodeCancelled);
		deliverEvent(&event);
		return;
	}
}

void MdbSlaveSpire::stateVendApprovingCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: stateSessionIdleCommandReset(); return;
		case CommandId_SessionComplete: stateSessionIdleCommandSessionComplete(); return;
		case CommandId_OrderRequest: stateVendApprovingCommandOrderRequest(data); return;
		case CommandId_VendCancel: stateVendApprovingCommandVendCancel(); return;
		case CommandId_Poll: stateVendApprovingCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveSpire::stateVendApprovingCommandOrderRequest(const uint8_t *data) {
	OrderRequestRequest *req = (OrderRequestRequest*)data;
	LOG_INFO(LOG_MDBSCL, "stateVendApprovingCommandOrderRequest " << req->pincodeLen);
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveSpire::stateVendApprovingCommandVendCancel() {
	LOG_INFO(LOG_MDBSCL, "stateVendApprovingCommandVendCancel");
	slaveSender->startData();
	slaveSender->addUint8(Status_VendDenied);
	REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
	state->set(State_SessionCancel);
	EventInterface event(deviceId, OrderDeviceInterface::Event_VendCancelled);
	deliverEvent(&event);
}

void MdbSlaveSpire::stateVendApprovingCommandPoll() {
	LOG_DEBUG(LOG_MDBSCL, "stateVendApprovingCommandPoll");
	switch(command) {
		case Command_ApproveVend: {
			LOG_INFO(LOG_MDBSCL, "ApproveVend");
			command = Command_None;
			slaveSender->startData();
			slaveSender->addUint8(States_OrderApproved);
			slaveSender->addUint16(orderCid);
			slaveSender->addUint8(order->getQuantity() - 1);
			REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
			slaveSender->sendData();
			gotoStateVending();
			return;
		}
		case Command_PinCode: {
			LOG_INFO(LOG_MDBSCL, "PinCode");
			command = Command_None;
			slaveSender->startData();
			slaveSender->addUint8(Status_VendDenied);
			slaveSender->addUint8(Status_Error);
			slaveSender->addUint8(Status_Manufacturer1_FaceIdFailed);
			REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
			slaveSender->sendData();
			gotoStatePinCode();
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
			LOG_INFO(LOG_MDBSCL, "SetCredit");
			command = Command_None;
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

void MdbSlaveSpire::gotoStateVending() {
	timer->start(MDB_CL_VENDING_TIMEOUT);
	state->set(State_Vending);
}

void MdbSlaveSpire::stateVendingCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
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
void MdbSlaveSpire::stateVendingCommandReset() {
	LOG_INFO(LOG_MDBSCL, "stateVendWaitCommandReset");
	timer->stop();
	procCommandReset();
	EventInterface event(deviceId, OrderDeviceInterface::Event_VendCompleted);
	deliverEvent(&event);
}

void MdbSlaveSpire::stateVendingCommandSessionComplete() {
	LOG_INFO(LOG_MDBSCL, "stateVendingCommandSessionComplete");
	timer->stop();
	procCommandSessionComplete();
	EventInterface event(deviceId, OrderDeviceInterface::Event_VendCancelled);
	deliverEvent(&event);
}

void MdbSlaveSpire::stateVendingCommandVendSuccess(const uint8_t *data) {
	VendSuccessRequest *req = (VendSuccessRequest*)data;
	LOG_INFO(LOG_MDBSCL, "stateVendingCommandVendSuccess " << req->itemNumber.get());
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	timer->stop();
	state->set(State_NextRequest);
	EventUint16Interface event(deviceId, OrderDeviceInterface::Event_VendCompleted, orderCid);
	deliverEvent(&event);
}

void MdbSlaveSpire::stateVendingCommandVendFailure() {
	LOG_INFO(LOG_MDBSCL, "stateVendingCommandVendFailure");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	timer->stop();
	state->set(State_NextRequest);
	EventUint16Interface event(deviceId, OrderDeviceInterface::Event_VendSkipped, orderCid);
	deliverEvent(&event);
}

void MdbSlaveSpire::stateVendingTimeout() {
	LOG_INFO(LOG_MDBSCL, "stateVendingTimeout");
	state->set(State_SessionCancel);
	EventInterface event(deviceId, OrderDeviceInterface::Event_VendCancelled);
	deliverEvent(&event);
}

void MdbSlaveSpire::stateNextRequestCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_ReaderDisable: procCommandReaderDisable(); return;
		case CommandId_ReaderEnable: procCommandReaderEnable(); return;
		case CommandId_SessionComplete: stateNextRequestCommandSessionComplete(); return;
		case CommandId_OrderRequest: stateNextRequestCommandOrderRequest(data); return;
		case CommandId_Poll: procCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveSpire::stateNextRequestCommandSessionComplete() {
	LOG_INFO(LOG_MDBSCL, "stateNextRequestCommandSessionComplete");
	procCommandSessionComplete();
	EventInterface event(deviceId, OrderDeviceInterface::Event_VendCancelled);
	deliverEvent(&event);
}

void MdbSlaveSpire::stateNextRequestCommandOrderRequest(const uint8_t *data) {
	OrderRequestRequest *req = (OrderRequestRequest*)data;
	LOG_INFO(LOG_MDBSCL, "stateNextRequestCommandOrderRequest " << req->pincodeLen);
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
	state->set(State_VendApproving);
}

void MdbSlaveSpire::stateSessionCancelCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_ReaderDisable: procCommandReaderDisable(); return;
		case CommandId_ReaderEnable: procCommandReaderEnable(); return;
		case CommandId_SessionComplete: procCommandSessionComplete(); return;
		case CommandId_Poll: stateSessionCancelCommandPoll(); return;
		case CommandId_VendRequest: stateSessionCancelVendRequest(); return;
		case CommandId_VendSuccess: stateSessionCancelVendSuccess(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveSpire::stateSessionCancelCommandPoll() {
	LOG_INFO(LOG_MDBSCL, "stateSessionCancelCommandPoll");
	slaveSender->startData();
	slaveSender->addUint8(Status_SessionCancelRequest);
	REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
}

void MdbSlaveSpire::stateSessionCancelVendRequest() {
	LOG_INFO(LOG_MDBSCL, "stateSessionCancelVendRequest");
	slaveSender->startData();
	slaveSender->addUint8(Status_VendDenied);
	REMOTE_LOG_OUT(RLOG_MDBSCL, slaveSender->getData(), slaveSender->getDataLen());
	slaveSender->sendData();
}

void MdbSlaveSpire::stateSessionCancelVendSuccess() {
	LOG_INFO(LOG_MDBSCL, "stateSessionCancelVendSuccess");
	REMOTE_LOG_OUT(RLOG_MDBSCL, Mdb::Control_ACK);
	slaveSender->sendAnswer(Mdb::Control_ACK);
}

void MdbSlaveSpire::stateSessionEndCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_ReaderDisable: procCommandReaderDisable(); return;
		case CommandId_ReaderEnable: procCommandReaderEnable(); return;
		case CommandId_SessionComplete: procCommandSessionComplete(); return;
		case CommandId_Poll: stateSessionEndCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveSpire::stateSessionEndCommandPoll() {
	LOG_INFO(LOG_MDBSCL, "stateSessionEndCommandPoll");
	pollData.addUint8(Status_EndSession);
	REMOTE_LOG_OUT(RLOG_MDBSCL, pollData.getData(), pollData.getLen());
	slaveSender->sendData(pollData.getData(), pollData.getLen());
	slaveReceiver->recvConfirm();
	gotoStateEnabled();
}

void MdbSlaveSpire::stateReaderCancelCommand(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	switch(commandId) {
		case CommandId_Reset: procCommandReset(); return;
		case CommandId_ReaderDisable: procCommandReaderDisable(); return;
		case CommandId_ReaderEnable: procCommandReaderEnable(); return;
		case CommandId_Poll: stateReaderCancelCommandPoll(); return;
		default: procUnwaitedPacket(commandId, data, dataLen); return;
	}
}

void MdbSlaveSpire::stateReaderCancelCommandPoll() {
	LOG_INFO(LOG_MDBSCL, "stateReaderCancelCommandPoll");
	pollData.addUint8(Status_Cancelled);
	REMOTE_LOG_OUT(RLOG_MDBSCL, pollData.getData(), pollData.getLen());
	slaveSender->sendData(pollData.getData(), pollData.getLen());
	slaveReceiver->recvConfirm();
	gotoStateEnabled();
}

void MdbSlaveSpire::procUnwaitedPacket(uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
	LOG_ERROR(LOG_MDBSCL, "procUnwaitedPacket " << state->get() << "," << commandId);
	(void)data;
	(void)dataLen;
}
