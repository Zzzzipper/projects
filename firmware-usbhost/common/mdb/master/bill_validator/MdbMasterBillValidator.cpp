#include "MdbMasterBillValidator.h"

#include "mdb/MdbProtocolBillValidator.h"
#include "utils/include/Hex.h"
#include "logger/RemoteLogger.h"
#include "logger/include/Logger.h"

using namespace Mdb::BillValidator;

MdbMasterBillValidator::MdbMasterBillValidator(MdbBillValidatorContext *context, EventEngineInterface *eventEngine) :
	MdbMaster(Mdb::Device_BillValidator, eventEngine),
	context(context),
	deviceId(eventEngine),
	enabling(false),
	enabled(false)
{
	context->setState(State_Idle);
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
}

EventDeviceId MdbMasterBillValidator::getDeviceId() {
	return deviceId;
}

void MdbMasterBillValidator::reset() {
	ATOMIC {
		LOG_ERROR(LOG_MDBMBV, "reset");
		gotoStateReset();
	}
}

bool MdbMasterBillValidator::isInited() {
	bool result = false;
	ATOMIC {
		result = (context->getState() > State_ExpansionIdentification);
	}
	return result;
}

void MdbMasterBillValidator::disable() {
	ATOMIC {
		LOG_WARN(LOG_MDBMBV, "Disable " << context->getState());
		this->enabling = false;
	}
}

void MdbMasterBillValidator::enable() {
	ATOMIC {
		LOG_WARN(LOG_MDBMBV, "Enable " << context->getState());
		this->enabling = true;
	}
}

void MdbMasterBillValidator::initMaster(MdbMasterSender *sender) {
	this->sender = sender;
}

void MdbMasterBillValidator::sendRequest() {
	LOG_DEBUG(LOG_MDBMBV, "sendRequest " << context->getState());
	switch(context->getState()) {
		case State_Reset: this->sendReset(); break;
		case State_ResetWait: this->sendResetWait(); break;
		case State_Setup: this->sendSetup(); break;
		case State_ExpansionIdentification: this->sendExpansionIdentification(); break;
		case State_BillType: this->sendBillType(); break;
		case State_Poll: this->statePollSend(); break;
		case State_Escrow: this->sendEscrow(); break;
		case State_EscrowConfirm: this->sendEscrowConfirm(); break;
		case State_Disable: stateDisableSend(); break;
		case State_Enable: stateEnableSend(); break;
		default: LOG_ERROR(LOG_MDBMBV, "Unsupported state " << context->getState());
	}
}

void MdbMasterBillValidator::recvResponse(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_DEBUG(LOG_MDBMBV, "recvResponse " << context->getState());
	LOG_TRACE_HEX(LOG_MDBMBV, data, dataLen);
	REMOTE_LOG_IN(RLOG_MDBMBV, data, dataLen);
	this->tryCount = 0;
	switch(context->getState()) {
		case State_Reset: recvReset(crc, data, dataLen); break;
		case State_ResetWait: recvResetWait(crc, data, dataLen); break;
		case State_Setup: recvSetup(crc, data, dataLen); break;
		case State_ExpansionIdentification: this->recvExpansionIdentification(crc, data, dataLen); break;
		case State_BillType: recvBillType(crc, data, dataLen); break;
		case State_Poll: statePollRecv(crc, data, dataLen); break;
		case State_Escrow: recvEscrow(crc, data, dataLen); break;
		case State_EscrowConfirm: recvEscrowConfirm(crc, data, dataLen); break;
		case State_Disable: stateDisableRecv(crc, data, dataLen); break;
		case State_Enable: stateEnableRecv(crc, data, dataLen); break;
		default: LOG_ERROR(LOG_MDBMBV, "Unsupported state " << context->getState());
	}
}

void MdbMasterBillValidator::timeoutResponse() {
	LOG_DEBUG(LOG_MDBMBV, "timeoutResponse " << context->getState());
	if(context->getState() != State_Reset) {
		this->tryCount += 1;
		if(this->tryCount >= MDB_TRY_NUMBER) {
			this->reset();
			EventUint16Interface event(deviceId, Event_Error, Error_NotResponsible);
			deliverEvent(&event);
		}
	}
}

void MdbMasterBillValidator::gotoStateReset() {
	tryCount = 0;
	command = CommandType_None;
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	context->incResetCount();
	context->setState(State_Reset);
}

void MdbMasterBillValidator::sendReset() {
	sender->startRequest();
	sender->addUint8(getType() | Command_Reset);
	REMOTE_LOG_OUT(RLOG_MDBMBV, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterBillValidator::recvReset(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBMBV, "Recv Reset");
	LOG_DEBUG_HEX(LOG_MDBMBV, data, dataLen);
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMBV, "Wrong answer");
		return;
	}

	LOG_INFO(LOG_MDBMBV, "Recv ACK");
	gotoStateResetWait();
}

void MdbMasterBillValidator::gotoStateResetWait() {
	repeatCount = 0;
	context->setState(State_ResetWait);
}

void MdbMasterBillValidator::sendResetWait() {
	sender->startRequest();
	sender->addUint8(getType() | Command_Poll);
	REMOTE_LOG_OUT(RLOG_MDBMBV, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterBillValidator::recvResetWait(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBMBV, "recvResetWait");
	LOG_DEBUG_HEX(LOG_MDBMBV, data, dataLen);
	if(crc == false) {
		LOG_DEBUG(LOG_MDBMBV, "No answer");
		repeatCount++;
		if(repeatCount >= MDB_JUST_RESET_COUNT) {
			LOG_ERROR(LOG_MDBMBV, "JustReset not found");
			gotoStateReset();
			return;
		}
		return;
	}

	sender->sendConfirm(Mdb::Control_ACK);
	for(uint16_t i = 0; i < dataLen; i++) {
		if(data[i] == Status_JustReset) {
			LOG_INFO(LOG_MDBMBV, "Recv JustReset");
			gotoStateSetup();
			return;
		}
	}
}

void MdbMasterBillValidator::gotoStateSetup() {
	LOG_INFO(LOG_MDBMBV, "gotoStateSetup");
	enabled = false;
	context->setStatus(Mdb::DeviceContext::Status_Init);
	context->setState(State_Setup);
}

void MdbMasterBillValidator::sendSetup() {
	LOG_INFO(LOG_MDBMBV, "sendSetup");
	sender->startRequest();
	sender->addUint8(getType() | Command_Setup);
	REMOTE_LOG_OUT(RLOG_MDBMBV, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterBillValidator::recvSetup(bool crc, const uint8_t *data, uint16_t dataLen) {
	if(crc == false || dataLen < sizeof(SetupResponse)) {
		LOG_ERROR(LOG_MDBMBV, "Wrong response");
		return;
	}
	sender->sendConfirm(Mdb::Control_ACK);

	SetupResponse *pkt = (SetupResponse*)data;
	uint16_t billNum = dataLen - sizeof(SetupResponse);
	context->init(pkt->level, pkt->decimalPlaces, pkt->scalingFactor.get(), pkt->bills, billNum);
	context->setCurrency(pkt->currency.get());

	LOG_INFO(LOG_MDBMBV, "level " << pkt->level);
	LOG_INFO(LOG_MDBMBV, "currency " << pkt->currency.get());
	LOG_INFO(LOG_MDBMBV, "scalingFactor " << pkt->scalingFactor.get());
	LOG_INFO(LOG_MDBMBV, "decimalPlaces " << pkt->decimalPlaces);
	LOG_INFO(LOG_MDBMBV, "stackerCapacity " << pkt->stackerCapacity.get());
	LOG_INFO(LOG_MDBMBV, "securityLevel " << pkt->securityLevel.get());
	LOG_INFO(LOG_MDBMBV, "escrow " << pkt->escrow);
	LOG_INFO_HEX(LOG_MDBMBV, pkt->bills, billNum);

	context->setState(State_ExpansionIdentification);
}

void MdbMasterBillValidator::sendExpansionIdentification() {
	LOG_INFO(LOG_MDBMBV, "sendExpansionIdentification");
	sender->startRequest();
	sender->addUint8(getType() | Command_Expansion);
	sender->addUint8(Subcommand_ExpansionIdentificationL1);
	REMOTE_LOG_OUT(RLOG_MDBMBV, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterBillValidator::recvExpansionIdentification(bool crc, const uint8_t *data, uint16_t dataLen) {
	if(crc == false || dataLen < sizeof(ExpansionIdentificationL1Response)) {
		LOG_ERROR(LOG_MDBMBV, "Wrong response");
		return;
	}
	sender->sendConfirm(Mdb::Control_ACK);

	ExpansionIdentificationL1Response *pkt = (ExpansionIdentificationL1Response*)data;
	LOG_INFO_HEX(LOG_MDBMBV, pkt->manufacturerCode, sizeof(pkt->manufacturerCode));
	LOG_INFO_HEX(LOG_MDBMBV, pkt->serialNumber, sizeof(pkt->serialNumber));
	LOG_INFO_HEX(LOG_MDBMBV, pkt->model, sizeof(pkt->model));
	LOG_INFO(LOG_MDBMBV, "softwareVersion " << pkt->softwareVersion.get());

	context->setManufacturer(pkt->manufacturerCode, sizeof(pkt->manufacturerCode));
	context->setModel(pkt->model, sizeof(pkt->model));
	context->setSerialNumber(pkt->serialNumber, sizeof(pkt->serialNumber));
	context->setSoftwareVersion(pkt->softwareVersion.get());
	context->setState(State_BillType);
}

void MdbMasterBillValidator::sendBillType() {
	LOG_INFO(LOG_MDBMBV, "sendBillType");
	sender->startRequest();
	sender->addUint8(getType() | Command_BillType);
	sender->addUint16(0);
	sender->addUint16(0);
	REMOTE_LOG_OUT(RLOG_MDBMBV, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterBillValidator::recvBillType(bool crc, const uint8_t *data, uint16_t dataLen) {
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMBV, "Wrong answer");
		return;
	}
	LOG_INFO(LOG_MDBMBV, "Recv ACK");
	gotoStatePoll();
	EventInterface event(deviceId, Event_Ready);
	deliverEvent(&event);
}

void MdbMasterBillValidator::gotoStatePoll() {
	LOG_DEBUG(LOG_MDBMBV, "gotoStatePoll");
	repeatCount = 0;
	context->setState(State_Poll);
}

void MdbMasterBillValidator::statePollSend() {
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

void MdbMasterBillValidator::statePollRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	if(crc == false) {
		return;
	}

	LOG_DEBUG(LOG_MDBMBV, "statePollRecv");
	LOG_DEBUG_HEX(LOG_MDBMBV, data, dataLen);
	for(uint16_t i = 0; i < dataLen; i++) {
		uint8_t status = data[i];
		if(status & BillMask_Sign) {
			uint8_t billRoute = status & BillMask_Route;
			uint8_t billIndex = status & BillMask_Type;
#if 0
			if(billRoute == BillEvent_EscrowPosition) {
				uint32_t nominal = context->getBillNominal(billIndex);
				LOG_WARN(LOG_MDBMBV, "Detected bill " << nominal);
				gotoStateEscrow();
				break;
			}
#else
			switch(billRoute) {
				case BillEvent_BillStacked:
				case BillEvent_BillToRecycler: {
					uint32_t nominal = context->getBillNominal(billIndex);
					LOG_WARN(LOG_MDBMBV, "Bill stacked " << nominal);
					context->registerLastBill(nominal);
					gotoStatePoll();
					Mdb::EventDeposite event(deviceId, Event_Deposite, Route_Stacked, nominal);
					deliverEvent(&event);
					break;
				}
				case BillEvent_BillReturned: {
					LOG_WARN(LOG_MDBMBV, "Recv BillReturned");
					gotoStatePoll();
					break;
				}
				case BillEvent_EscrowPosition: {
					uint32_t nominal = context->getBillNominal(billIndex);
					LOG_WARN(LOG_MDBMBV, "Detected bill " << nominal);
					gotoStateEscrow();
					break;
				}
			}
#endif
		} else if(status == Status_JustReset) {
			LOG_WARN(LOG_MDBMBV, "Recv JustReset");
			gotoStateSetup();
			break;
		} else if(status == Status_ValidatorDisabled) {
			LOG_DEBUG(LOG_MDBMBV, "Poll ValidatorDisabled");
		} else {
			LOG_ERROR(LOG_MDBMBV, "Poll status " << status);
		}
	}
	sender->sendConfirm(Mdb::Control_ACK);
}

void MdbMasterBillValidator::gotoStateEscrow() {
	LOG_DEBUG(LOG_MDBMBV, "gotoStateEscrow");
	context->setState(State_Escrow);
}

void MdbMasterBillValidator::sendEscrow() {
	LOG_DEBUG(LOG_MDBMBV, "sendEscrow");
	sender->startRequest();
	sender->addUint8(getType() | Command_Escrow);
	sender->addUint8(0x01);
	REMOTE_LOG_OUT(RLOG_MDBMBV, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterBillValidator::recvEscrow(bool crc, const uint8_t *data, uint16_t dataLen) {
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMBV, "Wrong response.");
		return;
	}
	LOG_INFO(LOG_MDBMBV, "Recv ACK");
	context->setState(State_EscrowConfirm);
}

void MdbMasterBillValidator::sendEscrowConfirm() {
	sender->startRequest();
	sender->addUint8(getType() | Command_Poll);
	REMOTE_LOG_OUT(RLOG_MDBMBV, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

// BILL STACKED, BILL RETURNED, INVALID ESCROW or BILL TO RECYCLER message within 30 seconds
void MdbMasterBillValidator::recvEscrowConfirm(bool crc, const uint8_t *data, uint16_t dataLen) {
	if(crc == false) {
		LOG_WARN(LOG_MDBMBV, "Unwaited ACK (hi Aurora)");
		return;
	}
	LOG_DEBUG(LOG_MDBMBV, "recvEscrowConfirm");
	LOG_DEBUG_HEX(LOG_MDBMBV, data, dataLen);
	for(uint16_t i = 0; i < dataLen; i++) {
		uint8_t status = data[i];
		if(status & BillMask_Sign) {
			uint8_t billRoute = status & BillMask_Route;
			uint8_t billIndex = status & BillMask_Type;
			switch(billRoute) {
				case BillEvent_BillStacked:
				case BillEvent_BillToRecycler: {
					uint32_t nominal = context->getBillNominal(billIndex);
					LOG_WARN(LOG_MDBMBV, "Bill stacked " << nominal);
					context->registerLastBill(nominal);
					gotoStatePoll();
					Mdb::EventDeposite event(deviceId, Event_Deposite, Route_Stacked, nominal);
					deliverEvent(&event);
					break;
				}
				case BillEvent_BillReturned: {
					LOG_WARN(LOG_MDBMBV, "Recv BillReturned");
					gotoStatePoll();
					break;
				}
				case BillEvent_EscrowPosition: {
					uint32_t nominal = context->getBillNominal(billIndex);
					LOG_WARN(LOG_MDBMBV, "Detected bill " << nominal);
					gotoStateEscrow();
					break;
				}
			}
		} else if(status == Status_InvalidEscrowRequest) {
			LOG_WARN(LOG_MDBMBV, "Recv InvalidEscrowRequest");
			gotoStatePoll();
			break;
		} else if(status == Status_JustReset) {
			LOG_WARN(LOG_MDBMBV, "Recv JustReset");
			gotoStateSetup();
			break;
		} else if(status == Status_ValidatorDisabled) {
			LOG_WARN(LOG_MDBMBV, "Poll ValidatorDisabled");
		} else {
			LOG_ERROR(LOG_MDBMBV, "Poll status " << status);
		}
	}
	sender->sendConfirm(Mdb::Control_ACK);
}

void MdbMasterBillValidator::stateDisableSend() {
	LOG_INFO(LOG_MDBMBV, "stateDisableSend");
	sender->startRequest();
	sender->addUint8(getType() | Command_BillType);
	sender->addUint16(0); // bill disable
	sender->addUint16(0); // escrow bill disable
	REMOTE_LOG_OUT(RLOG_MDBMBV, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterBillValidator::stateDisableRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBMBV, "stateDisableRecv " << crc << " " << data[0]);
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMBV, "Wrong answer");
		return;
	}
	LOG_INFO(LOG_MDBMBV, "Recv ACK");
	enabled = false;
	context->setStatus(Mdb::DeviceContext::Status_Disabled);
	gotoStatePoll();
}

void MdbMasterBillValidator::stateEnableSend() {
	LOG_INFO(LOG_MDBMBV, "stateEnableSend");
	sender->startRequest();
	sender->addUint8(getType() | Command_BillType);
	sender->addUint16(context->getMask());
	sender->addUint16(context->getMask());
	REMOTE_LOG_OUT(RLOG_MDBMBV, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}

void MdbMasterBillValidator::stateEnableRecv(bool crc, const uint8_t *data, uint16_t dataLen) {
	LOG_INFO(LOG_MDBMBV, "stateEnableRecv " << crc << " " << data[0]);
	if(crc == true || dataLen != 1 || data[0] != Mdb::Control_ACK) {
		LOG_ERROR(LOG_MDBMBV, "Wrong answer");
		return;
	}
	LOG_INFO(LOG_MDBMBV, "Recv ACK");
	enabled = true;
	context->setStatus(Mdb::DeviceContext::Status_Enabled);
	gotoStatePoll();
}

void MdbMasterBillValidator::sendPoll() {
	sender->startRequest();
	sender->addUint8(getType() | Command_Poll);
	REMOTE_LOG_OUT(RLOG_MDBMBV, sender->getData(), sender->getDataLen());
	sender->sendRequest();
}
