#include "fiscal_register/shtrih-m/include/ShtrihM.h"
#include "fiscal_register/shtrih-m/ShtrihmReceiver.h"
#include "fiscal_register/shtrih-m/ShtrihmProtocol.h"
#include "utils/include/DecimalPoint.h"
#include "logger/include/Logger.h"

#define TRY_MAX_NUMBER 10 // количество попыток настроить   “
#define SHORT_STATUS_TIMER 500

ShtrihM::ShtrihM(AbstractUart *uart, TimerEngine *timers, EventEngineInterface *eventEngine) :
	timers(timers),
	eventEngine(eventEngine),
	deviceId(eventEngine),
	state(State_Idle),
	packet(SHM_PACKET_MAX_SIZE)
{
	this->timer = timers->addTimer<ShtrihM, &ShtrihM::procTimer>(this);
	this->receiver = new ShtrihmReceiver(timers, uart, this);
	this->password = 30;
}

ShtrihM::~ShtrihM() {
	delete this->receiver;
	timers->deleteTimer(this->timer);
}

EventDeviceId ShtrihM::getDeviceId() {
	return deviceId;
}

void ShtrihM::sale(Fiscal::Sale *saleData, uint32_t decimalPoint) {
	LOG_DEBUG(LOG_FR, "sale");
	if(this->state != State_Idle) {
		LOG_ERROR(LOG_FR, "Wrong state " << state);
		return;
	}
	Fiscal::Product *product = saleData->getProduct(0);
	this->name.set(product->name.get());
	this->price = convertDecimalPoint(decimalPoint, FISCAL_DECIMAL_POINT, product->price);
	this->paymentType = saleData->paymentType;
	this->credit = convertDecimalPoint(decimalPoint, FISCAL_DECIMAL_POINT, saleData->credit);
	this->tryNumber = 0;
	gotoStateShortStatus();
}

void ShtrihM::closeShift() {
	LOG_DEBUG(LOG_FR, "closeShift");
	if(this->state != State_Idle) {
		LOG_ERROR(LOG_FR, "Wrong state " << state);
		return;
	}
	gotoStateShiftClose2();
}

void ShtrihM::procTimer() {
	LOG_DEBUG(LOG_FR, "procTimer");
	switch(state) {
	case State_WaitKKT: gotoStateShortStatus(); break;
	default: LOG_ERROR(LOG_FR, "Unwaited timeout");
	}
}

void ShtrihM::procRecvData(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_FR, "procRecvData");
	LOG_TRACE_HEX(LOG_FR, data, len);
	switch(state) {
	case State_ShortStatus: stateShortStatusResponse(data, len); break;
	case State_PrintContinue: statePrintContinueResponse(data, len); break;
	case State_ShiftOpen: stateShiftOpenResponse(data, len); break;
	case State_CheckOpen: stateCheckOpenResponse(data, len); break;
	case State_CheckAdd: stateCheckAddResponse(data, len); break;
	case State_CheckClose: stateCheckCloseResponse(data, len); break;
	case State_CheckReset: stateCheckResetResponse(data, len); break;
	case State_ShiftClose: stateShiftCloseResponse(data, len); break;
	case State_ShiftClose2: stateShiftClose2Response(data, len); break;
	default: LOG_ERROR(LOG_FR, "Unwaited data " << state);
	}
}

void ShtrihM::procRecvError() {
	LOG_DEBUG(LOG_FR, "procRecvError");
	procError();
}

void ShtrihM::gotoStateShortStatus() {
	LOG_DEBUG(LOG_FR, "gotoStateShortStatus");
	tryNumber++;
	if(tryNumber > TRY_MAX_NUMBER) {
		LOG_ERROR(LOG_FR, "Too much config tries.");
		procError();
		return;
	}

	ShmRequest *req = (ShmRequest*)packet.getData();
	req->command = ShmCommand_ShortStatus;
	req->password.set(password);
	packet.setLen(sizeof(*req));
	receiver->sendPacket(&packet);
	state = State_ShortStatus;
}

void ShtrihM::stateShortStatusResponse(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_FR, "stateShortStatusResponse");
	uint16_t respSize = sizeof(ShmShortStatusResponse);
	if(len < respSize) {
		LOG_ERROR(LOG_FR, "Response too small: exp=" << respSize << ", act=" << len);
		procError();
		return;
	}
	ShmShortStatusResponse *resp = (ShmShortStatusResponse*)data;
	LOG_DEBUG(LOG_FR, "error_code=" << resp->errorCode << ", mode=" << resp->mode);
	switch(resp->mode) {
		case ShmKKTMode_ShiftOpened24Less: gotoStateCheckOpen(); return;
		case ShmKKTMode_ShiftOpened24More: gotoStateShiftClose(); return;
		case ShmKKTMode_ShiftClosed: gotoStateShiftOpen(); return;
		case ShmKKTMode_Document: gotoStateCheckReset(); return;
		default: gotoStateWaitKKT(); return;
	}
}

void ShtrihM::gotoStatePrintContinue() {
	LOG_DEBUG(LOG_FR, "gotoStatePrintContinue");
	ShmRequest *req = (ShmRequest*)packet.getData();
	req->command = ShmCommand_PrintContinue;
	req->password.set(password);
	packet.setLen(sizeof(*req));
	receiver->sendPacket(&packet);
	state = State_PrintContinue;
}

void ShtrihM::statePrintContinueResponse(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_FR, "statePrintContinueResponse");
	ShmPrintContinueResponse *resp = (ShmPrintContinueResponse*)data;
	uint16_t respSize = sizeof(*resp);
	if(len < respSize) {
		LOG_ERROR(LOG_FR, "Response too small: exp=" << respSize << ", act=" << len);
		procError();
		return;
	}
	if(resp->errorCode == ShmError_OK) {
		gotoStateShortStatus();
	} else if(resp->errorCode == ShmError_Printing) {
		gotoStateWaitKKT();
	} else {
		LOG_ERROR(LOG_FR, "Shift print continue failed " << resp->errorCode);
		procError();
		return;
	}
}

void ShtrihM::gotoStateShiftOpen() {
	LOG_DEBUG(LOG_FR, "gotoStateOpenShift");
	ShmRequest *req = (ShmRequest*)packet.getData();
	req->command = ShmCommand_OpenShift;
	req->password.set(password);
	packet.setLen(sizeof(*req));
	receiver->sendPacket(&packet);
	state = State_ShiftOpen;
}

void ShtrihM::stateShiftOpenResponse(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_FR, "stateOpenShiftResponse");
	ShmResponse *resp = (ShmResponse*)data;
	uint16_t respSize = sizeof(*resp);
	if(len < respSize) {
		LOG_ERROR(LOG_FR, "Response too small: exp=" << respSize << ", act=" << len);
		procError();
		return;
	}
	if(resp->errorCode == ShmError_OK) {
		gotoStateShortStatus();
	} else if(resp->errorCode == ShmError_WaitPrint) {
		gotoStatePrintContinue();
	} else if(resp->errorCode == ShmError_Printing) {
		gotoStateWaitKKT();
	} else {
		LOG_ERROR(LOG_FR, "Shift opening failed " << resp->errorCode);
		procError();
		return;
	}
}

void ShtrihM::gotoStateShiftClose() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftClose");
	ShmRequest *req = (ShmRequest*)packet.getData();
	req->command = ShmCommand_ShiftClose;
	req->password.set(password);
	packet.setLen(sizeof(*req));
	receiver->sendPacket(&packet);
	state = State_ShiftClose;
}

void ShtrihM::stateShiftCloseResponse(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_FR, "stateShiftCloseResponse");
	ShmResponse *resp = (ShmResponse*)data;
	uint16_t respSize = sizeof(*resp);
	if(len < respSize) {
		LOG_ERROR(LOG_FR, "Response too small: exp=" << respSize << ", act=" << len);
		procError();
		return;
	}
	if(resp->errorCode == ShmError_OK) {
		gotoStateShortStatus();
	} else if(resp->errorCode == ShmError_WaitPrint) {
		gotoStatePrintContinue();
	} else if(resp->errorCode == ShmError_Printing) {
		gotoStateWaitKKT();
	} else {
		LOG_ERROR(LOG_FR, "Shift closing failed " << resp->errorCode);
		procError();
		return;
	}
}

void ShtrihM::gotoStateCheckReset() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckReset");
	ShmRequest *req = (ShmRequest*)packet.getData();
	req->command = ShmCommand_CheckReset;
	req->password.set(password);
	packet.setLen(sizeof(*req));
	receiver->sendPacket(&packet);
	state = State_CheckReset;
}

void ShtrihM::stateCheckResetResponse(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_FR, "stateCheckResetResponse");
	ShmResponse *resp = (ShmResponse*)data;
	uint16_t respSize = sizeof(*resp);
	if(len < respSize) {
		LOG_ERROR(LOG_FR, "Response too small: exp=" << respSize << ", act=" << len);
		procError();
		return;
	}
	if(resp->errorCode == ShmError_OK) {
		gotoStateShortStatus();
	} else if(resp->errorCode == ShmError_WaitPrint) {
		gotoStatePrintContinue();
	} else {
		LOG_ERROR(LOG_FR, "Check reseting failed " << resp->errorCode);
		procError();
		return;
	}
}

void ShtrihM::gotoStateWaitKKT() {
	timer->start(SHORT_STATUS_TIMER);
	state = State_WaitKKT;
}

void ShtrihM::gotoStateCheckOpen() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckOpen");
	packet.clear();
	packet.addUint8(ShmCommand_CheckOpen);
	packet.addUint8(password);
	packet.addUint8(0);
	packet.addUint8(0);
	packet.addUint8(0);
	packet.addUint8(ShmDocumentType_Sale);
	receiver->sendPacket(&packet);
	state = State_CheckOpen;
}

void ShtrihM::stateCheckOpenResponse(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_FR, "stateCheckOpenResponse");
	ShmResponse *resp = (ShmResponse*)data;
	uint16_t respSize = sizeof(*resp);
	if(len < respSize) {
		LOG_ERROR(LOG_FR, "Response too small: exp=" << respSize << ", act=" << len);
		procError();
		return;
	}
	if(resp->errorCode == ShmError_OK) {
		gotoStateCheckAdd();
	} else if(resp->errorCode == ShmError_WaitPrint) {
		gotoStatePrintContinue();
	} else if(resp->errorCode == ShmError_Printing) {
		gotoStateWaitKKT();
	} else if(resp->errorCode == ShmError_Shift24More) {
		gotoStateShiftClose();
	} else if(resp->errorCode == ShmError_ShiftClosed) {
		gotoStateShiftOpen();
	} else {
		LOG_ERROR(LOG_FR, "Check open failed " << resp->errorCode);
		procError();
		return;
	}
}

/*
 оманда: 80H. ƒлина сообщени€: 60 или 20+Y1,2 байт.
  ѕароль оператора (4 байта)
   оличество (5 байт) 0000000000Е9999999999
  ÷ена (5 байт) 0000000000Е9999999999
  Ќомер отдела (1 байт) 0Е16 Ц режим свободной продажи, 255 Ц режим продажи по коду товара1,3
  Ќалог 1 (1 байт) Ђ0ї Ц нет, Ђ1їЕЂ4ї Ц налогова€ группа
  Ќалог 2 (1 байт) Ђ0ї Ц нет, Ђ1їЕЂ4ї Ц налогова€ группа
  Ќалог 3 (1 байт) Ђ0ї Ц нет, Ђ1їЕЂ4ї Ц налогова€ группа
  Ќалог 4 (1 байт) Ђ0ї Ц нет, Ђ1їЕЂ4ї Ц налогова€ группа
  “екст4,5,6,7 (40 или до Y1,2 байт) строка названи€
  товара или строка "XXXX" кода товара1,3, где XXXX = 0001Е9999
 */
void ShtrihM::gotoStateCheckAdd() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckAdd");
	uint16_t reqSize = sizeof(ShmCheckAddSaleRequest);
	if(packet.getSize() < reqSize) {
		LOG_ERROR(LOG_FR, "Request buffer too small: exp=" << reqSize << ", act=" << packet.getSize());
		procError();
		return;
	}
	ShmCheckAddSaleRequest *req = (ShmCheckAddSaleRequest*)packet.getData();
	req->command = ShmCommand_CheckAddSale;
	req->password.set(password);
	req->number.set((uint16_t)1000); // непон€тно почему измерение товаров в тыс€чных (граммах?)
	req->price.set(price); // в минимальных единицах, то есть в копейках
	req->department = 0;
	req->tax.set((uint32_t)0);
	req->setName(name.getString());
	packet.setLen(sizeof(*req));
	receiver->sendPacket(&packet);
	state = State_CheckAdd;
}

void ShtrihM::stateCheckAddResponse(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_FR, "stateCheckAddResponse");
	ShmResponse *resp = (ShmResponse*)data;
	uint16_t respSize = sizeof(*resp);
	if(len < respSize) {
		LOG_ERROR(LOG_FR, "Response too small: exp=" << respSize << ", act=" << len);
		procError();
		return;
	}
	if(resp->errorCode == ShmError_OK) {
		gotoStateCheckClose();
	} else if(resp->errorCode == ShmError_WaitPrint) {
		gotoStatePrintContinue();
	} else if(resp->errorCode == ShmError_Shift24More) {
		gotoStateShiftClose();
	} else {
		LOG_ERROR(LOG_FR, "Check add failed " << resp->errorCode);
		procError();
		return;
	}
}

void ShtrihM::gotoStateCheckClose() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckClose");
	uint16_t reqSize = sizeof(ShmCheckCloseRequest);
	if(packet.getSize() < reqSize) {
		LOG_ERROR(LOG_FR, "Request buffer too small: exp=" << reqSize << ", act=" << packet.getSize());
		procError();
		return;
	}
	ShmCheckCloseRequest *req = (ShmCheckCloseRequest*)packet.getData();
	req->command = ShmCommand_CheckClose;
	req->password.set(password);
	if(paymentType == Fiscal::Payment_Cash) {
		req->setName("Ќаличными");
		req->cash.set(credit); // в минимальных единицах, то есть в копейках
		req->value2.set((uint16_t)0);
	} else {
		req->setName("Ёлектронными");
		req->cash.set((uint16_t)0);
		req->value2.set(credit);
	}
	req->value3.set((uint16_t)0);
	req->value4.set((uint16_t)0);
	req->discont.set((uint16_t)0);
	req->tax1 = 0;
	req->tax2 = 0;
	req->tax3 = 0;
	req->tax4 = 0;
	packet.setLen(sizeof(ShmCheckCloseRequest));
	receiver->sendPacket(&packet);
	state = State_CheckClose;
}

void ShtrihM::stateCheckCloseResponse(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_FR, "stateCheckCloseResponse");
	uint16_t respSize = sizeof(ShmCheckCloseResponse);
	if(len < respSize) {
		LOG_ERROR(LOG_FR, "Response too small: exp=" << respSize << ", act=" << len);
		procError();
		return;
	}
	ShmCheckCloseResponse *resp = (ShmCheckCloseResponse*)data;
	if(resp->errorCode == ShmError_OK) {
		state = State_Idle;
		EventInterface event(deviceId, Event_CommandOK);
		eventEngine->transmit(&event);
		return;
	} else if(resp->errorCode == ShmError_WaitPrint) {
		gotoStatePrintContinue();
		return;
	} else if(resp->errorCode == ShmError_Shift24More) {
		gotoStateShiftClose();
		return;
	} else {
		LOG_ERROR(LOG_FR, "Check close failed " << resp->errorCode);
		procError();
		return;
	}
}

void ShtrihM::gotoStateShiftClose2() {
	LOG_DEBUG(LOG_FR, "gotoStateShiftClose2");
	ShmRequest *req = (ShmRequest*)packet.getData();
	req->command = ShmCommand_ShiftClose;
	req->password.set(password);
	packet.setLen(sizeof(*req));
	receiver->sendPacket(&packet);
	state = State_ShiftClose2;
}

void ShtrihM::stateShiftClose2Response(const uint8_t *data, const uint16_t len) {
	LOG_DEBUG(LOG_FR, "stateShiftClose2Response");
	ShmResponse *resp = (ShmResponse*)data;
	uint16_t respSize = sizeof(*resp);
	if(len < respSize) {
		LOG_ERROR(LOG_FR, "Response too small: exp=" << respSize << ", act=" << len);
		procError();
		return;
	}
	if(resp->errorCode == ShmError_OK || resp->errorCode == ShmError_Printing) {
		LOG_INFO(LOG_FR, "Check successfully created");
		state = State_Idle;
		EventInterface event(deviceId, Event_CommandOK);
		eventEngine->transmit(&event);
		return;
	} else {
		LOG_ERROR(LOG_FR, "Shift closing failed " << resp->errorCode);
		procError();
		return;
	}
}

void ShtrihM::procError() {
	state = State_Idle;
	Fiscal::EventError event(deviceId);
	event.code = ConfigEvent::Type_FiscalUnknownError;
	eventEngine->transmit(&event);
}

void ShtrihM::printShortStatusResponse(const uint8_t *data, const uint16_t len) {
	uint16_t respSize = sizeof(ShmShortStatusResponse);
	if(len < respSize) {
		LOG_ERROR(LOG_FR, "Response too small: exp=" << respSize << ", act=" << len);
		return;
	}
	ShmShortStatusResponse *resp = (ShmShortStatusResponse*)data;
	LOG_DEBUG(LOG_FR, "command=" << resp->command);
	LOG_DEBUG(LOG_FR, "errorCode=" << resp->errorCode);
	LOG_DEBUG(LOG_FR, "operatorNumber=" << resp->operatorNumber);
	LOG_DEBUG(LOG_FR, "flags1=" << resp->flags1);
	LOG_DEBUG(LOG_FR, "flags2=" << resp->flags2);
	LOG_DEBUG(LOG_FR, "mode=" << resp->mode);
	LOG_DEBUG(LOG_FR, "submode=" << resp->submode);
	LOG_DEBUG(LOG_FR, "operNumber=" << resp->getOperNumber());
	LOG_DEBUG(LOG_FR, "reserveVoltage=" << resp->reserveVoltage);
	LOG_DEBUG(LOG_FR, "voltage=" << resp->voltage);
	LOG_DEBUG(LOG_FR, "frErrorCode=" << resp->frErrorCode);
	LOG_DEBUG(LOG_FR, "fnErrorCode=" << resp->fnErrorCode);
}
