#include "IngenicoCommandLayer.h"
#include "IngenicoProtocol.h"

#include "mdb/master/cashless/MdbMasterCashless.h"
#include "utils/include/StringParser.h"
#include "utils/include/DecimalPoint.h"
#include "utils/include/CodePage.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace Ingenico {

CommandLayer::CommandLayer(
	Mdb::DeviceContext *context,
	PacketLayerInterface *packetLayer,
	TcpIp *conn,
	TimerEngine *timerEngine,
	EventEngineInterface *eventEngine,
	uint32_t maxCredit
) :
	context(context),
	packetLayer(packetLayer),
	timerEngine(timerEngine),
	eventEngine(eventEngine),
	maxCredit(maxCredit),
	deviceId(eventEngine),
	commandQueue(4),
	req(INGENICO_PACKET_SIZE),
	deviceLan(packetLayer, conn, timerEngine),
	dialogDirect(packetLayer, &req, timerEngine, this)
{
/*
	this->context->setStatus(Mdb::DeviceContext::Status_NotFound);
	this->context->setState(State_Idle);
*/
	this->timer = timerEngine->addTimer<CommandLayer, &CommandLayer::procTimer>(this);
	this->packetLayer->setObserver(this);
}

CommandLayer::~CommandLayer() {
	timerEngine->deleteTimer(this->timer);
}

EventDeviceId CommandLayer::getDeviceId() {
	return deviceId;
}

void CommandLayer::reset() {
	LOG_INFO(LOG_ECL, "reset");
/*	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	context->setManufacturer((uint8_t*)INPAS_MANUFACTURER, sizeof(INPAS_MANUFACTURER));
	context->setModel((uint8_t*)INPAS_MODEL, sizeof(INPAS_MODEL));
	context->incResetCount();
	gotoStateWait();*/
//	req.addString("0,3,Для оплаты");
//	req.addString("1,1,картой нажмите");
//	req.addString("2,1,зеленую кнопку");
//	req.addString("3,0,");
	packetLayer->reset();
	gotoStateWaitText();
}

void CommandLayer::disable() {
	LOG_INFO(LOG_ECL, "disable");

}

void CommandLayer::enable() {
	LOG_INFO(LOG_ECL, "enable");

}

bool CommandLayer::sale(uint16_t productId, uint32_t productPrice) {
	LOG_INFO(LOG_ECL, "sale " << productId << "," << productPrice);
//	if(context->getState() == State_WaitText || context->getState() == State_WaitButton || context->getState() == State_Session) {
	this->productPrice = productPrice;
	gotoStateApproving();
	return true;
//	}
//	return false;

#if 0
	if(context->getState() != State_Wait && context->getState() != State_Session) {
		LOG_ERROR(LOG_ECL, "Wrong state " << context->getState());
		return false;
	}

	this->productPrice = productPrice;
	gotoStateRequest();
#else
/*
	req.clear();
	req.addUint8('0');
	req.addUint8(Control_ESC);
	req.addUint8('1');
	req.addUint8('6');
	req.addUint8('6');
	req.addUint8(Control_ESC);
	req.addUint8(Control_ESC);
	req.addUint8(Control_ESC);
	req.addUint8(Control_ESC);
	packetLayer->sendPacket(&req);
*/
/*
	req.clear();
	req.addUint8('1');
	req.addUint8('2');
	req.addUint8(Control_ESC);
	req.addUint8('1');
	req.addUint8(Control_ESC);
	req.addUint8(Control_ESC);
	req.addUint8(Control_ESC);
	req.addUint8(Control_ESC);
	packetLayer->sendPacket(&req);
*/
/*
x31;x1B;x31;x1B;x36;x34;x33;x1B;x31;x30;x30;x2E;x30;x31;x1B;x1B;
 31  1B  31  1B  36  34  33  1B  31  30  30  2E  30  31  1B  1B
 */
#if 0
	// info
	req.clear();
	req.addUint8(0x32); // operation
	req.addUint8(Control_ESC);
	req.addUint8(0x32); // pay
	req.addUint8(0x31);
	req.addUint8(Control_ESC);
	req.addUint8(Control_ESC);
	req.addUint8(Control_ESC);
	req.addUint8(Control_ESC);
	packetLayer->sendPacket(&req);
	LOG_HEX(req.getData(), req.getLen());
#endif
#if 1
	this->productPrice = productPrice;

	// pay = 100.01
	// context->money2value(
/*	uint32_t sum = context->money2value(productPrice);
	uint32_t rub = sum / 100;
	uint32_t cop = sum % 100;
	uint8_t sumStr[16];
	sumStr[]*/
#endif
#endif
	return true;
}

bool CommandLayer::saleComplete() {
	LOG_INFO(LOG_ECL, "saleComplete");
	if(context->getState() == State_Idle) {
		return false;
	}
	gotoStateClosing();
	return true;
}

bool CommandLayer::saleFailed() {
	LOG_INFO(LOG_ECL, "saleFailed");
	if(context->getState() == State_Idle) {
		return false;
	}
//	gotoStatePaymentCancel();*/
	gotoStateClosing();
	return true;
}

bool CommandLayer::closeSession() {
	LOG_INFO(LOG_ECL, "closeSession");
	if(context->getState() == State_Idle) {
		return false;
	}
/*	if(context->getState() == State_ApprovingStart || context->getState() == State_Approving || context->getState() == State_ApprovingEnd) {
		commandQueue.push(CommandType_PaymentCancel);
		return true;
	}*/
	gotoStateClosing();
	return true;
}

//char qrText[] = "0xDF^^https://en.wikipedia.org/wiki/Thorax~";
bool CommandLayer::printQrCode(const char *header, const char *footer, const char *text) {
	LOG_INFO(LOG_ECL, "printQrCode");
/*	if(context->getState() != State_Wait) {
		return false;
	}

	qrCodeData.clear();
	qrCodeData << "0xDF^^" << text << "~";
	qrCodeHeader.clear();
	qrCodeHeader << header << "\n" << footer;
	LOG_INFO(LOG_ECL, "qrCodeHeader=" << qrCodeHeader.getString() << qrCodeHeader.getLen());
	LOG_INFO(LOG_ECL, "qrCodeData=" << qrCodeData.getString() << qrCodeData.getLen());

	gotoStateQrCode();*/
	return true;
}

bool CommandLayer::verification() {
#if 0 // I344 mode
	//2/32 -
	//\x32\x1B\x33\x32\x1B\x1B
	req.clear();
	req.addUint8(0x49); //I
	req.addUint8(0x33); //3
	req.addUint8(0x34); //4
	req.addUint8(0x34); //4
	req.addUint8(0x3A); //:
	req.addUint8(0xFB);
	req.addUint8(0x);
	req.addUint8(0x);
	req.addUint8(0x);
	req.addUint8(0x);
	req.addUint8(0x);
#endif
#if 1
	// 2/1 verification
	req.clear();
	req.addNumber(2);
	req.addControl(Control_ESC);
	req.addNumber(1);
	req.addControl(Control_ESC);
	req.addControl(Control_ESC);
#endif
	packetLayer->sendPacket(req.getBuf());
	return true;
}

void CommandLayer::proc(Event *event) {
	switch(context->getState()) {
		case State_WaitText: stateWaitTextEvent(event); return;
		case State_WaitButton: stateWaitButtonEvent(event); return;
		case State_Session: stateSessionTextEvent(event); return;
		case State_ApprovingStart: stateApprovingStartEvent(event); return;
		default: LOG_ERROR(LOG_ECL, "Unwaited event " << context->getState() << "," << event->getType());
	}
}

void CommandLayer::procTimer() {
	switch(context->getState()) {
		case State_Closing: stateClosingTimeout(); return;
		default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << context->getState());
	}
}

void CommandLayer::procPacket(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_ECL, "procPacket" << dataLen);
	StringParser parser((char*)data, dataLen);
	StringBuilder cmd(32,32);
	uint16_t len = parser.getValue(":", (char*)cmd.getString(), cmd.getSize());
	if(len == 0) {
		return;
	}
	cmd.setLen(len);
	parser.skipEqual(":");
	LOG_INFO(LOG_ECL, "command=" << cmd.getString());

	if(cmd == "STATUS") {
		procStatus(&parser);
		return;
	}
	if(cmd == "DEVICEOPEN") {
		deviceLan.procDeviceOpen(&parser);
		return;
	}
	if(cmd == "IOCTL") {
		deviceLan.procIoctl(&parser);
		return;
	}
	if(cmd == "CONNECT") {
		deviceLan.procConnect(&parser);
		return;
	}
	if(cmd == "READ") {
		deviceLan.procRead(&parser);
		return;
	}
	if(cmd == "WRITE") {
		deviceLan.procWrite(&parser);
		return;
	}
	if(cmd == "DISCONNECT") {
		deviceLan.procDisconnect(&parser);
		return;
	}
	if(cmd == "DEVICECLOSE") {
		deviceLan.procDeviceClose(&parser);
		return;
	}
	if(cmd == "STORERC") {
		procStorerc(&parser);
		return;
	}
	if(cmd == "DL") {
		procDl(&parser);
		return;
	}
	if(cmd == "ENDTR") {
		procEndtr(&parser);
		return;
	}
	if(cmd == "PING") {
		procPing();
		return;
	}

	LOG_INFO(LOG_ECL, "data=" << parser.unparsed());
	req.clear();
	req.addString("OK");
	packetLayer->sendPacket(req.getBuf());
	LOG_ERROR(LOG_ECL, "Unwaited command " << cmd.getString());
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

void CommandLayer::procStorerc(StringParser *parser) {
	LOG_DEBUG(LOG_ECL, "procStorerc");
	switch(context->getState()) {
		case State_Approving: stateApprovingStorerc(parser); return;
		case State_ApprovingEnd: stateApprovingEndStorerc(parser); return;
		default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << context->getState());
	}
}

void CommandLayer::procDl(StringParser *parser) {
	LOG_DEBUG(LOG_ECL, "procDl " << parser->unparsed());
	switch(context->getState()) {
		case State_WaitText: dialogDirect.procDl(parser); return;
		case State_WaitButton: dialogDirect.procDl(parser); return;
		case State_Session: dialogDirect.procDl(parser); return;
		case State_ApprovingStart: dialogDirect.procDl(parser); return;
		case State_Vending:  dialogDirect.procDl(parser); return;
		case State_Closing:  dialogDirect.procDl(parser); return;
		default: LOG_ERROR(LOG_ECL, "Unwaited dl " << context->getState());
	}
}

void CommandLayer::procEndtr(StringParser *parser) {
	LOG_DEBUG(LOG_ECL, "procEndtr " << parser->unparsed());
	switch(context->getState()) {
		case State_WaitText: dialogDirect.procEndtr(); return;
		case State_WaitButton: dialogDirect.procEndtr(); return;
		case State_Session: dialogDirect.procEndtr(); return;
		case State_ApprovingStart: dialogDirect.procEndtr(); return;
		case State_ApprovingEnd: stateApprovingEndEndtr(); return;
		case State_Vending:  dialogDirect.procEndtr(); return;
		case State_Closing:  dialogDirect.procEndtr(); return;
		default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << context->getState());
	}
}

void CommandLayer::procStatus(StringParser *parser) {
	LOG_INFO(LOG_ECL, "data=" << parser->unparsed());
	req.clear();
	req.addString("OK");
	packetLayer->sendPacket(req.getBuf());
}

void CommandLayer::procPing() {
	LOG_DEBUG(LOG_ECL, "procPing");
	req.clear();
	req.addString("OK");
	packetLayer->sendPacket(req.getBuf());
}

bool CommandLayer::procCommand() {
	LOG_DEBUG(LOG_ECL, "procCommand");
	if(commandQueue.isEmpty() == true) {
		return false;
	}

	uint8_t command = commandQueue.pop();
/*	if(command == CommandType_Sverka) {
		gotoStateSverka1();
		return true;
	}*/
	if(command == CommandType_VendRequest) {
		gotoStateApproving();
		return true;
	}
/*
	if(command == CommandType_PaymentCancel) {
		gotoStatePaymentCancel();
		return true;
	}
	if(command == CommandType_SessionComplete) {
		gotoStateClosing();
		return true;
	}
	if(command == CommandType_QrCode) {
		gotoStateQrCode();
		return true;
	}*/

	return false;
}

void CommandLayer::gotoStateWaitText() {
	LOG_DEBUG(LOG_ECL, "gotoStateWaitText");
	dialogDirect.showText("0,0,   Для оплаты   \n1,0, картой нажмите \n2,0, зеленую кнопку \n3,0,                ");
	context->setState(State_WaitText);
}

void CommandLayer::stateWaitTextEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateWaitTextEvent");
	switch(event->getType()) {
	case DialogDirect::Event_Text: stateWaitTextEventText(); return;
	default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << context->getState());
	}
}

void CommandLayer::stateWaitTextEventText() {
	LOG_DEBUG(LOG_ECL, "stateWaitTextEventText");
	gotoStateWaitButton();
}

void CommandLayer::gotoStateWaitButton() {
	LOG_DEBUG(LOG_ECL, "gotoStateWaitButton");
	dialogDirect.waitButton();
	context->setState(State_WaitButton);
}

void CommandLayer::stateWaitButtonEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateWaitButtonEvent");
	switch(event->getType()) {
	case DialogDirect::Event_Button: stateWaitButtonEventButton(); return;
	case DialogDirect::Event_Cancel: gotoStateWaitText(); return;
	default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << context->getState());
	}
}

void CommandLayer::stateWaitButtonEventButton() {
	LOG_DEBUG(LOG_ECL, "stateWaitButtonEventButton");
	gotoStateSession();
	EventUint32Interface event(deviceId, MdbMasterCashless::Event_SessionBegin,  context->value2money(maxCredit));
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateSession() {
	LOG_DEBUG(LOG_ECL, "gotoStateSession");
	dialogDirect.showText("0,0,    Выберите    \n1,0,     товар      \n2,0, на клавиатуре  \n3,0,    автомата    ");
	context->setState(State_Session);
}

void CommandLayer::stateSessionTextEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateSessionTextEvent");
	switch(event->getType()) {
	case DialogDirect::Event_Text: dialogDirect.waitButton(); return;
	case DialogDirect::Event_Cancel: gotoStateWaitText(); return;
	default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << context->getState());
	}
}

void CommandLayer::gotoStateApprovingStart() {
	LOG_DEBUG(LOG_ECL, "gotoStateApprovingEndtr");
	dialogDirect.close();
	context->setState(State_ApprovingStart);
}

void CommandLayer::stateApprovingStartEvent(Event *event) {
	LOG_DEBUG(LOG_ECL, "stateApprovingEndtrEvent");
	switch(event->getType()) {
	case DialogDirect::Event_Close: gotoStateApproving(); return;
	default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << context->getState());
	}
}

void CommandLayer::gotoStateApproving() {
	LOG_DEBUG(LOG_ECL, "gotoStateApproving");
	if(dialogDirect.isEnabled()) {
		gotoStateApprovingStart();
		return;
	}
	req.clear();
	req.addNumber(1);
	req.addControl(Control_ESC);
	req.addNumber(1);
	req.addControl(Control_ESC);
	req.addNumber(643);
	req.addControl(Control_ESC);
	req.addNumber(context->money2value(productPrice));
	req.addString(".00");
	req.addControl(Control_ESC);
	req.addControl(Control_ESC);
	packetLayer->sendPacket(req.getBuf());
	context->setState(State_Approving);
}

void CommandLayer::stateApprovingStorerc(StringParser *parser) {
	LOG_DEBUG(LOG_ECL, "stateApprovingStorerc");
	req.clear();
	req.addString("OK");
	packetLayer->sendPacket(req.getBuf());

	LOG_INFO(LOG_ECL, "data=" << parser->unparsed());
	uint16_t result = 0;
	if(parser->getNumber(&result) == false) {
		LOG_INFO(LOG_ECL, "DENIED");
		approveResult = 1;
		gotoStateApprovingEnd();
		return;
	}

	approveResult = result;
	gotoStateApprovingEnd();
}

void CommandLayer::gotoStateApprovingEnd() {
	LOG_DEBUG(LOG_ECL, "gotoStateApprovingEnd");
	context->setState(State_ApprovingEnd);
}

void CommandLayer::stateApprovingEndStorerc(StringParser *parser) {
	(void)parser;
	LOG_DEBUG(LOG_ECL, "stateApprovingEndEndtr");
	req.clear();
	req.addString("OK");
	packetLayer->sendPacket(req.getBuf());
}

void CommandLayer::stateApprovingEndEndtr() {
	LOG_DEBUG(LOG_ECL, "stateApprovingEndEndtr");
	req.clear();
	req.addString("OK");
	packetLayer->sendPacket(req.getBuf());

	if(approveResult == 0) {
		LOG_INFO(LOG_ECL, "SUCCEED");
		gotoStateVending();
		MdbMasterCashlessInterface::EventApproved event(deviceId, Fiscal::Payment_Cashless, productPrice);
		eventEngine->transmit(&event);
		return;
	} else {
		LOG_INFO(LOG_ECL, "DENIED");
		gotoStateWaitText();
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::gotoStateVending() {
	LOG_DEBUG(LOG_ECL, "gotoStateVending");
	dialogDirect.showText("0,0, Товар оплачен. \n"
						  "1,0, Идет выдача... \n"
						  "2,0,    Спасибо     \n"
						  "3,0,  за покупку!   ");
	context->setState(State_Vending);
}

void CommandLayer::gotoStatePaymentCancel() {
	LOG_DEBUG(LOG_ECL, "gotoStatePaymentCancel");

}

void CommandLayer::gotoStateClosing() {
	LOG_DEBUG(LOG_ECL, "gotoStateClosing");
	timer->start(1);
	context->setState(State_Closing);
}

void CommandLayer::stateClosingTimeout() {
	LOG_DEBUG(LOG_ECL, "stateClosingTimeout");
	gotoStateWaitText();
}

/*
void CommandLayer::procTimer() {
	LOG_DEBUG(LOG_ECL, "procTimer");
	switch(context->getState()) {
	case State_Session: stateSessionTimeout(); break;
	case State_Request: stateRequestTimeout(); break;
	case State_Approving: stateApprovingTimeout(); break;
	case State_PaymentCancel: statePaymentCancelTimeout(); break;
	case State_PaymentCancelWait: statePaymentCancelWaitTimeout(); break;
	case State_Closing: stateClosingTimeout(); break;
	case State_QrCode: stateQrCodeTimeout(); break;
	case State_QrCodeWait: stateQrCodeWaitTimeout(); break;
	default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << context->getState());
	}
}

void CommandLayer::gotoStateWait() {
	LOG_DEBUG(LOG_ECL, "gotoStateWait");
	context->setState(State_Wait);
}

void CommandLayer::stateWaitPacket() {
	LOG_DEBUG(LOG_ECL, "stateWaitResponse");
	uint32_t credit;
	if(packet.getNumber(Tlv_Credit, &credit) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		context->incProtocolErrorCount();
		return;
	}

	uint16_t currency;
	if(packet.getNumber(Tlv_Currency, &currency) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		context->incProtocolErrorCount();
		return;
	}

	uint32_t decimalPoint;
	uint32_t scaleFactor;
	uint8_t mdbVersion;
	if(packet.getMdbOptions(Tlv_MdbOptions, &decimalPoint, &scaleFactor, &mdbVersion) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		context->incProtocolErrorCount();
		return;
	}

	LOG_DEBUG(LOG_ECL, "credit=" << credit);
	LOG_DEBUG(LOG_ECL, "currency=" << currency);
	LOG_DEBUG(LOG_ECL, "decimalPoint=" << decimalPoint);
	LOG_DEBUG(LOG_ECL, "scaleFactor=" << scaleFactor);
	LOG_DEBUG(LOG_ECL, "mdbVersion=" << mdbVersion);
	context->init(decimalPoint, scaleFactor);
	context->setCurrency(currency);
	context->setStatus(Mdb::DeviceContext::Status_Work);

	gotoStateSession();
	EventUint32Interface event(deviceId, MdbMasterCashless::Event_SessionBegin,  context->value2money(credit));
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateSession() {
	LOG_DEBUG(LOG_ECL, "gotoStateSession");
	timer->start(MDB_CL_SESSION_TIMEOUT);
	context->setState(State_Session);
}

void CommandLayer::stateSessionControl(uint8_t control) {
	LOG_DEBUG(LOG_ECL, "stateSessionControl");
	if(control == Control_EOT) {
		timer->stop();
		context->setState(State_Wait);
		EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateSessionTimeout() {
	LOG_DEBUG(LOG_ECL, "stateSessionTimeout");
	context->setState(State_Wait);
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateRequest() {
	LOG_DEBUG(LOG_ECL, "gotoStateRequest");
	req.clear();
	req.addNumber(Tlv_Credit, 8, context->money2value(productPrice));
	req.addNumber(Tlv_Currency, 3, context->getCurrency());
	req.addNumber(Tlv_OperationCode, 1, 1);
	packetLayer->sendPacket(req.getBuf());
	timer->start(INPAS_RECV_TIMEOUT);
	context->setState(State_Request);
}

void CommandLayer::stateRequestControl(uint8_t control) {
	LOG_DEBUG(LOG_ECL, "stateRequestControl " << control);
	if(control == Control_ACK) {
		gotoStateApproving();
		return;
	}
	if(control == Control_EOT) {
		timer->stop();
		context->setState(State_Wait);
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateRequestTimeout() {
	LOG_DEBUG(LOG_ECL, "stateRequestTimeout");
	context->setState(State_Wait);
	EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateApproving() {
	LOG_DEBUG(LOG_ECL, "gotoStateApproving");
	timer->start(MDB_CL_APPROVING_TIMEOUT);
	context->setState(State_Approving);
}

void CommandLayer::stateApprovingPacket() {
	LOG_DEBUG(LOG_ECL, "stateApprovingPacket");
	uint16_t operation;
	if(packet.getNumber(Tlv_OperationCode, &operation) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		context->incProtocolErrorCount();
		return;
	}

	if(operation == Operation_Wait) {
		LOG_DEBUG(LOG_ECL, "Operation wait");
		return;
	}

	if(operation != Operation_Sale) {
		LOG_DEBUG(LOG_ECL, "Unwaited operation " << operation);
		context->incProtocolErrorCount();
		return;
	}

	uint16_t result;
	if(packet.getNumber(Tlv_TransactionResult, &result) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		context->incProtocolErrorCount();
		return;
	}

	timer->stop();
	if(result == TransactionResult_Success) {
		LOG_INFO(LOG_ECL, "Payment approved");
		uint32_t credit;
		if(packet.getNumber(Tlv_Credit, &credit) == false) {
			LOG_ERROR(LOG_ECL, "Bad format");
			context->incProtocolErrorCount();
			return;
		}

		LOG_INFO(LOG_ECL, "credit=" << credit);
		context->setState(State_Wait);
		EventUint32Interface event(deviceId, MdbMasterCashless::Event_VendApproved, credit);
		eventEngine->transmit(&event);
		return;
	} else {
		LOG_INFO(LOG_ECL, "Payment denied");
		context->setState(State_Wait);
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateApprovingControl(uint8_t control) {
	LOG_DEBUG(LOG_ECL, "stateApprovingControl");
	if(control == Control_EOT) {
		context->setState(State_Wait);
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateApprovingTimeout() {
	LOG_DEBUG(LOG_ECL, "stateApprovingTimeout");
	context->setState(State_Wait);
	context->incProtocolErrorCount();
	EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStatePaymentCancel() {
	LOG_INFO(LOG_ECL, "gotoStatePaymentCancel");
	statePaymentCancelSend();
	repeatCount = 0;
	context->setState(State_PaymentCancel);
}

void CommandLayer::statePaymentCancelSend() {
	req.clear();
	req.addNumber(Tlv_Credit, 8, context->money2value(productPrice));
	req.addNumber(Tlv_Currency, 3, context->getCurrency());
	req.addNumber(Tlv_OperationCode, 2, 53);
	packetLayer->sendPacket(req.getBuf());
	timer->start(INPAS_RECV_TIMEOUT);
}

void CommandLayer::statePaymentCancelControl(uint8_t control) {
	LOG_INFO(LOG_ECL, "statePaymentCancelControl " << control);
	if(control == Control_ACK) {
		gotoStatePaymentCancelWait();
		return;
	}
	if(control == Control_EOT) {
		timer->stop();
		context->setState(State_Wait);
		EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::statePaymentCancelTimeout() {
	LOG_INFO(LOG_ECL, "statePaymentCancelTimeout");
	repeatCount++;
	if(repeatCount >= INPAS_CANCEL_TRY_NUMBER) {
		context->setState(State_Wait);
		EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
		eventEngine->transmit(&event);
		return;
	}
	statePaymentCancelSend();
}

void CommandLayer::gotoStatePaymentCancelWait() {
	LOG_DEBUG(LOG_ECL, "gotoStatePaymentCancelWait");
	timer->start(INPAS_CANCEL_TIMEOUT);
	context->setState(State_PaymentCancelWait);
}

void CommandLayer::statePaymentCancelWaitPacket() {
	LOG_DEBUG(LOG_ECL, "statePaymentCancelWaitPacket");
	uint16_t operation;
	if(packet.getNumber(Tlv_OperationCode, &operation) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		context->incProtocolErrorCount();
		return;
	}

	if(operation == Operation_Wait) {
		LOG_DEBUG(LOG_ECL, "Operation wait");
		return;
	}

	if(operation != Operation_Cancel) {
		LOG_DEBUG(LOG_ECL, "Unwaited operation " << operation);
		context->incProtocolErrorCount();
		return;
	}

	timer->stop();
	context->setState(State_Wait);
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void CommandLayer::statePaymentCancelWaitControl(uint8_t control) {
	LOG_DEBUG(LOG_ECL, "statePaymentCancelWaitControl");
	if(control == Control_EOT) {
		context->setState(State_Wait);
		EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::statePaymentCancelWaitTimeout() {
	LOG_DEBUG(LOG_ECL, "statePaymentCancelWaitTimeout");
	context->setState(State_Wait);
	context->incProtocolErrorCount();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void CommandLayer::stateClosingTimeout() {
	LOG_DEBUG(LOG_ECL, "stateClosingTimeout");
	context->setState(State_Wait);
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

//note: в Tlv_PrintText должны быть обязательно заданы хеадер и футер
//char qrText[] = "0xDF^^https://en.wikipedia.org/wiki/Thorax~";
void CommandLayer::gotoStateQrCode() {
	LOG_DEBUG(LOG_ECL, "gotoStateQrCode");
	req.clear();
	req.addNumber(Tlv_OperationCode, 2, 41);
	req.addNumber(Tlv_PrintTimeout, 2, 20);
	req.addNumber(Tlv_CommandMode, 1, 4);
	req.addString(Tlv_CheckShape, qrCodeData.getString(), qrCodeData.getLen());
	req.addString(Tlv_PrintText, qrCodeHeader.getString(), qrCodeHeader.getLen());
	LOG_HEX(req.getBuf()->getData(), req.getBuf()->getLen());
	packetLayer->sendPacket(req.getBuf());
	timer->start(30000);
	context->setState(State_QrCode);
}

void CommandLayer::stateQrCodeControl(uint8_t control) {
	LOG_DEBUG(LOG_ECL, "stateQrCodeControl " << control);
	if(control == Control_ACK) {
		gotoStateQrCodeWait();
		return;
	}
	if(control == Control_EOT) {
		LOG_ERROR(LOG_ECL, "QR-code failed");
		timer->stop();
		gotoStateWait();
		return;
	}
}

void CommandLayer::stateQrCodeTimeout() {
	LOG_DEBUG(LOG_ECL, "stateQrCodeTimeout");
	gotoStateWait();
}

void CommandLayer::gotoStateQrCodeWait() {
	LOG_DEBUG(LOG_ECL, "gotoStateQrCodeWait");
	context->setState(State_QrCodeWait);
}

void CommandLayer::stateQrCodeWaitPacket() {
	LOG_DEBUG(LOG_ECL, "stateQrCodeWaitPacket");
	uint32_t result;
	if(packet.getNumber(Tlv_CommandResult, &result) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		gotoStateWait();
		return;
	}

	StringBuilder request;
	if(packet.getString(Tlv_CashierRequest, &request) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		gotoStateWait();
		return;
	}

	LOG_DEBUG(LOG_ECL, "result=" << result);
	LOG_DEBUG(LOG_ECL, "request=" << request.getString());
	gotoStateWait();
}

void CommandLayer::stateQrCodeWaitTimeout() {
	LOG_DEBUG(LOG_ECL, "stateQrCodeWaitTimeout");
	gotoStateWait();
}
*/
}
