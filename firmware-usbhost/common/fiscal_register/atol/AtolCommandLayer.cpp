#if 1
#include "AtolCommandLayer.h"
#include "fiscal_storage/include/FiscalStorage.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "utils/include/DecimalPoint.h"
#include "utils/include/CodePage.h"
#include "utils/include/Number.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace Atol {

CommandLayer::CommandLayer(
	Fiscal::Context *context,
	const char *ipaddr,
	uint16_t port,
	TimerEngine *timers,
	TaskLayerInterface *taskLayer,
	EventEngineInterface *eventEngine,
	LedInterface *leds
) :
	context(context),
	ipaddr(ipaddr),
	port(port),
	timers(timers),
	taskLayer(taskLayer),
	eventEngine(eventEngine),
	leds(leds),
	deviceId(eventEngine),
	state(State_Idle),
	request(ATOL_PACKET_MAX_SIZE),
	doc(256),
	envelope(EVENT_DATA_SIZE)
{
	this->taskLayer->setObserver(this);
	this->timer = timers->addTimer<CommandLayer, &CommandLayer::procTimer>(this);
	this->password = 0;
}

CommandLayer::~CommandLayer() {
	this->timers->deleteTimer(this->timer);
}

EventDeviceId CommandLayer::getDeviceId() {
	return deviceId;
}

void CommandLayer::reset() {
	LOG_DEBUG(LOG_FR, "reset");
	this->command = Command_Reset;
	this->context->setStatus(Mdb::DeviceContext::Status_NotFound);
	this->context->setManufacturer((uint8_t*)ATOL_MANUFACTURER, strlen(ATOL_MANUFACTURER));
	this->context->setModel((uint8_t*)ATOL_MODEL, strlen(ATOL_MODEL));
	this->leds->setFiscal(LedInterface::State_InProgress);
	gotoStateConnect();
}

void CommandLayer::sale(Fiscal::Sale *saleData) {
#if 0
	LOG_DEBUG(LOG_FR, "sale");
	if(this->state != State_Idle) {
		LOG_ERROR(LOG_FR, "Wrong state " << state);
		return;
	}
	this->command = Command_Sale;
	uint32_t price = context->money2value(saleData->price);
	this->saleData = saleData;
	this->saleData->taxValue = context->calcTax(saleData->taxRate, price);
	this->saleData->fiscalRegister = 0;
	this->saleData->fiscalStorage = Fiscal::Status_Error;
	this->saleData->fiscalDocument = 0;
	this->saleData->fiscalSign = 0;
	this->tryNumber = 0;
	this->leds->setFiscal(LedInterface::State_InProgress);
	gotoStateConnect();
#endif
}

void CommandLayer::getLastSale() {
	LOG_DEBUG(LOG_FR, "getLastSale");
	this->command = Command_GetLastSale;
	gotoStateConnect();
}

void CommandLayer::closeShift() {
	LOG_DEBUG(LOG_FR, "closeShift");
	this->command = Command_CloseShift;
	gotoStateConnect();
}

void CommandLayer::procRecvData(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_FR, "procRecvData");
	LOG_TRACE_HEX(LOG_FR, data, len);
	switch(state) {
	case State_KktStatus: stateDeviceStatusResponse(data, len); break;
	case State_Status: stateStatusResponse(data, len); break;
	case State_PrinterStatus: stateStatePrinterResponse(data, len); break;
	case State_SaleOpen: stateSaleOpenResponse(data, len); break;
	case State_ShiftOpen: stateShiftOpenResponse(data, len); break;
	case State_CheckOpen: stateCheckOpenResponse(data, len); break;
	case State_FN105CheckAddStart: stateFN105CheckAddStartResponse(data, len); break;
	case State_FN105CheckAddEnd: stateFN105CheckAddEndResponse(data, len); break;
	case State_CheckClose: stateCheckCloseResponse(data, len); break;
	case State_CheckReset: stateCheckResetResponse(data, len); break;
	case State_ModeClose: stateModeCloseResponse(data, len); break;
	case State_ShiftClose1: stateShiftClose1Response(data, len); break;
	case State_ShiftClose2: stateShiftClose2Response(data, len); break;
	case State_ShiftClose3: stateShiftClose3Response(data, len); break;
	case State_ShiftClose5: stateShiftClose5Response(data, len); break;
	case State_LastSale: stateLastSaleResponse(data, len); break;
	case State_DocSize: stateDocSizeResponse(data, len); break;
	case State_DocData: stateDocDataResponse(data, len); break;
	default: LOG_ERROR(LOG_FR, "Unwaited data " << state);
	}
}

void CommandLayer::procError(TaskLayerObserver::Error error) {
	LOG_DEBUG(LOG_FR, "procError " << error);
	switch(state) {
	case State_Idle: return;
	case State_Connect: stateConnectError(error); return;
	case State_Disconnect: stateDisconnectError(error); return;
	default: procTaskLayerError(error);
	}
}

void CommandLayer::procTimer() {
	LOG_DEBUG(LOG_FR, "procTimer");
	switch(state) {
	case State_ShiftClose4: stateShiftClose4Timeout(); break;
	default: LOG_ERROR(LOG_FR, "Unwaited timeout " << state);
	}
}

void CommandLayer::gotoStateConnect() {
	LOG_DEBUG(LOG_FR, "gotoStateConnect");
	taskLayer->connect(ipaddr, port, TcpIp::Mode_TcpIp);
	state = State_Connect;
}

void CommandLayer::stateConnectError(TaskLayerObserver::Error error) {
	LOG_DEBUG(LOG_FR, "stateConnectError");
	switch(error) {
	case TaskLayerObserver::Error_OK: {
		switch(command) {
		case Command_None: return;
		case Command_Reset: gotoStateDeviceStatus(); return;
		case Command_Sale: gotoStateStatus(); return;
		case Command_GetLastSale: gotoStateLastSale(); return;
		case Command_CloseShift: gotoStateShiftClose2(); return;
		}
	}
	case TaskLayerObserver::Error_ConnectFailed: {
		state = State_Idle;
		Fiscal::EventError event(deviceId);
		event.code = ConfigEvent::Type_FiscalConnectError;
		context->removeAll();
		context->addError(event.code, event.data.getString());
		eventEngine->transmit(&event);
		leds->setFiscal(LedInterface::State_Failure);
		return;
	}
	default: {
		LOG_ERROR(LOG_FR, "Unwaited error " << state << "," << error);
		procTaskLayerError(error);
		return;
	}
	}
}

void CommandLayer::gotoStateDisconnect(EventInterface *event) {
	LOG_DEBUG(LOG_FR, "gotoStateDisconnect");
	if(taskLayer->disconnect() == false) {
		LOG_ERROR(LOG_FR, "Disconnect failed");
		state = State_Idle;
		eventEngine->transmit(event);
		return;
	}

	event->pack(&envelope);
	state = State_Disconnect;
}

void CommandLayer::stateDisconnectError(TaskLayerObserver::Error error) {
	LOG_DEBUG(LOG_FR, "stateDisconnectError");
	switch(error) {
	case TaskLayerObserver::Error_OK: {
		state = State_Idle;
		eventEngine->transmit(&envelope);
		return;
	}
	default: {
		LOG_ERROR(LOG_FR, "Unwaited error " << state << "," << error);
		eventEngine->transmit(&envelope);
		return;
	}
	}
}

void CommandLayer::gotoStateDeviceStatus() {
	LOG_DEBUG(LOG_FR, "gotoStateDeviceStatus");
	SubcommandRequest *req = (SubcommandRequest*)request.getData();
	req->password.set(password);
	req->command = Command_FiscalStorage;
	req->subcommand = FSSubcommand_FiscalNumber;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_KktStatus;
}

void CommandLayer::stateDeviceStatusResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateDeviceStatusResponse");
	LOG_HEX(data, dataLen);

	FSDocDataResponse *resp = (FSDocDataResponse *)data;
	if(resp->resultCode != 0x55 || resp->errorCode > 0) {
		LOG_ERROR(LOG_FR, "Wrong response " << resp->resultCode << "," << resp->errorCode);
		gotoStateStatus();
		return;
	}

	uint16_t numberLen = dataLen - sizeof(*resp);
	if(Sambery::stringToNumber((char*)resp->data, numberLen, &fiscalStorage) == 0) {
		LOG_ERROR(LOG_FR, "Wrong format");
		gotoStateStatus();
		return;
	}

	LOG_INFO(LOG_FR, "fiscalStorage=" << fiscalStorage);
	gotoStateStatus();
}

void CommandLayer::gotoStateStatus() {
	LOG_DEBUG(LOG_FR, "gotoStateStatus");
	Request *req = (Request*)request.getData();
	req->password.set(password);
	req->command = Command_Status;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_Status;
}

void CommandLayer::stateStatusResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateStatusResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	if(data[0] == 0x55) {
		Response *resp = (Response*)data;
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procResponseError(resp->errorCode);
		return;
	}
	respLen = sizeof(StatusResponse);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	StatusResponse *resp = (StatusResponse*)data;
	LOG_INFO(LOG_FR, "resultCode=" << resp->resultCode);
	LOG_INFO(LOG_FR, "cashier=" << resp->cashier.get());
	LOG_INFO(LOG_FR, "number=" << resp->number);
	LOG_INFO(LOG_FR, "data=" << resp->dataYear.get() << "." << resp->dataMonth.get() << "." << resp->dataDay.get());
	LOG_INFO(LOG_FR, "time=" << resp->timeHour.get() << ":" << resp->timeMinute.get() << ":" << resp->timeSecond.get());
	LOG_INFO(LOG_FR, "flags=" << resp->flags);
	LOG_INFO(LOG_FR, "serialNumber=" << resp->serialNumber.get());
	LOG_INFO(LOG_FR, "deviceId=" << resp->deviceId);
	LOG_INFO(LOG_FR, "DeviceVersion=" << (char)resp->deviceVersionMajor << "." << (char)resp->deviceVersionMinor);
	LOG_INFO(LOG_FR, "deviceMode=" << resp->deviceMode);
	LOG_INFO(LOG_FR, "checkNumber=" << resp->checkNumber.get());
	LOG_INFO(LOG_FR, "shiftNumber=" << resp->shiftNumber.get());
	LOG_INFO(LOG_FR, "checkState=" << resp->checkState);
	LOG_INFO(LOG_FR, "checkSum=" << resp->checkSum.get());
	LOG_INFO(LOG_FR, "decimalPoint=" << resp->decimalPoint);
	LOG_INFO(LOG_FR, "interfaceId=" << resp->interfaceId);

	context->setStatus(Mdb::DeviceContext::Status_Work);
	StringBuilder serialNumber;
	serialNumber << resp->serialNumber.get();
	context->setSerialNumber((uint8_t*)serialNumber.getString(), serialNumber.getLen());

	if(command == Command_Reset) {
		EventInterface event(deviceId, Fiscal::Register::Event_CommandOK);
		leds->setFiscal(LedInterface::State_Success);
		gotoStateDisconnect(&event);
		return;
	}

	uint8_t mode = resp->deviceMode & Mode_Mask;
	if(mode == Mode_Idle) {
		gotoStateSaleOpen();
		return;
	} else if(mode == Mode_Sale) {
		if(resp->flags & ModeFlag_ShiftOpened) {
			gotoStateCheckOpen();
			return;
		} else if(resp->checkState > 0) {
			gotoStateCheckReset();
			return;
		} else {
			gotoStateShiftOpen();
			return;
		}
	} else {
		gotoStateModeClose();
		return;
	}
}

void CommandLayer::gotoStatePrinterStatus() {
	LOG_DEBUG(LOG_FR, "gotoStatePrinterStatus");
	Request *req = (Request*)request.getData();
	req->password.set(password);
	req->command = Command_PrinterStatus;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_PrinterStatus;
}

void CommandLayer::stateStatePrinterResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateStatePrinterResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
#if 0
	if(data[0] == 0x55) {
		Response *resp = (Response*)data;
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procResponseError(resp->errorCode);
		return;
	}
	respLen = sizeof(PrinterStatusResponse);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	PrinterStatusResponse *resp = (PrinterStatusResponse*)data;
	LOG_INFO(LOG_FR, "resultCode=" << resp->resultCode);
	LOG_INFO(LOG_FR, "deviceMode=" << resp->deviceMode);
	LOG_INFO(LOG_FR, "printerMode=" << resp->printerMode);
#else
	LOG_DEBUG_HEX(LOG_FR, data, dataLen);
#endif
	if(command == Command_Sale) {
		gotoStateCheckOpen();
		return;
	} else {
		EventInterface event(deviceId, Fiscal::Register::Event_CommandOK);
		gotoStateDisconnect(&event);
		return;
	}
}

void CommandLayer::gotoStateSaleOpen() {
	LOG_DEBUG(LOG_FR, "gotoStateSaleOpen");
	ModeOpenRequest *req = (ModeOpenRequest*)request.getData();
	req->password1.set(password);
	req->command = Command_ModeOpen;
	req->mode = Mode_Sale;
	req->password2.set(1); // todo: ввод пароля из параметра
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_SaleOpen;
}

void CommandLayer::stateSaleOpenResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateSaleOpenResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	if(data[0] != 0x55) {
		LOG_ERROR(LOG_FR, "Unwaited response type " << data[0]);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateStatus();
		return;
	} else if(resp->errorCode == CommandError_ShiftMore24) {
		gotoStateShiftClose1();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procResponseError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateShiftOpen() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftOpen");
	ShiftOpenRequest *req = (ShiftOpenRequest*)request.getData();
	req->password.set(password);
	req->command = Command_ShiftOpen;
	req->flags = 0;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_ShiftOpen;
}

void CommandLayer::stateShiftOpenResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateShiftOpenResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateStatus();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procResponseError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateCheckOpen() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckOpen");
	CheckOpenRequest *req = (CheckOpenRequest*)request.getData();
	req->password.set(password);
	req->command = Command_CheckOpen;
	req->flags = 0;
	req->checkType = CheckType_Sale;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_CheckOpen;
}

void CommandLayer::stateCheckOpenResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateCheckOpenResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateFN105CheckAddStart();
		return;
	} else if(resp->errorCode == CommandError_ShiftMore24) {
		gotoStateShiftClose1();
		return;
	} else if(resp->errorCode == CommandError_CheckAlreadyOpened) {
		gotoStateCheckReset();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procResponseError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateFN105CheckAddStart() {
	LOG_DEBUG(LOG_FR, "gotoStateFN105CheckAddStart");
	FN105CheckAddStartRequest *req = (FN105CheckAddStartRequest*)request.getData();
	req->password.set(password);
	req->command = Command_FN105CheckAddStart;
	req->flags = 0;
	req->param = 1;
	req->reserved = 0;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_FN105CheckAddStart;
}

void CommandLayer::stateFN105CheckAddStartResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateFN105CheckAddStartResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateFN105CheckAddEnd();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procResponseError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateFN105CheckAddEnd() {
#if 0
	LOG_DEBUG(LOG_FR, "gotoStateFN105CheckAddEnd");
	uint32_t price = context->money2value(saleData->price);
	FN105CheckAddEndRequest *req = (FN105CheckAddEndRequest*)request.getData();
	req->password.set(password);
	req->command = Command_FN105CheckAddEnd;
	req->flags = 0;
	req->price.set(price);
	req->number.set(1000);
	req->value.set(price);
	req->taxRate = FiscalStorage::convertTaxRate2FN105(saleData->taxRate);
	req->taxSum.set(saleData->taxValue); // если 0, то ККТ считает налог самостоятельно
	req->section.set(0); // если нет необходимости использовать секции, то 0
	req->saleSubject.set(1);
	req->saleMethod.set(4);
	req->discountSign = 0;
	req->discountSize.set(0);
	req->reserved[0] = 0;
	req->reserved[1] = 0;
	uint16_t nameLen = req->setName(saleData->name.get());
	request.setLen(sizeof(*req) + nameLen);
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_FN105CheckAddEnd;
#endif
}

void CommandLayer::stateFN105CheckAddEndResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateFN105CheckAddEndResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateCheckClose();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procResponseError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateCheckClose() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckClose");
	CheckCloseRequest *req = (CheckCloseRequest*)request.getData();
	req->password.set(password);
	req->command = Command_CheckClose;
	req->flags = 0;
	req->paymentType.set(FiscalStorage::convertPaymentMethod2FN105(saleData->paymentType));
	req->enterSum.set(context->money2value(saleData->credit));
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_CheckClose;
}

void CommandLayer::stateCheckCloseResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateCheckCloseResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		LOG_INFO(LOG_FR, "CheckClose complete!");
		gotoStateLastSale();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procResponseError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateCheckReset() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckReset");
	Request *req = (Request*)request.getData();
	req->password.set(password);
	req->command = Command_CheckReset;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_CheckReset;
}

void CommandLayer::stateCheckResetResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateCheckResetResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateStatus();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procResponseError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateModeClose() {
	LOG_DEBUG(LOG_FR, "gotoStateModeClose");
	Request *req = (Request*)request.getData();
	req->password.set(password);
	req->command = Command_ModeClose;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_ModeClose;
}

void CommandLayer::stateModeCloseResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateModeCloseResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateStatus();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procResponseError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateShiftClose1() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftClose1");
	Request *req = (Request*)request.getData();
	req->password.set(password);
	req->command = Command_ModeClose;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_ShiftClose1;
}

void CommandLayer::stateShiftClose1Response(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateShiftClose1Response");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateShiftClose2();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procResponseError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateShiftClose2() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftClose2");
	ModeOpenRequest *req = (ModeOpenRequest*)request.getData();
	req->password1.set(password);
	req->command = Command_ModeOpen;
	req->mode = Mode_ReportZ;
	req->password2.set(30); // todo: ввод пароля из параметра
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_ShiftClose2;
}

void CommandLayer::stateShiftClose2Response(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateShiftClose2Response");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	if(data[0] != 0x55) {
		LOG_ERROR(LOG_FR, "Unwaited response type " << data[0]);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateShiftClose3();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procResponseError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateShiftClose3() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftClose3");
	Request *req = (Request*)request.getData();
	req->password.set(password);
	req->command = Command_ShiftClose;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_ShiftClose3;
}

void CommandLayer::stateShiftClose3Response(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateShiftClose3Response");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateShiftClose4();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procResponseError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateShiftClose4() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftClose4");
	timer->start(1000);
	state = State_ShiftClose4;
}

void CommandLayer::stateShiftClose4Timeout() {
	LOG_DEBUG(LOG_FR, "stateShiftClose4Timeout");
	gotoStateShiftClose5();
}

void CommandLayer::gotoStateShiftClose5() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftClose5");
	Request *req = (Request*)request.getData();
	req->password.set(password);
	req->command = Command_Status;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_ShiftClose5;
}

void CommandLayer::stateShiftClose5Response(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateShiftClose4Response");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	if(data[0] == 0x55) {
		Response *resp = (Response*)data;
		procResponseError(resp->errorCode);
		return;
	}
	respLen = sizeof(StatusResponse);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}

	StatusResponse *resp = (StatusResponse*)data;
	LOG_INFO(LOG_FR, "resultCode=" << resp->resultCode);
	LOG_INFO(LOG_FR, "cashier=" << resp->cashier.get());
	LOG_INFO(LOG_FR, "number=" << resp->number);
	LOG_INFO(LOG_FR, "data=" << resp->dataYear.get() << "." << resp->dataMonth.get() << "." << resp->dataDay.get());
	LOG_INFO(LOG_FR, "time=" << resp->timeHour.get() << ":" << resp->timeMinute.get() << ":" << resp->timeSecond.get());
	LOG_INFO(LOG_FR, "flags=" << resp->flags);
	LOG_INFO(LOG_FR, "serialNumber=" << resp->serialNumber.get());
	LOG_INFO(LOG_FR, "deviceId=" << resp->deviceId);
	LOG_INFO(LOG_FR, "deviceVersion=" << (char)resp->deviceVersionMajor << "." << (char)resp->deviceVersionMinor);
	LOG_INFO(LOG_FR, "deviceMode=" << resp->deviceMode);
	LOG_INFO(LOG_FR, "checkNumber=" << resp->checkNumber.get());
	LOG_INFO(LOG_FR, "shiftNumber=" << resp->shiftNumber.get());
	LOG_INFO(LOG_FR, "checkState=" << resp->checkState);
	LOG_INFO(LOG_FR, "checkSum=" << resp->checkSum.get());
	LOG_INFO(LOG_FR, "decimalPoint=" << resp->decimalPoint);
	LOG_INFO(LOG_FR, "interfaceId=" << resp->interfaceId);

	uint8_t mode = resp->getDeviceMode();
	uint8_t submode = resp->getDeviceSubMode();
	if((mode == Mode_ReportZ && submode == 2) || (mode = Mode_Addition && submode == 1)) {
		gotoStateShiftClose4();
		return;
	} else {
		gotoStateStatus();
		return;
	}
}

void CommandLayer::gotoStateLastSale() {
	LOG_DEBUG(LOG_FR, "gotoStateLastSale");
	RegisterReadRequest *req = (RegisterReadRequest*)request.getData();
	req->password.set(password);
	req->command = Command_RegisterRead;
	req->reg = Register_LastSale;
	req->param1 = 0;
	req->param2 = 0;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_LastSale;
}

void CommandLayer::stateLastSaleResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateLastSaleResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
#if 0
	if(data[0] == 0x55) {
		Response *resp = (Response*)data;
		switch(resp->errorCode) {
		default: LOG_ERROR(LOG_FR, "Unknown error " << resp->errorCode); return;
		}
	}
#endif
	respLen = sizeof(RegisterReadResponse);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}

	RegisterReadResponse *resp = (RegisterReadResponse*)data;
	LOG_INFO(LOG_FR, "resultCode=" << resp->resultCode);
	LOG_INFO(LOG_FR, "cashier=" << resp->errorCode);
	LOG_INFO(LOG_FR, "fdNumber=" << resp->fdNumber.get());
	LOG_INFO(LOG_FR, "type=" << resp->type);
	LOG_INFO(LOG_FR, "total=" << resp->total.get());
	LOG_INFO_WORD(LOG_FR, "datetime="); LOG_INFO_HEX(LOG_FR, resp->datetime.value, sizeof(resp->datetime.value));
	LOG_INFO_WORD(LOG_FR, "fiscalSign="); LOG_INFO_HEX(LOG_FR, resp->fiscalSign.value, sizeof(resp->fiscalSign.value));
	saleData->fiscalDatetime.year = from2to<uint8_t>(resp->datetime.value[2], 16, 10);
	saleData->fiscalDatetime.month = from2to<uint8_t>(resp->datetime.value[1], 16, 10);
	saleData->fiscalDatetime.day = from2to<uint8_t>(resp->datetime.value[0], 16, 10);
	saleData->fiscalDatetime.hour = from2to<uint8_t>(resp->datetime.value[3], 16, 10);
	saleData->fiscalDatetime.minute = from2to<uint8_t>(resp->datetime.value[4], 16, 10);
	saleData->fiscalDatetime.second = 0;
	saleData->fiscalRegister = 0;
	saleData->fiscalStorage = fiscalStorage;
	saleData->fiscalDocument = resp->fdNumber.get();
	saleData->fiscalSign = resp->fiscalSign.get();
	context->removeAll();
	context->setStatus(Mdb::DeviceContext::Status_Work);
	EventInterface event(deviceId, Fiscal::Register::Event_CommandOK);
	leds->setFiscal(LedInterface::State_Success);
	gotoStateDisconnect(&event);
}

void CommandLayer::gotoStateDocSize() {
	LOG_DEBUG(LOG_FR, "gotoStateDocSize");
	FSDocSizeRequest *req = (FSDocSizeRequest*)request.getData();
	req->password.set(password);
	req->command = Command_FiscalStorage;
	req->subcommand = FSSubcommand_DocSize;
	req->docNumber.set(docNumber);
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_DocSize;
}

void CommandLayer::stateDocSizeResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateDocSizeResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	Response *hdr = (Response*)data;
	if(data[0] != 0x55 || hdr->errorCode != Error_OK) {
		LOG_ERROR(LOG_FR, "Command failed " << hdr->errorCode);
		procResponseError(hdr->errorCode);
		return;
	}
	respLen = sizeof(FSDocSizeResponse);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}

	FSDocSizeResponse *resp = (FSDocSizeResponse*)data;
	LOG_INFO(LOG_FR, "resultCode=" << resp->resultCode);
	LOG_INFO(LOG_FR, "errorCode=" << resp->errorCode);
	LOG_INFO(LOG_FR, "docType=" << resp->docType.get());
	LOG_INFO(LOG_FR, "docSize=" << resp->docSize.get());
	docLen = 0;
	docSize = resp->docSize.get();
	doc.clear();
	gotoStateDocData();
}

void CommandLayer::gotoStateDocData() {
	LOG_DEBUG(LOG_FR, "gotoStateDocData");
	SubcommandRequest *req = (SubcommandRequest*)request.getData();
	req->password.set(password);
	req->command = Command_FiscalStorage;
	req->subcommand = FSSubcommand_DocData;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_DocData;
}

void CommandLayer::stateDocDataResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateDocDataResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}
	Response *hdr = (Response*)data;
	if(data[0] != 0x55 || hdr->errorCode != Error_OK) {
		LOG_ERROR(LOG_FR, "Command failed " << hdr->errorCode);
		procResponseError(hdr->errorCode);
		return;
	}
	respLen = sizeof(FSDocDataResponse);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebugError(__FILE__, __LINE__);
		return;
	}

	FSDocDataResponse *resp = (FSDocDataResponse*)data;
	LOG_INFO(LOG_FR, "resultCode=" << resp->resultCode);
	LOG_INFO(LOG_FR, "errorCode=" << resp->errorCode);
	doc.add(resp->data, dataLen - respLen);
	docLen += (dataLen - respLen);
	if(docLen >= docSize) {
		printSTLV(doc.getData(), doc.getLen());
		EventInterface event(deviceId, Fiscal::Register::Event_CommandOK);
		gotoStateDisconnect(&event);
		return;
	} else {
		LOG_INFO(LOG_FR, ">>>>>>>>>>>>>>>>>>>>>>>>docSize=" << docSize << ",docLen=" << docLen);
		gotoStateDocData();
		return;
	}
}

void CommandLayer::printSTLV(const uint8_t *data, const uint16_t dataLen) {
	uint16_t procLen = 0;
	while(procLen < dataLen) {
		procLen += printTLV(data + procLen, dataLen - procLen);
	}
}

uint16_t CommandLayer::printTLV(const uint8_t *data, const uint16_t dataLen) {
	if(dataLen < sizeof(FiscalStorage::Header)) {
		return dataLen;
	}
	FiscalStorage::Header *h = (FiscalStorage::Header*)data;
	LOG_INFO_WORD(LOG_FR, "tag=" << h->tag.get() << ", len=" << h->len.get() << ", data=");
	LOG_INFO_HEX(LOG_FR, h->data, h->len.get());
	return (sizeof(FiscalStorage::Header) + h->len.get());
}

void CommandLayer::procTaskLayerError(TaskLayerObserver::Error errorCode) {
	Fiscal::EventError event(deviceId);
	event.code = ConfigEvent::Type_FiscalUnknownError;
	event.data << "TL" << errorCode;
	context->removeAll();
	context->addError(event.code, event.data.getString());
	leds->setFiscal(LedInterface::State_Failure);
	gotoStateDisconnect(&event);
}

void CommandLayer::procResponseError(uint8_t errorCode) {
	Fiscal::EventError event(deviceId);
	switch(errorCode) {
	case CommandError_WrongPassword: {
		LOG_ERROR(LOG_FR, "Wrong password");
		event.code = ConfigEvent::Type_FiscalPassword;
		break;
	}
	case CommandError_PrinterNotFound: {
		LOG_ERROR(LOG_FR, "PrinterNotFound");
		event.code = ConfigEvent::Type_PrinterNotFound;
		break;
	}
	case CommandError_PrinterNoPaper: {
		LOG_ERROR(LOG_FR, "PrinterNoPaper");
		event.code = ConfigEvent::Type_PrinterNoPaper;
		break;
	}
	default: {
		LOG_ERROR(LOG_FR, "Unknown error " << errorCode);
		event.code = ConfigEvent::Type_FiscalUnknownError;
		event.data << errorCode;
	}
	}
	context->removeAll();
	context->addError(event.code, event.data.getString());
	leds->setFiscal(LedInterface::State_Failure);
	gotoStateDisconnect(&event);
}

void CommandLayer::procDebugError(const char *file, uint16_t line) {
	Fiscal::EventError event(deviceId);
	event.code = ConfigEvent::Type_FiscalLogicError;
	event.data << basename(file) << "#" << line;
	context->removeAll();
	context->addError(event.code, event.data.getString());
	leds->setFiscal(LedInterface::State_Failure);
	gotoStateDisconnect(&event);
}

}
#else
#include "AtolCommandLayer.h"
#include "fiscal_storage/include/FiscalStorage.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "config/event/ConfigEventList.h"
#include "utils/include/DecimalPoint.h"
#include "utils/include/CodePage.h"
#include "logger/include/Logger.h"

namespace Atol {

CommandLayer::CommandLayer(TimerEngine *timers, TaskLayerInterface *taskLayer, EventEngineInterface *eventEngine) :
	timers(timers),
	taskLayer(taskLayer),
	eventEngine(eventEngine),
	state(State_Idle),
	request(ATOL_PACKET_MAX_SIZE),
	doc(256),
	envelope(EVENT_DATA_SIZE)
{
	this->taskLayer->setObserver(this);
	this->timer = timers->addTimer<CommandLayer, &CommandLayer::procTimer>(this);
	this->password = 0;
}

CommandLayer::~CommandLayer() {
	this->timers->deleteTimer(this->timer);
}

void CommandLayer::sale(Fiscal::Sale *saleData, uint32_t decimalPoint) {
	LOG_DEBUG(LOG_FR, "sale");
	if(this->state != State_Idle) {
		LOG_ERROR(LOG_FR, "Wrong state " << state);
		return;
	}
	this->saleData = saleData;
	this->decimalPoint = decimalPoint;
	this->tryNumber = 0;
	gotoStateConnect();
}

void CommandLayer::getLastSale() {
	LOG_DEBUG(LOG_FR, "getLastSale");
	gotoStateLastSale();
}

void CommandLayer::closeShift() {
	LOG_DEBUG(LOG_FR, "closeShift");
	gotoStateShiftClose2();
}

void CommandLayer::procRecvData(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_FR, "procRecvData");
	LOG_TRACE_HEX(LOG_FR, data, len);
	switch(state) {
	case State_Status: stateStatusResponse(data, len); break;
	case State_SaleOpen: stateSaleOpenResponse(data, len); break;
	case State_ShiftOpen: stateShiftOpenResponse(data, len); break;
	case State_CheckOpen: stateCheckOpenResponse(data, len); break;
	case State_CheckAdd: stateCheckAddResponse(data, len); break;
	case State_FN105CheckAddStart: stateFN105CheckAddStartResponse(data, len); break;
	case State_FN105CheckAddEnd: stateFN105CheckAddEndResponse(data, len); break;
	case State_CheckClose: stateCheckCloseResponse(data, len); break;
	case State_CheckReset: stateCheckResetResponse(data, len); break;
	case State_ModeClose: stateModeCloseResponse(data, len); break;
	case State_ShiftClose1: stateShiftClose1Response(data, len); break;
	case State_ShiftClose2: stateShiftClose2Response(data, len); break;
	case State_ShiftClose3: stateShiftClose3Response(data, len); break;
	case State_ShiftClose5: stateShiftClose5Response(data, len); break;
	case State_LastSale: stateLastSaleResponse(data, len); break;
	case State_DocSize: stateDocSizeResponse(data, len); break;
	case State_DocData: stateDocDataResponse(data, len); break;
	default: LOG_ERROR(LOG_FR, "Unwaited data " << state);
	}
}

void CommandLayer::procRecvError(TaskLayerObserver::Error error) {
	LOG_DEBUG(LOG_FR, "procRecvError " << error);
	switch(state) {
	case State_Idle: return;
	case State_Connect: {
		switch(error) {
		case TaskLayerObserver::Error_OK: {
			gotoStateStatus();
			return;
		}
		default: {
			state = State_Idle;
			Fiscal::EventError event;
			event.code = ConfigEvent::Type_FiscalConnectError;
			eventEngine->transmit(&event);
			return;
		}
		}
	}
	default: {
		state = State_Idle;
		Fiscal::EventError event;
		event.code = ConfigEvent::Type_FiscalConnectError;
		eventEngine->transmit(&event);
		return;
	}
	}
}

void CommandLayer::procTimer() {
	LOG_DEBUG(LOG_FR, "procTimer");
	switch(state) {
	case State_ShiftClose4: stateShiftClose4Timeout(); break;
	default: LOG_ERROR(LOG_FR, "Unwaited timeout " << state);
	}
}

void CommandLayer::gotoStateConnect() {
	LOG_DEBUG(LOG_FR, "gotoStateConnect");
	taskLayer->connect("192.168.1.210", 5555, TcpIp::Mode_TcpIp);
	state = State_Connect;
}

void CommandLayer::gotoStateStatus() {
	LOG_DEBUG(LOG_FR, "gotoStateStatus");
	Request *req = (Request*)request.getData();
	req->password.set(password);
	req->command = Command_Status;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_Status;
}

void CommandLayer::stateStatusResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateStatusResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	if(data[0] == 0x55) {
		Response *resp = (Response*)data;
		LOG_ERROR(LOG_FR, "Command failed " << dataLen);
		procError(resp->errorCode);
		return;
	}
	respLen = sizeof(StatusResponse);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	StatusResponse *resp = (StatusResponse*)data;
	LOG_INFO(LOG_FR, "resultCode=" << resp->resultCode);
	LOG_INFO(LOG_FR, "cashier=" << resp->cashier.get());
	LOG_INFO(LOG_FR, "number=" << resp->number);
	LOG_INFO(LOG_FR, "data=" << resp->dataYear.get() << "." << resp->dataMonth.get() << "." << resp->dataDay.get());
	LOG_INFO(LOG_FR, "time=" << resp->timeHour.get() << ":" << resp->timeMinute.get() << ":" << resp->timeSecond.get());
	LOG_INFO(LOG_FR, "flags=" << resp->flags);
	LOG_INFO(LOG_FR, "serialNumber=" << resp->serialNumber.get());
	LOG_INFO(LOG_FR, "deviceId=" << resp->deviceId);
	LOG_INFO(LOG_FR, "DeviceVersion=" << (char)resp->deviceVersionMajor << "." << (char)resp->deviceVersionMinor);
	LOG_INFO(LOG_FR, "deviceMode=" << resp->deviceMode);
	LOG_INFO(LOG_FR, "checkNumber=" << resp->checkNumber.get());
	LOG_INFO(LOG_FR, "shiftNumber=" << resp->shiftNumber.get());
	LOG_INFO(LOG_FR, "checkState=" << resp->checkState);
	LOG_INFO(LOG_FR, "checkSum=" << resp->checkSum.get());
	LOG_INFO(LOG_FR, "decimalPoint=" << resp->decimalPoint);
	LOG_INFO(LOG_FR, "interfaceId=" << resp->interfaceId);

	uint8_t mode = resp->deviceMode & Mode_Mask;
	if(mode == Mode_Idle) {
		gotoStateSaleOpen();
		return;
	} else if(mode == Mode_Sale) {
		if(resp->flags & ModeFlag_ShiftOpened) {
			gotoStateCheckOpen();
			return;
		} else {
			gotoStateShiftOpen();
			return;
		}
	} else {
		gotoStateModeClose();
		return;
	}
}

void CommandLayer::stateDeviceInfoResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateDeviceInfoResponse");
	uint16_t respLen = sizeof(DeviceInfoResponse);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	DeviceInfoResponse *resp = (DeviceInfoResponse*)data;
	LOG_INFO(LOG_FR, "ErrorCode=" << resp->errorCode);
	LOG_INFO(LOG_FR, "ProtocolVersion=" << resp->protocolVersion);
	LOG_INFO(LOG_FR, "DeviceType=" << resp->deviceType);
	LOG_INFO(LOG_FR, "DeviceMode=" << resp->deviceMode[0] << "," << resp->deviceMode[1]);
	LOG_INFO(LOG_FR, "DeviceVersion=" << resp->deviceVersionMajor.get() << "." << resp->deviceVersionMinor.get() << "." << resp->deviceVersionBuild.get());
	LOG_INFO(LOG_FR, "DeviceName=");
	convertCp866ToWin1251(resp->deviceName, dataLen - offsetof(DeviceInfoResponse, deviceName));
	LOG_INFO_STR(LOG_FR, resp->deviceName, dataLen - offsetof(DeviceInfoResponse, deviceName));
}

void CommandLayer::gotoStateSaleOpen() {
	LOG_DEBUG(LOG_FR, "gotoStateSaleOpen");
	ModeOpenRequest *req = (ModeOpenRequest*)request.getData();
	req->password1.set(password);
	req->command = Command_ModeOpen;
	req->mode = Mode_Sale;
	req->password2.set(1); // todo: ввод пароля из параметра
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_SaleOpen;
}

void CommandLayer::stateSaleOpenResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateSaleOpenResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	if(data[0] != 0x55) {
		LOG_ERROR(LOG_FR, "Unwaited response type " << data[0]);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateStatus();
		return;
	} else if(resp->errorCode == CommandError_ShiftMore24) {
		gotoStateShiftClose1();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateShiftOpen() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftOpen");
	ShiftOpenRequest *req = (ShiftOpenRequest*)request.getData();
	req->password.set(password);
	req->command = Command_ShiftOpen;
	req->flags = 0;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_ShiftOpen;
}

void CommandLayer::stateShiftOpenResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateShiftOpenResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateStatus();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateCheckOpen() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckOpen");
	CheckOpenRequest *req = (CheckOpenRequest*)request.getData();
	req->password.set(password);
	req->command = Command_CheckOpen;
	req->flags = 0;
	req->checkType = CheckType_Sale;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_CheckOpen;
}

void CommandLayer::stateCheckOpenResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateCheckOpenResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateFN105CheckAddStart();
		return;
	} else if(resp->errorCode == CommandError_ShiftMore24) {
		gotoStateShiftClose1();
		return;
	} else if(resp->errorCode == CommandError_CheckAlreadyOpened) {
		gotoStateCheckReset();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateCheckAdd() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckAdd");
	uint32_t price = convertDecimalPoint(decimalPoint, FISCAL_DECIMAL_POINT, saleData->price);
	CheckAddRequest *req = (CheckAddRequest*)request.getData();
	req->password.set(password);
	req->command = Command_CheckAddSale;
	req->flags = 0;
	req->setName(saleData->name.get());
	req->price.set(price);
	req->number.set(1000);
	req->discountType = 0;
	req->discountSign = 0;
	req->discountSize.set(0);
	req->tax = 0;
	req->section.set(0);
	req->setText("");
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_CheckAdd;
}

void CommandLayer::stateCheckAddResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateCheckAddResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateCheckClose();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateFN105CheckAddStart() {
	LOG_DEBUG(LOG_FR, "gotoStateFN105CheckAddStart");
	FN105CheckAddStartRequest *req = (FN105CheckAddStartRequest*)request.getData();
	req->password.set(password);
	req->command = Command_FN105CheckAddStart;
	req->flags = 0;
	req->param = 1;
	req->reserved = 0;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_FN105CheckAddStart;
}

void CommandLayer::stateFN105CheckAddStartResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateFN105CheckAddStartResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateFN105CheckAddEnd();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateFN105CheckAddEnd() {
	LOG_DEBUG(LOG_FR, "gotoStateFN105CheckAddEnd");
	uint32_t price = convertDecimalPoint(decimalPoint, FISCAL_DECIMAL_POINT, saleData->price);
	FN105CheckAddEndRequest *req = (FN105CheckAddEndRequest*)request.getData();
	req->password.set(password);
	req->command = Command_FN105CheckAddEnd;
	req->flags = 0;
	req->price.set(price);
	req->number.set(1000);
	req->value.set(price);
	req->taxRate = FiscalStorage::convertTaxRate2FN105(saleData->taxRate);
	req->taxSum.set(0); // ККТ считает налог самостоятельно
	req->section.set(0); // если нет необходимости использовать секции, то 0
	req->saleSubject.set(1);
	req->saleMethod.set(4);
	req->discountSign = 0;
	req->discountSize.set(0);
	req->reserved[0] = 0;
	req->reserved[1] = 0;
	uint16_t nameLen = req->setName(saleData->name.get());
	request.setLen(sizeof(*req) + nameLen);
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_FN105CheckAddEnd;
}

void CommandLayer::stateFN105CheckAddEndResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateFN105CheckAddEndResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateCheckClose();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateCheckClose() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckClose");
	CheckCloseRequest *req = (CheckCloseRequest*)request.getData();
	req->password.set(password);
	req->command = Command_CheckClose;
	req->flags = 0;
	req->paymentType.set(Atol::convertPaymentType2Atol(saleData->paymentType));
	req->enterSum.set(convertDecimalPoint(decimalPoint, FISCAL_DECIMAL_POINT, saleData->credit));
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_CheckClose;
}

void CommandLayer::stateCheckCloseResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateCheckCloseResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		LOG_INFO(LOG_FR, "CheckClose complete!");
		state = State_Idle;
		EventInterface event(Fiscal::Register::Event_CommandOK);
		eventEngine->transmit(&event);
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateCheckReset() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckReset");
	Request *req = (Request*)request.getData();
	req->password.set(password);
	req->command = Command_CheckReset;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_CheckReset;
}

void CommandLayer::stateCheckResetResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateCheckResetResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateStatus();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateModeClose() {
	LOG_DEBUG(LOG_FR, "gotoStateModeClose");
	Request *req = (Request*)request.getData();
	req->password.set(password);
	req->command = Command_ModeClose;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_ModeClose;
}

void CommandLayer::stateModeCloseResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateModeCloseResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateStatus();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateShiftClose1() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftClose1");
	Request *req = (Request*)request.getData();
	req->password.set(password);
	req->command = Command_ModeClose;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_ShiftClose1;
}

void CommandLayer::stateShiftClose1Response(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateShiftClose1Response");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateShiftClose2();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateShiftClose2() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftClose2");
	ModeOpenRequest *req = (ModeOpenRequest*)request.getData();
	req->password1.set(password);
	req->command = Command_ModeOpen;
	req->mode = Mode_ReportZ;
	req->password2.set(30); // todo: ввод пароля из параметра
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_ShiftClose2;

}

void CommandLayer::stateShiftClose2Response(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateShiftClose2Response");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	if(data[0] != 0x55) {
		LOG_ERROR(LOG_FR, "Unwaited response type " << data[0]);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateShiftClose3();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateShiftClose3() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftClose3");
	Request *req = (Request*)request.getData();
	req->password.set(password);
	req->command = Command_ShiftClose;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_ShiftClose3;
}

void CommandLayer::stateShiftClose3Response(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateShiftClose3Response");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *resp = (Response*)data;
	if(resp->errorCode == CommandError_OK) {
		gotoStateShiftClose4();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Command failed " << resp->errorCode);
		procError(resp->errorCode);
		return;
	}
}

void CommandLayer::gotoStateShiftClose4() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftClose4");
	timer->start(1000);
	state = State_ShiftClose4;
}

void CommandLayer::stateShiftClose4Timeout() {
	LOG_DEBUG(LOG_FR, "stateShiftClose4Timeout");
	gotoStateShiftClose5();
}

void CommandLayer::gotoStateShiftClose5() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftClose5");
	Request *req = (Request*)request.getData();
	req->password.set(password);
	req->command = Command_Status;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_ShiftClose5;
}

void CommandLayer::stateShiftClose5Response(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateShiftClose4Response");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	if(data[0] == 0x55) {
		Response *resp = (Response*)data;
		procError(resp->errorCode);
		return;
	}
	respLen = sizeof(StatusResponse);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}

	StatusResponse *resp = (StatusResponse*)data;
	LOG_INFO(LOG_FR, "resultCode=" << resp->resultCode);
	LOG_INFO(LOG_FR, "cashier=" << resp->cashier.get());
	LOG_INFO(LOG_FR, "number=" << resp->number);
	LOG_INFO(LOG_FR, "data=" << resp->dataYear.get() << "." << resp->dataMonth.get() << "." << resp->dataDay.get());
	LOG_INFO(LOG_FR, "time=" << resp->timeHour.get() << ":" << resp->timeMinute.get() << ":" << resp->timeSecond.get());
	LOG_INFO(LOG_FR, "flags=" << resp->flags);
	LOG_INFO(LOG_FR, "serialNumber=" << resp->serialNumber.get());
	LOG_INFO(LOG_FR, "deviceId=" << resp->deviceId);
	LOG_INFO(LOG_FR, "DeviceVersion=" << (char)resp->deviceVersionMajor << "." << (char)resp->deviceVersionMinor);
	LOG_INFO(LOG_FR, "deviceMode=" << resp->deviceMode);
	LOG_INFO(LOG_FR, "checkNumber=" << resp->checkNumber.get());
	LOG_INFO(LOG_FR, "shiftNumber=" << resp->shiftNumber.get());
	LOG_INFO(LOG_FR, "checkState=" << resp->checkState);
	LOG_INFO(LOG_FR, "checkSum=" << resp->checkSum.get());
	LOG_INFO(LOG_FR, "decimalPoint=" << resp->decimalPoint);
	LOG_INFO(LOG_FR, "interfaceId=" << resp->interfaceId);

	uint8_t mode = resp->getDeviceMode();
	uint8_t submode = resp->getDeviceSubMode();
	if((mode == Mode_ReportZ && submode == 2) || (mode = Mode_Addition && submode == 1)) {
		gotoStateShiftClose4();
		return;
	} else {
		gotoStateStatus();
		return;
	}
}

void CommandLayer::gotoStateLastSale() {
	LOG_DEBUG(LOG_FR, "gotoStateLastSale");
	RegisterReadRequest *req = (RegisterReadRequest*)request.getData();
	req->password.set(password);
	req->command = Command_RegisterRead;
	req->reg = Register_LastSale;
	req->param1 = 0;
	req->param2 = 0;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_LastSale;
}

void CommandLayer::stateLastSaleResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateLastSaleResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
#if 0
	if(data[0] == 0x55) {
		Response *resp = (Response*)data;
		switch(resp->errorCode) {
		default: LOG_ERROR(LOG_FR, "Unknown error " << resp->errorCode); return;
		}
	}
#endif
	respLen = sizeof(RegisterReadResponse);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}

	RegisterReadResponse *resp = (RegisterReadResponse*)data;
	LOG_INFO(LOG_FR, "resultCode=" << resp->resultCode);
	LOG_INFO(LOG_FR, "cashier=" << resp->errorCode);
	LOG_INFO(LOG_FR, "fdNumber=" << resp->fdNumber.get());
	LOG_INFO(LOG_FR, "type=" << resp->type);
	LOG_INFO(LOG_FR, "total=" << resp->total.get());
	LOG_INFO_WORD(LOG_FR, "datetime="); LOG_DEBUG_HEX(LOG_FR, resp->datetime.value, sizeof(resp->datetime.value));
	LOG_INFO_WORD(LOG_FR, "fiscalSign="); LOG_DEBUG_HEX(LOG_FR, resp->fiscalSign.value, sizeof(resp->fiscalSign.value));
	docNumber = resp->fdNumber.get();
	gotoStateDocSize();
}

void CommandLayer::gotoStateDocSize() {
	LOG_DEBUG(LOG_FR, "gotoStateDocSize");
	FSDocSizeRequest *req = (FSDocSizeRequest*)request.getData();
	req->password.set(password);
	req->command = Command_FiscalStorage;
	req->subcommand = FSSubcommand_DocSize;
	req->docNumber.set(docNumber);
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_DocSize;
}

void CommandLayer::stateDocSizeResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateDocSizeResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *hdr = (Response*)data;
	if(data[0] != 0x55 || hdr->errorCode != Error_OK) {
		LOG_ERROR(LOG_FR, "Command failed " << hdr->errorCode);
		procError(hdr->errorCode);
		return;
	}
	respLen = sizeof(FSDocSizeResponse);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}

	FSDocSizeResponse *resp = (FSDocSizeResponse*)data;
	LOG_INFO(LOG_FR, "resultCode=" << resp->resultCode);
	LOG_INFO(LOG_FR, "errorCode=" << resp->errorCode);
	LOG_INFO(LOG_FR, "docType=" << resp->docType.get());
	LOG_INFO(LOG_FR, "docSize=" << resp->docSize.get());
	docLen = 0;
	docSize = resp->docSize.get();
	doc.clear();
	gotoStateDocData();
}

void CommandLayer::gotoStateDocData() {
	LOG_DEBUG(LOG_FR, "gotoStateDocData");
	SubcommandRequest *req = (SubcommandRequest*)request.getData();
	req->password.set(password);
	req->command = Command_FiscalStorage;
	req->subcommand = FSSubcommand_DocData;
	request.setLen(sizeof(*req));
	taskLayer->sendRequest(request.getData(), request.getLen());
	state = State_DocData;
}

void CommandLayer::stateDocDataResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateDocDataResponse");
	uint16_t respLen = sizeof(Response);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}
	Response *hdr = (Response*)data;
	if(data[0] != 0x55 || hdr->errorCode != Error_OK) {
		LOG_ERROR(LOG_FR, "Command failed " << hdr->errorCode);
		procError(hdr->errorCode);
		return;
	}
	respLen = sizeof(FSDocDataResponse);
	if(respLen > dataLen) {
		LOG_ERROR(LOG_FR, "Wrong response length " << respLen << "<>" << dataLen);
		procDebug(__FILE__, __LINE__);
		return;
	}

	FSDocDataResponse *resp = (FSDocDataResponse*)data;
	LOG_INFO(LOG_FR, "resultCode=" << resp->resultCode);
	LOG_INFO(LOG_FR, "errorCode=" << resp->errorCode);
	doc.add(resp->data, dataLen - respLen);
	docLen += (dataLen - respLen);
	if(docLen >= docSize) {
		printSTLV(doc.getData(), doc.getLen());
		state = State_Idle;
	} else {
		gotoStateDocData();
	}
}

void CommandLayer::printSTLV(const uint8_t *data, const uint16_t dataLen) {
	uint16_t procLen = 0;
	while(procLen < dataLen) {
		procLen += printTLV(data + procLen, dataLen - procLen);
	}
}

uint16_t CommandLayer::printTLV(const uint8_t *data, const uint16_t dataLen) {
	if(dataLen < sizeof(FiscalStorage::Header)) {
		return dataLen;
	}
	FiscalStorage::Header *h = (FiscalStorage::Header*)data;
	LOG_INFO_WORD(LOG_FR, "tag=" << h->tag.get() << ", len=" << h->len.get() << ", data=");
	LOG_INFO_HEX(LOG_FR, h->data, h->len.get());
	return (sizeof(FiscalStorage::Header) + h->len.get());
}

void CommandLayer::procError(uint8_t returnCode) {
	state = State_Idle;
	Fiscal::EventError event;
	switch(returnCode) {
	case CommandError_WrongPassword: {
		LOG_ERROR(LOG_FR, "Wrong password");
		event.code = ConfigEvent::Type_FiscalPassword;
		break;
	}
	case CommandError_PrinterNotFound: {
		LOG_ERROR(LOG_FR, "PrinterNotFound");
		event.code = ConfigEvent::Type_PrinterNotFound;
		break;
	}
	case CommandError_PrinterNoPaper: {
		LOG_ERROR(LOG_FR, "PrinterNoPaper");
		event.code = ConfigEvent::Type_PrinterNoPaper;
		break;
	}
	default: {
		LOG_ERROR(LOG_FR, "Unknown error " << returnCode);
		event.code = ConfigEvent::Type_FiscalUnknownError;
		event.data << returnCode;
	}
	}
	eventEngine->transmit(&event);
}

void CommandLayer::procDebug(const char *file, uint16_t line) {
	state = State_Idle;
	Fiscal::EventError event;
	event.code = ConfigEvent::Type_FiscalLogicError;
	event.data << basename(file) << "#" << line;
	eventEngine->transmit(&event);
}

}
#endif
