#include "ChekOnlineCommandLayer.h"
#include "ChekOnlineProtocol.h"
#include "fiscal_storage/include/FiscalStorage.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "utils/include/DecimalPoint.h"
#include "utils/include/CodePage.h"
#include "logger/include/Logger.h"

#include <string.h>

#define CHEKONLINE_ID_SIZE 32 //todo: уточнить размер
#define CHEKONLINE_REQUEST_SIZE 2000
#define CHEKONLINE_RECV_TIMEOUT 40000

namespace ChekOnline {

TaxRate convertTaxRate2ChekOnline(uint8_t taxRate) {
	switch(taxRate) {
	case Fiscal::TaxRate_NDSNone: return ChekOnline::TaxRate_NDSNone;
	case Fiscal::TaxRate_NDS0: return ChekOnline::TaxRate_NDS0;
	case Fiscal::TaxRate_NDS10: return ChekOnline::TaxRate_NDS10;
	case Fiscal::TaxRate_NDS20: return ChekOnline::TaxRate_NDS20;
	default: return ChekOnline::TaxRate_NDS20;
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
	id(CHEKONLINE_ID_SIZE, CHEKONLINE_ID_SIZE),
	buf(CHEKONLINE_REQUEST_SIZE, CHEKONLINE_REQUEST_SIZE),
	envelope(EVENT_DATA_SIZE),
	parser(deviceId, &buf)
{
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	context->setManufacturer((uint8_t*)CHEKONLINE_MANUFACTURER, strlen(CHEKONLINE_MANUFACTURER));
	context->setModel((uint8_t*)CHEKONLINE_MODEL, strlen(CHEKONLINE_MODEL));
	this->timer = timers->addTimer<CommandLayer, &CommandLayer::procTimer>(this);
	this->conn->setObserver(this);
}

CommandLayer::~CommandLayer() {
	this->timers->deleteTimer(this->timer);
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
	this->leds->setFiscal(LedInterface::State_InProgress);
	gotoStateConnect();
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

void CommandLayer::procTimer() {
	LOG_DEBUG(LOG_FR, "procTimer");
	switch(state) {
	case State_CheckRecv: stateCheckRecvTimeout(); break;
	default: LOG_ERROR(LOG_FRP, "Unwaited timeout state=" << state);
	}
}

void CommandLayer::proc(Event *event) {
	LOG_DEBUG(LOG_FR, "proc " << state);
	switch(state) {
	case State_Connect: stateConnectEvent(event); break;
	case State_CheckSend: stateCheckSendEvent(event); break;
	case State_CheckRecv: stateCheckRecvEvent(event); break;
	case State_Disconnect: stateDisconnectEvent(event); break;
	default: LOG_ERROR(LOG_FRP, "Unwaited data state=" << state << "," << event->getType());
	}
}

void CommandLayer::gotoStateConnect() {
	if(conn->connect(fiscal->getKktAddr(), fiscal->getKktPort(), TcpIp::Mode_TcpIpOverSsl) == false) {
		procError(ConfigEvent::Type_FiscalConnectError);
		return;
	}
	state = State_Connect;
}

void CommandLayer::stateConnectEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "stateConnectEvent");
	switch(event->getType()) {
	case TcpIp::Event_ConnectOk: {
		gotoStateCheckSend();
		return;
	}
	case TcpIp::Event_ConnectError:
	case TcpIp::Event_Close: {
		procError(ConfigEvent::Type_FiscalConnectError);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::gotoStateCheckSend() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckSend");
	makeRequest();
	if(conn->send(buf.getData(), buf.getLen()) == false) {
		procError(ConfigEvent::Type_FiscalUnknownError);
		return;
	}
	state = State_CheckSend;
}

void CommandLayer::makeRequest() {
	LOG_DEBUG(LOG_FR, "makeRequest");
	generateId();

	buf.clear();
	makeCheck();
	uint16_t dataLen = buf.getLen();

	buf.clear();
	buf << "POST /fr/api/v2/Complex HTTP/1.1\r\n";
	buf << "Host: " << fiscal->getKktAddr() << ":" << fiscal->getKktPort() << "\r\n";
	buf << "Content-Type: application/json; charset=utf-8\r\n";
	buf << "Content-Length: " << dataLen << "\r\n\r\n";

	makeCheck();
	LOG_STR(buf.getData(), buf.getLen());
}

void CommandLayer::generateId() {
	uint32_t seconds = realtime->getUnixTimestamp();
	id.clear();
	id << boot->getImei();
	id << seconds;
}

void CommandLayer::makeCheck() {
	uint32_t price = context->money2value(saleData->price);
	buf << "{\"Device\": \"auto\"";
	buf << ",\"ClientId\": \"\"";
	buf << ",\"Password\": 1";
	buf << ",\"RequestId\": \"" << id << "\"";
	buf << ",\"Lines\": [";
	buf << "{\"Qty\": 1000";
	buf << ",\"Price\": " << price;
	buf << ",\"PayAttribute\": 4";
	buf << ",\"TaxId\": " << convertTaxRate2ChekOnline(saleData->taxRate);
	buf << ",\"Description\": \""; convertWin1251ToJsonUnicode(saleData->name.get(), &buf); buf << "\"";
	buf << "}]";
	if(saleData->paymentType == Fiscal::Payment_Cash) {
		buf << ",\"Cash\": " << price;
	} else {
		buf << ",\"NonCash\": [" << price << "]";
	}
	buf << ",\"TaxMode\": " << FiscalStorage::convertTaxSystem2FN105(saleData->taxSystem);
	buf << ",\"Terminal\": \""; convertWin1251ToJsonUnicode(fiscal->getAutomatNumber(), &buf); buf << "\"";
	buf << ",\"Place\": \""; convertWin1251ToJsonUnicode(fiscal->getPointName(), &buf); buf << "\"";
	buf << ",\"Address\": \""; convertWin1251ToJsonUnicode(fiscal->getPointAddr(), &buf); buf << "\"";
	buf << ",\"FullResponse\": false";
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
		procError(ConfigEvent::Type_FiscalUnknownError);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::gotoStateCheckRecv() {
	LOG_DEBUG(LOG_FR, "gotoStateCheckRecv");
	parser.start();
	if(conn->recv(parser.getBuf(), parser.getBufSize()) == false) {
		procError(ConfigEvent::Type_FiscalUnknownError);
		return;
	}
	timer->start(CHEKONLINE_RECV_TIMEOUT);
	state = State_CheckRecv;
}

void CommandLayer::stateCheckRecvEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "stateCheckRecvEvent");
	switch(event->getType()) {
	case TcpIp::Event_RecvDataOk: {
		LOG_TRACE_HEX(LOG_FR, buf.getData(), event->getUint16());
		timer->stop();

		parser.parse(event->getUint16());
		if(parser.getResult() != ResponseParser::Result_Ok) {
			LOG_DEBUG_STR(LOG_FR, buf.getData(), buf.getLen());
			Fiscal::EventError *error = parser.getError();
			context->removeAll();
			context->addError(error->code, error->data.getString());
			error->pack(&envelope);
			leds->setFiscal(LedInterface::State_Failure);
			gotoStateDisconnect();
			return;
		}

		saleData->fiscalRegister = parser.getFiscalRegister();
		saleData->fiscalStorage = parser.getFiscalStorage();
		saleData->fiscalDocument = parser.getFiscalDocument();
		saleData->fiscalSign = parser.getFiscalSign();

		context->removeAll();
		EventInterface event(deviceId, Fiscal::Register::Event_CommandOK);
		event.pack(&envelope);
		leds->setFiscal(LedInterface::State_Success);
		gotoStateDisconnect();
		return;
	}
	case TcpIp::Event_RecvDataError:
	case TcpIp::Event_Close: {
		timer->stop();
		procError(ConfigEvent::Type_FiscalUnknownError);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::stateCheckRecvTimeout() {
	LOG_DEBUG(LOG_FR, "stateCheckRecvTimeout");
	procError(ConfigEvent::Type_FiscalUnknownError);
}

void CommandLayer::gotoStateDisconnect() {
	LOG_DEBUG(LOG_FR, "gotoStateDisconnect");
	conn->close();
	state = State_Disconnect;
}

void CommandLayer::stateDisconnectEvent(Event *event) {
	LOG_DEBUG(LOG_FR, "stateDisconnectEvent");
	switch(event->getType()) {
	case TcpIp::Event_Close: {
		state = State_Idle;
		eventEngine->transmit(&envelope);
		return;
	}
	default: LOG_ERROR(LOG_FRP, "Unwaited event " << state << "," << event->getType());
	}
}

void CommandLayer::procError(uint16_t errorCode) {
	LOG_DEBUG(LOG_FR, "procError");
	Fiscal::EventError event(deviceId);
	event.code = errorCode;
	event.pack(&envelope);
	context->removeAll();
	context->addError(event.code, event.data.getString());
	leds->setFiscal(LedInterface::State_Failure);
	gotoStateDisconnect();
}

}
