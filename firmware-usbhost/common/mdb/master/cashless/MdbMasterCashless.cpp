#include "MdbMasterCashless.h"

#include "mdb/MdbProtocolCashless.h"
#include "logger/RemoteLogger.h"
#include "logger/include/Logger.h"

#include <string.h>

using namespace Mdb::Cashless;

MdbMasterCashless::MdbMasterCashless(Mdb::Device type, Mdb::DeviceContext *context, TimerEngine *timerEngine, EventEngineInterface *eventEngine) :
	MdbMaster(type, eventEngine),
	context(context),
	timerEngine(timerEngine),
	deviceId(eventEngine),
	refundAble(false),
	enabling(false),
	enabled(false)
{
	context->setState(State_Idle);
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	timer = timerEngine->addTimer<MdbMasterCashless, &MdbMasterCashless::procTimer>(this, TimerEngine::ProcInTick);
}

MdbMasterCashless::~MdbMasterCashless() {
	timerEngine->deleteTimer(timer);
}

EventDeviceId MdbMasterCashless::getDeviceId() {
	return deviceId;
}

void MdbMasterCashless::reset() {
	ATOMIC {
		LOG_ERROR(LOG_MDBMCL, "reset");
		gotoStateReset();
	}
}

bool MdbMasterCashless::isRefundAble() {
	return refundAble;
}

void MdbMasterCashless::disable() {
	ATOMIC {
		LOG_WARN(LOG_MDBMCL, "Disable " << context->getState());
		enabling = false;
	}
}

void MdbMasterCashless::enable() {
	ATOMIC {
		LOG_WARN(LOG_MDBMCL, "Enable " << context->getState());
		enabling = true;
	}
}

bool MdbMasterCashless::revalue(uint32_t credit) {
	bool result = false;
	ATOMIC {
		LOG_WARN(LOG_MDBMCL, "revalue " << credit);
		if(context->getState() != State_Session) {
			LOG_ERROR(LOG_MDBMCL, "Wrong state " << context->getState());
			continue;
		}
		this->command = CommandType_Revalue;
		this->productPrice = credit;
		result = true;
	}
	return result;
}

bool MdbMasterCashless::sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) {
	(void)productName;
	(void)wareId;
	bool result = false;
	ATOMIC {
		LOG_WARN(LOG_MDBMCL, "sale " << productId << "," << productPrice);
		if(context->getState() != State_Session) {
			LOG_ERROR(LOG_MDBMCL, "Wrong state " << context->getState());
			continue;
		}
		this->command = CommandType_VendRequest;
		this->productId = productId;
		this->productPrice = productPrice;
		result = true;
	}
	return result;
}

bool MdbMasterCashless::saleComplete() {
	bool result = false;
	ATOMIC {
		LOG_INFO(LOG_MDBMCL, "saleComplete");
		if(context->getState() != State_Vending) {
			LOG_ERROR(LOG_MDBMCL, "Wrong state " << context->getState());
			continue;
		}
		command = CommandType_VendSuccess;
		result = true;
	}
	return result;
}

bool MdbMasterCashless::saleFailed() {
	bool result = false;
	ATOMIC {
		LOG_INFO(LOG_MDBMCL, "saleFailed");
		if(context->getState() != State_Vending) {
			LOG_ERROR(LOG_MDBMCL, "Wrong state " << context->getState());
			continue;
		}
		command = CommandType_VendFailure;
		result = true;
	}
	return result;
}

bool MdbMasterCashless::closeSession() {
	bool result = false;
	ATOMIC {
		LOG_INFO(LOG_MDBMCL, "closeSession");
		if(context->getState() < State_Session) {
			LOG_ERROR(LOG_MDBMCL, "Wrong state " << context->getState());
			continue;
		}
		command = CommandType_SessionClose;
		result = true;
	}
	return result;
}

void MdbMasterCashless::initMaster(MdbMasterSender *sender) {
	this->sender = sender;
}

void MdbMasterCashless::sendRequest() {
	LOG_DEBUG(LOG_MDBMCL, "sendRequest " << context->getState());
	switch(context->getState()) {
		case State_Reset: sendReset(); break;
		case State_ResetWait: sendResetWait(); break;
		case State_SetupConfig: sendSetupConfig(); break;
		case State_SetupConfigWait: sendPoll(); break;
		case State_SetupPrices: sendSetupPrices(); break;
		case State_ExpansionIdentification: sendExpansionIdentification(); break;
		case State_ExpansionIdentificationWait: sendPoll(); break;
		case State_Work: stateWorkSend(); break;
		case State_Enable: stateEnableSend(); break;
		case State_Disable: stateDisableSend(); break;
        case State_Session: stateSessionSend(); break;
		case State_Revalue: stateRevalueSend(); break;
		case State_RevalueWait: sendPoll(); break;
		case State_VendRequest: stateVendRequestSend(); break;
		case State_VendApproving: stateVendApprovingSend(); break;
		case State_Vending: stateVendingSend(); break;		
		case State_VendCancel: stateVendCancelSend(); break;
		case State_VendCancelling: stateVendCancellingSend(); break;
		case State_VendSuccess: stateVendSuccessSend(); break;
		case State_VendFailure: stateVendFailureSend(); break;
		case State_SessionComplete: stateSessionCompleteSend(); break;
		case State_SessionEnd: stateSessionEndSend(); break;
		default: LOG_ERROR(LOG_MDBMCL, "Unsupported state " << context->getState());
	}
}

void MdbMasterCashless::recvResponse(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_DEBUG(LOG_MDBMCL, "recvResponse " << context->getState() << "," << crc << "," << dataLen);
	LOG_TRACE_HEX(LOG_MDBMCL, data, dataLen);
	REMOTE_LOG_IN(RLOG_MDBMCL, data, dataLen);
	this->tryCount = 0;
	switch(context->getState()) {
		case State_Reset: recvReset(crc, data, dataLen); break;
		case State_ResetWait: recvResetWait(crc, data, dataLen); break;
		case State_SetupConfig: recvSetupConfig(crc, data, dataLen); break;
		case State_SetupConfigWait: recvSetupConfig(crc, data, dataLen); break;
		case State_SetupPrices: recvSetupPrices(); break;
		case State_ExpansionIdentification: recvExpansionIdentification(crc, data, dataLen); break;
		case State_ExpansionIdentificationWait: recvExpansionIdentification(crc, data, dataLen); break;
		case State_Work: stateWorkRecv(crc, data, dataLen); break;
		case State_Enable: stateEnableRecv(crc, data, dataLen); break;
        case State_Disable: stateDisableRecv(crc, data, dataLen); break;
        case State_Session: stateSessionRecv(crc, data, dataLen); break;
		case State_Revalue: stateRevalueRecv(crc, data, dataLen); break;
		case State_RevalueWait: stateRevalueRecv(crc, data, dataLen); break;
		case State_VendRequest: stateVendRequestRecv(crc, data, dataLen); break;
		case State_VendApproving: stateVendApprovingRecv(crc, data, dataLen); break;
		case State_Vending: stateVendingRecv(crc, data, dataLen); break;
		case State_VendCancel: stateVendCancelRecv(crc, data, dataLen); break;
		case State_VendCancelling: stateVendCancellingRecv(crc, data, dataLen); break;
		case State_VendSuccess: stateVendSuccessRecv(crc, data, dataLen); break;
		case State_VendFailure: stateVendFailureRecv(crc, data, dataLen); break;
		case State_SessionComplete: stateSessionCompleteRecv(crc, data, dataLen); break;
		case State_SessionEnd: stateSessionEndRecv(crc, data, dataLen); break;
		default: LOG_ERROR(LOG_MDBMCL, "Unsupported state " << context->getState());
	}
}

void MdbMasterCashless::timeoutResponse() {
	LOG_DEBUG(LOG_MDBMCL, "timeoutResponse " << context->getState() << "," << tryCount);
	if(context->getState() != State_Reset) {
		tryCount++;
		if(tryCount >= MDB_TRY_NUMBER) {
			LOG_INFO(LOG_MDBMCL, "Too many tries");
			gotoStateReset();
			EventUint16Interface event(deviceId, Event_Error, Error_NotResponsible);
			deliverEvent(&event);
		}
	}
}

void MdbMasterCashless::procTimer() {
	LOG_ERROR(LOG_MDBMCL, "procTimer " << context->getState());
	switch(context->getState()) {
	case State_Session: gotoSessionComplete(); return;
	case State_VendRequest: stateSessionEndTimeout(); return;
	case State_VendApproving: gotoStateVendCancel(); return;
	case State_VendSuccess:
	case State_VendFailure:
	case State_VendCancel:
	case State_VendCancelling:
	case State_SessionComplete:
	case State_SessionEnd: stateSessionEndTimeout(); return;
	default: LOG_ERROR(LOG_SM, "Unwaited timeout " << context->getState());
	}
}

void MdbMasterCashless::gotoStateReset() {
	LOG_DEBUG(LOG_MDBMCL, "gotoStateReset");
	tryCount = 0;
	command = CommandType_None;
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	context->incResetCount();
	context->setState(State_Reset);
}

void MdbMasterCashless::sendReset() {
	LOG_DEBUG(LOG_MDBMCL, "sendReset");
	sender->startRequest();
	sender->addUint8(getType() | Command_Reset);
	REMOTE_LOG_OUT(RLOG_MDBMCL, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCashless::recvReset(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBMCL, "Recv Reset");
	LOG_DEBUG_HEX(LOG_MDBMCL, data, dataLen);
	if(crc == true) {
		LOG_ERROR(LOG_MDBMCL, "Unstandard answer");
		sender->sendConfirm(Mdb::Control_ACK);
		for(uint16_t i = 0; i < dataLen; i++) {
			if(data[i] == Status_JustReset) {
				LOG_INFO(LOG_MDBMCL, "Recv JustReset");
				gotoStateSetupConfig();
				return;
			}
		}
		return;
	}

	if(dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMCL, "Wrong answer");
		return;
	}

	LOG_INFO(LOG_MDBMCL, "Recv ACK");
	gotoStateResetWait();
}

void MdbMasterCashless::gotoStateResetWait() {
	repeatCount = 0;
	context->setState(State_ResetWait);
}

void MdbMasterCashless::sendResetWait() {
	sender->startRequest();
	sender->addUint8(getType() | Command_Poll);
	REMOTE_LOG_OUT(RLOG_MDBMCL, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCashless::recvResetWait(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBMCL, "recvResetWait");
	LOG_DEBUG_HEX(LOG_MDBMCL, data, dataLen);
	if(crc == false) {
		LOG_DEBUG(LOG_MDBMCL, "No answer");
		repeatCount++;
		if(repeatCount >= MDB_JUST_RESET_COUNT) {
			LOG_ERROR(LOG_MDBMCL, "JustReset not found");
			gotoStateReset();
			return;
		}
		return;
	}

	sender->sendConfirm(Mdb::Control_ACK);
	for(uint16_t i = 0; i < dataLen; i++) {
		if(data[i] == Status_JustReset) {
			LOG_INFO(LOG_MDBMCL, "Recv JustReset");
			gotoStateSetupConfig();
			return;
		}
	}
}

void MdbMasterCashless::sendPoll() {
    LOG_DEBUG(LOG_MDBMCL, "sendPoll");
	sender->startRequest();
	sender->addUint8(getType() | Command_Poll);
	REMOTE_LOG_OUT(RLOG_MDBMCL, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCashless::gotoStateSetupConfig() {
	LOG_INFO(LOG_MDBMCL, "gotoStateSetupConfig");
	enabled = false;
	timer->stop();
	context->setStatus(Mdb::DeviceContext::Status_Init);
	context->setState(State_SetupConfig);
}

void MdbMasterCashless::sendSetupConfig() {
	LOG_INFO(LOG_MDBMCL, "sendSetupConfig");
	SetupConfigRequest req;
	req.command = getType() | Command_Setup;
	req.subcommand = Subcommand_SetupConfig;
	req.featureLevel = Mdb::FeatureLevel_2;
	req.columnsOnDisplay = 0;
	req.rowsOnDisplay = 0;
	req.displayInfo = 0;
	REMOTE_LOG_OUT(RLOG_MDBMCL, &req, sizeof(req));
	sender->sendRequest(&req, sizeof(req));
}

void MdbMasterCashless::recvSetupConfig(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBMCL, "recvSetupConfig");
	if(crc == false) {
		LOG_INFO(LOG_MDBMCL, "Confirm instead data " << tryCount);
		tryCount++;
		if(tryCount > MDB_TRY_NUMBER) {
			LOG_INFO(LOG_MDBMCL, "Too many tries");
			gotoStateReset();
			return;
		}
		context->setState(State_SetupConfigWait);
		return;
	}
	if(dataLen < sizeof(SetupConfigResponse)) {
		LOG_ERROR(LOG_MDBMCL, "Wrong response");
		context->incProtocolErrorCount();
		return;
	}
	sender->sendConfirm(Mdb::Control_ACK);

	SetupConfigResponse *pkt = (SetupConfigResponse*)data;
	context->init(pkt->decimalPlaces, pkt->scaleFactor);
	context->setCurrency(pkt->currency.get());
	refundAble = (pkt->options & Mdb::Cashless::SetupConifgOption_RefundAble) > 0;

	LOG_INFO(LOG_MDBMCL, "featureLevel " << pkt->featureLevel);
	LOG_INFO(LOG_MDBMCL, "currency " << pkt->currency.get());
	LOG_INFO(LOG_MDBMCL, "scaleFactor " << pkt->scaleFactor);
	LOG_INFO(LOG_MDBMCL, "decimalPlaces " << pkt->decimalPlaces);
	LOG_INFO(LOG_MDBMCL, "maxRespTime " << pkt->maxRespTime);
	LOG_INFO(LOG_MDBMCL, "options " << pkt->options);
	LOG_INFO(LOG_MDBMCL, "refundAble " << refundAble);

	context->setState(State_SetupPrices);
}

void MdbMasterCashless::sendSetupPrices() {
	LOG_INFO(LOG_MDBMCL, "sendSetupPrices");
	SetupPricesL1Request req;
	req.command = getType() | Command_Setup;
	req.subcommand = Subcommand_SetupPrices;
	req.maximumPrice = MdbCashlessMaximumPrice;
	req.minimunPrice = 0;
	REMOTE_LOG_OUT(RLOG_MDBMCL, &req, sizeof(req));
	sender->sendRequest(&req, sizeof(req));
}

void MdbMasterCashless::recvSetupPrices() {
	LOG_INFO(LOG_MDBMCL, "recvSetupPrices");
	context->setState(State_ExpansionIdentification);
}

void MdbMasterCashless::sendExpansionIdentification() {
	LOG_INFO(LOG_MDBMCL, "sendExpansionIdentification");
	ExpansionRequestIdRequest req;
	req.command = getType() | Command_Expansion;
	req.subcommand = Subcommand_ExpansionRequestId;
	strncpy((char*)req.manufacturerCode, MDB_MANUFACTURER_CODE, sizeof(req.manufacturerCode));
	strncpy((char*)req.serialNumber, "0123456789AB", sizeof(req.serialNumber));
	strncpy((char*)req.modelNumber, "0123456789AB", sizeof(req.modelNumber));
	req.softwareVersion.set(100);
	REMOTE_LOG_OUT(RLOG_MDBMCL, &req, sizeof(req));
	sender->sendRequest(&req, sizeof(req));
}

void MdbMasterCashless::recvExpansionIdentification(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBMCL, "recvExpansionIdentification");
	if(crc == false) {
		LOG_INFO(LOG_MDBMCL, "Confirm instead data");
		context->setState(State_ExpansionIdentificationWait);
		return;
	}
	uint16_t respLen = sizeof(ExpansionRequestIdResponseL1);
	if(dataLen < respLen) {
		LOG_ERROR(LOG_MDBMCL, "Wrong response " << dataLen << "<>" << respLen);
		context->incProtocolErrorCount();
		return;
	}
	sender->sendConfirm(Mdb::Control_ACK);

	ExpansionRequestIdResponseL1 *resp = (ExpansionRequestIdResponseL1*)data;
	LOG_INFO(LOG_MDBMCL, "perepheralId " << resp->perepheralId);
	LOG_INFO_HEX(LOG_MDBMCL, resp->manufacturerCode, sizeof(resp->manufacturerCode));
	LOG_INFO_HEX(LOG_MDBMCL, resp->serialNumber, sizeof(resp->serialNumber));
	LOG_INFO_HEX(LOG_MDBMCL, resp->modelNumber, sizeof(resp->modelNumber));
	LOG_INFO(LOG_MDBMCL, "softwareVersion " << resp->softwareVersion.get());

	context->setManufacturer(resp->manufacturerCode, sizeof(resp->manufacturerCode));
	context->setModel(resp->modelNumber, sizeof(resp->modelNumber));
	context->setSerialNumber(resp->serialNumber, sizeof(resp->serialNumber));
	context->setSoftwareVersion(resp->softwareVersion.get());
	gotoStateWork();
	EventInterface event(deviceId, Event_Ready);
	deliverEvent(&event);
}

void MdbMasterCashless::gotoStateWork() {
	LOG_DEBUG(LOG_MDBMCL, "gotoStateWork");
	repeatCount = 0;
	timer->stop();
	context->setState(State_Work);
}

void MdbMasterCashless::stateWorkSend() {
	LOG_DEBUG(LOG_MDBMCL, "sendWork " << repeatCount);
	if(enabling != enabled) {
		if(enabling == false) {
			stateDisableSend();
			context->setState(State_Disable);
			return;
		} else {
			stateEnableSend();
			context->setState(State_Enable);
			return;
		}
	}

	repeatCount++;
	if(repeatCount > MDB_REPEAT_COUNT) {
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
		default: sendPoll();
	}
}

void MdbMasterCashless::stateWorkRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	if(crc == false) {
		LOG_DEBUG(LOG_MDBMCL, "Recv ACK");
		tryCount = 0;
		return;
	}
	LOG_INFO(LOG_MDBMCL, "stateWorkRecv " << dataLen);
	LOG_INFO_HEX(LOG_MDBMCL, data, dataLen);
	sender->sendConfirm(Mdb::Control_ACK);

	for(uint16_t i = 0; i < dataLen;) {
        uint8_t status = data[i];
		if(status == Status_BeginSession) {
			i += procBeginSession(data + i, dataLen - i);
			break;
		} else if(status == Status_JustReset) {
			LOG_INFO(LOG_MDBMCL, "Recv JustReset");
			gotoStateSetupConfig();
			break;
		} else {
			i++;
		}
    }
}

uint16_t MdbMasterCashless::procBeginSession(const uint8_t *data, uint16_t dataLen) {
	if(dataLen < sizeof(BeginSession)) {
		LOG_ERROR(LOG_MDBMCL, "Wrong poll response");
		return dataLen;
	}

	BeginSession *resp = (BeginSession*)data;
	uint32_t credit = context->value2money(resp->fundsAvailable.get());
	LOG_ERROR(LOG_MDBMCL, "Begin session " << resp->fundsAvailable.get() << "," << credit);
	timer->start(MDB_CL_SESSION_TIMEOUT);
	context->setState(State_Session);
	EventUint32Interface event(deviceId, Event_SessionBegin, credit);
	deliverEvent(&event);
	return sizeof(BeginSession);
}

void MdbMasterCashless::stateEnableSend() {
	if(enabling != enabled) {
		LOG_INFO(LOG_MDBMCL, "stateEnableSend");
	} else {
		LOG_DEBUG(LOG_MDBMCL, "stateEnableSend");
	}
	sender->startRequest();
	sender->addUint8(getType() | Command_Reader);
	sender->addUint8(Subcommand_ReaderEnable);
	REMOTE_LOG_OUT(RLOG_MDBMCL, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCashless::stateEnableRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_MDBMCL, "stateEnableRecv " << crc << " " << data[0]);
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMCL, "Wrong answer");
		context->incProtocolErrorCount();
		return;
	}

	if(enabling != enabled) {
		LOG_INFO(LOG_MDBMCL, "Recv ACK");
	} else {
		LOG_DEBUG(LOG_MDBMCL, "Recv ACK");
	}
	enabled = true;
	context->setStatus(Mdb::DeviceContext::Status_Enabled);
	gotoStateWork();
}

void MdbMasterCashless::stateDisableSend() {
	if(enabling != enabled) {
		LOG_INFO(LOG_MDBMCL, "stateDisableSend");
	} else {
		LOG_DEBUG(LOG_MDBMCL, "stateDisableSend");
	}
	sender->startRequest();
	sender->addUint8(getType() | Command_Reader);
	sender->addUint8(Subcommand_ReaderDisable);
	REMOTE_LOG_OUT(RLOG_MDBMCL, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCashless::stateDisableRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_MDBMCL, "stateDisableRecv " << crc << " " << data[0]);
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMCL, "Wrong answer");
		context->incProtocolErrorCount();
		return;
	}

	if(enabling != enabled) {
		LOG_INFO(LOG_MDBMCL, "Recv ACK");
	} else {
		LOG_DEBUG(LOG_MDBMCL, "Recv ACK");
	}
	enabled = false;
	context->setStatus(Mdb::DeviceContext::Status_Disabled);
	gotoStateWork();
}

void MdbMasterCashless::stateSessionSend() {
    LOG_DEBUG(LOG_MDBMCL, "stateSessionSend");
    switch(command) {
		case CommandType_Revalue: {
			command = CommandType_None;
			stateRevalueSend();
			context->setState(State_Revalue);
			break;
		}
		case CommandType_VendRequest: {
			command = CommandType_None;
			stateVendRequestSend();
			gotoStateVendRequest();
            break;
        }
		case CommandType_SessionClose: {
			command = CommandType_None;
			stateSessionCompleteSend();
			gotoSessionComplete();
			break;
		}
        default: sendPoll();
    }
}

void MdbMasterCashless::stateSessionRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_MDBMCL, "stateSessionRecv");
	LOG_DEBUG_HEX(LOG_MDBMCL, data, dataLen);
	if(crc == false) {
		LOG_DEBUG(LOG_MDBMCL, "Recv ACK");
		return;
	}
	sender->sendConfirm(Mdb::Control_ACK);
	for(uint16_t i = 0; i < dataLen; i++) {
		uint8_t status = data[i];
		if(status == Status_SessionCancelRequest) {
			LOG_INFO(LOG_MDBMCL, "Session cancel request");
			gotoSessionComplete();
		}
	}
}

void MdbMasterCashless::stateRevalueSend() {
	LOG_INFO(LOG_MDBMCL, "stateRevalueSend");
	RevalueRequestL2 req;
	req.command = getType() | Command_Revalue;
	req.subcommand = Subcommand_RevalueRequest;
	req.amount.set(context->money2value(productPrice));
	REMOTE_LOG_OUT(RLOG_MDBMCL, &req, sizeof(req));
	sender->sendRequest(&req, sizeof(req));
}

//todo: отдельный метод для проверки результата poll'а
void MdbMasterCashless::stateRevalueRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBMCL, "stateRevalueRecv " << crc << " " << data[0]);
	if(crc == false) {
		if(data[0] == Mdb::Control_ACK) {
			LOG_ERROR(LOG_MDBMCL, "Recv ACK");
			tryCount = 0;
			context->setState(State_RevalueWait);
			return;
		} else {
			LOG_ERROR(LOG_MDBMCL, "Wrong answer");
			context->incProtocolErrorCount();
			context->setState(State_Session);
			EventInterface event(deviceId, Event_RevalueDenied);
			deliverEvent(&event);
			return;
		}
	}

	sender->sendConfirm(Mdb::Control_ACK);
	timer->start(MDB_CL_SESSION_TIMEOUT);
	if(data[0] == Status_RevalueApproved) {
		LOG_INFO(LOG_MDBMCL, "Revalue succeed");
		context->setState(State_Session);
		EventInterface event(deviceId, Event_RevalueApproved);
		deliverEvent(&event);
		return;
	} else {
		LOG_INFO(LOG_MDBMCL, "Revalue failed");
		context->setState(State_Session);
		EventInterface event(deviceId, Event_RevalueDenied);
		deliverEvent(&event);
		return;
	}

}

void MdbMasterCashless::gotoStateVendRequest() {
	LOG_INFO(LOG_MDBMCL, "gotoStateVendRequest");
	timer->start(MDB_NON_RESPONSE_TIME);
	context->setState(State_VendRequest);
}

void MdbMasterCashless::stateVendRequestSend() {
	LOG_INFO(LOG_MDBMCL, "sendVendRequest");
	VendRequestRequest req;
	req.command = getType() | Command_Vend;
	req.subcommand = Subcommand_VendRequest;
	req.itemPrice.set(context->money2value(productPrice));
	req.itemNumber.set(productId);
	REMOTE_LOG_OUT(RLOG_MDBMCL, &req, sizeof(req));
	sender->sendRequest(&req, sizeof(req));
}

void MdbMasterCashless::stateVendRequestRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBMCL, "recvVendRequest");
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMCL, "Wrong answer");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_MDBMCL, "Recv ACK");
	timer->start(MDB_CL_APPROVING_TIMEOUT);
	context->setState(State_VendApproving);
}

void MdbMasterCashless::stateVendApprovingSend() {
	LOG_DEBUG(LOG_MDBMCL, "stateVendApprovingSend");
	switch(command) {
		case CommandType_SessionClose: {
			command = CommandType_None;
			stateVendCancelSend();
			gotoStateVendCancel();
			break;
		}
		default: sendPoll();
	}
}

void MdbMasterCashless::stateVendApprovingRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_MDBMCL, "stateVendApprovingRecv");
	LOG_DEBUG_HEX(LOG_MDBMCL, data, dataLen);
	if(crc == false) {
		LOG_DEBUG(LOG_MDBMCL, "Recv ACK");
		return;
	}
	sender->sendConfirm(Mdb::Control_ACK);

	for(uint16_t i = 0; i < dataLen;) {
		uint8_t status = data[i];
		if(status == Status_VendApproved) {
			i += procVendApproved(data + i, dataLen - i);
		} else if(status == Status_VendDenied) {
			i += procVendDenied(dataLen - i);
		} else {
			i++;
		}
	}
}

uint16_t MdbMasterCashless::procVendApproved(const uint8_t *data, uint16_t dataLen) {
	if(dataLen < sizeof(VendApproved)) {
		LOG_ERROR(LOG_MDBMCL, "Wrong poll response");
		return dataLen;
	}
	VendApproved *resp = (VendApproved*)data;
	uint32_t price = context->value2money(resp->vendAmount.get());
	LOG_ERROR(LOG_MDBMCL, "Vend approved " << resp->vendAmount.get() << "," << price);
	timer->stop();
	context->setState(State_Vending);
	MdbMasterCashlessInterface::EventApproved event(deviceId, Fiscal::Payment_Cashless, price);
	deliverEvent(&event);
	return dataLen;
}

uint16_t MdbMasterCashless::procVendDenied(uint16_t dataLen) {
	LOG_ERROR(LOG_MDBMCL, "Vend denied");
	timer->start(MDB_CL_SESSION_TIMEOUT);
	context->setState(State_Session);
	EventInterface event(deviceId, Event_VendDenied);
	deliverEvent(&event);
	return dataLen;
}

void MdbMasterCashless::gotoStateVendCancel() {
	LOG_INFO(LOG_MDBMCL, "stateVendCancelSend");
	timer->start(MDB_NON_RESPONSE_TIME);
	context->setState(State_VendCancel);
}

void MdbMasterCashless::stateVendCancelSend() {
	LOG_INFO(LOG_MDBMCL, "stateVendCancelSend");
	sender->startRequest();
	sender->addUint8(getType() | Command_Vend);
	sender->addUint8(Subcommand_VendCancel);
	REMOTE_LOG_OUT(RLOG_MDBMCL, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterCashless::stateVendCancelRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBMCL, "stateVendCancelRecv");
	LOG_DEBUG_HEX(LOG_MDBMCL, data, dataLen);
	if(crc == false) {
		LOG_DEBUG(LOG_MDBMCL, "Recv ACK");
		context->setState(State_VendCancelling);
		return;
	}
	sender->sendConfirm(Mdb::Control_ACK);

	LOG_ERROR(LOG_MDBMCL, "Vend canceled");
	gotoSessionComplete();
}

void MdbMasterCashless::stateVendCancellingSend() {
	LOG_DEBUG(LOG_MDBMCL, "stateVendCancellingSend");
	switch(command) {
		default: sendPoll();
	}
}

void MdbMasterCashless::stateVendCancellingRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_MDBMCL, "stateVendCancellingRecv");
	LOG_DEBUG_HEX(LOG_MDBMCL, data, dataLen);
	if(crc == false) {
		LOG_DEBUG(LOG_MDBMCL, "Recv ACK");
		return;
	}
	sender->sendConfirm(Mdb::Control_ACK);

	for(uint16_t i = 0; i < dataLen;) {
		uint8_t status = data[i];
		if(status == Status_VendDenied) {
			LOG_ERROR(LOG_MDBMCL, "Vend canceled");
			gotoSessionComplete();
			return;
		}
	}
}

void MdbMasterCashless::stateVendingSend() {
	LOG_DEBUG(LOG_MDBMCL, "stateVendingSend");
	switch(command) {
		case CommandType_VendSuccess: {
			command = CommandType_None;
			stateVendSuccessSend();
			gotoStateVendSuccess();
			break;
		}
		case CommandType_VendFailure:
		case CommandType_SessionClose: {
			command = CommandType_None;
			stateVendFailureSend();
			gotoStateVendFailure();
			break;
		}
		default: sendPoll();
	}
}

void MdbMasterCashless::stateVendingRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_MDBMCL, "stateVendingRecv");
	LOG_DEBUG_HEX(LOG_MDBMCL, data, dataLen);
	if(crc == false) {
		LOG_DEBUG(LOG_MDBMCL, "Recv ACK");
		return;
	}
	sender->sendConfirm(Mdb::Control_ACK);
}

void MdbMasterCashless::gotoStateVendSuccess() {
	LOG_INFO(LOG_MDBMCL, "gotoStateVendSuccess");
	timer->start(MDB_NON_RESPONSE_TIME);
	context->setState(State_VendSuccess);
}

void MdbMasterCashless::stateVendSuccessSend() {
	LOG_INFO(LOG_MDBMCL, "sendVendSuccess");
	VendSuccessRequest req;
	req.command = getType() | Command_Vend;
	req.subcommand = Subcommand_VendSuccess;
	req.itemNumber.set(productId);
	REMOTE_LOG_OUT(RLOG_MDBMCL, &req, sizeof(req));
	sender->sendRequest(&req, sizeof(req));
}

void MdbMasterCashless::stateVendSuccessRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBMCL, "recvVendSuccess " << crc << " " << data[0]);
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMCL, "Wrong answer");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_MDBMCL, "Recv ACK");
	gotoSessionComplete();
}

void MdbMasterCashless::gotoStateVendFailure() {
	LOG_INFO(LOG_MDBMCL, "gotoStateVendFailure");
	timer->start(MDB_NON_RESPONSE_TIME);
	context->setState(State_VendFailure);
}

void MdbMasterCashless::stateVendFailureSend() {
	LOG_INFO(LOG_MDBMCL, "sendVendFailure");
	Mdb::Header req;
	req.command = getType() | Command_Vend;
	req.subcommand = Subcommand_VendFailure;
	REMOTE_LOG_OUT(RLOG_MDBMCL, &req, sizeof(req));
	sender->sendRequest(&req, sizeof(req));
}

void MdbMasterCashless::stateVendFailureRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBMCL, "recvVendFailure " << crc << " " << data[0]);
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMCL, "Wrong answer");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_MDBMCL, "Recv ACK");
	gotoSessionComplete();
}

void MdbMasterCashless::gotoSessionComplete() {
	LOG_INFO(LOG_MDBMCL, "gotoSessionComplete");
	timer->start(MDB_NON_RESPONSE_TIME);
	context->setState(State_SessionComplete);
}

void MdbMasterCashless::stateSessionCompleteSend() {
	LOG_INFO(LOG_MDBMCL, "sendSessionComplete");
	Mdb::Header req;
	req.command = getType() | Command_Vend;
	req.subcommand = Subcommand_SessionComplete;
	REMOTE_LOG_OUT(RLOG_MDBMCL, &req, sizeof(req));
	sender->sendRequest(&req, sizeof(req));
}

void MdbMasterCashless::stateSessionCompleteRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBMCL, "recvSessionComplete " << crc << " " << data[0]);
	if(crc == true) {
		if(data[0] == Mdb::Cashless::Status_EndSession) {
			LOG_INFO(LOG_MDBMCL, "Recv SessionEnd");
			procSessionEnd();
			return;
		} else {
			LOG_ERROR(LOG_MDBMCL, "Wrong answer");
			LOG_DEBUG_HEX(LOG_MDBMCL, data, dataLen);
			context->incProtocolErrorCount();
			return;
		}
	} else {
		if(data[0] == Mdb::Control_ACK) {
			LOG_INFO(LOG_MDBMCL, "Recv ACK");
			context->setState(State_SessionEnd);
			return;
		} else {
			LOG_ERROR(LOG_MDBMCL, "Wrong confirm");
			LOG_DEBUG_HEX(LOG_MDBMCL, data, dataLen);
			context->incProtocolErrorCount();
			return;
		}
	}
}

void MdbMasterCashless::stateSessionEndSend() {
	LOG_DEBUG(LOG_MDBMCL, "stateSessionEndSend");
	switch(command) {
		default: sendPoll();
	}
}

void MdbMasterCashless::stateSessionEndRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_MDBMCL, "stateSessionEndRecv");
	LOG_DEBUG_HEX(LOG_MDBMCL, data, dataLen);
	if(crc == false) {
		LOG_DEBUG(LOG_MDBMCL, "Recv ACK");
		return;
	}
	sender->sendConfirm(Mdb::Control_ACK);

	for(uint16_t i = 0; i < dataLen;) {
		uint8_t status = data[i];
		if(status == Status_EndSession) {
			i += procSessionEnd();
			break;
		} else {
			i++;
		}
	}
}

uint16_t MdbMasterCashless::procSessionEnd() {
	LOG_ERROR(LOG_MDBMCL, "procSessionEnd");
	timer->stop();
	gotoStateWork();
	EventInterface event(deviceId, Event_SessionEnd);
	deliverEvent(&event);
	return 1;
}

void MdbMasterCashless::stateSessionEndTimeout() {
	LOG_INFO(LOG_MDBMCL, "stateSessionEndTimeout");
	gotoStateReset();
	context->incProtocolErrorCount();
	EventInterface event(deviceId, Event_SessionEnd);
	deliverEvent(&event);
}
