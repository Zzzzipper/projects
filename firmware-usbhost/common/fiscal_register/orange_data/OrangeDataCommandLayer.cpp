#include "OrangeDataCommandLayer.h"
#include "OrangeDataProtocol.h"

#include "fiscal_storage/include/FiscalStorage.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "sim900/include/GsmDriver.h"
#include "utils/include/DecimalPoint.h"
#include "utils/include/CodePage.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace OrangeData {

CommandLayer::CommandLayer(
	ConfigModem *config,
	Fiscal::Context *context,
	RsaSignInterface *sign,
	TcpIp *conn,
	TimerEngine *timers,
	EventEngineInterface *eventEngine,
	RealTimeInterface *realtime,
	LedInterface *leds
) :
	boot(config->getBoot()),
	fiscal(config->getFiscal()),
	context(context),
	sign(sign),
	conn(conn),
	timers(timers),
	eventEngine(eventEngine),
	realtime(realtime),
	leds(leds),
	id(ORANGEDATA_ID_SIZE, ORANGEDATA_ID_SIZE),
	buf(ORANGEDATA_REQUEST_SIZE, ORANGEDATA_REQUEST_SIZE),
	envelope(EVENT_DATA_SIZE),
	tryCount(0)
{
	context->setState(State_Idle);
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	context->setManufacturer((uint8_t*)ORANGEDATA_MANUFACTURER, strlen(ORANGEDATA_MANUFACTURER));
	context->setModel((uint8_t*)ORANGEDATA_MODEL, strlen(ORANGEDATA_MODEL));
	this->deviceId = eventEngine->getDeviceId();
	this->timer = timers->addTimer<CommandLayer, &CommandLayer::procTimer>(this);
	this->conn->setObserver(this);
	this->checkParser = new OrangeData::CheckResponseParser(deviceId, &buf);
	this->pollParser = new OrangeData::PollResponseParser(deviceId, &buf);
	this->eventEngine->subscribe(this, GlobalId_Sim900);
}

CommandLayer::~CommandLayer() {
	delete checkParser;
	timers->deleteTimer(timer);
}

uint16_t CommandLayer::getDeviceId() {
	return deviceId;
}

void CommandLayer::reset() {
	LOG_DEBUG(LOG_FR, "reset");
	timer->stop();
	this->leds->setFiscal(LedInterface::State_Success);
	context->setStatus(Mdb::DeviceContext::Status_Work);
	context->setState(State_Idle);
}

void CommandLayer::sale(Fiscal::Sale *saleData) {
	LOG_DEBUG(LOG_FR, "sale");
	if(context->getState() != State_Idle) {
		LOG_ERROR(LOG_FR, "Wrong state " << context->getState());
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
	LOG_DEBUG(LOG_FR, "proc " << context->getState());
	switch(context->getState()) {
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
		default: LOG_ERROR(LOG_FR, "Unwaited data state=" << context->getState() << "," << event->getType());
	}
}

void CommandLayer::proc(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_FR, "proc");
	switch(context->getState()) {
		case State_CheckTryDelay: stateCheckTryDelayEnvelope(envelope); return;
		case State_PollTryDelay: statePollTryDelayEnvelope(envelope); return;
		default: LOG_ERROR(LOG_FR, "Unwaited envelope " << context->getState() << "," << envelope->getType());
	}
}

void CommandLayer::procTimer() {
	LOG_DEBUG(LOG_FR, "procTimer");
	switch(context->getState()) {
	case State_CheckRecv: stateCheckRecvTimeout(); break;
	case State_CheckTryDelay: stateCheckTryDelayTimeout(); break;
	case State_PollDelay: statePollDelayTimeout(); break;
	case State_PollRecv: statePollRecvTimeout(); break;
	case State_PollTryDelay: statePollTryDelayTimeout(); break;
	default: LOG_ERROR(LOG_FR, "Unwaited timeout state=" << context->getState());
	}
}

void CommandLayer::gotoStateCheckConnect() {
	LOG_MEMORY_USAGE("orange");
	if(conn->connect(fiscal->getKktAddr(), fiscal->getKktPort(), TcpIp::Mode_TcpIpOverSsl) == false) {
		gotoStateCheckTryDisconnect(ConfigEvent::Type_FiscalConnectError, __LINE__);
		return;
	}
	context->setState(State_CheckConnect);
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
	default: LOG_ERROR(LOG_FR, "Unwaited event " << context->getState() << "," << event->getType());
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
	context->setState(State_CheckSend);
}

void CommandLayer::makeCheckRequest() {
	LOG_DEBUG(LOG_FR, "makeRequest");
	generateId();

	buf.clear();
	makeCheckBody();
	uint16_t dataLen = buf.getLen();
	if(sign->sign(buf.getData(), buf.getLen()) == false) {
		LOG_ERROR(LOG_FR, "Sign failed");
		procError(ConfigEvent::Type_FiscalUnknownError, __LINE__);
		return;
	}

	buf.clear();
	buf << "POST /api/v2/documents/ HTTP/1.1\r\n";
	buf << "Host: " << fiscal->getKktAddr() << ":" << fiscal->getKktPort() << "\r\n";
	buf << "Content-Type: application/json; charset=utf-8\r\n";
	buf << "X-Signature: ";
	uint16_t signLen = sign->base64encode(buf.getData() + buf.getLen(), buf.getSize() - buf.getLen());
	if(signLen <= 0) {
		LOG_ERROR(LOG_FR, "base64encode failed");
		procError(ConfigEvent::Type_FiscalUnknownError, __LINE__);
		return;
	}
	buf.setLen(buf.getLen() + signLen);
	buf << "\r\nContent-Length: " << dataLen << "\r\n\r\n";

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
	buf << "{\"id\": \"" << id << "\"";
	if(fiscal->getKkt() == ConfigFiscal::Kkt_EphorOrangeData) { buf << ",\"key\": \"4010004\""; }
	if(fiscal->getKkt() == ConfigFiscal::Kkt_OrangeData) { buf << ",\"key\": \"" << fiscal->getINN() << "\""; }
	buf << ",\"inn\": \"" << fiscal->getINN() << "\"";
	if(strlen(fiscal->getGroup()) == 0) {
		buf << ",\"group\": \"Vend\"";
	} else {
		buf << ",\"group\": \"" << fiscal->getGroup() << "\"";
	}
	buf << ",\"content\": ";

	buf << "{\"type\": 1"; // приход
	buf << ",\"positions\": [";
	buf << "{\"quantity\": 1.000";
	buf << ",\"price\": "; makeMoney(saleData->price);
	buf << ",\"tax\": " << FiscalStorage::convertTaxRate2FN105(saleData->taxRate);
	buf << ",\"text\": \""; convertWin1251ToJsonUnicode(saleData->name.get(), &buf); buf << "\"";
	buf << "}]";

	buf << ",\"checkClose\": {";
	buf << "\"payments\": [";
	buf << "{\"type\": " << getPaymentType(saleData->paymentType) << ",\"amount\": "; makeMoney(saleData->price); buf << "}";
	buf << "]";
	buf << ",\"taxationSystem\": " << convertTaxSystem(saleData->taxSystem) << "}";
	buf << ",\"automatNumber\": \""; convertWin1251ToJsonUnicode(fiscal->getAutomatNumber(), &buf); buf << "\"";
	buf << ",\"settlementPlace\": \"";  convertWin1251ToJsonUnicode(fiscal->getPointName(), &buf); buf << "\"";
	buf << ",\"settlementAddress\": \""; convertWin1251ToJsonUnicode(fiscal->getPointAddr(), &buf); buf << "\"";
	buf << "}}";
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
	default: LOG_ERROR(LOG_FR, "Unwaited event " << context->getState() << "," << event->getType());
	}
}

void CommandLayer::gotoStateCheckRecv() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckRecv");
	checkParser->start();
	if(conn->recv(checkParser->getBuf(), checkParser->getBufSize()) == false) {
		gotoStateCheckTryDisconnect(ConfigEvent::Type_FiscalUnknownError, __LINE__);
		return;
	}
	timer->start(ORANGEDATA_RECV_TIMEOUT);
	context->setState(State_CheckRecv);
}

void CommandLayer::stateCheckRecvEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "stateCheckRecvEvent");
	switch(event->getType()) {
	case TcpIp::Event_RecvDataOk: {
		LOG_DEBUG_HEX(LOG_FR, buf.getData(), event->getUint16());
		timer->stop();

		checkParser->parse(event->getUint16());
		if(checkParser->getResult() != CheckResponseParser::Result_Ok) {
			Fiscal::EventError *error = checkParser->getError();
			saleData->fiscalStorage = Fiscal::Status_Error;
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
	default: LOG_ERROR(LOG_FR, "Unwaited event " << context->getState() << "," << event->getType());
	}
}

void CommandLayer::stateCheckRecvTimeout() {
	LOG_DEBUG(LOG_FR, "stateCheckRecvTimeout");
	gotoStateCheckTryDisconnect(ConfigEvent::Type_FiscalUnknownError, __LINE__);
}

void CommandLayer::gotoStateCheckDisconnect() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckDisconnect");
	conn->close();
	context->setState(State_CheckDisconnect);
}

void CommandLayer::stateCheckDisconnectEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "stateCheckDisconnectEvent");
	switch(event->getType()) {
	case TcpIp::Event_Close: gotoStatePollDelay(); return;
	default: LOG_ERROR(LOG_FR, "Unwaited event " << context->getState() << "," << event->getType());
	}
}

void CommandLayer::gotoStateCheckTryDisconnect(uint16_t errorCode, uint32_t line, const char *suffix) {
	LOG_DEBUG(LOG_FR, "gotoStateNextTryDisconnect " << tryCount);
	tryCount++;
	if(tryCount > ORANGEDATA_TRY_MAX) {
		procError(errorCode, line, suffix);
		return;
	}

	conn->close();
	context->incResetCount();
	context->setState(State_CheckTryDisconnect);
}

void CommandLayer::stateCheckTryDisconnectEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "stateNextTryDisconnectEvent");
	switch(event->getType()) {
		case TcpIp::Event_Close: gotoStateCheckTryDelay(); return;
		default: LOG_ERROR(LOG_FR, "Unwaited event " << context->getState() << "," << event->getType());
	}
}

void CommandLayer::gotoStateCheckTryDelay() {
	LOG_DEBUG(LOG_FR, "gotoStateNextTryDelay");
	timer->start(ORANGEDATA_TRY_DELAY);
	context->setState(State_CheckTryDelay);
}

void CommandLayer::stateCheckTryDelayEnvelope(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_FR, "stateCheckTryDelayEnvelope");
	switch(envelope->getType()) {
		case Gsm::Driver::Event_NetworkUp: {
			timer->stop();
			gotoStateCheckConnect();
			return;
		}
		default: LOG_ERROR(LOG_FR, "Unwaited event " << context->getState() << "," << envelope->getType());
	}
}

void CommandLayer::stateCheckTryDelayTimeout() {
	LOG_DEBUG(LOG_FR, "stateNextTryDelayTimeout");
	gotoStateCheckConnect();
}

void CommandLayer::gotoStatePollDelay() {
	LOG_DEBUG(LOG_FR, "gotoStatePollDelay");
	timer->start(ORANGEDATA_POLL_DELAY);
	context->setState(State_PollDelay);
}

void CommandLayer::statePollDelayTimeout() {
	LOG_DEBUG(LOG_FR, "statePollDelayTimeout");
	gotoStatePollConnect();
}

void CommandLayer::gotoStatePollConnect() {
	if(conn->connect(fiscal->getKktAddr(), fiscal->getKktPort(), TcpIp::Mode_TcpIpOverSsl) == false) {
		gotoStatePollTryDisconnect(ConfigEvent::Type_FiscalConnectError, __LINE__);
		return;
	}
	context->setState(State_PollConnect);
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
	default: LOG_ERROR(LOG_FR, "Unwaited event " << context->getState() << "," << event->getType());
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
	context->setState(State_PollSend);
}

void CommandLayer::makePollRequest() {
	LOG_DEBUG(LOG_FR, "makePollRequest");
	buf.clear();
	buf << "GET /api/v2/documents/" << fiscal->getINN() << "/status/" << id << " HTTP/1.1\r\n";
	buf << "Host: " << fiscal->getKktAddr() << ":" << fiscal->getKktPort() << "\r\n";
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
	default: LOG_ERROR(LOG_FR, "Unwaited event " << context->getState() << "," << event->getType());
	}
}

void CommandLayer::gotoStatePollRecv() {
	LOG_DEBUG(LOG_FR, "gotoStatePollRecv");
	pollParser->start();
	if(conn->recv(pollParser->getBuf(), pollParser->getBufSize()) == false) {
		gotoStatePollTryDisconnect(ConfigEvent::Type_FiscalUnknownError, __LINE__);
		return;
	}
	timer->start(ORANGEDATA_RECV_TIMEOUT);
	context->setState(State_PollRecv);
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
	default: LOG_ERROR(LOG_FR, "Unwaited event " << context->getState() << "," << event->getType());
	}
}

void CommandLayer::statePollRecvEventRecv(Event *event) {
	LOG_DEBUG(LOG_FR, "statePollRecvEventRecv");
	LOG_DEBUG_HEX(LOG_FR, buf.getData(), event->getUint16());
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
	context->setState(State_PollDisconnect);
}

void CommandLayer::statePollDisconnectEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "statePollDisconnectEvent");
	switch(event->getType()) {
	case TcpIp::Event_Close: {
		context->setState(State_Idle);
		if(eventEngine->transmit(&envelope) == false) {
			LOG_ERROR(LOG_FR, "transmit failed");
		}
		return;
	}
	default: LOG_ERROR(LOG_FR, "Unwaited event " << context->getState() << "," << event->getType());
	}
}

void CommandLayer::gotoStatePollTryDisconnect(uint16_t errorCode, uint32_t line, const char *suffix) {
	LOG_DEBUG(LOG_FR, "gotoStatePollTryDisconnect " << tryCount);
	tryCount++;
	if(tryCount > ORANGEDATA_TRY_MAX) {
		procError(errorCode, line, suffix);
		return;
	}

	conn->close();
	context->incResetCount();
	context->setState(State_PollTryDisconnect);
}

void CommandLayer::statePollTryDisconnectEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "statePollTryDisconnectEvent");
	switch(event->getType()) {
		case TcpIp::Event_Close: gotoStatePollTryDelay(); return;
		default: LOG_ERROR(LOG_FR, "Unwaited event " << context->getState() << "," << event->getType());
	}
}

void CommandLayer::gotoStatePollTryDelay() {
	LOG_DEBUG(LOG_FR, "gotoStatePollTryDelay");
	timer->start(ORANGEDATA_TRY_DELAY);
	context->setState(State_PollTryDelay);
}

void CommandLayer::statePollTryDelayEnvelope(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_FR, "statePollTryDelay");
	switch(envelope->getType()) {
		case Gsm::Driver::Event_NetworkUp: {
			timer->stop();
			gotoStatePollConnect();
			return;
		}
		default: LOG_ERROR(LOG_FR, "Unwaited event " << context->getState() << "," << envelope->getType());
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
