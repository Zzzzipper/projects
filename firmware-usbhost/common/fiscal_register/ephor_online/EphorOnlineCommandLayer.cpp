#include "EphorOnlineCommandLayer.h"
#include "fiscal_storage/include/FiscalStorage.h"
#include "fiscal_register/include/FiscalRegister.h"
#include "utils/include/DecimalPoint.h"
#include "utils/include/CodePage.h"
#include "logger/include/Logger.h"

#include <string.h>

#define ORANGEDATA_MANUFACTURER	"EPR"
#define ORANGEDATA_MODEL		"Ephor Online"
#define ORANGEDATA_ID_SIZE 32
#define ORANGEDATA_REQUEST_SIZE 2000
#define ORANGEDATA_RECV_TIMEOUT 10000

namespace EphorOnline {

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
	id(ORANGEDATA_ID_SIZE, ORANGEDATA_ID_SIZE),
	buf(ORANGEDATA_REQUEST_SIZE, ORANGEDATA_REQUEST_SIZE),
	envelope(EVENT_DATA_SIZE),
	parser(deviceId, &buf)
{
	this->timer = timers->addTimer<CommandLayer, &CommandLayer::procTimer>(this);
	this->conn->setObserver(this);
}

CommandLayer::~CommandLayer() {
	this->timers->deleteTimer(this->timer);
}

void CommandLayer::reset() {
	LOG_DEBUG(LOG_FR, "reset");
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	context->setManufacturer((uint8_t*)ORANGEDATA_MANUFACTURER, strlen(ORANGEDATA_MANUFACTURER));
	context->setModel((uint8_t*)ORANGEDATA_MODEL, strlen(ORANGEDATA_MODEL));
	timer->stop();
	state = State_Idle;
}

void CommandLayer::sale(Fiscal::Sale *saleData) {
	LOG_DEBUG(LOG_FR, "sale");
	if(this->state != State_Idle) {
		LOG_ERROR(LOG_FR, "Wrong state " << state);
		procError(ConfigEvent::Type_FiscalLogicError);
		return;
	}

	this->command = Command_Sale;
	this->saleData = saleData;
	this->saleData->taxValue = saleData->products.getTaxValue();
	this->saleData->fiscalRegister = 0;
	this->saleData->fiscalStorage = 0;
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
		LOG_ERROR(LOG_FRP, "Send failed");
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
	buf << "POST /api/1.0/fiscal/Ticket.php?action=Add HTTP/1.1\r\n";
	buf << "Host: " << fiscal->getKktAddr() << ":" << fiscal->getKktPort() << "\r\n";
	buf << "Content-Type: application/json; charset=utf-8\r\n";
	buf << "Content-Length: " << dataLen << "\r\n\r\n";

	makeCheck();
	LOG_STR((char*)buf.getData(), buf.getLen());
}

void CommandLayer::generateId() {
	uint32_t seconds = realtime->getUnixTimestamp();
	id.clear();
	id << boot->getImei();
	id << seconds;
}

void CommandLayer::makeCheck() {
	buf << "{\"id\":\"" << id << "\"";
	buf << ",\"inn\":\"" << fiscal->getINN() << "\"";
	Fiscal::Product *product = saleData->getProduct(0);
	buf << ",\"name\":\"" << product->name.get() << "\"";
	buf << ",\"price\":" << product->price;
	buf << ",\"tax_rate\":" << product->taxRate;
	buf << ",\"payment_type\":" << saleData->paymentType;
	buf << ",\"credit\":" << saleData->credit;
	buf << ",\"tax_system\":" << saleData->taxSystem;
	buf << ",\"automat_id\":\"" << fiscal->getAutomatNumber() << "\"";
	buf << ",\"point_name\":\"" << fiscal->getPointName() << "\"";
	buf << ",\"point_addr\":\"" << fiscal->getPointAddr() << "\"";
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
	timer->start(ORANGEDATA_RECV_TIMEOUT);
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
			LOG_DEBUG_HEX(LOG_FR, buf.getData(), buf.getLen());
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
		context->setStatus(Mdb::DeviceContext::Status_Work);
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
