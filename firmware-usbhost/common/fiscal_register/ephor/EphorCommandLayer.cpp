#include "EphorCommandLayer.h"
#include "EphorProtocol.h"
#include "fiscal_storage/include/FiscalStorage.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "utils/include/DecimalPoint.h"
#include "utils/include/CodePage.h"
#include "sim900/include/GsmDriver.h"
#include "logger/include/Logger.h"

#include <string.h>

#define EPHOR_PATH_MAX_SIZE 128
#define EPHOR_DATA_MAX_SIZE 1024

namespace Ephor {

CommandLayer::CommandLayer(
	ConfigModem *config,
	Fiscal::Context *context,
	TcpIp *conn,
	TimerEngine *timers,
	EventEngineInterface *eventEngine,
	RealTimeInterface *realtime,
	LedInterface *leds
) :
	boot(config->getBoot()),
	fiscal(config->getFiscal()),
	context(context),
	timers(timers),
	eventEngine(eventEngine),
	realtime(realtime),
	leds(leds),
	deviceId(eventEngine),
	state(State_Idle),
	reqPath(EPHOR_PATH_MAX_SIZE, EPHOR_PATH_MAX_SIZE),
	reqData(EPHOR_DATA_MAX_SIZE, EPHOR_DATA_MAX_SIZE),
	respData(EPHOR_DATA_MAX_SIZE, EPHOR_DATA_MAX_SIZE),
	respParser(deviceId, &resp)
{
	this->timer = timers->addTimer<CommandLayer, &CommandLayer::procTimer>(this);
	this->conn = new Http::Client(timers, conn);
	this->conn->setObserver(this);
	this->eventEngine->subscribe(this, GlobalId_Sim900);
}

CommandLayer::~CommandLayer() {
	delete conn;
	timers->deleteTimer(timer);
}

EventDeviceId CommandLayer::getDeviceId() {
	return deviceId;
}

void CommandLayer::reset() {
	LOG_DEBUG(LOG_FR, "reset");
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	context->setManufacturer((uint8_t*)EPHOR_MANUFACTURER, strlen(EPHOR_MANUFACTURER));
	context->setModel((uint8_t*)EPHOR_MODEL, strlen(EPHOR_MODEL));
	timer->stop();
	this->leds->setFiscal(LedInterface::State_Success);
	context->setStatus(Mdb::DeviceContext::Status_Work);
	state = State_Idle;
}

void CommandLayer::sale(Fiscal::Sale *saleData) {
	LOG_DEBUG(LOG_FR, "sale");
	if(this->state != State_Idle) {
		LOG_ERROR(LOG_FR, "Wrong state " << state);
		procCheckError(ConfigEvent::Type_FiscalLogicError, __LINE__);
		return;
	}

#if 0
	uint32_t price = context->money2value(saleData->getPrice());
	this->saleData = saleData;
	realtime->getDateTime(&this->saleData->datetime);
	this->saleData->taxValue = context->calcTax(saleData->taxRate, price);
#else
	this->saleData = saleData;
	realtime->getDateTime(&this->saleData->datetime);
	this->saleData->taxValue = 0;
	for(uint16_t i = 0; i < saleData->getProductNum(); i++) {
		Fiscal::Product *product = saleData->getProduct(i);
		uint32_t price = context->money2value(product->price);
		this->saleData->taxValue += context->calcTax(product->taxRate, price);
	}
#endif
	this->saleData->fiscalRegister = 0;
	this->saleData->fiscalStorage = Fiscal::Status_Error;
	this->saleData->fiscalDocument = 0;
	this->saleData->fiscalSign = 0;
	this->tryCount = 0;
	this->leds->setFiscal(LedInterface::State_InProgress);

	makeCheckRequest();
	gotoStateCheck();
}

void CommandLayer::getLastSale() {
	LOG_DEBUG(LOG_FR, "getLastSale");
}

void CommandLayer::closeShift() {
	LOG_DEBUG(LOG_FR, "closeShift");
}

void CommandLayer::proc(Event *event) {
	LOG_DEBUG(LOG_FR, "proc " << state);
	switch(state) {
		case State_Check: stateCheckEvent(event); break;
		case State_Poll: statePollEvent(event); break;
		default: LOG_ERROR(LOG_FR, "Unwaited data state=" << state << "," << event->getType());
	}
}

void CommandLayer::proc(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_FR, "proc");
	switch(state) {
		case State_CheckTryDelay: stateCheckTryDelayEnvelope(envelope); return;
		case State_PollTryDelay: statePollTryDelayEnvelope(envelope); return;
		default: LOG_ERROR(LOG_FR, "Unwaited envelope " << state << "," << envelope->getType());
	}
}

void CommandLayer::procTimer() {
	LOG_DEBUG(LOG_FR, "procTimer");
	switch(state) {
	case State_CheckTryDelay: stateCheckTryDelayTimeout(); break;
	case State_PollDelay: statePollDelayTimeout(); break;
	case State_PollTryDelay: statePollTryDelayTimeout(); break;
	default: LOG_ERROR(LOG_FR, "Unwaited timeout state=" << state);
	}
}

void CommandLayer::gotoStateCheck() {
	LOG_DEBUG(LOG_FR, "gotoStateCheck");
	if(conn->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_FR, "SendRequest failed");
		gotoStateCheckTryDelay(ConfigEvent::Type_FiscalUnknownError, __LINE__);
		return;
	}

	state = State_Check;
}

void CommandLayer::makeCheckRequest() {
	reqPath.clear();
	reqPath << "/api/1.0/Fiscal.php?action=Reg&login=" << boot->getImei() << "&password=" << boot->getServerPassword() << "&_dc=" << realtime->getUnixTimestamp();

	req.method = Http::Request::Method_POST;
	req.serverName = boot->getServerDomain();
	req.serverPort = boot->getServerPort();
	req.serverPath = reqPath.getString();
	req.keepAlive = false;
	req.phpSessionId = NULL;
	req.data = &reqData;
	makeCheckBody();

	resp.phpSessionId = NULL;
	resp.data = &respData;
}

void CommandLayer::makeCheckBody() {
	uint32_t price = context->money2value(saleData->getPrice());
	reqData.clear();
	reqData << "{\"date\":\""; datetime2string(&saleData->datetime, &reqData); reqData << "\"";
	reqData << ",\"point_addr\":\""; convertWin1251ToJsonUnicode(fiscal->getPointAddr(), &reqData); reqData << "\"";
	reqData << ",\"point_name\":\""; convertWin1251ToJsonUnicode(fiscal->getPointName(), &reqData); reqData << "\"";
	reqData << ",\"automat_number\":\""; convertWin1251ToJsonUnicode(fiscal->getAutomatNumber(), &reqData); reqData << "\"";
	reqData << ",\"items\":[";
#if 0
	reqData << "{\"select_id\":\""; convertWin1251ToJsonUnicode(saleData->selectId.get(), &reqData); reqData<< "\"";
	reqData << ",\"ware_id\":" << saleData->wareId;
	reqData << ",\"name\":\""; convertWin1251ToJsonUnicode(saleData->name.get(), &reqData); reqData << "\"";
	reqData << ",\"device\":\""; convertWin1251ToJsonUnicode(saleData->device.get(), &reqData); reqData << "\"";
	reqData << ",\"price_list\":" << saleData->priceList;
	reqData << ",\"price\":" << price;
	reqData << ",\"amount\":1000";
	reqData << ",\"tax_rate\":" << saleData->taxRate;
	reqData << "}]";
#else
	for(uint16_t i = 0; i < saleData->getProductNum(); i++) {
		Fiscal::Product *product = saleData->getProduct(i);
		if(i > 0) { reqData << ","; }
		reqData << "{\"select_id\":\""; convertWin1251ToJsonUnicode(product->selectId.get(), &reqData); reqData<< "\"";
		reqData << ",\"ware_id\":" << product->wareId;
		reqData << ",\"name\":\""; convertWin1251ToJsonUnicode(product->name.get(), &reqData); reqData << "\"";
		reqData << ",\"device\":\""; convertWin1251ToJsonUnicode(saleData->device.get(), &reqData); reqData << "\"";
		reqData << ",\"price_list\":" << saleData->priceList;
		reqData << ",\"price\":" << context->money2value(product->price);
		reqData << ",\"amount\": " << product->quantity * 1000;
		reqData << ",\"tax_rate\":" << product->taxRate;
		reqData << "}";
	}
	reqData << "]";
#endif
	reqData << ",\"tax_system\":" << saleData->taxSystem;
	if(saleData->paymentType == Fiscal::Payment_Cash) {
		reqData << ",\"cash\":" << price;
		reqData << ",\"cashless\":0";
	} else {
		reqData << ",\"cash\":0";
		reqData << ",\"cashless\":" << price;
	}
	reqData << "}";
}

void CommandLayer::stateCheckEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "stateCheckEvent");
	switch(event->getType()) {
	case Http::Client::Event_RequestComplete: stateCheckEventRequestComplete(); return;
	case Http::Client::Event_RequestError: gotoStateCheckTryDelay(ConfigEvent::Type_FiscalConnectError, __LINE__); return;
	default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::stateCheckEventRequestComplete() {
	LOG_DEBUG(LOG_FR, "stateCheckEventRequestComplete");
	respParser.parse();
	if(respParser.getResult() != CheckResponseParser::Result_Busy) {
		leds->setFiscal(LedInterface::State_Failure);
		state = State_Idle;
		Fiscal::EventError *error = respParser.getError();
		context->removeAll();
		context->addError(error->code, error->data.getString());
		eventEngine->transmit(error);
		return;
	}

	tryCount = 0;
	id = respParser.getId();
	saleData->fiscalStorage = Fiscal::Status_InQueue;
	gotoStatePollDelay();
}

void CommandLayer::gotoStateCheckTryDelay(uint16_t errorCode, uint32_t line, const char *suffix) {
	LOG_DEBUG(LOG_FR, "gotoStateCheckTryDelay");
	tryCount++;
	if(tryCount > EPHOR_TRY_MAX) {
		procCheckError(errorCode, line, suffix);
		return;
	}

	timer->start(EPHOR_TRY_DELAY);
	state = State_CheckTryDelay;
}

void CommandLayer::stateCheckTryDelayEnvelope(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_FR, "stateCheckTryDelayEnvelope");
	switch(envelope->getType()) {
		case Gsm::Driver::Event_NetworkUp: {
			timer->stop();
			gotoStateCheck();
			return;
		}
		default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << envelope->getType());
	}
}

void CommandLayer::stateCheckTryDelayTimeout() {
	LOG_DEBUG(LOG_FR, "stateCheckTryDelayTimeout");
	gotoStateCheck();
}

void CommandLayer::procCheckError(uint16_t errorCode, uint32_t line, const char *suffix) {
	LOG_DEBUG(LOG_FR, "procCheckError");
	leds->setFiscal(LedInterface::State_Failure);
	state = State_Idle;
	Fiscal::EventError event(deviceId);
	event.code = errorCode;
	event.data.clear();
	event.data << "od" << line;
	if(suffix != NULL) { event.data << ";" << suffix; }
	context->removeAll();
	context->addError(event.code, event.data.getString());
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStatePollDelay() {
	LOG_DEBUG(LOG_FR, "gotoStatePollDelay");
	timer->start(EPHOR_POLL_DELAY);
	state = State_PollDelay;
}

void CommandLayer::statePollDelayTimeout() {
	LOG_DEBUG(LOG_FR, "statePollDelayTimeout");
	gotoStatePoll();
}

void CommandLayer::gotoStatePoll() {
	LOG_DEBUG(LOG_FR, "gotoStatePoll");
	reqPath.clear();
	reqPath << "/api/1.0/Fiscal.php?action=Get&login=" << boot->getImei() << "&password=" << boot->getServerPassword() << "&_dc=" << realtime->getUnixTimestamp();

	req.method = Http::Request::Method_GET;
	req.serverName = boot->getServerDomain();
	req.serverPort = boot->getServerPort();
	req.serverPath = reqPath.getString();
	req.keepAlive = false;
	req.phpSessionId = NULL;
	req.data = &reqData;
	makePollBody();

	resp.phpSessionId = NULL;
	resp.data = &respData;

	if(conn->sendRequest(&req, &resp) == false) {
		LOG_ERROR(LOG_FR, "SendRequest failed");
		gotoStatePollTryDelay(ConfigEvent::Type_FiscalUnknownError, __LINE__);
		return;
	}

	state = State_Poll;
}

void CommandLayer::makePollBody() {
	LOG_DEBUG(LOG_FR, "makePollBody");
	reqData.clear();
	reqData << "{\"id\":\"" << id << "\"}";
}

void CommandLayer::statePollEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "statePollEvent");
	switch(event->getType()) {
	case Http::Client::Event_RequestComplete: statePollEventRequestComplete(); return;
	case Http::Client::Event_RequestError: gotoStatePollTryDelay(ConfigEvent::Type_FiscalConnectError, __LINE__); return;
	default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::statePollEventRequestComplete() {
	LOG_DEBUG(LOG_FR, "statePollEventRequestComplete");
	respParser.parse();
	if(respParser.getResult() == CheckResponseParser::Result_Ok) {
		LOG_INFO(LOG_FR, "Check complete");
		saleData->fiscalDatetime.set(respParser.getFiscalDatetime());
		saleData->fiscalRegister = respParser.getFiscalRegister();
		saleData->fiscalStorage = respParser.getFiscalStorage();
		saleData->fiscalDocument = respParser.getFiscalDocument();
		saleData->fiscalSign = respParser.getFiscalSign();
		context->removeAll();
		leds->setFiscal(LedInterface::State_Success);
		state = State_Idle;
		EventInterface event(deviceId, Fiscal::Register::Event_CommandOK);
		eventEngine->transmit(&event);
		return;
	} else if(respParser.getResult() == CheckResponseParser::Result_Busy) {
		LOG_INFO(LOG_FR, "Check in progress");
		gotoStatePollTryDelay(ConfigEvent::Type_FiscalCompleteNoData, __LINE__);
		return;
	} else {
		LOG_INFO(LOG_FR, "Check error " << respParser.getResult());
		procPollError();
		return;
	}
}

void CommandLayer::gotoStatePollTryDelay(uint16_t errorCode, uint32_t line, const char *suffix) {
	LOG_DEBUG(LOG_FR, "gotoStatePollTryDelay");
	tryCount++;
	if(tryCount > EPHOR_TRY_MAX) {
		procPollError();
		return;
	}
	timer->start(EPHOR_TRY_DELAY);
	state = State_PollTryDelay;
}

void CommandLayer::statePollTryDelayEnvelope(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_FR, "statePollTryDelayEnvelope");
	switch(envelope->getType()) {
		case Gsm::Driver::Event_NetworkUp: {
			timer->stop();
			gotoStatePoll();
			return;
		}
		default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << envelope->getType());
	}
}

void CommandLayer::statePollTryDelayTimeout() {
	LOG_DEBUG(LOG_FR, "statePollTryDelayTimeout");
	gotoStatePoll();
}

void CommandLayer::procPollError() {
	LOG_DEBUG(LOG_FR, "procPollError");
	saleData->fiscalDatetime.set(&saleData->datetime);
	saleData->fiscalRegister = 0;
	saleData->fiscalStorage = Fiscal::Status_InQueue;
	saleData->fiscalDocument = 0;
	saleData->fiscalSign = 0;
	context->removeAll();
	leds->setFiscal(LedInterface::State_Success);
	state = State_Idle;
	EventInterface event(deviceId, Fiscal::Register::Event_CommandOK);
	eventEngine->transmit(&event);
}

}
