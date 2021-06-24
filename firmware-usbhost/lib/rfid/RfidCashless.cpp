#include "RfidCashless.h"

#include "common/logger/include/Logger.h"
#include "common/beeper/include/Gramophone.h"
#include "common/utils/include/CodePage.h"

namespace Rfid {

#define NFC_SESSION_TIMEOUT 30000
#define NFC_APPROVING_TIMEOUT 120000
#define NFC_VENDING_TIMEOUT 120000
#define NFC_DISCOUNT_TIMEOUT 120000
#define NFC_SERVICE_TIMEOUT 60000
//+++
#define NFC_MAX_TRY_NUMBER 3
#define REQUEST_PATH_MAX_SIZE 368
#define RESPONSE_DATA_MAX_SIZE 256
#define JSON_NODE_MAX 30
//+++

Cashless::Cashless(
	ConfigModem *config,
	ClientContext *client,
	TcpIp *tcpConn,
	RealTimeInterface *realtime,
	TimerEngine *timerEngine,
	GramophoneInterface *gramophone,
	ScreenInterface *screen,
	EventEngineInterface *eventEngine
) :
	config(config),
	client(client),
	realtime(realtime),
	context(config->getAutomat()->getNfcContext()),
	converter(context->getMasterDecimalPoint()),
	jsonParser(JSON_NODE_MAX),
	timerEngine(timerEngine),
	gramophone(gramophone),
	screen(screen),
	eventEngine(eventEngine),
	deviceId(eventEngine),
	enabling(false)
{
	context->setState(State_Idle);
	context->init(2, 1);
	converter.setDeviceDecimalPoint(0);
	melody = new MelodyNfc;
	timer = timerEngine->addTimer<Cashless, &Cashless::procTimer>(this);
	httpClient = new Http::Client(timerEngine, tcpConn);
	httpTransport = new Http::Transport(httpClient, NFC_MAX_TRY_NUMBER);
	httpTransport->setObserver(this);
	req.serverName = config->getBoot()->getServerDomain();
	req.serverPort = config->getBoot()->getServerPort();
	reqPath = new StringBuilder(REQUEST_PATH_MAX_SIZE, REQUEST_PATH_MAX_SIZE);
	reqData = new StringBuilder(REQUEST_PATH_MAX_SIZE, REQUEST_PATH_MAX_SIZE);
	respData = new StringBuilder(RESPONSE_DATA_MAX_SIZE, RESPONSE_DATA_MAX_SIZE);
	productName = new StringBuilder(50, 50);
	eventEngine->subscribe(this, GlobalId_Rfid);
}

Cashless::~Cashless() {
	timerEngine->deleteTimer(timer);
	delete httpTransport;
	delete httpClient;
	delete melody;
}

EventDeviceId Cashless::getDeviceId() {
	return deviceId;
}

void Cashless::reset() {
	LOG_INFO(LOG_RFID, "reset");
	screen->clear();
	gotoStateDisabled();
}

bool Cashless::isRefundAble() {
	return false;
}

void Cashless::disable() {
	LOG_DEBUG(LOG_RFID, "disable");
	enabling = false;
	if(context->getState() == State_Enabled) {
		gotoStateDisabled();
	}
}

void Cashless::enable() {
	LOG_DEBUG(LOG_RFID, "enable");
	enabling = true;
	if(context->getState() == State_Disabled) {
		gotoStateEnabled();
	}
}

bool Cashless::revalue(uint32_t credit) {
	return false;
}

bool Cashless::sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) {
	LOG_WARN(LOG_RFID, "sale " << productId << "," << productPrice);
	if(context->getState() != State_Session) {
		LOG_ERROR(LOG_RFID, "Wrong state " << context->getState());
		return false;
	}
	this->productId = productId;
	this->productPrice = productPrice;
	this->productName->set(productName);
	this->wareId = wareId;
	gotoStateApproving();
	return true;
}

bool Cashless::saleComplete() {
	gotoStateClosing();
	return true;
}

bool Cashless::saleFailed() {
	gotoStateClosing();
	return true;
}

bool Cashless::closeSession() {
	gotoStateClosing();
	return true;
}

void Cashless::service() {
	LOG_DEBUG(LOG_RFID, "service");
	if(context->getState() != State_Service) {
		gotoStateService();
	} else {
		gotoStateEnabled();
	}
}

void Cashless::proc(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_RFID, "proc " << context->getState() << "," << envelope->getType());
	switch(context->getState()) {
		case State_Idle: return;
		case State_Disabled: stateEnabledEvent(envelope); return;
		case State_Enabled: stateEnabledEvent(envelope); return;
		case State_Session: stateSessionEvent(envelope); break;
		case State_Discount: stateDiscountEvent(envelope); return;
		case State_Service: stateServiceEvent(envelope); return;
		default: LOG_ERROR(LOG_RFID, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void Cashless::proc(Event *event) {
	LOG_DEBUG(LOG_RFID, "proc " << event->getType());
	switch(context->getState()) {
	case State_Checking: stateCheckingEvent(event); break;
	case State_Approving: stateApprovingEvent(event); break;
	case State_Registration: stateRegistrationEvent(event); break;
	default: LOG_ERROR(LOG_RFID, "Unwaited event " << context->getState());
	}
}

void Cashless::procTimer() {
	LOG_ERROR(LOG_RFID, "procTimer " << context->getState());
	switch(context->getState()) {
	case State_Checking: stateCheckingTimeout(); return;
	case State_Session: stateSessionTimeout(); return;
	case State_Approving: stateApprovingTimeout(); return;
	case State_Closing: stateClosingTimeout(); return;
	case State_Discount: stateDiscountTimeout(); return;
	case State_Service: gotoStateEnabled(); return;
	case State_Registration: stateRegistrationTimeout(); return;
	default: LOG_ERROR(LOG_RFID, "Unwaited timeout " << context->getState());
	}
}

void Cashless::gotoStateDisabled() {
	LOG_DEBUG(LOG_RFID, "gotoStateDisabled");
	if(enabling == true) {
		gotoStateEnabled();
		return;
	}
//	screen->drawText("Подождите\n\nАвтомат не готов\n\nк работе", 3, 0xFFFF, 0);
	screen->drawText("", 3, 0xFFFF, 0);
	context->setState(State_Disabled);
}

void Cashless::gotoStateEnabled() {
	LOG_DEBUG(LOG_RFID, "gotoStateEnabled");
	if(enabling == false) {
		gotoStateDisabled();
		return;
	}
//	screen->drawImage();
//	screen->drawText("Приложите карту\n\nчтобы активировать\n\nскидку или баллы", 3, 0, 0xFFFF);
	screen->drawText("Добро пожаловать", 3, 0, 0xFFFF);
	timer->stop();
	context->setState(State_Enabled);
}

void Cashless::stateEnabledEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_RFID, "stateWaitEvent");
	if(envelope->getType() != Event_Card) {
		return;
	}

	EventCard event1;
	if(event1.open(envelope) == false) {
		LOG_ERROR(LOG_RFID, "Envelope open failed");
		return;
	}

	LOG_INFO(LOG_RFID, ">>>>>>>>>>>>>RFID");
	LOG_INFO_HEX(LOG_RFID, event1.getUid()->getVal(), event1.getUid()->getLen());
	uid.set(event1.getUid()->getVal(), event1.getUid()->getLen());
	gotoStateChecking();
}

void Cashless::gotoStateChecking() {
	LOG_DEBUG(LOG_RFID, "gotoStateChecking");

	reqPath->clear();
	*reqPath << "/api/1.0/EPay.php?action=Check"
			 << "&login=" << config->getBoot()->getImei()
			 << "&password=" << config->getBoot()->getServerPassword()
			 << "&_dc=" << realtime->getUnixTimestamp();

	reqData->clear();
	*reqData << "{";
	*reqData << "\"card_uid\":\"";
	uint8_t *uidVal = uid.getVal();
	for(uint16_t i = 0; i < uid.getSize(); i++) {
		reqData->addHex(uidVal[i]);
	}
	*reqData << "\"}";

	req.method = Http::Request::Method_POST;
	req.serverPath = reqPath->getString();
	req.keepAlive = false;
	req.phpSessionId = NULL;
	req.data = reqData;
	resp.phpSessionId = NULL;
	resp.data = respData;

	if(httpTransport->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_RFID, "sendRequest failed");
		return;
	}

	screen->drawProgress("Проверка карты");
	timer->start(NFC_APPROVING_TIMEOUT);
	context->setState(State_Checking);
}

void Cashless::stateCheckingEvent(Event *event) {
	LOG_DEBUG(LOG_RFID, "stateCheckingEvent");
	switch(event->getType()) {
		case Http::ClientInterface::Event_RequestComplete: stateCheckingEventRequestComplete(); return;
		case Http::ClientInterface::Event_RequestError: stateCheckingEventRequestError(); return;
	}
}

void Cashless::stateCheckingEventRequestComplete() {
	LOG_INFO(LOG_RFID, "stateCheckingEventRequestComplete");
	LOG_INFO_HEX(LOG_RFID, resp.data->getData(), resp.data->getLen());
	if(jsonParser.parse((char*)resp.data->getData(), resp.data->getLen()) == false) {
		LOG_ERROR(LOG_RFID, "Json parse failed");
		stateCheckingEventRequestError();
		return;
	}

	JsonNode *nodeRoot = jsonParser.getRoot();
	if(nodeRoot == NULL || nodeRoot->getType() != JsonNode::Type_Object) {
		LOG_ERROR(LOG_RFID, "Wrong json format");
		stateCheckingEventRequestError();
		return;
	}

	JsonNode *nodeResult = nodeRoot->getChild("success");
	if(nodeResult == NULL || nodeResult->getType() != JsonNode::Type_Member) {
		LOG_ERROR(LOG_RFID, "Wrong response format");
		stateCheckingEventRequestError();
		return;
	}

	JsonNode *nodeResultValue = nodeResult->getChild();
	if(nodeResultValue == NULL || nodeResultValue->getType() != JsonNode::Type_TRUE) {
		LOG_ERROR(LOG_RFID, "Wrong result");
		stateCheckingEventRequestError();
		return;
	}

	JsonNode *respData = nodeRoot->getField("data", JsonNode::Type_Array);
	if(respData == NULL) {
		LOG_ERROR(LOG_RFID, "Wrong response format");
		stateCheckingEventRequestError();
		return;
	}

    JsonNode *nodeEntry = respData->getChildByIndex(0);
    if(nodeEntry == NULL || nodeEntry->getType() != JsonNode::Type_Object) {
        LOG_ERROR(LOG_RFID, "Wrong response format");
        stateCheckingEventRequestError();
        return;
    }

	JsonNode *nodeResult2 = nodeEntry->getChild("success");
	if(nodeResult2 == NULL || nodeResult2->getType() != JsonNode::Type_Member) {
		LOG_ERROR(LOG_RFID, "Wrong response format");
		stateCheckingEventRequestError();
		return;
	}

	JsonNode *nodeResultValue2 = nodeResult2->getChild();
	if(nodeResultValue == NULL || nodeResultValue2->getType() != JsonNode::Type_TRUE) {
		LOG_ERROR(LOG_RFID, "Wrong result");
		stateCheckingEventRequestError();
		return;
	}

	JsonNode *nodeData = nodeEntry->getField("data", JsonNode::Type_Object);
	if(nodeData == NULL) {
		LOG_ERROR(LOG_RFID, "Wrong response format");
		stateCheckingEventRequestError();
		return;
	}

	if(nodeData->getNumberField("type", &type) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'type' not found");
		stateCheckingEventRequestError();
		return;
	}

	if(nodeData->getNumberField("credit", &credit) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'type' not found");
		stateCheckingEventRequestError();
		return;
	}

	if(nodeData->getStringField("first_name", client->getFirstNameStr()) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'first_name' not found");
		stateCheckingEventRequestError();
		return;
	}

	if(nodeData->getStringField("last_name", client->getLastNameStr()) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'last_name' not found");
		stateCheckingEventRequestError();
		return;
	}

	if(type == Type_Unlimited) {
		LOG_INFO(LOG_RFID, "CheckOK>>>>>>>>>>>>>");
		client->setLoyality(Loyality_Mifare, uid.getVal(), uid.getLen());
		client->setDiscount(0);
		credit = config->getAutomat()->getMaxCredit();
		gramophone->play(melody);
		reqData->clear();
		*reqData << "Здравствуйте\n\n";
		*reqData << client->getFirstName();
		if((client->getFirstNameStr()->getLen() + client->getLastNameStr()->getLen()) > 17) { *reqData << "\n\n"; } else { *reqData << " "; }
		*reqData << client->getLastName() << "\n\n";
		*reqData << "Безлимит активен";
		screen->drawText(reqData->getString());
		gotoStateSession();
		EventUint32Interface event(deviceId, MdbMasterCashless::Event_SessionBegin, credit); // todo: credit
		eventEngine->transmit(&event);
	} else if(type == Type_OnlineWallet) {
		LOG_INFO(LOG_RFID, "CheckOK>>>>>>>>>>>>>");
		client->setLoyality(Loyality_Mifare, uid.getVal(), uid.getLen());
		client->setDiscount(0);
		credit = context->value2money(credit);
		uint32_t sessionCredit = credit > config->getAutomat()->getMaxCredit() ? config->getAutomat()->getMaxCredit() : credit;
		gramophone->play(melody);
		reqData->clear();
		*reqData << "Здравствуйте\n\n";
		*reqData << client->getFirstName();
		if((client->getFirstNameStr()->getLen() + client->getLastNameStr()->getLen()) > 17) { *reqData << "\n\n"; } else { *reqData << " "; }
		*reqData << client->getLastName() << "\n\n";
		*reqData << "Ваш баланс " << converter.convertMasterToDevice(credit) << " руб";
		screen->drawText(reqData->getString());
		gotoStateSession();
		EventUint32Interface event(deviceId, MdbMasterCashless::Event_SessionBegin, sessionCredit); // todo: credit
		eventEngine->transmit(&event);
	} else if(type == Type_Discount) {
		LOG_INFO(LOG_RFID, "CheckOK>>>>>>>>>>>>>");
		client->setLoyality(Loyality_Mifare, uid.getVal(), uid.getLen());
		client->setDiscount(credit);
		credit = 0;
		gramophone->play(melody);
		reqData->clear();
		*reqData << "Здравствуйте\n\n";
		*reqData << client->getFirstName();
		if((client->getFirstNameStr()->getLen() + client->getLastNameStr()->getLen()) > 17) { *reqData << "\n\n"; } else { *reqData << " "; }
		*reqData << client->getLastName() << "\n\n";
		*reqData << "Ваша скидка " << client->getDiscount() << "%";
		screen->drawText(reqData->getString());
		gotoStateDiscount();
	} else {
		LOG_ERROR(LOG_FR, "Unsupported type " << type);
		stateCheckingEventRequestError();
		return;
	}
}

void Cashless::stateCheckingEventRequestError() {
	LOG_INFO(LOG_RFID, "stateCheckingEventRequestError");
	LOG_INFO_HEX(LOG_RFID, resp.data->getData(), resp.data->getLen());
	screen->clear();
	gotoStateEnabled();
}

void Cashless::stateCheckingTimeout() {
	LOG_INFO(LOG_RFID, "stateCheckingTimeout");
	gotoStateEnabled();
}

void Cashless::gotoStateSession() {
	LOG_DEBUG(LOG_RFID, "gotoStateSession");
	timer->start(NFC_SESSION_TIMEOUT);
	context->setState(State_Session);
}

void Cashless::stateSessionEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_RFID, "stateSessionEvent");
	if(envelope->getType() != Event_Card) {
		return;
	}

	EventCard event1;
	if(event1.open(envelope) == false) {
		LOG_ERROR(LOG_RFID, "Envelope open failed");
		return;
	}

	if(uid.equal(event1.getUid()->getVal(), event1.getUid()->getLen()) == true) {
		LOG_ERROR(LOG_RFID, "Rfid equal");
		timer->start(NFC_DISCOUNT_TIMEOUT);
		return;
	}

	LOG_INFO(LOG_RFID, ">>>>>>>>>>>>>RFID");
	LOG_INFO_HEX(LOG_RFID, event1.getUid()->getVal(), event1.getUid()->getLen());
	uid.set(event1.getUid()->getVal(), event1.getUid()->getLen());
	gotoStateChecking();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void Cashless::stateSessionTimeout() {
	LOG_DEBUG(LOG_RFID, "stateSessionTimeout");
	gotoStateEnabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void Cashless::gotoStateApproving() {
	LOG_DEBUG(LOG_RFID, "gotoStateApproving");

	reqPath->clear();
	*reqPath << "/api/1.0/EPay.php?action=Pay"
			 << "&login=" << config->getBoot()->getImei()
			 << "&password=" << config->getBoot()->getServerPassword()
			 << "&_dc=" << realtime->getUnixTimestamp();

	reqData->clear();
	*reqData << "{";
	*reqData << "\"card_uid\":\"";
	uint8_t *uidVal = uid.getVal();
	for(uint16_t i = 0; i < uid.getSize(); i++) {
		reqData->addHex(uidVal[i]);
	}
	*reqData << "\",";
	*reqData << "\"ware_id\":" << wareId << ",";
	*reqData << "\"product_cid\":" << productId << ",";
	*reqData << "\"product_name\":\""; convertWin1251ToJsonUnicode(productName->getString(), reqData); *reqData << "\",";
	*reqData << "\"product_price\":" << context->money2value(productPrice);
	*reqData << "}";

	req.method = Http::Request::Method_POST;
	req.serverPath = reqPath->getString();
	req.keepAlive = false;
	req.phpSessionId = NULL;
	req.data = reqData;
	resp.phpSessionId = NULL;
	resp.data = respData;

	if(httpTransport->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_RFID, "sendRequest failed");
		return;
	}

	screen->drawProgress("Связь с сервером");
	timer->start(NFC_APPROVING_TIMEOUT);
	context->setState(State_Approving);
}

void Cashless::stateApprovingEvent(Event *event) {
	LOG_DEBUG(LOG_RFID, "stateApprovingEvent");
	switch(event->getType()) {
		case Http::ClientInterface::Event_RequestComplete: stateApprovingEventRequestComplete(); return;
		case Http::ClientInterface::Event_RequestError: stateApprovingEventRequestError(); return;
	}
}

void Cashless::stateApprovingEventRequestComplete() {
	LOG_INFO(LOG_RFID, "stateApprovingEventRequestComplete");
	LOG_INFO_HEX(LOG_RFID, resp.data->getData(), resp.data->getLen());
	if(jsonParser.parse((char*)resp.data->getData(), resp.data->getLen()) == false) {
		LOG_ERROR(LOG_RFID, "Json parse failed");
		stateApprovingEventRequestError();
		return;
	}

	JsonNode *nodeRoot = jsonParser.getRoot();
	if(nodeRoot == NULL || nodeRoot->getType() != JsonNode::Type_Object) {
		LOG_ERROR(LOG_RFID, "Wrong json format");
		stateApprovingEventRequestError();
		return;
	}

	JsonNode *nodeResult = nodeRoot->getChild("success");
	if(nodeResult == NULL || nodeResult->getType() != JsonNode::Type_Member) {
		LOG_ERROR(LOG_RFID, "Wrong response format");
		stateApprovingEventRequestError();
		return;
	}

	JsonNode *nodeResultValue = nodeResult->getChild();
	if(nodeResultValue == NULL || nodeResultValue->getType() != JsonNode::Type_TRUE) {
		LOG_ERROR(LOG_RFID, "Wrong result");
		stateApprovingEventRequestError();
		return;
	}

	JsonNode *nodeData = nodeRoot->getField("data", JsonNode::Type_Array);
	if(nodeData == NULL) {
		LOG_ERROR(LOG_RFID, "Wrong response format");
		stateApprovingEventRequestError();
		return;
	}

    JsonNode *nodeEntry = nodeData->getChildByIndex(0);
    if(nodeEntry == NULL || nodeEntry->getType() != JsonNode::Type_Object) {
        LOG_ERROR(LOG_RFID, "Wrong response format");
        stateApprovingEventRequestError();
        return;
    }

	JsonNode *nodeResult2 = nodeEntry->getChild("success");
	if(nodeResult2 == NULL || nodeResult2->getType() != JsonNode::Type_Member) {
		LOG_ERROR(LOG_RFID, "Wrong response format");
		stateApprovingEventRequestError();
		return;
	}

	JsonNode *nodeResultValue2 = nodeResult2->getChild();
	if(nodeResultValue == NULL || nodeResultValue2->getType() != JsonNode::Type_TRUE) {
		LOG_ERROR(LOG_RFID, "Wrong result");
		stateApprovingEventRequestError();
		return;
	}

	LOG_ERROR(LOG_RFID, "PayOK>>>>>>>>>>>>>");
	screen->drawProgress("Выдача товара");
	timer->start(NFC_VENDING_TIMEOUT);
	context->setState(State_Vending);
	MdbMasterCashlessInterface::EventApproved event(deviceId, Fiscal::Payment_Ephor, productPrice);
	eventEngine->transmit(&event);
}

void Cashless::stateApprovingEventRequestError() {
	LOG_INFO(LOG_RFID, "stateApprovingEventRequestError");
	LOG_INFO_HEX(LOG_RFID, resp.data->getData(), resp.data->getLen());
	gotoStateSession();
	EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
	eventEngine->transmit(&event);
}

void Cashless::stateApprovingTimeout() {
	LOG_INFO(LOG_RFID, "stateApprovingTimeout");
	gotoStateSession();
	EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
	eventEngine->transmit(&event);
}

void Cashless::gotoStateClosing() {
	timer->start(1);
	context->setState(State_Closing);
}

void Cashless::stateClosingTimeout() {
	gotoStateEnabled();
	EventInterface event(deviceId, MdbMasterCashless::Event_SessionEnd);
	eventEngine->transmit(&event);
}

void Cashless::gotoStateDiscount() {
	LOG_DEBUG(LOG_RFID, "gotoStateDiscount");
	timer->start(NFC_DISCOUNT_TIMEOUT);
	context->setState(State_Discount);
}

void Cashless::stateDiscountEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_RFID, "stateDiscountEvent");
	if(envelope->getType() != Event_Card) {
		return;
	}

	EventCard event1;
	if(event1.open(envelope) == false) {
		LOG_ERROR(LOG_RFID, "Envelope open failed");
		return;
	}

	if(uid.equal(event1.getUid()->getVal(), event1.getUid()->getLen()) == true) {
		LOG_ERROR(LOG_RFID, "Rfid equal");
		timer->start(NFC_DISCOUNT_TIMEOUT);
		return;
	}

	LOG_INFO(LOG_RFID, ">>>>>>>>>>>>>RFID");
	LOG_INFO_HEX(LOG_RFID, event1.getUid()->getVal(), event1.getUid()->getLen());
	uid.set(event1.getUid()->getVal(), event1.getUid()->getLen());
	gotoStateChecking();
}

void Cashless::stateDiscountTimeout() {
	LOG_INFO(LOG_RFID, "stateDiscountTimeout");
	gotoStateEnabled();
}

void Cashless::gotoStateService(bool showText) {
	LOG_DEBUG(LOG_RFID, "gotoStateService");
	if(showText == true) { screen->drawText("Режим регистрации:\n\n1.Приложите карту\n\n2.Дождитесь завершения\n\nпередачи данных\n\n3.Активируйте карту", 2); }
	timer->start(NFC_SERVICE_TIMEOUT);
	context->setState(State_Service);
}

void Cashless::stateServiceEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_RFID, "stateServiceEvent");
	if(envelope->getType() != Rfid::Event_Card) {
		return;
	}

	Rfid::EventCard event1;
	if(event1.open(envelope) == false) {
		LOG_ERROR(LOG_RFID, "Envelope open failed");
		return;
	}

	LOG_INFO(LOG_RFID, ">>>>>>>>>>>>>RFID");
	LOG_INFO_HEX(LOG_RFID, event1.getUid()->getVal(), event1.getUid()->getLen());
	uid.set(event1.getUid()->getVal(), event1.getUid()->getLen());
	gramophone->play(melody);
	gotoStateRegistration();
}

void Cashless::gotoStateRegistration() {
	LOG_DEBUG(LOG_RFID, "gotoStateRegistration");
	reqPath->clear();
	*reqPath << "/api/1.0/EPay.php?action=Reg"
			 << "&login=" << config->getBoot()->getImei()
			 << "&password=" << config->getBoot()->getServerPassword()
			 << "&_dc=" << realtime->getUnixTimestamp();

	reqData->clear();
	*reqData << "{";
	*reqData << "\"card_uid\":\"";
	uint8_t *uidVal = uid.getVal();
	for(uint16_t i = 0; i < uid.getSize(); i++) {
		reqData->addHex(uidVal[i]);
	}
	*reqData << "\"}";

	req.method = Http::Request::Method_POST;
	req.serverPath = reqPath->getString();
	req.keepAlive = false;
	req.phpSessionId = NULL;
	req.data = reqData;
	resp.phpSessionId = NULL;
	resp.data = respData;

	if(httpTransport->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_RFID, "sendRequest failed");
		return;
	}

	screen->drawProgress("Передача данных");
	timer->start(NFC_APPROVING_TIMEOUT);
	context->setState(State_Registration);
}

void Cashless::stateRegistrationEvent(Event *event) {
	LOG_DEBUG(LOG_RFID, "stateRegistrationEvent");
	switch(event->getType()) {
		case Http::ClientInterface::Event_RequestComplete: stateRegistrationEventRequestComplete(); return;
		case Http::ClientInterface::Event_RequestError: stateRegistrationEventRequestError(); return;
	}
}

void Cashless::stateRegistrationEventRequestComplete() {
	LOG_INFO(LOG_RFID, "stateRegistrationEventRequestComplete");
	LOG_INFO_HEX(LOG_RFID, resp.data->getData(), resp.data->getLen());
	if(jsonParser.parse((char*)resp.data->getData(), resp.data->getLen()) == false) {
		LOG_ERROR(LOG_RFID, "Json parse failed");
		stateRegistrationEventRequestError();
		return;
	}

	JsonNode *nodeRoot = jsonParser.getRoot();
	if(nodeRoot == NULL || nodeRoot->getType() != JsonNode::Type_Object) {
		LOG_ERROR(LOG_RFID, "Wrong json format");
		stateRegistrationEventRequestError();
		return;
	}

	JsonNode *nodeResult = nodeRoot->getChild("success");
	if(nodeResult == NULL || nodeResult->getType() != JsonNode::Type_Member) {
		LOG_ERROR(LOG_RFID, "Wrong response format");
		stateRegistrationEventRequestError();
		return;
	}

	JsonNode *nodeResultValue = nodeResult->getChild();
	if(nodeResultValue == NULL || nodeResultValue->getType() != JsonNode::Type_TRUE) {
		LOG_ERROR(LOG_RFID, "Wrong result");
		stateRegistrationEventRequestError();
		return;
	}

	JsonNode *nodeData = nodeRoot->getField("data", JsonNode::Type_Array);
	if(nodeData == NULL) {
		LOG_ERROR(LOG_RFID, "Wrong response format");
		stateRegistrationEventRequestError();
		return;
	}

    JsonNode *nodeEntry = nodeData->getChildByIndex(0);
    if(nodeEntry == NULL || nodeEntry->getType() != JsonNode::Type_Object) {
        LOG_ERROR(LOG_RFID, "Wrong response format");
        stateRegistrationEventRequestError();
        return;
    }

	JsonNode *nodeResult2 = nodeEntry->getChild("success");
	if(nodeResult2 == NULL || nodeResult2->getType() != JsonNode::Type_Member) {
		LOG_ERROR(LOG_RFID, "Wrong response format");
		stateRegistrationEventRequestError();
		return;
	}

	JsonNode *nodeResultValue2 = nodeResult2->getChild();
	if(nodeResultValue == NULL || nodeResultValue2->getType() != JsonNode::Type_TRUE) {
		LOG_ERROR(LOG_RFID, "Wrong result");
		stateRegistrationEventRequestError();
		return;
	}

	LOG_ERROR(LOG_RFID, "RegOK>>>>>>>>>>>>>");
	screen->drawText("Карта успешно\n\nдобавлена");
	gotoStateService(false);
}

void Cashless::stateRegistrationEventRequestError() {
	LOG_INFO(LOG_RFID, "stateRegistrationEventRequestError");
	LOG_INFO_HEX(LOG_RFID, resp.data->getData(), resp.data->getLen());
	screen->drawText("Ошибка передачи\n\nданных на сервер");
	gotoStateService(false);
	EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
	eventEngine->transmit(&event);
}

void Cashless::stateRegistrationTimeout() {
	LOG_INFO(LOG_RFID, "stateRegistrationTimeout");
	screen->drawText("Ошибка передачи\n\nданных на сервер");
	gotoStateService(false);
	EventInterface event(deviceId, MdbMasterCashless::Event_VendDenied);
	eventEngine->transmit(&event);
}

}
