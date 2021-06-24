#include "TwocanCommandLayer.h"
#include "TwocanProtocol.h"

#include "mdb/master/cashless/MdbMasterCashless.h"
#include "utils/include/DecimalPoint.h"
#include "utils/include/CodePage.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace Twocan {

#if 1
CommandLayer::CommandLayer(
		Mdb::DeviceContext *context,
		PacketLayerInterface *packetLayer,
		TcpIp *conn,
		TimerEngine *timers,
		EventEngineInterface *eventEngine,
		uint32_t credit_
) :
					context(context),
					packetLayer(packetLayer),
					deviceLan(packetLayer, conn),
					timers(timers),
					eventEngine(eventEngine),
					deviceId(eventEngine),
					commandQueue(4),
					receive(TWOCAN_PACKET_SIZE),
					send(TWOCAN_PACKET_SIZE),
					qrCodeHeader(60, 60),
					qrCodeData(120, 120),
					terminalId(16, 16),
					transactResultCode(TransactionResult_Unknown),
					credit(credit_)
{
	this->context->setStatus(Mdb::DeviceContext::Status_NotFound);
	this->context->setState(State_Idle);
	this->context->init(2, 1);
	this->packetLayer->setObserver(this);
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
	context->setManufacturer((uint8_t*)TWOCAN_MANUFACTURER, sizeof(TWOCAN_MANUFACTURER));
	context->setModel((uint8_t*)TWOCAN_MODEL, sizeof(TWOCAN_MODEL));
	context->incResetCount();
	commandQueue.clear();
	packetLayer->reset();
	gotoStateInitDelay();
}

void CommandLayer::disable() {
	LOG_INFO(LOG_ECL, "disable");

}

void CommandLayer::enable() {
	LOG_INFO(LOG_ECL, "enable");

}

bool CommandLayer::sale(uint16_t productId, uint32_t productPrice) {
	LOG_INFO(LOG_ECL, "sale " << productId << "," << productPrice);

	if(context->getState() < State_Wait) {
		LOG_ERROR(LOG_ECL, "Wrong state " << context->getState());
		return false;
	}

	this->productPrice = productPrice;

	if(context->getState() != State_Wait && context->getState() != State_Session) {
		LOG_INFO(LOG_ECL, "Push Command_VendRequest");
		this->commandQueue.push(Command_VendRequest);
		return true;
	}

	gotoStateRequest();
	return true;
}

bool CommandLayer::saleComplete() {
	LOG_INFO(LOG_ECL, "saleComplete");
	if(context->getState() == State_Idle) {
		return false;
	}

	// Извещение мастеру, что продажа завершилась успешно
	send.clear();
	send.addUint8(Control_ACK);
	send.addUint8(TransactionResult_Success);
	packetLayer->sendPacket(&send);

	timer->start(1);
	context->setState(State_Closing);
	return true;
}

bool CommandLayer::saleFailed() {
	LOG_INFO(LOG_ECL, "saleFailed");
	if(context->getState() == State_Idle) {
		return false;
	}

	gotoStatePaymentCancel();
	return true;
}

bool CommandLayer::closeSession() {
	LOG_INFO(LOG_ECL, "closeSession");
	if(context->getState() == State_Idle) {
		return false;
	}

	if(context->getState() != State_Wait) {
		this->commandQueue.push(Command_SessionComplete);
		return true;
	}

	gotoStateClosing();
	return true;
}

//char qrText[] = "0xDF^^https://en.wikipedia.org/wiki/Thorax~";
bool CommandLayer::printQrCode(const char *header, const char *footer, const char *text) {
	LOG_INFO(LOG_ECL, "printQrCode");
	if(context->getState() == State_Idle) {
		return false;
	}

	qrCodeData.clear();
	qrCodeData << "0xDF^^" << text << "~";
	qrCodeHeader.clear();
	qrCodeHeader << header << "\n" << footer;
	LOG_INFO(LOG_ECL, "qrCodeHeader=" << qrCodeHeader.getString() << qrCodeHeader.getLen());
	LOG_INFO(LOG_ECL, "qrCodeData=" << qrCodeData.getString() << qrCodeData.getLen());

	if(context->getState() != State_Wait) {
		this->commandQueue.push(Command_QrCode);
		return true;
	}

	gotoStateQrCode();
	return true;
}

bool CommandLayer::verification() {
	LOG_DEBUG(LOG_ECL, "verification");
	if(context->getState() == State_Idle) {
		return false;
	}

	if(context->getState() != State_Wait) {
		this->commandQueue.push(Command_Verification);
		return true;
	}

	gotoStateVerification();
	return true;
}

void CommandLayer::procPacket(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_ECL, "procPacket " << dataLen);
	LOG_TRACE_HEX(LOG_ECL, data, dataLen);
	packetLayer->sendControl(Control_ACK);

	if(dataLen > 0) {
		LOG_TRACE_HEX(LOG_ECL, data, dataLen);
		transactResultCode = data[0];
	}

	switch(context->getState()) {
	case State_Init: stateInitPacket(); break;
	case State_Wait: stateWaitPacket(); break;
	case State_Approving: stateApprovingPacket(); break;
	case State_PaymentCancelWait: statePaymentCancelWaitPacket(); break;
	case State_QrCodeWait: stateQrCodeWaitPacket(); break;
	case State_Verification: stateVerificationPacket(); break;
	default: LOG_ERROR(LOG_ECL, "Unwaited data " << context->getState());
	}
}

void CommandLayer::procControl(uint8_t control) {
	LOG_DEBUG(LOG_ECL, "procControl");

	switch(context->getState()) {
	case State_Init: stateInitControl(control); break;
	case State_Session: stateSessionControl(control); break;
	case State_Request: stateRequestControl(control); break;
	case State_Approving: stateApprovingControl(control); break;
	case State_PaymentCancel: statePaymentCancelControl(control); break;
	case State_PaymentCancelWait: statePaymentCancelWaitControl(control); break;
	case State_QrCode: stateQrCodeControl(control); break;
	case State_Verification: stateVerificationControl(control); break;
	default: LOG_ERROR(LOG_ECL, "Unwaited data " << context->getState() << "," << control);
	}
}

void CommandLayer::procError(Error error) {
	LOG_DEBUG(LOG_ECL, "procError " << error);
}

void CommandLayer::procTimer() {
	LOG_DEBUG(LOG_ECL, "procTimer");
	switch(context->getState()) {
	case State_Init: stateInitTimeout(); break;
	case State_InitDelay: stateInitDelayTimeout(); break;
	case State_Wait: stateWaitTimeout(); break;
	case State_Session: stateSessionTimeout(); break;
	case State_Request: stateRequestTimeout(); break;
	case State_Approving: stateApprovingTimeout(); break;
	case State_PaymentCancel: statePaymentCancelTimeout(); break;
	case State_PaymentCancelWait: statePaymentCancelWaitTimeout(); break;
	case State_Closing: stateClosingTimeout(); break;
	case State_QrCode: stateQrCodeTimeout(); break;
	case State_QrCodeWait: stateQrCodeWaitTimeout(); break;
	case State_Verification: stateVerificationTimeout(); break;
	default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << context->getState());
	}
}

bool CommandLayer::procCommand() {
	LOG_DEBUG(LOG_ECL, "procCommand");
	if(commandQueue.isEmpty() == true) {
		return false;
	}

	uint8_t command = commandQueue.pop();
	if(command == Command_Verification) {
		gotoStateVerification();
		return true;
	}
	if(command == Command_VendRequest) {
		gotoStateRequest();
		return true;
	}
	if(command == Command_SessionComplete) {
		gotoStateClosing();
		return true;
	}
	if(command == Command_QrCode) {
		gotoStateQrCode();
		return true;
	}

	return false;
}

void CommandLayer::gotoStateInit() {
	LOG_DEBUG(LOG_ECL, "------------------------------------->gotoStateInit");
	send.clear();
	send.addUint8(Tlv_OperationCode);
	send.addUint8(Operation_CheckLink);
	packetLayer->sendPacket(&send);
	timer->start(TWOCAN_RECV_TIMEOUT);
	context->setState(State_Init);
}

void CommandLayer::stateInitControl(uint8_t control) {
	LOG_DEBUG(LOG_ECL, "------------------------------------->stateInitControl, control = " << control);
	if(control == Control_ACK) {
		timer->start(TWOCAN_INIT_TIEMOUT);
		context->setStatus(Mdb::DeviceContext::Status_Init);
		return;
	}
	if(control == Control_EOT) {
		gotoStateInitDelay();
		return;
	}
}

void CommandLayer::stateInitPacket() {
	LOG_DEBUG(LOG_ECL, "------------------------------------->stateInitPacket");
	uint16_t operation = transactResultCode;

	if(operation == Operation_Wait) {
		LOG_DEBUG(LOG_ECL, "Operation wait");
		timer->start(TWOCAN_INIT_TIEMOUT);
		context->setStatus(Mdb::DeviceContext::Status_Init);
		return;
	}

	if(operation != Operation_CheckLink) {
		LOG_DEBUG(LOG_ECL, "Unwaited operation " << operation);
		context->incProtocolErrorCount();
		return;
	}

	uint16_t result = TransactionResult_Success;

	timer->stop();
	if(result == TransactionResult_Success) {
		LOG_INFO(LOG_ECL, "Link OK");
		context->setStatus(Mdb::DeviceContext::Status_Work);
		gotoStateWait();
		return;
	} else {
		LOG_INFO(LOG_ECL, "Link ERROR");
		context->setStatus(Mdb::DeviceContext::Status_Error);
		gotoStateInitDelay();
		return;
	}
}

void CommandLayer::stateInitTimeout() {
	LOG_DEBUG(LOG_ECL, "stateInitTimeout");
	gotoStateInitDelay();
}

void CommandLayer::gotoStateInitDelay() {
	LOG_DEBUG(LOG_ECL, "gotoStateInitDelay");
	timer->start(TWOCAN_INIT_DELAY);
	context->setState(State_InitDelay);
}

void CommandLayer::stateInitDelayTimeout() {
	LOG_DEBUG(LOG_ECL, "stateInitDelayTimeout");
	gotoStateInit();
}

void CommandLayer::gotoStateWait() {
	LOG_DEBUG(LOG_ECL, "------------------------------------->gotoStateWait");
	timer->start(TWOCAN_WAIT_DELAY);
	context->setState(State_Wait);
}

void CommandLayer::stateWaitPacket() {
	LOG_DEBUG(LOG_ECL, "------------------------------------->stateWaitPacket");
	uint16_t currency = context->getCurrency();
	uint32_t decimalPoint = context->getDecimalPoint();
	uint32_t scaleFactor = context->getScalingFactor();

	LOG_DEBUG(LOG_ECL, "credit=" << credit);
	LOG_DEBUG(LOG_ECL, "currency=" << currency);
	LOG_DEBUG(LOG_ECL, "decimalPoint=" << decimalPoint);
	LOG_DEBUG(LOG_ECL, "scaleFactor=" << scaleFactor);

	context->init(decimalPoint, scaleFactor);
	context->setCurrency(currency);

	gotoStateSession();
	EventUint32Interface event(deviceId, MdbMasterCashless::Event_SessionBegin,  context->value2money(credit));
	eventEngine->transmit(&event);
}

void CommandLayer::stateWaitTimeout() {
	LOG_DEBUG(LOG_ECL, "stateWaitTimeout");
	procCommand();
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
		gotoStateWait();
		EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateSessionTimeout() {
	LOG_DEBUG(LOG_ECL, "stateSessionTimeout");
	gotoStateWait();

	// Извещение мастеру, что продажа завершилась успешно
	send.clear();
	send.addUint8(Control_ACK);
	send.addUint8(TransactionResult_Success);
	packetLayer->sendPacket(&send);

	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateRequest() {
	LOG_DEBUG(LOG_ECL, "gotoStateRequest");
	// TODO:
	send.clear();
	send.addUint8(Tlv_OperationCode);
	send.addUint8(Operation_Sale);
	send.add(&this->productPrice, sizeof(this->productPrice));
	packetLayer->sendPacket(&send);
	timer->start(TWOCAN_RECV_TIMEOUT);
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
		gotoStateWait();
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateRequestTimeout() {
	LOG_DEBUG(LOG_ECL, "stateRequestTimeout");
	gotoStateWait();
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
	uint16_t operation = Operation_Sale;

	if(operation == Operation_Wait) {
		LOG_DEBUG(LOG_ECL, "Operation wait");
		return;
	}

	if(operation != Operation_Sale) {
		LOG_DEBUG(LOG_ECL, "Unwaited operation " << operation);
		context->incProtocolErrorCount();
		return;
	}

	timer->stop();
	if(transactResultCode == TransactionResult_Success) {
		LOG_INFO(LOG_ECL, "Payment approved");
		uint32_t credit = productPrice;
		LOG_INFO(LOG_ECL, "credit=" << credit);
		context->setStatus(Mdb::DeviceContext::Status_Work);
		gotoStateWait();
		MdbMasterCashlessInterface::EventApproved event(deviceId, Fiscal::Payment_Cashless, context->value2money(credit));
		eventEngine->transmit(&event);
		return;
	} else if(transactResultCode == TransactionResult_Denied || transactResultCode == TransactionResult_Aborted) {
		LOG_INFO(LOG_ECL, "Payment denied");
		context->setStatus(Mdb::DeviceContext::Status_Work);
		gotoStateWait();
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
		return;
	} else {
		LOG_INFO(LOG_ECL, "Payment error");
		context->setStatus(Mdb::DeviceContext::Status_Error);
		gotoStateWait();
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateApprovingControl(uint8_t control) {
	LOG_DEBUG(LOG_ECL, "stateApprovingControl, control " << control);
	if(control == Control_EOT) {
		gotoStateWait();
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateApprovingTimeout() {
	LOG_DEBUG(LOG_ECL, "stateApprovingTimeout");
	context->incProtocolErrorCount();
	gotoStateWait();
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
	//TODO:
	//	req.clear();
	//	req.addNumber(Tlv_Credit, 8, context->money2value(productPrice));
	//	req.addNumber(Tlv_Currency, 3, context->getCurrency());
	//	req.addNumber(Tlv_OperationCode, 2, 53);
	//	packetLayer->sendPacket(req.getBuf());
	//	timer->start(TWOCAN_RECV_TIMEOUT);
}

void CommandLayer::statePaymentCancelControl(uint8_t control) {
	LOG_INFO(LOG_ECL, "statePaymentCancelControl " << control);
	if(control == Control_ACK) {
		gotoStatePaymentCancelWait();
		return;
	}
	if(control == Control_EOT) {
		timer->stop();
		gotoStateWait();

		// Извещение мастеру, что продажа завершилась успешно
		send.clear();
		send.addUint8(Control_ACK);
		send.addUint8(TransactionResult_Success);
		packetLayer->sendPacket(&send);

		EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::statePaymentCancelTimeout() {
	LOG_INFO(LOG_ECL, "statePaymentCancelTimeout");
	repeatCount++;
	if(repeatCount >= TWOCAN_CANCEL_TRY_NUMBER) {
		gotoStateWait();

		// Извещение мастеру, что продажа завершилась успешно
		send.clear();
		send.addUint8(Control_ACK);
		send.addUint8(TransactionResult_Success);
		packetLayer->sendPacket(&send);

		EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
		eventEngine->transmit(&event);
		return;
	}
	statePaymentCancelSend();
}

void CommandLayer::gotoStatePaymentCancelWait() {
	LOG_DEBUG(LOG_ECL, "gotoStatePaymentCancelWait");
	timer->start(TWOCAN_CANCEL_TIMEOUT);
	context->setState(State_PaymentCancelWait);
}

void CommandLayer::statePaymentCancelWaitPacket() {
	LOG_DEBUG(LOG_ECL, "statePaymentCancelWaitPacket");
	uint16_t operation;
	// TODO:
	//	if(packet.getNumber(Tlv_OperationCode, &operation) == false) {
	//		LOG_ERROR(LOG_ECL, "Bad format");
	//		context->incProtocolErrorCount();
	//		return;
	//	}

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
	gotoStateWait();

	// Извещение мастеру, что продажа завершилась успешно
	send.clear();
	send.addUint8(Control_ACK);
	send.addUint8(TransactionResult_Success);
	packetLayer->sendPacket(&send);

	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void CommandLayer::statePaymentCancelWaitControl(uint8_t control) {
	LOG_DEBUG(LOG_ECL, "statePaymentCancelWaitControl");
	if(control == Control_EOT) {
		gotoStateWait();

		// Извещение мастеру, что продажа завершилась успешно
		send.clear();
		send.addUint8(Control_ACK);
		send.addUint8(TransactionResult_Success);
		packetLayer->sendPacket(&send);

		EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::statePaymentCancelWaitTimeout() {
	LOG_DEBUG(LOG_ECL, "statePaymentCancelWaitTimeout");
	context->incProtocolErrorCount();
	gotoStateWait();

	// Извещение мастеру, что продажа завершилась успешно
	send.clear();
	send.addUint8(Control_ACK);
	send.addUint8(TransactionResult_Success);
	packetLayer->sendPacket(&send);

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
	gotoStateWait();

	// Извещение мастеру, что продажа завершилась успешно
	send.clear();
	send.addUint8(Control_ACK);
	send.addUint8(TransactionResult_Success);
	packetLayer->sendPacket(&send);

	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

//note: ? Tlv_PrintText ?????? ???? ??????????? ?????? ?????? ? ?????
//char qrText[] = "0xDF^^https://en.wikipedia.org/wiki/Thorax~";
void CommandLayer::gotoStateQrCode() {
	LOG_DEBUG(LOG_ECL, "gotoStateQrCode");
	// TODO:
	//	req.clear();
	//	req.addNumber(Tlv_OperationCode, 2, 41);
	//	req.addNumber(Tlv_PrintTimeout, 2, TWOCAN_QRCODE_TIMEOUT / 1000);
	//	req.addNumber(Tlv_CommandMode, 1, 4);
	//	req.addString(Tlv_CheckShape, qrCodeData.getString(), qrCodeData.getLen());
	//	req.addString(Tlv_PrintText, qrCodeHeader.getString(), qrCodeHeader.getLen());
	//	packetLayer->sendPacket(req.getBuf());
	//	timer->start(TWOCAN_RECV_TIMEOUT);
	//	context->setState(State_QrCode);
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
	timer->start(TWOCAN_QRCODE_TIMEOUT*2);
	context->setState(State_QrCodeWait);
}

void CommandLayer::stateQrCodeWaitPacket() {
	LOG_DEBUG(LOG_ECL, "stateQrCodeWaitPacket");
	uint32_t mode;
	// TODO:
	//	if(packet.getNumber(Tlv_CommandMode, &mode) == false) {
	//		LOG_ERROR(LOG_ECL, "Bad format");
	//		gotoStateWait();
	//		return;
	//	}

	uint32_t result;
	// TODO:
	//	if(packet.getNumber(Tlv_CommandResult, &result) == false) {
	//		LOG_ERROR(LOG_ECL, "Bad format");
	//		gotoStateWait();
	//		return;
	//	}

	LOG_DEBUG(LOG_ECL, "mode=" << mode);
	LOG_DEBUG(LOG_ECL, "result=" << result);
	gotoStateWait();
}

void CommandLayer::stateQrCodeWaitTimeout() {
	LOG_DEBUG(LOG_ECL, "stateQrCodeWaitTimeout");
	gotoStateWait();
}

void CommandLayer::gotoStateVerification() {
	LOG_DEBUG(LOG_ECL, "gotoStateVerification");
	StringBuilder text;
	text << "Hello world";

	// TODO:
	//	req.clear();
	//	req.addNumber(Tlv_OperationCode, 2, Operation_Message);
	//	req.addNumber(Tlv_PrintTimeout, 3, 999);
	//	req.addNumber(Tlv_CommandMode, 1, 2);
	//	req.addString(Tlv_PrintText, text.getString(), text.getLen());
	//	packetLayer->sendPacket(req.getBuf());
	//	timer->start(30000);
	//	context->setState(State_Verification);
}

void CommandLayer::stateVerificationControl(uint8_t control) {
	LOG_DEBUG(LOG_ECL, "stateVerificationControl " << control);
	if(control == Control_ACK) {
		timer->start(TWOCAN_VERIFICATION_TIEMOUT);
		return;
	}

	if(control == Control_EOT) {
		LOG_ERROR(LOG_ECL, "QR-code failed");
		timer->stop();
		gotoStateWait();
		return;
	}
}

void CommandLayer::stateVerificationPacket() {
	LOG_DEBUG(LOG_ECL, "stateVerificationPacket");
	uint16_t operation;
	// TODO:
	//	if(packet.getNumber(Tlv_OperationCode, &operation) == false) {
	//		LOG_ERROR(LOG_ECL, "Bad format");
	//		context->incProtocolErrorCount();
	//		return;
	//	}

	if(operation == Operation_Wait) {
		LOG_DEBUG(LOG_ECL, "Operation wait");
		return;
	}

	if(operation != Operation_Verification) {
		LOG_DEBUG(LOG_ECL, "Unwaited operation " << operation);
		context->incProtocolErrorCount();
		return;
	}

	uint16_t result;
	// TODO:
	//	if(packet.getNumber(Tlv_TransactionResult, &result) == false) {
	//		LOG_ERROR(LOG_ECL, "Bad format");
	//		context->incProtocolErrorCount();
	//		return;
	//	}

	timer->stop();
	if(result == TransactionResult_Success) {
		LOG_INFO(LOG_ECL, "Verification succeed");
		gotoStateWait();
		return;
	} else {
		LOG_INFO(LOG_ECL, "Verification failed");
		gotoStateInit();
		return;
	}
}

void CommandLayer::stateVerificationTimeout() {
	LOG_DEBUG(LOG_ECL, "stateVerificationTimeout");
	gotoStateWait();
}

#else
CommandLayer::CommandLayer(
		Mdb::DeviceContext *context,
		PacketLayerInterface *packetLayer,
		TcpIp *conn,
		TimerEngine *timers,
		EventEngineInterface *eventEngine
) :
					context(context),
					packetLayer(packetLayer),
					deviceLan(packetLayer, conn),
					timers(timers),
					eventEngine(eventEngine),
					deviceId(eventEngine),
					commandQueue(4),
					packet(20),
					req(TWOCAN_PACKET_SIZE),
					buf(TWOCAN_PACKET_SIZE),
					qrCodeHeader(60, 60),
					qrCodeData(120, 120),
					terminalId(16, 16)
{
	this->context->setStatus(Mdb::DeviceContext::Status_NotFound);
	this->context->setState(State_Idle);
	this->context->init(2, 1);
	this->packetLayer->setObserver(this);
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
	context->setManufacturer((uint8_t*)TWOCAN_MANUFACTURER, sizeof(TWOCAN_MANUFACTURER));
	context->setModel((uint8_t*)TWOCAN_MODEL, sizeof(TWOCAN_MODEL));
	context->incResetCount();
	commandQueue.clear();
	packetLayer->reset();
	gotoStateInitDelay();
}

void CommandLayer::disable() {
	LOG_INFO(LOG_ECL, "disable");

}

void CommandLayer::enable() {
	LOG_INFO(LOG_ECL, "enable");

}

bool CommandLayer::sale(uint16_t productId, uint32_t productPrice) {
	LOG_INFO(LOG_ECL, "sale " << productId << "," << productPrice);

	if(context->getState() < State_Wait) {
		LOG_ERROR(LOG_ECL, "Wrong state " << context->getState());
		return false;
	}

	this->productPrice = productPrice/100;

	if(context->getState() != State_Wait && context->getState() != State_Session) {
		this->commandQueue.push(Command_VendRequest);
		return true;
	}

	gotoStateRequest();
	return true;
}

bool CommandLayer::saleComplete() {
	LOG_INFO(LOG_ECL, "saleComplete");
	if(context->getState() == State_Idle) {
		return false;
	}

	timer->start(1);
	context->setState(State_Closing);
	return true;
}

bool CommandLayer::saleFailed() {
	LOG_INFO(LOG_ECL, "saleFailed");
	if(context->getState() == State_Idle) {
		return false;
	}

	gotoStatePaymentCancel();
	return true;
}

bool CommandLayer::closeSession() {
	LOG_INFO(LOG_ECL, "closeSession");
	if(context->getState() == State_Idle) {
		return false;
	}

	if(context->getState() != State_Wait) {
		this->commandQueue.push(Command_SessionComplete);
		return true;
	}

	gotoStateClosing();
	return true;
}

//char qrText[] = "0xDF^^https://en.wikipedia.org/wiki/Thorax~";
bool CommandLayer::printQrCode(const char *header, const char *footer, const char *text) {
	LOG_INFO(LOG_ECL, "printQrCode");
	if(context->getState() == State_Idle) {
		return false;
	}

	qrCodeData.clear();
	qrCodeData << "0xDF^^" << text << "~";
	qrCodeHeader.clear();
	qrCodeHeader << header << "\n" << footer;
	LOG_INFO(LOG_ECL, "qrCodeHeader=" << qrCodeHeader.getString() << qrCodeHeader.getLen());
	LOG_INFO(LOG_ECL, "qrCodeData=" << qrCodeData.getString() << qrCodeData.getLen());

	if(context->getState() != State_Wait) {
		this->commandQueue.push(Command_QrCode);
		return true;
	}

	gotoStateQrCode();
	return true;
}

bool CommandLayer::verification() {
	LOG_DEBUG(LOG_ECL, "verification");
	if(context->getState() == State_Idle) {
		return false;
	}

	if(context->getState() != State_Wait) {
		this->commandQueue.push(Command_Verification);
		return true;
	}

	gotoStateVerification();
	return true;
}

void CommandLayer::procPacket(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_ECL, "procPacket");
	LOG_TRACE_HEX(LOG_ECL, data, dataLen);
	packetLayer->sendControl(Control_ACK);

	if(packet.parse(data, dataLen) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		LOG_ERROR_HEX(LOG_ECL, data, dataLen);
		context->incProtocolErrorCount();
		return;
	}

	uint16_t operation;
	if(packet.getNumber(Tlv_OperationCode, &operation) == true) {
		if(operation == Operation_Net) {
			LOG_DEBUG(LOG_ECL, "Operation net");
			deviceLan.procRequest(&packet);
			return;
		}
	}

	switch(context->getState()) {
	case State_Init: stateInitPacket(); break;
	case State_Wait: stateWaitPacket(); break;
	case State_Approving: stateApprovingPacket(); break;
	case State_PaymentCancelWait: statePaymentCancelWaitPacket(); break;
	case State_QrCodeWait: stateQrCodeWaitPacket(); break;
	case State_Verification: stateVerificationPacket(); break;
	default: LOG_ERROR(LOG_ECL, "Unwaited data " << context->getState());
	}
}

void CommandLayer::procControl(uint8_t control) {
	LOG_DEBUG(LOG_ECL, "procControl");

	stateApprovingControl(control);
	return;

	switch(context->getState()) {
	case State_Init: stateInitControl(control); break;
	case State_Session: stateSessionControl(control); break;
	case State_Request: stateRequestControl(control); break;
	case State_Approving: stateApprovingControl(control); break;
	case State_PaymentCancel: statePaymentCancelControl(control); break;
	case State_PaymentCancelWait: statePaymentCancelWaitControl(control); break;
	case State_QrCode: stateQrCodeControl(control); break;
	case State_Verification: stateVerificationControl(control); break;
	default: LOG_ERROR(LOG_ECL, "Unwaited data " << context->getState() << "," << control);
	}
}

void CommandLayer::procError(Error error) {
	LOG_DEBUG(LOG_ECL, "procError " << error);
}

void CommandLayer::procTimer() {
	LOG_DEBUG(LOG_ECL, "procTimer");
	switch(context->getState()) {
	case State_Init: stateInitTimeout(); break;
	case State_InitDelay: stateInitDelayTimeout(); break;
	case State_Wait: stateWaitTimeout(); break;
	case State_Session: stateSessionTimeout(); break;
	case State_Request: stateRequestTimeout(); break;
	case State_Approving: stateApprovingTimeout(); break;
	case State_PaymentCancel: statePaymentCancelTimeout(); break;
	case State_PaymentCancelWait: statePaymentCancelWaitTimeout(); break;
	case State_Closing: stateClosingTimeout(); break;
	case State_QrCode: stateQrCodeTimeout(); break;
	case State_QrCodeWait: stateQrCodeWaitTimeout(); break;
	case State_Verification: stateVerificationTimeout(); break;
	default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << context->getState());
	}
}

bool CommandLayer::procCommand() {
	LOG_DEBUG(LOG_ECL, "procCommand");
	if(commandQueue.isEmpty() == true) {
		return false;
	}

	uint8_t command = commandQueue.pop();
	if(command == Command_Verification) {
		gotoStateVerification();
		return true;
	}
	if(command == Command_VendRequest) {
		gotoStateRequest();
		return true;
	}
	if(command == Command_SessionComplete) {
		gotoStateClosing();
		return true;
	}
	if(command == Command_QrCode) {
		gotoStateQrCode();
		return true;
	}

	return false;
}

void CommandLayer::gotoStateInit() {
	LOG_DEBUG(LOG_ECL, "gotoStateInit");
	req.clear();
	req.addNumber(Tlv_OperationCode, 2, Operation_CheckLink);
	req.addString(Tlv_TerminalId, 0, 0);
	packetLayer->sendPacket(req.getBuf());
	timer->start(TWOCAN_RECV_TIMEOUT);
	context->setState(State_Init);
//	LOG_DEBUG(LOG_ECL, "gotoStateInit");
//	buf.clear();
//	buf.addUint8(0xF0);
//	buf.addUint8(0x02);
//	buf.addUint8(0x00);
//	packetLayer->sendPacket(&buf);
//	timer->start(TWOCAN_RECV_TIMEOUT);
//	context->setState(State_Init);
}

void CommandLayer::stateInitControl(uint8_t control) {
	LOG_DEBUG(LOG_ECL, "stateInitPacket");
	if(control == Control_ACK) {
		timer->start(TWOCAN_INIT_TIEMOUT);
		context->setStatus(Mdb::DeviceContext::Status_Init);
		return;
	}
	if(control == Control_EOT) {
		gotoStateInitDelay();
		return;
	}
}

void CommandLayer::stateInitPacket() {
	LOG_DEBUG(LOG_ECL, "stateInitPacket");
	uint16_t operation;
	if(packet.getNumber(Tlv_OperationCode, &operation) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		context->incProtocolErrorCount();
		return;
	}

	if(operation == Operation_Wait) {
		LOG_DEBUG(LOG_ECL, "Operation wait");
		timer->start(TWOCAN_INIT_TIEMOUT);
		context->setStatus(Mdb::DeviceContext::Status_Init);
		return;
	}

	if(operation != Operation_CheckLink) {
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

	if(packet.getString(Tlv_TerminalId, &terminalId) == true) {
		context->setSerialNumber(terminalId.getData(), terminalId.getLen());
	}

	timer->stop();
	if(result == TransactionResult_Success) {
		LOG_INFO(LOG_ECL, "Link OK");
		context->setStatus(Mdb::DeviceContext::Status_Work);
		gotoStateWait();
		return;
	} else {
		LOG_INFO(LOG_ECL, "Link ERROR");
		context->setStatus(Mdb::DeviceContext::Status_Error);
		gotoStateInitDelay();
		return;
	}
}

void CommandLayer::stateInitTimeout() {
	LOG_DEBUG(LOG_ECL, "stateInitTimeout");
	gotoStateInitDelay();
}

void CommandLayer::gotoStateInitDelay() {
	LOG_DEBUG(LOG_ECL, "gotoStateInitDelay");
	timer->start(TWOCAN_INIT_DELAY);
	context->setState(State_InitDelay);
}

void CommandLayer::stateInitDelayTimeout() {
	LOG_DEBUG(LOG_ECL, "stateInitDelayTimeout");
	gotoStateInit();
}

void CommandLayer::gotoStateWait() {
	LOG_DEBUG(LOG_ECL, "gotoStateWait");
	timer->start(TWOCAN_WAIT_DELAY);
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

	gotoStateSession();
	EventUint32Interface event(deviceId, MdbMasterCashless::Event_SessionBegin,  context->value2money(credit));
	eventEngine->transmit(&event);
}

void CommandLayer::stateWaitTimeout() {
	LOG_DEBUG(LOG_ECL, "stateWaitTimeout");
	procCommand();
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
		gotoStateWait();
		EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateSessionTimeout() {
	LOG_DEBUG(LOG_ECL, "stateSessionTimeout");
	gotoStateWait();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateRequest() {
	LOG_DEBUG(LOG_ECL, "gotoStateRequest");
	buf.clear();
	req.addNumber(Tlv_Credit, 8, context->money2value(productPrice));
	req.addNumber(Tlv_Currency, 3, context->getCurrency());
	req.addNumber(Tlv_OperationCode, 1, 1);
	packetLayer->sendPacket(&buf);
	timer->start(TWOCAN_RECV_TIMEOUT);
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
		gotoStateWait();
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateRequestTimeout() {
	LOG_DEBUG(LOG_ECL, "stateRequestTimeout");
	gotoStateWait();
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
		context->setStatus(Mdb::DeviceContext::Status_Work);
		gotoStateWait();
		MdbMasterCashlessInterface::EventApproved event(deviceId, Fiscal::Payment_Cashless, context->value2money(credit));
		eventEngine->transmit(&event);
		return;
	} else if(result == TransactionResult_Denied || result == TransactionResult_Aborted) {
		LOG_INFO(LOG_ECL, "Payment denied");
		context->setStatus(Mdb::DeviceContext::Status_Work);
		gotoStateWait();
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
		return;
	} else {
		LOG_INFO(LOG_ECL, "Payment error");
		context->setStatus(Mdb::DeviceContext::Status_Error);
		gotoStateWait();
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateApprovingControl(uint8_t control) {
	LOG_DEBUG(LOG_ECL, "stateApprovingControl");
	if(control == Control_EOT) {
		gotoStateWait();
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateApprovingTimeout() {
	LOG_DEBUG(LOG_ECL, "stateApprovingTimeout");
	context->incProtocolErrorCount();
	gotoStateWait();
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
	timer->start(TWOCAN_RECV_TIMEOUT);
}

void CommandLayer::statePaymentCancelControl(uint8_t control) {
	LOG_INFO(LOG_ECL, "statePaymentCancelControl " << control);
	if(control == Control_ACK) {
		gotoStatePaymentCancelWait();
		return;
	}
	if(control == Control_EOT) {
		timer->stop();
		gotoStateWait();
		EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::statePaymentCancelTimeout() {
	LOG_INFO(LOG_ECL, "statePaymentCancelTimeout");
	repeatCount++;
	if(repeatCount >= TWOCAN_CANCEL_TRY_NUMBER) {
		gotoStateWait();
		EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
		eventEngine->transmit(&event);
		return;
	}
	statePaymentCancelSend();
}

void CommandLayer::gotoStatePaymentCancelWait() {
	LOG_DEBUG(LOG_ECL, "gotoStatePaymentCancelWait");
	timer->start(TWOCAN_CANCEL_TIMEOUT);
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
	gotoStateWait();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void CommandLayer::statePaymentCancelWaitControl(uint8_t control) {
	LOG_DEBUG(LOG_ECL, "statePaymentCancelWaitControl");
	if(control == Control_EOT) {
		gotoStateWait();
		EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::statePaymentCancelWaitTimeout() {
	LOG_DEBUG(LOG_ECL, "statePaymentCancelWaitTimeout");
	context->incProtocolErrorCount();
	gotoStateWait();
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
	gotoStateWait();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

//note: ? Tlv_PrintText ?????? ???? ??????????? ?????? ?????? ? ?????
//char qrText[] = "0xDF^^https://en.wikipedia.org/wiki/Thorax~";
void CommandLayer::gotoStateQrCode() {
	LOG_DEBUG(LOG_ECL, "gotoStateQrCode");
	req.clear();
	req.addNumber(Tlv_OperationCode, 2, 41);
	req.addNumber(Tlv_PrintTimeout, 2, TWOCAN_QRCODE_TIMEOUT / 1000);
	req.addNumber(Tlv_CommandMode, 1, 4);
	req.addString(Tlv_CheckShape, qrCodeData.getString(), qrCodeData.getLen());
	req.addString(Tlv_PrintText, qrCodeHeader.getString(), qrCodeHeader.getLen());
	packetLayer->sendPacket(req.getBuf());
	timer->start(TWOCAN_RECV_TIMEOUT);
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
	timer->start(TWOCAN_QRCODE_TIMEOUT*2);
	context->setState(State_QrCodeWait);
}

void CommandLayer::stateQrCodeWaitPacket() {
	LOG_DEBUG(LOG_ECL, "stateQrCodeWaitPacket");
	uint32_t mode;
	if(packet.getNumber(Tlv_CommandMode, &mode) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		gotoStateWait();
		return;
	}

	uint32_t result;
	if(packet.getNumber(Tlv_CommandResult, &result) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		gotoStateWait();
		return;
	}

	LOG_DEBUG(LOG_ECL, "mode=" << mode);
	LOG_DEBUG(LOG_ECL, "result=" << result);
	gotoStateWait();
}

void CommandLayer::stateQrCodeWaitTimeout() {
	LOG_DEBUG(LOG_ECL, "stateQrCodeWaitTimeout");
	gotoStateWait();
}

void CommandLayer::gotoStateVerification() {
	LOG_DEBUG(LOG_ECL, "gotoStateVerification");
	StringBuilder text;
	text << "Hello world";

	req.clear();
	req.addNumber(Tlv_OperationCode, 2, Operation_Message);
	req.addNumber(Tlv_PrintTimeout, 3, 999);
	req.addNumber(Tlv_CommandMode, 1, 2);
	req.addString(Tlv_PrintText, text.getString(), text.getLen());
	packetLayer->sendPacket(req.getBuf());
	timer->start(30000);
	context->setState(State_Verification);
}

void CommandLayer::stateVerificationControl(uint8_t control) {
	LOG_DEBUG(LOG_ECL, "stateVerificationControl " << control);
	if(control == Control_ACK) {
		timer->start(TWOCAN_VERIFICATION_TIEMOUT);
		return;
	}

	if(control == Control_EOT) {
		LOG_ERROR(LOG_ECL, "QR-code failed");
		timer->stop();
		gotoStateWait();
		return;
	}
}

void CommandLayer::stateVerificationPacket() {
	LOG_DEBUG(LOG_ECL, "stateVerificationPacket");
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

	if(operation != Operation_Verification) {
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
		LOG_INFO(LOG_ECL, "Verification succeed");
		gotoStateWait();
		return;
	} else {
		LOG_INFO(LOG_ECL, "Verification failed");
		gotoStateInit();
		return;
	}
}

void CommandLayer::stateVerificationTimeout() {
	LOG_DEBUG(LOG_ECL, "stateVerificationTimeout");
	gotoStateWait();
}
#endif

}
