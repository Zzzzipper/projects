#include "TerminalFaCommandLayer.h"
#include "TerminalFaProtocol.h"
#include "fiscal_storage/include/FiscalStorage.h"
#include "utils/include/DecimalPoint.h"
#include "utils/include/CodePage.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace TerminalFa {

CommandLayer::CommandLayer(PacketLayerInterface *packetLayer, TimerEngine *timers, EventEngineInterface *eventEngine) :
	packetLayer(packetLayer),
	timers(timers),
	eventEngine(eventEngine),
	state(State_Idle),
	request(TERMINALFA_PACKET_MAX_SIZE)
{
	this->deviceId = eventEngine->getDeviceId();
	this->packetLayer->setObserver(this);
	this->timer = timers->addTimer<CommandLayer, &CommandLayer::procTimer>(this);
}

CommandLayer::~CommandLayer() {
	timers->deleteTimer(this->timer);
}

uint16_t CommandLayer::getDeviceId() {
	return deviceId;
}

void CommandLayer::sale(Fiscal::Sale *saleData, uint32_t decimalPoint) {
	LOG_DEBUG(LOG_FR, "sale");
	if(this->state != State_Idle) {
		LOG_ERROR(LOG_FR, "Wrong state " << state);
		return;
	}

	this->saleData = saleData;
	this->decimalPoint = decimalPoint;
	gotoStateStatus();
}

void CommandLayer::getLastSale() {
	LOG_DEBUG(LOG_FR, "getLastSale");
//	gotoStateLastSale();
}

void CommandLayer::closeShift() {
	LOG_DEBUG(LOG_FR, "closeShift");
//	gotoStateShiftClose2();
}

void CommandLayer::procRecvData(uint8_t *data, uint16_t len) {
	LOG_DEBUG(LOG_FR, "procRecvData");
	LOG_TRACE_HEX(LOG_FR, data, len);
	switch(state) {
	case State_Status: stateStatusResponse(data, len); break;
	case State_FSState: stateFSStateResponse(data, len); break;
	case State_ShiftOpenStart: stateShiftOpenStartResponse(data, len); break;
	case State_ShiftOpenFinish: stateShiftOpenFinishResponse(data, len); break;
	case State_ShiftCloseStart: stateShiftCloseStartResponse(data, len); break;
	case State_ShiftCloseFinish: stateShiftCloseFinishResponse(data, len); break;
	case State_DocumentReset: stateDocumentResetResponse(data, len); break;
	case State_CheckOpen: stateCheckOpenResponse(data, len); break;
	case State_CheckAdd: stateCheckAddResponse(data, len); break;
	case State_CheckPayment: stateCheckPaymentResponse(data, len); break;
	case State_CheckClose: stateCheckCloseResponse(data, len); break;
	default: LOG_ERROR(LOG_FR, "Unwaited data " << state);
	}
}

void CommandLayer::procRecvError(PacketLayerInterface::Error error) {
	LOG_DEBUG(LOG_FR, "procRecvError");
	if(state == State_Idle) {
		return;
	}

	state = State_Idle;
	Fiscal::EventError event(deviceId);
	event.code = ConfigEvent::Type_FiscalUnknownError;
	eventEngine->transmit(&event);
}

void CommandLayer::procTimer() {
	LOG_DEBUG(LOG_FR, "procTimer");
	switch(state) {
//	case State_ShiftClose4: stateShiftClose4Timeout(); break;
	default: LOG_ERROR(LOG_FR, "Unwaited timeout " << state);
	}
}

void CommandLayer::gotoStateStatus() {
	LOG_DEBUG(LOG_FR, "gotoStateStatus");
	Request *req = (Request*)request.getData();
	req->command = Command_Status;
	request.setLen(sizeof(*req));
	packetLayer->sendPacket(&request);
	state = State_Status;
}

void CommandLayer::stateStatusResponse(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateStatusResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *hdr = (Response*)data;
	if(hdr->result == Result_Error) {
		ErrorResponse *error = (ErrorResponse*)data;
		LOG_ERROR(LOG_FR, "Command failed " << dataLen);
		procProtocolError(error->code);
		return;
	}
	respLen = sizeof(StatusResponse);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}

	StatusResponse *resp = (StatusResponse*)data;
	LOG_INFO(LOG_FR, "result=" << resp->result);
	LOG_INFO_WORD(LOG_FR, "serial="); LOG_INFO_STR(LOG_FR, resp->serial, sizeof(resp->serial));
	LOG_INFO(LOG_FR, "data=" << resp->datetime[0] << "." << resp->datetime[1] << "." << resp->datetime[2]);
	LOG_INFO(LOG_FR, "time=" << resp->datetime[3] << ":" << resp->datetime[4]);
	LOG_INFO(LOG_FR, "criticalError=" << resp->criticalError);
	LOG_INFO(LOG_FR, "printerState=" << resp->printerState);
	LOG_INFO(LOG_FR, "fnState=" << resp->fnState);
	LOG_INFO(LOG_FR, "lifePhase=" << resp->lifePhase);

	if(resp->lifePhase != LifePhase_Fiscal) {
		LOG_ERROR(LOG_FR, "Wrong KKT state");
		procError(ConfigEvent::Type_FiscalNotInited);
		return;
	}

	gotoStateFSState();
}

void CommandLayer::gotoStateFSState() {
	LOG_DEBUG(LOG_FR, "gotoStateFSState");
	Request *req = (Request*)request.getData();
	req->command = Command_FSState;
	request.setLen(sizeof(*req));
	packetLayer->sendPacket(&request);
	state = State_FSState;
}

void CommandLayer::stateFSStateResponse(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateFSStateResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *hdr = (Response*)data;
	if(hdr->result == Result_Error) {
		ErrorResponse *error = (ErrorResponse*)data;
		LOG_ERROR(LOG_FR, "Command failed " << dataLen);
		procProtocolError(error->code);
		return;
	}
	respLen = sizeof(FSStateResponse);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}

	FSStateResponse *resp = (FSStateResponse*)data;
	if(resp->shiftState == 0) {
		LOG_INFO(LOG_FR, "Shift closed");
		gotoStateShiftOpenStart();
		return;
	} else if(resp->docType != DocType_None) {
		LOG_INFO(LOG_FR, "Document already opened");
		gotoStateDocumentReset();
		return;
	} else {
		LOG_INFO(LOG_FR, "Shift opened");
		gotoStateCheckOpen();
		return;
	}
}

void CommandLayer::gotoStateShiftOpenStart() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftOpenStart");
	ShiftOpenStartRequest *req = (ShiftOpenStartRequest*)request.getData();
	req->command = Command_ShiftOpenStart;
	req->printing = 1;
	request.setLen(sizeof(*req));
	packetLayer->sendPacket(&request);
	state = State_ShiftOpenStart;
}

void CommandLayer::stateShiftOpenStartResponse(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateShiftOpenStartResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *hdr = (Response*)data;
	if(hdr->result == Result_Error) {
		ErrorResponse *error = (ErrorResponse*)data;
		LOG_ERROR(LOG_FR, "Command failed " << dataLen);
		procProtocolError(error->code);
		return;
	}

	gotoStateShiftOpenFinish();
}

void CommandLayer::gotoStateShiftOpenFinish() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftOpenFinish");
	Request *req = (Request*)request.getData();
	req->command = Command_ShiftOpenFinish;
	request.setLen(sizeof(*req));
	packetLayer->sendPacket(&request);
	state = State_ShiftOpenFinish;
}

void CommandLayer::stateShiftOpenFinishResponse(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateShiftOpenFinishResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *hdr = (Response*)data;
	if(hdr->result == Result_Error) {
		ErrorResponse *error = (ErrorResponse*)data;
		LOG_ERROR(LOG_FR, "Command failed " << dataLen);
		procProtocolError(error->code);
		return;
	}

	gotoStateFSState();
}

void CommandLayer::gotoStateShiftCloseStart() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftCloseStart");
	ShiftOpenStartRequest *req = (ShiftOpenStartRequest*)request.getData();
	req->command = Command_ShiftCloseStart;
	req->printing = 1;
	request.setLen(sizeof(*req));
	packetLayer->sendPacket(&request);
	state = State_ShiftCloseStart;
}

void CommandLayer::stateShiftCloseStartResponse(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateShiftOpenStartResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *hdr = (Response*)data;
	if(hdr->result == Result_Error) {
		ErrorResponse *error = (ErrorResponse*)data;
		LOG_ERROR(LOG_FR, "Command failed " << dataLen);
		procProtocolError(error->code);
		return;
	}

	gotoStateShiftCloseFinish();
}

void CommandLayer::gotoStateShiftCloseFinish() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftCloseFinish");
	Request *req = (Request*)request.getData();
	req->command = Command_ShiftCloseFinish;
	request.setLen(sizeof(*req));
	packetLayer->sendPacket(&request);
	state = State_ShiftCloseFinish;
}

void CommandLayer::stateShiftCloseFinishResponse(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateShiftCloseFinishResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *hdr = (Response*)data;
	if(hdr->result == Result_Error) {
		ErrorResponse *error = (ErrorResponse*)data;
		LOG_ERROR(LOG_FR, "Command failed " << dataLen);
		procProtocolError(error->code);
		return;
	}

	gotoStateShiftOpenStart();
}

void CommandLayer::gotoStateDocumentReset() {
	LOG_DEBUG(LOG_FR, "gotoStateDocumentReset");
	Request *req = (Request*)request.getData();
	req->command = Command_DocumentReset;
	request.setLen(sizeof(*req));
	packetLayer->sendPacket(&request);
	state = State_DocumentReset;
}

void CommandLayer::stateDocumentResetResponse(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateDocumentResetResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *hdr = (Response*)data;
	if(hdr->result == Result_Error) {
		ErrorResponse *error = (ErrorResponse*)data;
		LOG_ERROR(LOG_FR, "Command failed " << dataLen);
		procProtocolError(error->code);
		return;
	}

	gotoStateFSState();
}

void CommandLayer::gotoStateCheckOpen() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckOpen");
	Request *req = (Request*)request.getData();
	req->command = Command_CheckOpen;
	request.setLen(sizeof(*req));
	packetLayer->sendPacket(&request);
	state = State_CheckOpen;
}

void CommandLayer::stateCheckOpenResponse(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateCheckOpenResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *hdr = (Response*)data;
	if(hdr->result == Result_Error) {
		ErrorResponse *error = (ErrorResponse*)data;
		if(error->code == Error_ShiftTooOld) {
			LOG_ERROR(LOG_FR, "Shitf too old");
			gotoStateShiftCloseStart();
			return;
		} else {
			LOG_ERROR(LOG_FR, "Command failed " << dataLen);
			procProtocolError(error->code);
			return;
		}
	}

	gotoStateCheckAdd();
}

void CommandLayer::fillProductTlv(Fiscal::Sale *saleData, Buffer *buf) {
#if 0
	char name[ConfigProductNameSize + 1];
	strncpy(name, saleData->name.get(), sizeof(name));
	convertWin1251ToCp866((uint8_t*)name, strlen(name));
	uint32_t price = convertDecimalPoint(decimalPoint, FISCAL_DECIMAL_POINT, saleData->price);
	FiscalStorage::TaxRate fnTaxRate = FiscalStorage::convertTaxRate2FN105(saleData->taxRate);
	FiscalStorage::addTlvString(FiscalStorage::Tag_ProductName, name, buf);
	FiscalStorage::addTlvUint32(FiscalStorage::Tag_ProductPrice, price, buf);
	FiscalStorage::addTlvFUint32(FiscalStorage::Tag_ProductNumber, 0, 1, buf);
	FiscalStorage::addTlvUint32(FiscalStorage::Tag_TaxRate, fnTaxRate, buf);
	FiscalStorage::addTlvUint32(FiscalStorage::Tag_PaymentMethod, 4, buf);
#endif
}

void CommandLayer::gotoStateCheckAdd() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckAdd");
	request.clear();
	fillProductTlv(saleData, &request);
	uint16_t productTlvLen = request.getLen();

	request.clear();
	request.addUint8(Command_CheckAdd);
	addTlvHeader(FiscalStorage::Tag_Product, productTlvLen, &request);
	fillProductTlv(saleData, &request);

	packetLayer->sendPacket(&request);
	state = State_CheckAdd;
}

void CommandLayer::stateCheckAddResponse(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateCheckAddResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *hdr = (Response*)data;
	if(hdr->result == Result_Error) {
		ErrorResponse *error = (ErrorResponse*)data;
		LOG_ERROR(LOG_FR, "Command failed " << dataLen);
		procProtocolError(error->code);
		return;
	}

	gotoStateCheckPayment();
}

void CommandLayer::gotoStateCheckPayment() {
#if 0
	LOG_DEBUG(LOG_FR, "gotoStateCheckPayment");
	uint32_t price = convertDecimalPoint(decimalPoint, FISCAL_DECIMAL_POINT, saleData->price);
	FiscalStorage::TaxSystem fsTaxSystem = FiscalStorage::convertTaxSystem2FN105(saleData->taxSystem);

	request.clear();
	request.addUint8(Command_CheckPayment);
	FiscalStorage::addTlvUint32(FiscalStorage::Tag_TaxSystem, fsTaxSystem, &request);
	if(saleData->paymentType == Fiscal::Payment_Cash) {
		FiscalStorage::addTlvUint32(FiscalStorage::Tag_PaymentCash, price, &request);
		FiscalStorage::addTlvUint32(FiscalStorage::Tag_PaymentCashless, 0, &request);
	} else {
		FiscalStorage::addTlvUint32(FiscalStorage::Tag_PaymentCash, 0, &request);
		FiscalStorage::addTlvUint32(FiscalStorage::Tag_PaymentCashless, price, &request);
	}
	FiscalStorage::addTlvUint32(FiscalStorage::Tag_Prepayment, 0, &request);
	FiscalStorage::addTlvUint32(FiscalStorage::Tag_Postpay, 0, &request);
	FiscalStorage::addTlvUint32(FiscalStorage::Tag_PaymentHZ, 0, &request);
	FiscalStorage::addTlvString(FiscalStorage::Tag_ClientMail, "", &request);
	packetLayer->sendPacket(&request);
	state = State_CheckPayment;
#endif
}

void CommandLayer::stateCheckPaymentResponse(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateCheckPaymentResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *hdr = (Response*)data;
	if(hdr->result == Result_Error) {
		ErrorResponse *error = (ErrorResponse*)data;
		LOG_ERROR(LOG_FR, "Command failed " << dataLen);
		procProtocolError(error->code);
		return;
	}

	gotoStateCheckClose();
}

void CommandLayer::gotoStateCheckClose() {
#if 0
	LOG_DEBUG(LOG_FR, "gotoStateCheckClose");
	uint32_t price = convertDecimalPoint(decimalPoint, FISCAL_DECIMAL_POINT, saleData->price);
	CheckCloseRequest *req = (CheckCloseRequest*)request.getData();
	req->command = Command_CheckClose;
	req->sign = 1;
	req->total.set(price);
	request.setLen(sizeof(*req));
	LOG_DEBUG_HEX(LOG_FR, request.getData(), request.getLen());
	packetLayer->sendPacket(&request);
	state = State_CheckClose;
#endif
}

void CommandLayer::stateCheckCloseResponse(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateCheckCloseResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *hdr = (Response*)data;
	if(hdr->result == Result_Error) {
		ErrorResponse *error = (ErrorResponse*)data;
		LOG_ERROR(LOG_FR, "Command failed " << dataLen);
		procProtocolError(error->code);
		return;
	}

	LOG_INFO(LOG_FR, "CheckClose complete!");
	state = State_Idle;
	EventInterface event(deviceId, Fiscal::Register::Event_CommandOK);
	eventEngine->transmit(&event);
}

void CommandLayer::procProtocolError(uint8_t returnCode) {
	state = State_Idle;
	Fiscal::EventError event(deviceId);
	switch(returnCode) {
	default: {
		LOG_ERROR(LOG_FR, "Unknown error " << returnCode);
		event.code = ConfigEvent::Type_FiscalUnknownError;
		event.data << returnCode;
	}
	}
	eventEngine->transmit(&event);
}

void CommandLayer::procError(ConfigEvent::Code errorCode) {
	state = State_Idle;
	Fiscal::EventError event(deviceId);
	event.code = errorCode;
	eventEngine->transmit(&event);
}

void CommandLayer::procDebug(const char *file, uint16_t line) {
	state = State_Idle;
	Fiscal::EventError event(deviceId);
	event.code = ConfigEvent::Type_FiscalLogicError;
	event.data << basename(file) << "#" << line;
	eventEngine->transmit(&event);
}

}
