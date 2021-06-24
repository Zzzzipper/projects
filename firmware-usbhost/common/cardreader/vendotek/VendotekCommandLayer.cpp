#include "VendotekCommandLayer.h"
#include "VendotekProtocol.h"

#include "mdb/master/cashless/MdbMasterCashless.h"
#include "utils/include/DecimalPoint.h"
#include "utils/include/CodePage.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace Vendotek {

CommandLayer::CommandLayer(
	Mdb::DeviceContext *context,
	PacketLayerInterface *packetLayer,
	TcpIp *conn,
	TimerEngine *timerEngine,
	EventEngineInterface *eventEngine,
	RealTimeInterface *realtime,
	uint32_t maxCredit
) :
	context(context),
	packetLayer(packetLayer),
	deviceLan(packetLayer, conn),
	timerEngine(timerEngine),
	eventEngine(eventEngine),
	realtime(realtime),
	deviceId(eventEngine),
	maxCredit(maxCredit),
	req(VENDOTEK_PACKET_SIZE),
	resp(VENDOTEK_PACKET_SIZE),
	operationId(0),
	messageName(VENDOTEK_MESSAGENAME_SIZE),
	qrCodeData(120, 120)
{
	this->context->setManufacturer((uint8_t*)VENDOTEK_MANUFACTURER, sizeof(VENDOTEK_MANUFACTURER));
	this->context->setModel((uint8_t*)VENDOTEK_MODEL, sizeof(VENDOTEK_MODEL));
	this->context->setStatus(Mdb::DeviceContext::Status_NotFound);
	this->context->setState(State_Idle);
	this->context->init(2, 1);
	this->packetLayer->setObserver(this);
	this->timer = timerEngine->addTimer<CommandLayer, &CommandLayer::procTimer>(this);
	this->cancelTimer = timerEngine->addTimer<CommandLayer, &CommandLayer::procCancelTimer>(this);
}

CommandLayer::~CommandLayer() {
	timerEngine->deleteTimer(this->cancelTimer);
	timerEngine->deleteTimer(this->timer);
}

EventDeviceId CommandLayer::getDeviceId() {
	return deviceId;
}

void CommandLayer::reset() {
	LOG_INFO(LOG_ECL, "reset");
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	context->incResetCount();
	packetLayer->reset();
	enabled = false;
	command = CommandType_None;
	gotoStateInit();
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
	if(context->getState() == State_Enabled) {
		this->command = CommandType_VendRequest;
		this->productPrice = productPrice;
		return true;
	} else if(context->getState() == State_Session || context->getState() == State_QrCode) {
		this->productPrice = productPrice;
		gotoStateApproving();
		return true;
	} else {
		LOG_ERROR(LOG_ECL, "Wrong state " << context->getState());
		return false;
	}
}

bool CommandLayer::saleComplete() {
	LOG_INFO(LOG_ECL, "saleComplete");
	if(context->getState() != State_Vending) {
		return false;
	}
	gotoStateFin(true);
	return true;
}

bool CommandLayer::saleFailed() {
	LOG_INFO(LOG_ECL, "saleFailed");
	if(context->getState() != State_Vending) {
		return false;
	}
	gotoStateFin(false);
	return true;
}

bool CommandLayer::closeSession() {
	LOG_INFO(LOG_ECL, "closeSession");
	if(context->getState() == State_Idle) {
		return false;
	}
	if(context->getState() == State_Approving) {
		gotoStateAborting();
		return true;
	}
	timer->stop();
	cancelTimer->start(1);
	context->setState(State_Closing);
	return true;
}

bool CommandLayer::printQrCode(const char *header, const char *footer, const char *text) {
	(void)header;
	(void)footer;
	LOG_INFO(LOG_ECL, "printQrCode");
	LOG_DEBUG(LOG_ECL, "qrCode=" << text);
	if(context->getState() == State_Idle) {
		return false;
	}

	command = CommandType_QrCode;
	qrCodeData.clear();
	qrCodeData.set(text);
	return true;
}

bool CommandLayer::verification() {
	return false;
}

void CommandLayer::procPacket(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_ECL, "procPacket");
	LOG_TRACE_HEX(LOG_ECL, data, dataLen);
	if(resp.parse(data, dataLen) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		LOG_ERROR_HEX(LOG_ECL, data, dataLen);
		context->incProtocolErrorCount();
		return;
	}
#if LOG_ECL >= LOG_LEVEL_TRACE
	resp.print();
#endif

	if(resp.getString(Type_MessageName, &messageName) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		LOG_ERROR_HEX(LOG_ECL, data, dataLen);
		context->incProtocolErrorCount();
		return;
	}

	if(messageName == "CON") {
		deviceLan.procRequestCon(&resp);
		return;
	} else if(messageName == "DAT") {
		deviceLan.procRequestDat(&resp);
		return;
	} else if(messageName == "DSC") {
		deviceLan.procRequestDsc();
		return;
	}

	switch(context->getState()) {
	case State_Init: stateInitPacket(); break;
	case State_Enabled: stateEnabledPacket(); break;
	case State_Session: stateSessionPacket(); break;
	case State_Approving: stateApprovingPacket(); break;
	case State_Fin: stateFinPacket(); break;
	case State_Aborting: stateAbortingPacket(); break;
	case State_QrCode: stateQrCodePacket(); break;
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

void CommandLayer::procTimer() {
	LOG_DEBUG(LOG_ECL, "procTimer");
	switch(context->getState()) {
	case State_Init: stateInitTimeout(); break;
	case State_Enabled: stateEnabledTimeout(); break;
	case State_Session: stateSessionTimeout(); break;
	default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << context->getState());
	}
}

void CommandLayer::procCancelTimer() {
	LOG_DEBUG(LOG_ECL, "procCancelTimer");
	switch(context->getState()) {
	case State_Init: stateInitTimeoutCancel(); break;
	case State_Enabled: stateEnabledTimeoutCancel(); break;
	case State_Session: stateSessionTimeoutCancel(); break;
	case State_Approving: stateApprovingTimeoutCancel(); break;
	case State_Fin: stateFinTimeoutCancel(); break;
	case State_Closing: stateClosingTimeoutCancel(); break;
	case State_Aborting: stateAbortingTimeoutCancel(); break;
	case State_QrCode: stateQrCodeTimeoutCancel(); break;
	default: LOG_ERROR(LOG_ECL, "Unwaited timeout " << context->getState());
	}
}

void CommandLayer::gotoStateInit() {
	LOG_DEBUG(LOG_ECL, "gotoStateInit");
	errorCount = 0;
	operationId = 0;
	timer->start(1);
	cancelTimer->stop();
	context->setState(State_Init);
}

void CommandLayer::stateInitTimeout() {
	LOG_DEBUG(LOG_ECL, "stateInitTimeout");
#if 0
	if(command == CommandType_QrCode) {
		command = CommandType_None;
		gotoStateQrCode();
		return;
	}
#endif
	DateTime datetime;
	realtime->getDateTime(&datetime);

	cancelTimer->start(VENDOTEK_RECV_TIMEOUT);
	req.clear();
	req.addString(Type_MessageName, "IDL", 3);
	req.addNumber(Type_OperationNumber, 8, operationId);
	req.addDateTime(Type_LocalTime, &datetime);
	packetLayer->sendPacket(req.getBuf());
}

void CommandLayer::stateInitTimeoutCancel() {
	LOG_DEBUG(LOG_ECL, "stateInitTimeoutCancel");
	timer->start(1);
}

void CommandLayer::stateInitPacket() {
	LOG_DEBUG(LOG_ECL, "stateInitPacket");
	if(messageName == "IDL") {
		stateInitPacketIdl();
		return;
	} else {
		LOG_ERROR(LOG_ECL, "Unsupported message name " << messageName.getString());
		context->incProtocolErrorCount();
		return;
	}
}

void CommandLayer::stateInitPacketIdl() {
#if 0
	LOG_DEBUG(LOG_ECL, "stateInitPacketIdl");
	timer->start(VENDOTEK_KEEPALIVE_TIMEOUT);
	uint32_t operationLast = 0;
	if(resp.getNumber(Type_OperationNumber, &operationLast) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_ECL, "OperationNumber=" << operationLast);
	operationId = operationLast;
	gotoStateEnabled();
#else
	LOG_DEBUG(LOG_ECL, "stateInitPacketIdl");
	timer->start(VENDOTEK_KEEPALIVE_TIMEOUT);
	uint32_t operationLast = 0;
	if(resp.getNumber(Type_OperationNumber, &operationLast) == true) {
		LOG_ERROR(LOG_ECL, "OperationNumber found");
		operationId = operationLast;
	}

	LOG_INFO(LOG_ECL, "OperationNumber=" << operationLast);
	gotoStateEnabled();
#endif
}

void CommandLayer::gotoStateEnabled() {
	gotoStateEnabled(1);
}

void CommandLayer::gotoStateEnabled(uint32_t timeout) {
	LOG_DEBUG(LOG_ECL, "gotoStateEnabled");
	timer->start(timeout);
	cancelTimer->stop();
	context->setState(State_Enabled);
}

void CommandLayer::stateEnabledTimeout() {
	LOG_DEBUG(LOG_ECL, "stateEnabledTimeout");
	if(command == CommandType_VendRequest) {
		command = CommandType_None;
		gotoStateApproving();
		return;
	} else if(command == CommandType_QrCode) {
		command = CommandType_None;
		gotoStateQrCode();
		return;
	}

	DateTime datetime;
	realtime->getDateTime(&datetime);
	cancelTimer->start(VENDOTEK_RECV_TIMEOUT);
	req.clear();
	req.addString(Type_MessageName, enabled == true ? "IDL" : "DIS", 3);
	req.addNumber(Type_OperationNumber, 8, operationId);
	req.addDateTime(Type_LocalTime, &datetime);
	packetLayer->sendPacket(req.getBuf());
}

void CommandLayer::stateEnabledTimeoutCancel() {
	LOG_DEBUG(LOG_ECL, "stateEnabledTimeoutCancel");
	timer->start(1);
}

void CommandLayer::stateEnabledPacket() {
	LOG_DEBUG(LOG_ECL, "stateEnabledPacket");
	if(messageName == "IDL") {
		stateEnabledPacketIdl();
		return;
	} else if(messageName == "DIS") {
		stateEnabledPacketDis();
		return;
	} else if(messageName == "STA") {
		stateEnabledPacketSta();
		return;
	} else {
		LOG_ERROR(LOG_ECL, "Unsupported message name " << messageName.getString());
		resp.print();
		context->incProtocolErrorCount();
		return;
	}
}

void CommandLayer::stateEnabledPacketIdl() {
#if 0
	LOG_DEBUG(LOG_ECL, "stateEnabledPacketIdl");
	uint32_t operationLast = 0;
	if(resp.getNumber(Type_OperationNumber, &operationLast) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_ECL, "OperationNumber=" << operationLast);
	operationId = operationLast;
	this->context->setStatus(Mdb::DeviceContext::Status_Enabled);
	timer->start(VENDOTEK_KEEPALIVE_TIMEOUT);
#else
	LOG_DEBUG(LOG_ECL, "stateEnabledPacketIdl");
	uint32_t operationLast = 0;
	if(resp.getNumber(Type_OperationNumber, &operationLast) == true) {
		LOG_ERROR(LOG_ECL, "OperationNumber found");
		operationId = operationLast;
	}

	LOG_INFO(LOG_ECL, "OperationNumber=" << operationLast);
	this->context->setStatus(Mdb::DeviceContext::Status_Enabled);
	timer->start(VENDOTEK_KEEPALIVE_TIMEOUT);
#endif
}

void CommandLayer::stateEnabledPacketDis() {
#if 0
	LOG_DEBUG(LOG_ECL, "stateEnabledPacketDis");
	uint32_t operationLast = 0;
	if(resp.getNumber(Type_OperationNumber, &operationLast) == false) {
		LOG_ERROR(LOG_ECL, "Bad format");
		context->incProtocolErrorCount();
		return;
	}

	LOG_INFO(LOG_ECL, "OperationNumber=" << operationLast);
	operationId = operationLast;
	this->context->setStatus(Mdb::DeviceContext::Status_Disabled);
	timer->start(VENDOTEK_KEEPALIVE_TIMEOUT);
#else
	LOG_DEBUG(LOG_ECL, "stateEnabledPacketDis");
	uint32_t operationLast = 0;
	if(resp.getNumber(Type_OperationNumber, &operationLast) == true) {
		LOG_ERROR(LOG_ECL, "OperationNumber found");
		operationId = operationLast;
	}

	LOG_INFO(LOG_ECL, "OperationNumber=" << operationLast);
	this->context->setStatus(Mdb::DeviceContext::Status_Disabled);
	timer->start(VENDOTEK_KEEPALIVE_TIMEOUT);
#endif
}

void CommandLayer::stateEnabledPacketSta() {
	LOG_DEBUG(LOG_ECL, "stateEnabledPacketSta");
	gotoStateSession();
	EventUint32Interface event(deviceId, MdbMasterCashless::Event_SessionBegin, maxCredit);
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateSession() {
	LOG_DEBUG(LOG_ECL, "gotoStateSession");
	timer->stop();
	cancelTimer->start(VENDOTEK_REQUEST_TIMEOUT);
	context->setState(State_Session);
}

void CommandLayer::stateSessionTimeout() {
	LOG_DEBUG(LOG_ECL, "stateSessionTimeout");
	if(command == CommandType_VendRequest) {
		command = CommandType_None;
		gotoStateApproving();
		return;
	}
}

void CommandLayer::stateSessionTimeoutCancel() {
	LOG_DEBUG(LOG_ECL, "stateSessionTimeoutCancel");
	gotoStateEnabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void CommandLayer::stateSessionPacket() {
	LOG_DEBUG(LOG_ECL, "stateSessionPacket");
	if(messageName == "IDL") {
		timer->start(VENDOTEK_KEEPALIVE_TIMEOUT);
		return;
	} else {
		LOG_ERROR(LOG_ECL, "Unsupported message name " << messageName.getString());
		resp.print();
		context->incProtocolErrorCount();
		return;
	}
}

void CommandLayer::gotoStateApproving() {
	operationId++;
	LOG_DEBUG(LOG_ECL, "gotoStateApproving " << operationId);
	req.clear();
	req.addString(Type_MessageName, "VRP", 3);
	req.addNumber(Type_OperationNumber, operationId);
	req.addNumber(Type_Amount, context->money2value(productPrice));
	packetLayer->sendPacket(req.getBuf());
	timer->stop();
	cancelTimer->start(VENDOTEK_APPROVING_TIMEOUT);
	context->setState(State_Approving);
}

void CommandLayer::stateApprovingPacket() {
	LOG_DEBUG(LOG_ECL, "stateRequestPacket");
	if(messageName == "VRP") {
		uint32_t amountValue = 0;
		if(resp.getNumber(Type_Amount, &amountValue) == false) {
			LOG_ERROR(LOG_ECL, "Bad format");
			context->incProtocolErrorCount();
			return;
		}

		uint32_t amountMoney = context->value2money(amountValue);
		if(amountMoney >= productPrice) {
			LOG_INFO(LOG_ECL, "Approved " << amountMoney);
			gotoStateVending();
			MdbMasterCashlessInterface::EventApproved event(deviceId, Fiscal::Payment_Cashless, productPrice);
			eventEngine->transmit(&event);
		} else {
			LOG_INFO(LOG_ECL, "Denied");
			gotoStateEnabled(VENDOTEK_RESULT_TIMEOUT);
			EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
			eventEngine->transmit(&event);
		}
		return;
	} else {
		LOG_ERROR(LOG_ECL, "Unsupported message name " << messageName.getString());
		resp.print();
		context->incProtocolErrorCount();
		return;
	}
}

void CommandLayer::stateApprovingTimeoutCancel() {
	LOG_DEBUG(LOG_ECL, "stateApprovingTimeoutCancel");
	gotoStateEnabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateVending() {
	LOG_DEBUG(LOG_ECL, "gotoStateVending");
	timer->stop();
	cancelTimer->start(VENDOTEK_VENDING_TIMEOUT);
	context->setState(State_Vending);
}

void CommandLayer::stateVendingTimeoutCancel() {
	LOG_DEBUG(LOG_ECL, "stateVendingTimeoutCancel");
	gotoStateEnabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateFin(bool result) {
	LOG_DEBUG(LOG_ECL, "gotoStateFin");
	vendResult = result;
	req.clear();
	req.addString(Type_MessageName, "FIN", 3);
	req.addNumber(Type_OperationNumber, 8, operationId);
	req.addNumber(Type_Amount, 8, vendResult ? productPrice : 0);
	packetLayer->sendPacket(req.getBuf());
	timer->stop();
	cancelTimer->start(VENDOTEK_APPROVING_TIMEOUT);
	context->setState(State_Fin);
}

void CommandLayer::stateFinPacket() {
	LOG_DEBUG(LOG_ECL, "stateFinPacket");
	if(messageName == "FIN") {
		LOG_INFO(LOG_ECL, "Payment complete");
		EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
		eventEngine->transmit(&event);
		gotoStateEnabled();
		return;
	} else {
		LOG_ERROR(LOG_ECL, "Unsupported message name " << messageName.getString());
		resp.print();
		context->incProtocolErrorCount();
		return;
	}
}

void CommandLayer::stateFinTimeoutCancel() {
	LOG_DEBUG(LOG_ECL, "stateFinTimeoutCancel");
	gotoStateEnabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateAborting() {
	operationId++;
	LOG_DEBUG(LOG_ECL, "gotoStateAborting " << operationId);
	req.clear();
	req.addString(Type_MessageName, "ABR", 3);
	req.addNumber(Type_OperationNumber, 8, operationId);
	packetLayer->sendPacket(req.getBuf());
	timer->stop();
	cancelTimer->start(VENDOTEK_APPROVING_TIMEOUT);
	context->setState(State_Aborting);
}

void CommandLayer::stateAbortingPacket() {
	LOG_DEBUG(LOG_ECL, "stateAbortingPacket");
	if(messageName == "VRP") {
		gotoStateEnabled();
		EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
		eventEngine->transmit(&event);
		return;
	} else {
		LOG_ERROR(LOG_ECL, "Unsupported message name " << messageName.getString());
		resp.print();
		context->incProtocolErrorCount();
		return;
	}
}

void CommandLayer::stateAbortingTimeoutCancel() {
	LOG_DEBUG(LOG_ECL, "stateAbortingTimeoutCancel");
	gotoStateEnabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
	eventEngine->transmit(&event);
}

void CommandLayer::stateClosingTimeoutCancel() {
	LOG_DEBUG(LOG_ECL, "stateClosingTimeoutCancel");
	gotoStateEnabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateQrCode() {
	LOG_DEBUG(LOG_ECL, "gotoStateQrCode");
	req.clear();
	req.addString(Type_MessageName, enabled == true ? "IDL" : "DIS", 3);
	req.addNumber(Type_OperationNumber, 8, operationId);
	req.addString(Type_QrCode, qrCodeData.getString(), qrCodeData.getLen());
	req.addNumber(Type_DisplayTime, VENDOTEK_QRCODE_TIMEOUT);
	packetLayer->sendPacket(req.getBuf());
	timer->stop();
	cancelTimer->start(VENDOTEK_QRCODE_TIMEOUT);
	context->setState(State_QrCode);
}

void CommandLayer::stateQrCodePacket() {
	LOG_DEBUG(LOG_ECL, "stateQrCodePacket");
#if 0
	if(messageName == "IDL") {
		gotoStateEnabled();
		return;
	} else if(messageName == "DIS") {
		gotoStateEnabled();
		return;
#else
	if(messageName == "IDL") {
		return;
	} else if(messageName == "DIS") {
		return;
#endif
	} else {
		LOG_ERROR(LOG_ECL, "Unsupported message name " << messageName.getString());
		resp.print();
		context->incProtocolErrorCount();
		return;
	}
}

void CommandLayer::stateQrCodeTimeoutCancel() {
	LOG_DEBUG(LOG_ECL, "stateQrCodeTimeoutCancel");
	gotoStateEnabled();
}

}
