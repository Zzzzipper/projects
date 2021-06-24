#include "include/RpSystem1Fa.h"
#include "fiscal_register/rp_system_1fa/RpSystem1FaProtocol.h"
#include "fiscal_register/rp_system_1fa/RpSystem1FaPacketLayerCom.h"
#include "fiscal_register/rp_system_1fa/RpSystem1FaPacketLayerTcp.h"
#include "config/include/ConfigModem.h"
#include "utils/include/DecimalPoint.h"
#include "logger/include/Logger.h"

namespace RpSystem1Fa {

FiscalRegister::FiscalRegister(uint16_t interfaceType, AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine) :
	timers(timers),
	eventEngine(eventEngine),
	deviceId(eventEngine),
	state(State_Idle),
	request(RPSYSTEM1FA_PACKET_MAX_SIZE)
{
	this->timer = timers->addTimer<FiscalRegister, &FiscalRegister::procTimer>(this);
	if(interfaceType == ConfigFiscal::KktInterface_Ethernet) {
		this->packetLayer = new PacketLayerTcp(timers, uart);
	} else {
		this->packetLayer = new PacketLayerCom(timers, uart);
	}
	this->packetLayer->setObserver(this);
	this->password = 30;
}

FiscalRegister::~FiscalRegister() {
	delete packetLayer;
	timers->deleteTimer(timer);
}

EventDeviceId FiscalRegister::getDeviceId() {
	return deviceId;
}

#if 0
void FiscalRegister::sale(const char *name, PaymentType paymentType, uint32_t credit, uint32_t value, uint32_t decimalPoint) {
	LOG_DEBUG(LOG_FR, "sale");
	if(state != State_Idle) {
		LOG_ERROR(LOG_FR, "Wrong state " << state);
		return;
	}
	this->name = name;
//	this->credit = convertDecimalPoint(decimalPoint, FISCAL_DECIMAL_POINT, credit);
	this->price = convertDecimalPoint(decimalPoint, FISCAL_DECIMAL_POINT, value);
//	this->tryNumber = 0;
	gotoStateStatus();
}
#else
void FiscalRegister::sale(Fiscal::Sale *saleData, uint32_t decimalPoint) {
	LOG_DEBUG(LOG_FR, "sale");
	if(state != State_Idle) {
		LOG_ERROR(LOG_FR, "Wrong state " << state);
		return;
	}
	Fiscal::Product *product = saleData->getProduct(0);
	this->name.set(product->name.get());
//	this->credit = convertDecimalPoint(decimalPoint, FISCAL_DECIMAL_POINT, credit);
	this->price = convertDecimalPoint(decimalPoint, FISCAL_DECIMAL_POINT, product->price);
//	this->tryNumber = 0;
	gotoStateStatus();
}
#endif

void FiscalRegister::closeShift() {
//	LOG_DEBUG(LOG_FR, "closeShift");
//	gotoStateShiftClose2();
}

void FiscalRegister::procRecvData(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_FR, "procRecvData " << len);
	LOG_TRACE_HEX(LOG_FR, data, len);
	switch(state) {
	case State_Status: stateStatusResponse(data, len); break;
	case State_ShiftOpen: stateShiftOpenResponse(data, len); break;
	case State_CheckOpen: stateCheckOpenResponse(data, len); break;
	case State_CheckAdd: stateCheckAddResponse(data, len); break;
	case State_CheckClose: stateCheckCloseResponse(data, len); break;
	case State_CheckCancel: stateCheckCancelResponse(data, len); break;
	case State_ShiftClose: stateShiftCloseResponse(data, len); break;
	default: LOG_ERROR(LOG_FR, "Unwaited data " << state);
	}
}

void FiscalRegister::gotoStateStatus() {
	LOG_DEBUG(LOG_FR, "gotoStateStatus");

	Request *req = (Request*)request.getData();
	req->command = Command_Status;
	req->password.set(password);
	request.setLen(sizeof(*req));

	packetLayer->sendPacket(&request);
	state = State_Status;
}

void FiscalRegister::stateStatusResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateStatusResponse");
	uint16_t headerSize = sizeof(Response);
	if(headerSize > dataLen) {
		LOG_ERROR(LOG_FR, "Response too small (exp=" << headerSize << ", act=" << dataLen << ")");
		procError();
		return;
	}
	Response *header = (Response*)data;
	if(header->command != Command_Status) {
		LOG_ERROR(LOG_FR, "Wrong response result " << header->command);
		procError();
		return;
	}
	if(header->errorCode != Error_OK) {
		LOG_ERROR(LOG_FR, "Unhandled error " << header->errorCode);
		procError();
		return;
	}

	uint16_t respSize = sizeof(StatusResponse);
	if(respSize > dataLen) {
		LOG_ERROR(LOG_FR, "Response too small (exp=" << respSize << ", act=" << dataLen << ")");
		procError();
		return;
	}
	StatusResponse *resp = (StatusResponse*)data;
	LOG_INFO(LOG_FR, "resultCode=" << resp->errorCode);
	LOG_INFO(LOG_FR, "software " << resp->softwareVersion[0] << "." << resp->softwareVersion[1] << "." << resp->softwareBuild.get());
	LOG_INFO(LOG_FR, "state=" << resp->state);
	LOG_INFO(LOG_FR, "lifePhase=" << resp->lifePhase);
	LOG_INFO(LOG_FR, "printerLastState=" << resp->printerLastState);
	LOG_INFO(LOG_FR, "fnDate=20" << resp->fnDate[2] << "." << resp->fnDate[1] << "." << resp->fnDate[0]);
	LOG_INFO(LOG_FR, "fnFlags=" << resp->fnFlags);
	LOG_INFO(LOG_FR, "printerState=" << resp->printerState);
	LOG_INFO(LOG_FR, "date=20" << resp->date[2] << "." << resp->date[1] << "." << resp->date[0]);
	LOG_INFO(LOG_FR, "time=" << resp->time[0] << ":" << resp->time[0] << ":" << resp->time[0]);

	if(resp->lifePhase != LifePhase_Fiscal) {
		LOG_ERROR(LOG_FR, "Wrong life phase (exp=3, act=" << resp->lifePhase << ")");
		procError();
		return;
	}
	if(resp->printerState != PrinterState_Idle) {
		LOG_ERROR(LOG_FR, "Wrong printer state (exp=0, act=" << resp->printerState << ")");
		procError();
		return;
	}
	if(resp->state == FrState_ShiftOpened) {
		gotoStateCheckOpen();
		return;
	} else if(resp->state == FrState_Idle || resp->state == FrState_ShiftClosed) {
		gotoStateShiftOpen();
		return;
	} else if(resp->state == FrState_Shift24More) {
		gotoStateShiftClose();
		return;
	} else if(resp->state == FrState_DocumentIn) {
		gotoStateCheckCancel();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Unhandled fr state " << resp->state);
		procError();
		return;
	}
}

void FiscalRegister::gotoStateShiftOpen() {
	LOG_DEBUG(LOG_FR, "gotoStateOpenShift");

	Request *req = (Request*)request.getData();
	req->command = Command_ShiftOpen;
	req->password.set(password);
	request.setLen(sizeof(*req));

	packetLayer->sendPacket(&request);
	state = State_ShiftOpen;
}

void FiscalRegister::stateShiftOpenResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateShiftOpenResponse");
	uint16_t headerSize = sizeof(Response);
	if(headerSize > dataLen) {
		LOG_ERROR(LOG_FR, "Response too small (exp=" << headerSize << ", act=" << dataLen << ")");
		procError();
		return;
	}
	Response *header = (Response*)data;
	if(header->command != Command_ShiftOpen) {
		LOG_ERROR(LOG_FR, "Wrong response result " << header->command);
		procError();
		return;
	}
	if(header->errorCode == Error_OK) {
		gotoStateStatus();
	} else {
		LOG_ERROR(LOG_FR, "Unhandled error " << header->errorCode);
		procError();
		return;
	}
}

void FiscalRegister::gotoStateCheckOpen() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckOpen");

	CheckOpenRequest *req = (CheckOpenRequest*)request.getData();
	req->command = Command_CheckOpen;
	req->password.set(password);
	req->documentType = DocumentType_In;
	request.setLen(sizeof(*req));

	packetLayer->sendPacket(&request);
	state = State_CheckOpen;
}

void FiscalRegister::stateCheckOpenResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateCheckOpenResponse");
	uint16_t headerSize = sizeof(Response);
	if(headerSize > dataLen) {
		LOG_ERROR(LOG_FR, "Response too small (exp=" << headerSize << ", act=" << dataLen << ")");
		procError();
		return;
	}
	Response *header = (Response*)data;
	if(header->command != Command_CheckOpen) {
		LOG_ERROR(LOG_FR, "Wrong response result " << header->command);
		procError();
		return;
	}
	if(header->errorCode == Error_OK) {
		gotoStateCheckAdd();
		return;
	} else if(header->errorCode == Error_ShiftClosed) {
		gotoStateShiftOpen();
		return;
	} else if(header->errorCode == Error_Shift24More) {
		gotoStateShiftClose();
		return;
	} else if(header->errorCode == Error_DocumentOpened) {
		gotoStateCheckCancel();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Unhandled error " << header->errorCode);
		procError();
		return;
	}
}

void FiscalRegister::gotoStateCheckAdd() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckAdd");

	CheckAddRequest *req = (CheckAddRequest*)request.getData();
	req->command = Command_CheckAddSale;
	req->password.set(password);
	req->number.set(1000); // количество измеряется в тысячных
	req->price.set(price);
	req->paymentType = PaymentType_FullPayAndTake; // todo: уточнить что сюда задавать
	req->taxType = 3; // todo: выяснить что сюда задавать
	req->setName(name.getString(), name.getLen());
	request.setLen(sizeof(*req) + name.getLen());

	packetLayer->sendPacket(&request);
	state = State_CheckAdd;
}

void FiscalRegister::stateCheckAddResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateCheckAddResponse");
	uint16_t headerSize = sizeof(Response);
	if(headerSize > dataLen) {
		LOG_ERROR(LOG_FR, "Response too small (exp=" << headerSize << ", act=" << dataLen << ")");
		procError();
		return;
	}
	Response *header = (Response*)data;
	if(header->command != Command_CheckAddSale) {
		LOG_ERROR(LOG_FR, "Wrong response result " << header->command);
		procError();
		return;
	}
	if(header->errorCode == Error_OK) {
		gotoStateCheckClose();
		return;
	} else if(header->errorCode == Error_Shift24More) {
		gotoStateShiftClose();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Unhandled error " << header->errorCode);
		procError();
		return;
	}
}

void FiscalRegister::gotoStateCheckClose() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckClose");

	CheckCloseRequest *req = (CheckCloseRequest*)request.getData();
	req->command = Command_CheckClose;
	req->password.set(password);
	req->cash.set(price);
	req->cashless1.set(0);
	req->cashless2.set(0);
	req->cashless3.set(0);
	request.setLen(sizeof(*req));

	packetLayer->sendPacket(&request);
	state = State_CheckClose;
}

void FiscalRegister::stateCheckCloseResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateCheckCloseResponse");
	uint16_t headerSize = sizeof(Response);
	if(headerSize > dataLen) {
		LOG_ERROR(LOG_FR, "Response too small (exp=" << headerSize << ", act=" << dataLen << ")");
		procError();
		return;
	}
	Response *header = (Response*)data;
	if(header->command != Command_CheckClose) {
		LOG_ERROR(LOG_FR, "Wrong response result " << header->command);
		procError();
		return;
	}
	if(header->errorCode == Error_OK) {
		LOG_INFO(LOG_FR, "Check successfully created");
		state = State_Idle;
		EventInterface event(deviceId, Event_CommandOK);
		eventEngine->transmit(&event);
		return;
	} else if(header->errorCode == Error_Shift24More) {
		gotoStateShiftClose();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Unhandled error " << header->errorCode);
		procError();
		return;
	}
}

void FiscalRegister::gotoStateCheckCancel() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckCancel");

	Request *req = (Request*)request.getData();
	req->command = Command_CheckCancel;
	req->password.set(password);
	request.setLen(sizeof(*req));

	packetLayer->sendPacket(&request);
	state = State_CheckCancel;
}

void FiscalRegister::stateCheckCancelResponse(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "stateCheckCancelResponse");
	uint16_t headerSize = sizeof(Response);
	if(headerSize > dataLen) {
		LOG_ERROR(LOG_FR, "Response too small (exp=" << headerSize << ", act=" << dataLen << ")");
		procError();
		return;
	}
	Response *header = (Response*)data;
	if(header->command != Command_CheckCancel) {
		LOG_ERROR(LOG_FR, "Wrong response result " << header->command);
		procError();
		return;
	}
	if(header->errorCode == Error_OK) {
		gotoStateStatus();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Unhandled error " << header->errorCode);
		procError();
		return;
	}
}

void FiscalRegister::gotoStateShiftClose() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftClose");

	Request *req = (Request*)request.getData();
	req->command = Command_ShiftClose;
	req->password.set(password);
	request.setLen(sizeof(*req));

	packetLayer->sendPacket(&request);
	state = State_ShiftClose;
}

void FiscalRegister::stateShiftCloseResponse(const uint8_t *data, const uint16_t dataLen) {
	uint16_t headerSize = sizeof(Response);
	if(headerSize > dataLen) {
		LOG_ERROR(LOG_FR, "Response too small (exp=" << headerSize << ", act=" << dataLen << ")");
		procError();
		return;
	}
	Response *header = (Response*)data;
	if(header->command != Command_ShiftClose) {
		LOG_ERROR(LOG_FR, "Wrong response result " << header->command);
		procError();
		return;
	}
	if(header->errorCode == Error_OK) {
		gotoStateStatus();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Unhandled error " << header->errorCode);
		procError();
		return;
	}
}

void FiscalRegister::procRecvError() {
	LOG_DEBUG(LOG_FR, "procRecvError");
	if(state == State_Idle) {
		LOG_ERROR(LOG_FR, "Unwaited recv error " << state);
		return;
	}
	procError();
}

void FiscalRegister::procTimer() {
	LOG_DEBUG(LOG_FR, "procTimer " << state);
	switch(state) {
	default: LOG_ERROR(LOG_FR, "Unwaited timeout " << state);
	}
}

void FiscalRegister::procError() {
	state = State_Idle;
	Fiscal::EventError event(deviceId);
	event.code = ConfigEvent::Type_FiscalUnknownError;
	eventEngine->transmit(&event);
}

}
