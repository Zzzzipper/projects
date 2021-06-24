#include "NanokassaCommandLayer.h"
#include "NanokassaProtocol.h"
#include "fiscal_storage/include/FiscalStorage.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "utils/include/DecimalPoint.h"
#include "utils/include/CodePage.h"
#include "sim900/include/GsmDriver.h"
#include "logger/include/Logger.h"

#include <string.h>

#define NANOKASSA_CHECK_ADDR "q.nanokassa.ru"
#define NANOKASSA_CHECK_PORT 80
#define NANOKASSA_POLL_ADDR "fp.nanokassa.com"
#define NANOKASSA_POLL_PORT 80
#define NANOKASSA_ID_SIZE 32 //todo: уточнить размер
#define NANOKASSA_REQUEST_SIZE 2000
#define NANOKASSA_RECV_TIMEOUT 10000

namespace Nanokassa {

TaxRate convertTaxRate2Nanokassa(uint8_t taxRate) {
	switch(taxRate) {
	case Fiscal::TaxRate_NDSNone: return Nanokassa::TaxRate_NDSNone;
	case Fiscal::TaxRate_NDS0: return Nanokassa::TaxRate_NDS0;
	case Fiscal::TaxRate_NDS10: return Nanokassa::TaxRate_NDS10;
	case Fiscal::TaxRate_NDS20: return Nanokassa::TaxRate_NDS20;
	default: return Nanokassa::TaxRate_NDS20;
	}
}

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
	conn(conn),
	timers(timers),
	eventEngine(eventEngine),
	realtime(realtime),
	leds(leds),
	deviceId(eventEngine),
	state(State_Idle),
	kassaid(CERT_SIZE, CERT_SIZE),
	id(NANOKASSA_ID_SIZE, NANOKASSA_ID_SIZE),
	buf(NANOKASSA_REQUEST_SIZE, NANOKASSA_REQUEST_SIZE),
	envelope(EVENT_DATA_SIZE),
	tryCount(0)
{
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	context->setManufacturer((uint8_t*)NANOKASSA_MANUFACTURER, strlen(NANOKASSA_MANUFACTURER));
	context->setModel((uint8_t*)NANOKASSA_MODEL, strlen(NANOKASSA_MODEL));
	this->timer = timers->addTimer<CommandLayer, &CommandLayer::procTimer>(this);
	this->conn->setObserver(this);
	this->checkParser = new Nanokassa::CheckResponseParser(deviceId, &buf);
	this->pollParser = new Nanokassa::PollResponseParser(deviceId, &buf);
	this->eventEngine->subscribe(this, GlobalId_Sim900);
	this->fiscal->getSignPrivateKey()->load(&kassaid);
}

CommandLayer::~CommandLayer() {
	delete checkParser;
	timers->deleteTimer(timer);
}

EventDeviceId CommandLayer::getDeviceId() {
	return deviceId;
}

void CommandLayer::reset() {
	LOG_DEBUG(LOG_FR, "reset");
	timer->stop();
	this->leds->setFiscal(LedInterface::State_Success);
	context->setStatus(Mdb::DeviceContext::Status_Work);
	state = State_Idle;
}

void CommandLayer::sale(Fiscal::Sale *saleData) {
	LOG_DEBUG(LOG_FR, "sale");
	if(this->state != State_Idle) {
		LOG_ERROR(LOG_FR, "Wrong state " << state);
		procError(ConfigEvent::Type_FiscalLogicError, __LINE__);
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
	this->tryCount = 0;
	this->leds->setFiscal(LedInterface::State_InProgress);

	makeCheckRequest();
	gotoStateCheckConnect();
}

void CommandLayer::getLastSale() {
	LOG_DEBUG(LOG_FR, "getLastSale");
//	this->command = Command_GetLastSale;
//	gotoStateConnect();
}

void CommandLayer::closeShift() {
	LOG_DEBUG(LOG_FR, "closeShift");
//	this->command = Command_CloseShift;
//	gotoStateConnect();
}

void CommandLayer::proc(Event *event) {
	LOG_DEBUG(LOG_FR, "proc " << state);
	switch(state) {
		case State_CheckConnect: stateCheckConnectEvent(event); break;
		case State_CheckSend: stateCheckSendEvent(event); break;
		case State_CheckRecv: stateCheckRecvEvent(event); break;
		case State_CheckDisconnect: stateCheckDisconnectEvent(event); break;
		case State_CheckTryDisconnect: stateCheckTryDisconnectEvent(event); break;
		case State_PollConnect: statePollConnectEvent(event); break;
		case State_PollSend: statePollSendEvent(event); break;
		case State_PollRecv: statePollRecvEvent(event); break;
		case State_PollDisconnect: statePollDisconnectEvent(event); break;
		case State_PollTryDisconnect: statePollTryDisconnectEvent(event); break;
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
	case State_CheckRecv: stateCheckRecvTimeout(); break;
	case State_CheckTryDelay: stateCheckTryDelayTimeout(); break;
	case State_PollDelay: statePollDelayTimeout(); break;
	case State_PollRecv: statePollRecvTimeout(); break;
	case State_PollTryDelay: statePollTryDelayTimeout(); break;
	default: LOG_ERROR(LOG_FR, "Unwaited timeout state=" << state);
	}
}

void CommandLayer::gotoStateCheckConnect() {
	if(conn->connect(NANOKASSA_CHECK_ADDR, NANOKASSA_CHECK_PORT, TcpIp::Mode_TcpIp) == false) {
		gotoStateCheckTryDisconnect(ConfigEvent::Type_FiscalConnectError, __LINE__);
		return;
	}
	state = State_CheckConnect;
}

void CommandLayer::stateCheckConnectEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "stateCheckConnectEvent");
	switch(event->getType()) {
	case TcpIp::Event_ConnectOk: {
		gotoStateCheckSend();
		return;
	}
	case TcpIp::Event_ConnectError: {
		EventError *errorEvent = (EventError*)event;
		gotoStateCheckTryDisconnect(ConfigEvent::Type_FiscalConnectError, __LINE__, errorEvent->trace.getString());
		return;
	}
	case TcpIp::Event_Close: {
		gotoStateCheckTryDisconnect(ConfigEvent::Type_FiscalConnectError, __LINE__);
		return;
	}
	default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::gotoStateCheckSend() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckSend");
	if(conn->send(buf.getData(), buf.getLen()) == false) {
		LOG_ERROR(LOG_FR, "Send failed");
		gotoStateCheckTryDisconnect(ConfigEvent::Type_FiscalUnknownError, __LINE__);
		return;
	}
	saleData->fiscalStorage = Fiscal::Status_Unknown;
	state = State_CheckSend;
}

void CommandLayer::makeCheckRequest() {
	LOG_DEBUG(LOG_FR, "makeRequest");
	generateId();

	buf.clear();
	makeCheckBody();
	uint16_t dataLen = buf.getLen();

	buf.clear();
	buf << "POST /srv/gws/gw17.php HTTP/1.1\r\n";
	buf << "Host: " << NANOKASSA_CHECK_ADDR << ":" << NANOKASSA_CHECK_PORT << "\r\n";
	buf << "Content-Type: application/json; charset=utf-8\r\n";
	buf << "Content-Length: " << dataLen << "\r\n\r\n";

	makeCheckBody();
	LOG_DEBUG_STR(LOG_FR, (char*)buf.getData(), buf.getLen());
}

void CommandLayer::generateId() {
	uint32_t seconds = realtime->getUnixTimestamp();
	id.clear();
	id << boot->getImei();
	id << seconds;
}

void CommandLayer::makeMoney(uint32_t value) {
	uint32_t money = context->money2value(value);
	uint32_t money1 = money / 100;
	uint32_t money2 = money % 100;
	buf << money1 << "." << money2;
}

void CommandLayer::makeCheckBody() {
	uint32_t price = context->money2value(saleData->price);
	buf << "{\"id\": \"" << id << "\"";
	buf << ",\"unit-id\": 1234";
	buf << ",\"token\": \"" << kassaid << "\"";
	buf << ",\"address\": \""; convertWin1251ToJsonUnicode(fiscal->getPointAddr(), &buf); buf << "\"";
	buf << ",\"place\": \""; convertWin1251ToJsonUnicode(fiscal->getPointName(), &buf); buf << "\"";
	buf << ",\"vm-id\": \""; convertWin1251ToJsonUnicode(fiscal->getAutomatNumber(), &buf); buf << "\"";
	buf << ",\"items\": [";
	buf << "{\"name\": \""; convertWin1251ToJsonUnicode(saleData->name.get(), &buf); buf << "\"";
	buf << ",\"price\": " << price;
	buf << ",\"amount\": 1000";
	buf << ",\"vat\": " << convertTaxRate2Nanokassa(saleData->taxRate);
	buf << "}]";
	buf << ",\"tax_scheme\": " << FiscalStorage::convertTaxSystem2FN105(saleData->taxSystem);
	if(saleData->paymentType == Fiscal::Payment_Cash) {
		buf << ",\"cash\": " << price;
		buf << ",\"cashless\": 0";
	} else {
		buf << ",\"cash\": 0";
		buf << ",\"cashless\": " << price;
	}
	buf << "}";
}

void CommandLayer::stateCheckSendEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "stateCheckSendEvent");
	switch(event->getType()) {
	case TcpIp::Event_SendDataOk: {
		gotoStateCheckRecv();
		return;
	}
	case TcpIp::Event_SendDataError:
	case TcpIp::Event_Close: {
		gotoStateCheckTryDisconnect(ConfigEvent::Type_FiscalUnknownError, __LINE__);
		return;
	}
	default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::gotoStateCheckRecv() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckRecv");
	checkParser->start();
	if(conn->recv(checkParser->getBuf(), checkParser->getBufSize()) == false) {
		gotoStateCheckTryDisconnect(ConfigEvent::Type_FiscalUnknownError, __LINE__);
		return;
	}
	timer->start(NANOKASSA_RECV_TIMEOUT);
	state = State_CheckRecv;
}

void CommandLayer::stateCheckRecvEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "stateCheckRecvEvent");
	switch(event->getType()) {
	case TcpIp::Event_RecvDataOk: {
		LOG_DEBUG_HEX(LOG_FR, buf.getData(), event->getUint16());
		if(event->getUint16() == 0) {
			gotoStateCheckRecv();
			return;
		}
		timer->stop();

		checkParser->parse(event->getUint16());
		if(checkParser->getResult() != CheckResponseParser::Result_Ok) {
			Fiscal::EventError *error = checkParser->getError();
			context->removeAll();
			context->addError(error->code, error->data.getString());
			error->pack(&envelope);
			leds->setFiscal(LedInterface::State_Failure);
			gotoStatePollDisconnect();
			return;
		}

		tryCount = 0;
		saleData->fiscalStorage = Fiscal::Status_InQueue;
		gotoStateCheckDisconnect();
		return;
	}
	case TcpIp::Event_RecvDataError:
	case TcpIp::Event_Close: {
		timer->stop();
		gotoStateCheckTryDisconnect(ConfigEvent::Type_FiscalUnknownError, __LINE__);
		return;
	}
	default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::stateCheckRecvTimeout() {
	LOG_DEBUG(LOG_FR, "stateCheckRecvTimeout");
	gotoStateCheckTryDisconnect(ConfigEvent::Type_FiscalUnknownError, __LINE__);
}

void CommandLayer::gotoStateCheckDisconnect() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckDisconnect");
	conn->close();
	state = State_CheckDisconnect;
}

void CommandLayer::stateCheckDisconnectEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "stateCheckDisconnectEvent");
	switch(event->getType()) {
	case TcpIp::Event_Close: gotoStatePollDelay(); return;
	default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::gotoStateCheckTryDisconnect(uint16_t errorCode, uint32_t line, const char *suffix) {
	LOG_DEBUG(LOG_FR, "gotoStateNextTryDisconnect " << tryCount);
	tryCount++;
	if(tryCount > NANOKASSA_TRY_MAX) {
		procError(errorCode, line, suffix);
		return;
	}

	conn->close();
	state = State_CheckTryDisconnect;
}

void CommandLayer::stateCheckTryDisconnectEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "stateNextTryDisconnectEvent");
	switch(event->getType()) {
		case TcpIp::Event_Close: gotoStateCheckTryDelay(); return;
		default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::gotoStateCheckTryDelay() {
	LOG_DEBUG(LOG_FR, "gotoStateNextTryDelay");
	timer->start(NANOKASSA_TRY_DELAY);
	state = State_CheckTryDelay;
}

void CommandLayer::stateCheckTryDelayEnvelope(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_FR, "stateCheckTryDelayEnvelope");
	switch(envelope->getType()) {
		case Gsm::Driver::Event_NetworkUp: {
			timer->stop();
			gotoStateCheckConnect();
			return;
		}
		default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << envelope->getType());
	}
}

void CommandLayer::stateCheckTryDelayTimeout() {
	LOG_DEBUG(LOG_FR, "stateNextTryDelayTimeout");
	gotoStateCheckConnect();
}

void CommandLayer::gotoStatePollDelay() {
	LOG_DEBUG(LOG_FR, "gotoStatePollDelay");
	timer->start(NANOKASSA_POLL_DELAY);
	state = State_PollDelay;
}

void CommandLayer::statePollDelayTimeout() {
	LOG_DEBUG(LOG_FR, "statePollDelayTimeout");
	gotoStatePollConnect();
}

void CommandLayer::gotoStatePollConnect() {
	if(conn->connect(NANOKASSA_POLL_ADDR, NANOKASSA_POLL_PORT, TcpIp::Mode_TcpIp) == false) {
		gotoStatePollTryDisconnect(ConfigEvent::Type_FiscalConnectError, __LINE__);
		return;
	}
	state = State_PollConnect;
}

void CommandLayer::statePollConnectEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "statePollConnectEvent");
	switch(event->getType()) {
	case TcpIp::Event_ConnectOk: {
		gotoStatePollSend();
		return;
	}
	case TcpIp::Event_ConnectError: {
		EventError *errorEvent = (EventError*)event;
		gotoStatePollTryDisconnect(ConfigEvent::Type_FiscalConnectError, __LINE__, errorEvent->trace.getString());
		return;
	}
	case TcpIp::Event_Close: {
		gotoStatePollTryDisconnect(ConfigEvent::Type_FiscalConnectError, __LINE__);
		return;
	}
	default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::gotoStatePollSend() {
	LOG_DEBUG(LOG_FR, "gotoStatePollSend");
	makePollRequest();
	if(conn->send(buf.getData(), buf.getLen()) == false) {
		LOG_ERROR(LOG_FR, "Send failed");
		gotoStatePollTryDisconnect(ConfigEvent::Type_FiscalUnknownError, __LINE__);
		return;
	}
	state = State_PollSend;
}

void CommandLayer::makePollRequest() {
	LOG_DEBUG(LOG_FR, "makePollRequest");
	buf.clear();
	buf << "GET /st_hshuihqhvzvav_ss/receipt/" << id << " HTTP/1.1\r\n";
	buf << "Host: " << NANOKASSA_POLL_ADDR << ":" << NANOKASSA_POLL_PORT << "\r\n";
	buf << "Content-Type: application/json; charset=utf-8\r\n";
	buf << "Content-Length: 0\r\n\r\n";
	LOG_DEBUG_STR(LOG_FR, (char*)buf.getData(), buf.getLen());
}

void CommandLayer::statePollSendEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "statePollSendEvent");
	switch(event->getType()) {
	case TcpIp::Event_SendDataOk: {
		gotoStatePollRecv();
		return;
	}
	case TcpIp::Event_SendDataError:
	case TcpIp::Event_Close: {
		gotoStatePollTryDisconnect(ConfigEvent::Type_FiscalUnknownError, __LINE__);
		return;
	}
	default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::gotoStatePollRecv() {
	LOG_DEBUG(LOG_FR, "gotoStatePollRecv");
	pollParser->start();
	if(conn->recv(pollParser->getBuf(), pollParser->getBufSize()) == false) {
		gotoStatePollTryDisconnect(ConfigEvent::Type_FiscalUnknownError, __LINE__);
		return;
	}
	timer->start(NANOKASSA_RECV_TIMEOUT);
	state = State_PollRecv;
}

void CommandLayer::statePollRecvEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "statePollRecvEvent");
	switch(event->getType()) {
	case TcpIp::Event_RecvDataOk: statePollRecvEventRecv(event); return;
	case TcpIp::Event_RecvDataError:
	case TcpIp::Event_Close: {
		timer->stop();
		gotoStatePollTryDisconnect(ConfigEvent::Type_FiscalUnknownError, __LINE__);
		return;
	}
	default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::statePollRecvEventRecv(Event *event) {
	LOG_DEBUG(LOG_FR, "statePollRecvEventRecv");
	LOG_DEBUG_HEX(LOG_FR, buf.getData(), event->getUint16());
	if(event->getUint16() == 0) {
		gotoStatePollRecv();
		return;
	}

	timer->stop();

	pollParser->parse(event->getUint16());
	if(pollParser->getResult() == PollResponseParser::Result_Ok) {
		LOG_INFO(LOG_FR, "Check complete");
		saleData->fiscalDatetime.set(pollParser->getFiscalDatetime());
		saleData->fiscalRegister = pollParser->getFiscalRegister();
		saleData->fiscalStorage = pollParser->getFiscalStorage();
		saleData->fiscalDocument = pollParser->getFiscalDocument();
		saleData->fiscalSign = pollParser->getFiscalSign();
		context->removeAll();
		leds->setFiscal(LedInterface::State_Success);
		EventInterface event(deviceId, Fiscal::Register::Event_CommandOK);
		event.pack(&envelope);
		gotoStatePollDisconnect();
		return;
	} else if(pollParser->getResult() == PollResponseParser::Result_Busy) {
		LOG_INFO(LOG_FR, "Check in progress");
		gotoStatePollTryDisconnect(ConfigEvent::Type_FiscalCompleteNoData, __LINE__);
		return;
	} else {
		LOG_INFO(LOG_FR, "Check error " << pollParser->getResult());
		Fiscal::EventError *error = pollParser->getError();
		context->removeAll();
		context->addError(error->code, error->data.getString());
		error->pack(&envelope);
		leds->setFiscal(LedInterface::State_Failure);
		gotoStatePollDisconnect();
		return;
	}
}

void CommandLayer::statePollRecvTimeout() {
	LOG_DEBUG(LOG_FR, "statePollRecvTimeout");
	gotoStatePollTryDisconnect(ConfigEvent::Type_FiscalUnknownError, __LINE__);
}

void CommandLayer::gotoStatePollDisconnect() {
	LOG_DEBUG(LOG_FR, "gotoStatePollDisconnect");
	conn->close();
	state = State_PollDisconnect;
}

void CommandLayer::statePollDisconnectEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "statePollDisconnectEvent");
	switch(event->getType()) {
	case TcpIp::Event_Close: {
		state = State_Idle;
		if(eventEngine->transmit(&envelope) == false) {
			LOG_ERROR(LOG_FR, "transmit failed");
		}
		return;
	}
	default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::gotoStatePollTryDisconnect(uint16_t errorCode, uint32_t line, const char *suffix) {
	LOG_DEBUG(LOG_FR, "gotoStatePollTryDisconnect " << tryCount);
	tryCount++;
	if(tryCount > NANOKASSA_TRY_MAX) {
		procError(errorCode, line, suffix);
		return;
	}

	conn->close();
	state = State_PollTryDisconnect;
}

void CommandLayer::statePollTryDisconnectEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "statePollTryDisconnectEvent");
	switch(event->getType()) {
		case TcpIp::Event_Close: gotoStatePollTryDelay(); return;
		default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::gotoStatePollTryDelay() {
	LOG_DEBUG(LOG_FR, "gotoStatePollTryDelay");
	timer->start(NANOKASSA_TRY_DELAY);
	state = State_PollTryDelay;
}

void CommandLayer::statePollTryDelayEnvelope(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_FR, "statePollTryDelay");
	switch(envelope->getType()) {
		case Gsm::Driver::Event_NetworkUp: {
			timer->stop();
			gotoStatePollConnect();
			return;
		}
		default: LOG_ERROR(LOG_FR, "Unwaited event " << state << "," << envelope->getType());
	}
}

void CommandLayer::statePollTryDelayTimeout() {
	LOG_DEBUG(LOG_FR, "statePollTryDelayTimeout");
	gotoStatePollConnect();
}

void CommandLayer::procError(uint16_t errorCode, uint32_t line, const char *suffix) {
	LOG_DEBUG(LOG_FR, "procError");
	Fiscal::EventError event(deviceId);
	event.code = errorCode;
	event.data.clear();
	event.data << "od" << line;
	if(suffix != NULL) { event.data << ";" << suffix; }
	event.pack(&envelope);
	context->removeAll();
	context->addError(event.code, event.data.getString());
	leds->setFiscal(LedInterface::State_Failure);
	gotoStatePollDisconnect();
}

uint8_t CommandLayer::getPaymentType(uint8_t paymentType) {
	switch(paymentType) {
	case Fiscal::Payment_Cash: return 1;
	case Fiscal::Payment_Cashless: return 2;
	default: return 1;
	}
}

uint8_t CommandLayer::convertTaxSystem(uint8_t taxSystem) {
	switch(taxSystem) {
	case Fiscal::TaxSystem_OSN: return 0;
	case Fiscal::TaxSystem_USND: return 1;
	case Fiscal::TaxSystem_USNDMR: return 2;
	case Fiscal::TaxSystem_ENVD: return 3;
	case Fiscal::TaxSystem_ESN: return 4;
	case Fiscal::TaxSystem_Patent: return 5;
	default: return 0;
	}
}

}
