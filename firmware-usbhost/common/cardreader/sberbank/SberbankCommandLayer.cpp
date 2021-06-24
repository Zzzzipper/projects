#include "SberbankCommandLayer.h"
#include "SberbankProtocol.h"

#include "mdb/master/cashless/MdbMasterCashless.h"
#include "fiscal_register/include/FiscalSale.h"
#include "utils/include/DecimalPoint.h"
#include "utils/include/CodePage.h"
#include "logger/RemoteLogger.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace Sberbank {

CommandLayer::CommandLayer(
	Mdb::DeviceContext *context,
	PacketLayerInterface *packetLayer,
	TcpIp *conn,
	TimerEngine *timers,
	EventEngineInterface *eventEngine,
	uint32_t maxCredit
) :
	context(context),
	packetLayer(packetLayer),
	deviceLan(packetLayer, conn),
	printer(packetLayer),
	timers(timers),
	eventEngine(eventEngine),
	deviceId(eventEngine),
	commandQueue(4),
	maxCredit(maxCredit),
	sendBuf(SBERBANK_COMMAND_SIZE),
	recvBuf(SBERBANK_COMMAND_SIZE),
	qrCodeData(QR_TEXT_SIZE, QR_TEXT_SIZE)
{
	this->context->setStatus(Mdb::DeviceContext::Status_NotFound);
	this->context->setState(State_Idle);
	this->context->init(2, 1);
	this->packetLayer->setObserver(this);
	this->sverkaTimer = timers->addTimer<CommandLayer, &CommandLayer::procSverkaTimer>(this);
	this->pollTimer = timers->addTimer<CommandLayer, &CommandLayer::procPollTimer>(this);
	this->timer = timers->addTimer<CommandLayer, &CommandLayer::procTimer>(this);
}

CommandLayer::~CommandLayer() {
	timers->deleteTimer(this->timer);
}

EventDeviceId CommandLayer::getDeviceId() {
	return deviceId;
}

void CommandLayer::reset() {
	LOG_INFO(LOG_ECL, "reset");
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	context->setManufacturer((uint8_t*)SBERBANK_MANUFACTURER, sizeof(SBERBANK_MANUFACTURER));
	context->setModel((uint8_t*)SBERBANK_MODEL, sizeof(SBERBANK_MODEL));
	context->incResetCount();
	packetLayer->reset();
	sverkaTimer->start(SBERBANK_SVERKA_FIRST_TIMEOUT);
	commandQueue.clear();
	enabled = false;
	gotoStateDisabled();
}

void CommandLayer::disable() {
	LOG_INFO(LOG_ECL, "disable");
	enabled = false;
}

void CommandLayer::enable() {
	LOG_INFO(LOG_ECL, "enable");
	enabled = true;
}

bool CommandLayer::sale(uint16_t productId, uint32_t productPrice) {
	LOG_INFO(LOG_ECL, "sale " << productId << "," << productPrice);
	if(context->getState() == State_Session) {
		this->productPrice = productPrice;
		gotoStatePayment();
		return true;
	}
	if(context->getState() == State_Disabled || context->getState() == State_Enabled) {
		this->productPrice = productPrice;
		this->commandQueue.push(CommandType_VendRequest);
		return true;
	}
	return false;
}

bool CommandLayer::saleComplete() {
	LOG_INFO(LOG_ECL, "saleComplete");
	if(context->getState() != State_Vending) {
		return false;
	}
	gotoStateClosing();
	return true;
}

bool CommandLayer::saleFailed() {
	LOG_INFO(LOG_ECL, "saleFailed");
	if(context->getState() != State_Vending) {
		return false;
	}
	gotoStatePaymentCancel();
	return true;
}

bool CommandLayer::closeSession() {
	LOG_INFO(LOG_ECL, "closeSession " << context->getState());
	if(context->getState() == State_Session) {
		gotoStateClosing();
		return true;
	}
	if(context->getState() == State_Payment) {
		gotoStatePaymentAbort();
		return true;
	}
	this->commandQueue.push(CommandType_SessionComplete);
	return true;
}

bool CommandLayer::printQrCode(const char *header, const char *footer, const char *text) {
	(void)header;
	(void)footer;
	(void)text;
	LOG_INFO(LOG_ECL, "printQrCode");
	this->commandQueue.push(CommandType_QrCode);
	qrCodeData.set(text);
	return true;
}

bool CommandLayer::verification() {
	LOG_INFO(LOG_ECL, "verification");
	this->commandQueue.push(CommandType_Sverka);
	return true;
}

void CommandLayer::procPacket(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_ECL, "procPacket");
	LOG_TRACE_HEX(LOG_ECL, data, dataLen);
	Packet *packet = (Packet*)data;
	if(packet->len.get() != (dataLen - sizeof(Packet))) {
		LOG_ERROR(LOG_ECL, "Wrong data size");
		return;
	}

	switch(packet->command) {
	case Command_MasterCall: commandMasterCall(packet); return;
	}

	switch(context->getState()) {
	case State_Disabled: stateEnabledRecv(data, dataLen); break;
	case State_Enabled: stateEnabledRecv(data, dataLen); break;
	case State_Sverka1: stateSverka1Recv(data, dataLen); break;
	case State_Sverka2: stateSverka2Recv(data, dataLen); break;
	case State_SessionBegin: stateSessionBeginRecv(data, dataLen); break;
	case State_PaymentAbort: statePaymentAbortRecv(data, dataLen); break;
	case State_Payment: statePaymentRecv(data, dataLen); break;
	case State_PaymentCancel: statePaymentCancelRecv(data, dataLen); break;
	case State_QrCode: stateQrCodeRecv(data, dataLen); break;
	default: LOG_ERROR(LOG_ECL, "Unwaited data " << context->getState());
	}
}

void CommandLayer::procError(Error error) {
#if 0
	LOG_DEBUG(LOG_ECL, "procError " << error);
	if(state == State_Idle) {
		return;
	}

	state = State_Idle;
	Fiscal::EventError event;
	event.code = ConfigEvent::Type_FiscalUnknownError;
	eventEngine->transmit(&event);
#else
	LOG_DEBUG(LOG_ECL, "procError " << error);
#endif
}

void CommandLayer::procPollTimer() {
	LOG_DEBUG(LOG_ECL, "procPollTimer");
	switch(context->getState()) {
	case State_Disabled: stateEnabledPollTimeout(); break;
	case State_Enabled: stateEnabledPollTimeout(); break;
	default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << context->getState());
	}
}

void CommandLayer::procTimer() {
	LOG_DEBUG(LOG_ECL, "procTimer");
	switch(context->getState()) {
	case State_Sverka1: gotoStateDisabled(); break;
	case State_Sverka2: gotoStateDisabled(); break;
	case State_Session: stateSessionTimeout(); break;
	case State_PaymentAbort: statePaymentAbortTimeout(); break;
	case State_Payment: statePaymentTimeout(); break;
	case State_PaymentCancel: statePaymentCancelTimeout(); break;
	case State_Vending: stateVendingTimeout(); break;
	case State_Closing: stateClosingTimeout(); break;
	case State_QrCode: stateQrCodeTimeout(); break;
	default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << context->getState());
	}
}

void CommandLayer::procSverkaTimer() {
	LOG_DEBUG(LOG_ECL, "procSverkaTimer");
	commandQueue.push(CommandType_Sverka);
	sverkaTimer->start(SBERBANK_SVERKA_NEXT_TIMEOUT);
}

void CommandLayer::commandMasterCall(Packet* packet) {
	MasterCallRequest *req = (MasterCallRequest*)packet;
	LOG_DEBUG(LOG_ECL, "commandMasterCall " << req->device << "/" << req->instruction);
	switch(req->device) {
	case Device_Lan: deviceLan.procRequest(req); return;
	case Device_Printer: printer.procRequest(req); return;
	default: commandMasterCallUnsupported(req); return;
	}
}

void CommandLayer::commandMasterCallUnsupported(MasterCallRequest *req) {
	LOG_ERROR(LOG_ECL, "commandMasterCallUnsupported " << req->device);
	Packet *resp = (Packet*)sendBuf.getData();
	resp->command = 252;
	resp->len.set(0);
	resp->num.set(req->num.get() | SBERBANK_RESPONSE_FLAG);
	sendBuf.setLen(sizeof(Packet));
	packetLayer->sendPacket(&sendBuf);
}

bool CommandLayer::procCommand() {
	LOG_DEBUG(LOG_ECL, "procCommand");
	if(commandQueue.isEmpty() == true) {
		return false;
	}

	uint8_t command = commandQueue.pop();
	if(command == CommandType_Sverka) {
		gotoStateSverka1();
		return true;
	}
	if(command == CommandType_VendRequest) {
		gotoStatePayment();
		return true;
	}
	if(command == CommandType_SessionComplete) {
		gotoStateClosing();
		return true;
	}
	if(command == CommandType_QrCode) {
		gotoStateQrCode();
		return true;
	}

	return false;
}

void CommandLayer::gotoStateDisabled() {
	LOG_DEBUG(LOG_ECL, "gotoStateDisabled");
	if(procCommand() == true) {
		return;
	}
	if(enabled == true) {
		gotoStateEnabled();
		return;
	}
	pollTimer->start(SBERBANK_POLL_DELAY);
	context->setState(State_Disabled);
}

void CommandLayer::gotoStateEnabled() {
	LOG_DEBUG(LOG_ECL, "gotoStateEnabled");
	if(procCommand() == true) {
		return;
	}
	if(enabled == false) {
		gotoStateDisabled();
		return;
	}
	pollTimer->start(SBERBANK_POLL_DELAY);
	context->setState(State_Enabled);
}

void CommandLayer::stateEnabledPollTimeout() {
	LOG_DEBUG(LOG_ECL, "stateEnabledPollTimeout");
	if(procCommand() == true) {
		return;
	}

#if 1
	PollRequest *req = (PollRequest*)sendBuf.getData();
	req->command = Command_Poll;
	req->len.set(0x0C);
	req->num.set(0x000a441d);
	if(enabled == true) { req->param1.set(0xD3); } else { req->param1.set(0x81); }
	req->param2.set(0);
	req->param3.set(1);
	sendBuf.setLen(sizeof(PollRequest));
	packetLayer->sendPacket(&sendBuf);
	pollTimer->start(SBERBANK_POLL_RECV_TIMEOUT);
#else
	LOG_DEBUG(LOG_ECL, "stateWaitTimeout");
	if(enabled == true) {
		TransactionRequest *req = (TransactionRequest*)sendBuf.getData();
		req->command = Command_Transaction;
		req->len.set(sizeof(TransactionRequest) - sizeof(Packet));
		req->num.set(0x000CE2D6);
		req->sum.set(0);
		req->cardType = 0;
		req->operation = Operation_WaitButton;
		memset(req->cardRoad2, 0, sizeof(req->cardRoad2));
		req->cardRoad2[0] = '2';
		req->requestId.set(0);
		memset(req->transactionNum, 0, sizeof(req->transactionNum));
		req->flags.set(0);
		sendBuf.setLen(sizeof(TransactionRequest));
		packetLayer->sendPacket(&sendBuf);
	}
	pollTimer->start(SBERBANK_POLL_RECV_TIMEOUT);
#endif
}

void CommandLayer::stateEnabledRecv(const uint8_t *data, uint16_t dataLen) {
	PollResponse *resp = (PollResponse*)data;
	if(dataLen < sizeof(PollResponse)) {
		LOG_ERROR(LOG_ECL, "Bad format");
		context->incProtocolErrorCount();
		return;
	}

	LOG_DEBUG(LOG_ECL, "stateEnabledRecv" << resp->value.get());
	context->setStatus(Mdb::DeviceContext::Status_Work);
	if(resp->value.get() == 8000) {
		gotoStateSessionBegin();
		return;
	} else {
		gotoStateEnabled();
		return;
	}
}

void CommandLayer::gotoStateSverka1() {
	LOG_DEBUG(LOG_ECL, "gotoStateSverka1");
	PollRequest *req = (PollRequest*)sendBuf.getData();
	req->command = Command_Poll;
	req->len.set(0x0C);
	req->num.set(0x0000A441D);
	req->param1.set(0x81);
	req->param2.set(0);
	req->param3.set(0xFFFFFFFF);
	sendBuf.setLen(sizeof(PollRequest));
	packetLayer->sendPacket(&sendBuf);
	timer->start(SBERBANK_POLL_RECV_TIMEOUT);
	context->setState(State_Sverka1);
}

void CommandLayer::stateSverka1Recv(const uint8_t *data, uint16_t dataLen) {
	(void)data;
	LOG_INFO(LOG_ECL, "stateSverka1Recv " << dataLen);
	gotoStateSverka2();
}

void CommandLayer::gotoStateSverka2() {
	LOG_DEBUG(LOG_ECL, "gotoStateSverka2");
	PaymentRequest *req = (PaymentRequest*)sendBuf.getData();
	req->command = Command_Transaction;
	req->len.set(0x0045);
	req->num.set(0x000CE2D6);
	req->sum.set(0);
	req->cardType = 0;
	req->operation = Operation_SverkaItogov;
	memset(req->cardRoad2, 0, sizeof(req->cardRoad2));
	req->requestId.set(0);
	memset(req->transactionNum, 0, sizeof(req->transactionNum));
	req->flags.set(0);
	req->dataLen = 0;
	sendBuf.setLen(sizeof(PaymentRequest));
	timer->start(SBERBANK_VERIFICATION_TIMEOUT);
	packetLayer->sendPacket(&sendBuf);
	context->setState(State_Sverka2);
}

void CommandLayer::stateSverka2Recv(const uint8_t *data, uint16_t dataLen) {
	(void)data;
	LOG_INFO(LOG_ECL, "stateSverka2Recv " << dataLen);
	gotoStateDisabled();
}

void CommandLayer::gotoStateSessionBegin() {
	LOG_DEBUG(LOG_ECL, "gotoStateSessionBegin");
	PollRequest *req = (PollRequest*)sendBuf.getData();
	req->command = Command_Poll;
	req->len.set(0x0C);
	req->num.set(0x000a441d);
	req->param1.set(0xcd);
	req->param2.set(0);
	req->param3.set(0xFFFFFFFF);
	sendBuf.setLen(sizeof(PollRequest));
	packetLayer->sendPacket(&sendBuf);
	context->setState(State_SessionBegin);
}

void CommandLayer::stateSessionBeginRecv(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECL, "stateSessionBeginRecv");
	PollResponse *resp = (PollResponse*)data;
	if(dataLen < sizeof(PollResponse)) {
		LOG_ERROR(LOG_ECL, "Bad format");
		context->incProtocolErrorCount();
		return;
	}
	LOG_INFO(LOG_ECL, "Session begin response " << resp->value.get());
	gotoStateSession();
	EventUint32Interface event(deviceId, MdbMasterCashless::Event_SessionBegin,  maxCredit);
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateSession() {
	LOG_DEBUG(LOG_ECL, "gotoStateSession");
	timer->start(MDB_CL_SESSION_TIMEOUT);
	context->setState(State_Session);
}

void CommandLayer::stateSessionTimeout() {
	LOG_DEBUG(LOG_ECL, "stateSessionTimeout");
	gotoStateDisabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStatePaymentAbort() {
	LOG_DEBUG(LOG_ECL, "gotoStatePaymentAbort");
	Packet *resp = (Packet*)sendBuf.getData();
	resp->command = Command_Abort;
	resp->len.set(0x0000);
	resp->num.set(0x00076ef7);
	sendBuf.setLen(sizeof(Packet));
	packetLayer->sendPacket(&sendBuf);
	timer->start(MDB_CL_APPROVING_TIMEOUT);
	context->setState(State_PaymentAbort);
}

void CommandLayer::statePaymentAbortTimeout() {
	LOG_DEBUG(LOG_ECL, "statePaymentAbortTimeout");
	gotoStateEnabled();
}

void CommandLayer::statePaymentAbortRecv(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECL, "statePaymentAbortRecv");
	PaymentResponse *resp = (PaymentResponse*)data;
	if(dataLen < sizeof(PaymentResponse)) {
		LOG_ERROR(LOG_ECL, "Bad format" << dataLen << "m" << (uint32_t)sizeof(PaymentResponse));
		LOG_DEBUG_HEX(LOG_ECL, data, dataLen);
		context->incProtocolErrorCount();
		return;
	}
	LOG_INFO(LOG_ECL, "Payment response " << resp->result << "/" << resp->result2.get());
	if(resp->result2.get() == 0) {
		gotoStatePaymentCancel();
		return;
	} else {
		gotoStateEnabled();
		return;
	}
}

void CommandLayer::gotoStatePayment() {
	LOG_DEBUG(LOG_ECL, "gotoStatePayment");
	PaymentRequest *req = (PaymentRequest*)sendBuf.getData();
	req->command = Command_Transaction;
	req->len.set(0x0045);
	req->num.set(0x00076ef6);
	req->sum.set(context->money2value(productPrice));
	req->cardType = 0;
	req->operation = Operation_Payment;
	memset(req->cardRoad2, 0, sizeof(req->cardRoad2));
	req->requestId.set(0);
	memset(req->transactionNum, 0, sizeof(req->transactionNum));
	req->flags.set(0);
	req->dataLen = 0;
	sendBuf.setLen(sizeof(PaymentRequest));
	packetLayer->sendPacket(&sendBuf);
	timer->start(MDB_CL_APPROVING_TIMEOUT);
	context->setState(State_Payment);
}

void CommandLayer::statePaymentTimeout() {
	LOG_DEBUG(LOG_ECL, "statePaymentTimeout");
	REMOTE_LOG(RLOG_ECL, "timeout");
	gotoStateDisabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
	eventEngine->transmit(&event);
}

void CommandLayer::statePaymentRecv(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECL, "statePaymentRecv");
	REMOTE_LOG_IN(RLOG_ECL, data, dataLen);
	PaymentResponse *resp = (PaymentResponse*)data;
	if(dataLen < sizeof(PaymentResponse)) {
		LOG_ERROR(LOG_ECL, "Bad format" << dataLen << "m" << (uint32_t)sizeof(PaymentResponse));
		LOG_DEBUG_HEX(LOG_ECL, data, dataLen);
		context->incProtocolErrorCount();
		return;
	}
	LOG_INFO(LOG_ECL, "Payment response " << resp->result << "/" << resp->result2.get()); // 0/4322
	if(resp->result2.get() == 0) {
		uint32_t sberbankValue = context->value2money(resp->points.get());
		uint32_t cashlessValue = productPrice - sberbankValue;
		gotoStateVending();
		MdbMasterCashlessInterface::EventApproved event(deviceId, Fiscal::Payment_Cashless, cashlessValue, Fiscal::Payment_Sberbank, sberbankValue);
		eventEngine->transmit(&event);
	} else {
		gotoStateSession();
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
	}
}

void CommandLayer::gotoStateVending() {
	LOG_DEBUG(LOG_ECL, "gotoStateVending");
	timer->start(MDB_CL_VENDING_TIMEOUT);
	context->setState(State_Vending);
}

void CommandLayer::stateVendingTimeout() {
	LOG_DEBUG(LOG_ECL, "stateVendingTimeout()");
	gotoStatePaymentCancel();
}

void CommandLayer::gotoStatePaymentCancel() {
	LOG_DEBUG(LOG_ECL, "gotoStatePaymentCancel");
	PaymentRequest *req = (PaymentRequest*)sendBuf.getData();
	req->command = Command_Transaction;
	req->len.set(0x0045);
	req->num.set(0x00076eff);
	req->sum.set(context->money2value(productPrice));
	req->cardType = 0;
	req->operation = Operation_PaymentCancel;
	memset(req->cardRoad2, 0, sizeof(req->cardRoad2));
	req->requestId.set(0);
	memset(req->transactionNum, 0, sizeof(req->transactionNum));
	req->flags.set(0);
	req->dataLen = 0;
	sendBuf.setLen(sizeof(PaymentRequest));
	packetLayer->sendPacket(&sendBuf);
	timer->start(MDB_CL_APPROVING_TIMEOUT);
	context->setState(State_PaymentCancel);
}

void CommandLayer::statePaymentCancelTimeout() {
	LOG_DEBUG(LOG_ECL, "statePaymentTimeout");
	gotoStateDisabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void CommandLayer::statePaymentCancelRecv(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECL, "statePaymentCancelRecv");
	PaymentResponse *resp = (PaymentResponse*)data;
	if(dataLen < sizeof(PaymentResponse)) {
		LOG_ERROR(LOG_ECL, "Bad format" << dataLen << "m" << (uint32_t)sizeof(PaymentResponse));
		context->incProtocolErrorCount();
		return;
	}
	LOG_INFO(LOG_ECL, "Payment cancel " << resp->result << "/" << resp->result2.get());
	gotoStateEnabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateClosing() {
	LOG_DEBUG(LOG_ECL, "gotoStateClosing");
	timer->start(1);
	context->setState(State_Closing);
}

void CommandLayer::stateClosingTimeout() {
	LOG_DEBUG(LOG_ECL, "stateClosingTimeout");
	gotoStateEnabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

/*
Реализовано только для (PAX D200 и Castles MP200) .
В конце каждой операции управляющее ПО, может послать команду
для вывода фискального чека на экран пинпада. Это делается путем
подачи команды CMD_TRANSACTION со значением «Тип операции» = 0x39.
В запросе в поле «Дополнительные данные» в теге 0xDF55UL должен
содержаться чек в следующе виде «t=20170216T133600&s=780.00&fn=9999078900001327&i=91&fp=1697013557&n=1».
Расшифровка чека( t= - дата, T – время, &s= - цена, &fn= - номер фн,
&i= - номер фискального документа (фд), &fp=фискальный признак,
&n= - вид кассового чека (приход, возврат прихода, расход, возврат расхода)).

struct PaymentRequest {
	uint8_t command;
	BEUint2 len;
	BEUint4 num;
	BEUint4 sum;
	uint8_t cardType;
	uint8_t depNum;
	uint8_t operation;
	uint8_t cardRoad2[40];
	BEUint4 requestId;
	uint8_t transactionNum[13];
	BEUint4 flags;
	uint8_t dataLen;
	uint8_t data[0];
};

command(6D)
len(4D00)
num(F76E0700)
sum(00000000)
cardType(00)
depNum(00)
operation(39)
cardRoad2[40]
00000000000000000000
00000000000000000000
00000000000000000000
00000000000000000000
requestId(453959F4)
transactionNum(00000000000000000000000000)
flags(0004000
 */
void CommandLayer::gotoStateQrCode() {
	LOG_DEBUG(LOG_ECL, "gotoStateQrCode");
	PaymentRequest *req = (PaymentRequest*)sendBuf.getData();
	req->command = Command_Transaction;
	req->num.set(0x00076ef7);
	req->sum.set(0);
	req->cardType = 0;
	req->depNum = 0;
	req->operation = Operation_QrCode;
	memset(req->cardRoad2, 0, sizeof(req->cardRoad2));
	req->requestId.set(0);
	memset(req->transactionNum, 0, sizeof(req->transactionNum));
	req->flags.set(0);

	BerTlv *tlv = (BerTlv*)req->data;
	tlv->tag.set(0xDF55);
	tlv->len = qrCodeData.getLen();
	memcpy(tlv->data, qrCodeData.getData(), qrCodeData.getLen());

	req->dataLen = sizeof(*tlv) + qrCodeData.getLen();
	req->len.set(sizeof(PaymentRequest) - sizeof(Packet) + req->dataLen);
	sendBuf.setLen(sizeof(PaymentRequest) + req->dataLen);
	timer->start(SBERBANK_QRCODE_TIMEOUT);
	context->setState(State_QrCode);
	packetLayer->sendPacket(&sendBuf);
}

void CommandLayer::stateQrCodeTimeout() {
	LOG_DEBUG(LOG_ECL, "stateQrCodeTimeout");
	gotoStateDisabled();
}

void CommandLayer::stateQrCodeRecv(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECL, "stateQrCodeRecv");
	PaymentResponse *resp = (PaymentResponse*)data;
	if(dataLen < sizeof(PaymentResponse)) {
		LOG_ERROR(LOG_ECL, "Bad format" << dataLen << "m" << (uint32_t)sizeof(PaymentResponse));
		LOG_DEBUG_HEX(LOG_ECL, data, dataLen);
		context->incProtocolErrorCount();
		return;
	}
	LOG_INFO(LOG_ECL, "Payment response " << resp->result << "/" << resp->result2.get());
	gotoStateDisabled();
}

}
