#include "MdbSnifferComGateway.h"

#include "common/mdb/MdbProtocolComGateway.h"
#include "common/logger/include/Logger.h"

using namespace Mdb::ComGateway;

MdbSnifferComGateway::MdbSnifferComGateway(Mdb::Device deviceType, Mdb::DeviceContext *context, EventEngineInterface *eventEngine) :
	MdbSniffer(deviceType, eventEngine),
	deviceId(eventEngine),
	context(context),
	state(State_Idle),
	enabled(false),
	level(0)
{
#if 0
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
	packetLayer = new MdbSlavePacketReceiver(getType(), this, packets, sizeof(packets)/sizeof(packets[0]));
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
#endif
}

MdbSnifferComGateway::~MdbSnifferComGateway() {
#if 0
	delete packetLayer;
#endif
}

void MdbSnifferComGateway::reset() {
#if 0
	LOG_DEBUG(LOG_MDBSCLS, "reset");
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	enabled = false;
	state = State_Sale;
#endif
}

bool MdbSnifferComGateway::isEnable() {
	return enabled;
}

void MdbSnifferComGateway::initSlave(MdbSlave::Sender *, MdbSlave::Receiver *receiver) {
#if 0
	LOG_DEBUG(LOG_MDBSCLS, "initSlave");
	packetLayer->init(receiver);
#endif
}

void MdbSnifferComGateway::recvCommand(const uint8_t command) {
#if 0
	packetLayer->recvCommand(command);
#endif
}

void MdbSnifferComGateway::recvSubcommand(const uint8_t subcommand) {
#if 0
	packetLayer->recvSubcommand(subcommand);
#endif
}

void MdbSnifferComGateway::recvRequest(const uint8_t *data, uint16_t len) {
#if 0
	LOG_DEBUG(LOG_MDBSCLS, "recvRequest");
	LOG_DEBUG_HEX(LOG_MDBSCLS, data, len);
	packetLayer->recvRequest(data, len);
#endif
}

void MdbSnifferComGateway::recvRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t dataLen) {
#if 0
	(void)dataLen;
	LOG_DEBUG(LOG_MDBSCLS, "recvRequestPacket " << state << "," << commandId);
	switch(state) {
		case State_Idle: return;
		default: stateSaleCommand(commandId, data);
	}
#endif
}

void MdbSnifferComGateway::recvConfirm(uint8_t control) {
#if 0
	LOG_DEBUG(LOG_MDBSCLS, "recvConfirm " << state << "," << control);
#endif
}

void MdbSnifferComGateway::procResponse(const uint8_t *data, uint16_t len, bool crc) {
#if 0
	LOG_DEBUG(LOG_MDBSCLS, "procResponse " << state);
	LOG_DEBUG_HEX(LOG_MDBSCLS, data, len);
	switch(state) {
		case State_Idle: return;
		case State_Sale: return;
		case State_SetupConfig: stateSetupConfigResponse(data, len, crc); return;
		case State_ExpansionIdentification: stateExpansionIdentificationResponse(data, len, crc); return;
		case State_VendRequest: stateVendRequestResponse(data, len, crc); return;
		case State_VendSuccess: stateVendSuccessResponse(data, len, crc); return;
		case State_Poll: statePollResponse(data, len, crc); return;
		case State_NotPoll: stateNotPollResponse(data, len, crc); return;
		default: LOG_ERROR(LOG_MDBSCL, "Unwaited packet " << state << "," << crc);
	}
#endif
}

#if 0
void MdbSnifferComGateway::stateSaleCommand(const uint16_t commandId, const uint8_t *data) {
	LOG_DEBUG(LOG_MDBSCLS, "stateSaleCommand");
	switch(commandId) {
		case CommandId_Reset: return;
		case CommandId_Poll: stateSaleCommandPoll(); return;
		case CommandId_SetupConfig: stateSaleCommandSetupConfig(); return;
		case CommandId_ExpansionRequestId: stateSaleCommandExpansionIdentification(); return;
		case CommandId_SetupPricesL1: return;
		case CommandId_ReaderDisable: stateSaleCommandReaderDisable(); return;
		case CommandId_ReaderEnable: stateSaleCommandReaderEnable(); return;
		case CommandId_VendRequest: stateSaleCommandVendRequest(data); return;
		case CommandId_VendSuccess: stateSaleCommandVendSuccess(data); return;
		case CommandId_SessionComplete: return;
		default: {
			LOG_ERROR(LOG_MDBSCL, "Unwaited packet " << state << "," << commandId);
			state = State_NotPoll;
		}
	}
}

void MdbSnifferComGateway::stateSaleCommandPoll() {
	state = State_Poll;
}

void MdbSnifferComGateway::stateSaleCommandSetupConfig() {
	LOG_INFO(LOG_MDBSCL, "stateSaleCommandSetupConfig");
	state = State_SetupConfig;
}

void MdbSnifferComGateway::stateSaleCommandExpansionIdentification() {
	LOG_INFO(LOG_MDBSCL, "stateSaleExpansionIdentification");
	state = State_ExpansionIdentification;
}

void MdbSnifferComGateway::stateSaleCommandVendRequest(const uint8_t *data) {
	VendRequestRequest *req = (VendRequestRequest*)data;
	cashlessId = req->itemNumber.get();
	price = context->value2money(req->itemPrice.get());
	LOG_INFO(LOG_MDBSCL, "stateSaleCommandVendRequest " << cashlessId << "," << price << "(" << req->itemNumber.get() << "," << req->itemPrice.get() << ")");
	state = State_VendRequest;
}

void MdbSnifferComGateway::stateSaleCommandVendSuccess(const uint8_t *data) {
	VendSuccessRequest *req = (VendSuccessRequest*)data;
	LOG_INFO(LOG_MDBSCL, "stateSaleCommandVendSuccess " << req->itemNumber.get());
	state = State_VendSuccess;
}

void MdbSnifferComGateway::stateSetupConfigResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_INFO(LOG_MDBSCLS, "stateSetupConfigResponse " << crc);
	if(crc == false) {
		LOG_INFO(LOG_MDBSCLS, "Response in poll");
		state = State_Sale;
		return;
	}
	uint16_t expLen = sizeof(SetupConfigResponse);
	if(len < expLen) {
		LOG_ERROR(LOG_MDBSCLS, "Wrong repsonse size");
		state = State_Sale;
		return;
	}

	SetupConfigResponse *pkt = (SetupConfigResponse*)data;
	context->init(pkt->decimalPlaces, pkt->scaleFactor);
	context->setCurrency(pkt->currency.get());
	level = pkt->featureLevel;

	LOG_INFO(LOG_MDBSCLS, "featureLevel " << pkt->featureLevel);
	LOG_INFO(LOG_MDBSCLS, "currency " << pkt->currency.get());
	LOG_INFO(LOG_MDBSCLS, "scaleFactor " << pkt->scaleFactor);
	LOG_INFO(LOG_MDBSCLS, "decimalPlaces " << pkt->decimalPlaces);
	LOG_INFO(LOG_MDBSCLS, "maxRespTime " << pkt->maxRespTime);
	LOG_INFO(LOG_MDBSCLS, "options " << pkt->options);
	context->setStatus(Mdb::DeviceContext::Status_Init);
	state = State_Sale;
}

void MdbSnifferComGateway::stateExpansionIdentificationResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_INFO(LOG_MDBSCLS, "stateExpansionIdentificationResponse " << crc);
	if(crc == false) {
		LOG_INFO(LOG_MDBSCLS, "Response in poll");
		state = State_Sale;
		return;
	}
	uint16_t expLen = sizeof(ExpansionRequestIdResponseL1);
	if(len < expLen) {
		LOG_ERROR(LOG_MDBSCLS, "Wrong repsonse size " << len << "<" << expLen);
		state = State_Sale;
		return;
	}

	ExpansionRequestIdResponseL1 *pkt = (ExpansionRequestIdResponseL1*)data;
	LOG_INFO(LOG_MDBMCL, "perepheralId " << pkt->perepheralId);
	LOG_INFO_HEX(LOG_MDBMCL, pkt->manufacturerCode, sizeof(pkt->manufacturerCode));
	LOG_INFO_HEX(LOG_MDBMCL, pkt->serialNumber, sizeof(pkt->serialNumber));
	LOG_INFO_HEX(LOG_MDBMCL, pkt->modelNumber, sizeof(pkt->modelNumber));
	LOG_INFO(LOG_MDBMBV, "softwareVersion " << pkt->softwareVersion.get());

	context->setManufacturer(pkt->manufacturerCode, sizeof(pkt->manufacturerCode));
	context->setModel(pkt->modelNumber, sizeof(pkt->modelNumber));
	context->setSerialNumber(pkt->serialNumber, sizeof(pkt->serialNumber));
	context->setSoftwareVersion(pkt->softwareVersion.get());
	state = State_Sale;
}

void MdbSnifferComGateway::stateSaleCommandReaderDisable() {
	LOG_INFO(LOG_MDBSCLS, "stateSaleCommandReaderDisable");
	context->setStatus(Mdb::DeviceContext::Status_Disabled);
	if(enabled == true) {
		enabled = false;
		EventInterface event(deviceId, Event_Disable);
		deliverEvent(&event);
	}
}

void MdbSnifferComGateway::stateSaleCommandReaderEnable() {
	LOG_INFO(LOG_MDBSCLS, "stateSaleCommandReaderDisable");
	context->setStatus(Mdb::DeviceContext::Status_Enabled);
	if(enabled == false) {
		enabled = true;
		EventInterface event(deviceId, Event_Enable);
		deliverEvent(&event);
	}
}

void MdbSnifferComGateway::stateVendRequestResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_INFO(LOG_MDBSCLS, "stateVendRequestResponse " << crc);
	state = State_Sale;
}

void MdbSnifferComGateway::stateVendSuccessResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_INFO(LOG_MDBSCLS, "stateVendSuccessResponse " << crc << "," << cashlessId << "," << price);
	if(crc == true || data[0] != Mdb::Control_ACK) {
		LOG_INFO(LOG_MDBSCLS, "Unwaited response");
		state = State_Sale;
		return;
	}
	state = State_Sale;
	MdbSnifferComGateway::EventVend vend(deviceId, cashlessId, price);
	deliverEvent(&vend);
}

void MdbSnifferComGateway::statePollResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_DEBUG(LOG_MDBSCLS, "statePollResponse " << crc);
	if(crc == false) {
		state = State_Sale;
		return;
	}
	LOG_INFO(LOG_MDBSCLS, "stateSaleCommandPoll");
	LOG_INFO_HEX(LOG_MDBSCLS, data, len);
	switch(data[0]) {
		case Status_ReaderConfigData: stateSetupConfigResponse(data, len, crc);	return;
		case Status_PeripheralID: stateExpansionIdentificationResponse(data, len, crc);	return;
	}
	state = State_Sale;
}

void MdbSnifferComGateway::stateNotPollResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_INFO(LOG_MDBSCLS, "stateNotPollResponse " << crc);
	LOG_INFO_HEX(LOG_MDBSCLS, data, len);
	state = State_Sale;
}
#endif
