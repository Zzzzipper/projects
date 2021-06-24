#include "EcpClientCommandLayer.h"
#include "EcpProtocol.h"
#include "logger/include/Logger.h"

namespace Ecp {

ClientCommandLayer::ClientCommandLayer(TimerEngine *timers, ClientPacketLayerInterface *packetLayer) :
	packetLayer(packetLayer),
	timers(timers),
	state(State_Idle),
	request(ECP_PACKET_MAX_SIZE)
{
	this->packetLayer->setObserver(this);
	this->timer = timers->addTimer<ClientCommandLayer, &ClientCommandLayer::procTimer>(this);
}

ClientCommandLayer::~ClientCommandLayer() {
	timers->deleteTimer(this->timer);
}

void ClientCommandLayer::setObserver(EventObserver *observer) {
	courier.setRecipient(observer);
}

bool ClientCommandLayer::connect() {
	LOG_INFO(LOG_ECP, "connect");
	if(packetLayer->connect() == false) {
		LOG_ERROR(LOG_ECP, "Connect faileed");
		return false;
	}
	state = State_Connecting;
	return true;
}

void ClientCommandLayer::disconnect() {
	LOG_INFO(LOG_ECP, "disconnect");
	packetLayer->disconnect();
	state = State_Disconnecting;
}

bool ClientCommandLayer::uploadData(Ecp::Destination destination, Dex::DataGenerator *generator) {
	LOG_DEBUG(LOG_ECP, "uploadData");
	if(state != State_Ready) {
		LOG_ERROR(LOG_ECP, "Wrong state " << state);
		return false;
	}
	this->destination = destination;
	this->generator = generator;
	gotoStateUploadStart();
	return true;
}

bool ClientCommandLayer::downloadData(Ecp::Source source, Dex::DataParser *parser) {
	LOG_DEBUG(LOG_ECP, "downloadData " << source);
	if(state != State_Ready) {
		LOG_ERROR(LOG_ECP, "Wrong state " << state);
		return false;
	}
	this->source = source;
	this->parser = parser;
	gotoStateDownloadStart();
	return true;
}

bool ClientCommandLayer::getTableInfo(uint16_t tableId) {
	if(state != State_Ready) {
		LOG_ERROR(LOG_ECP, "Wrong state " << state);
		return false;
	}
	TableInfoRequest *req = (TableInfoRequest*)request.getData();
	req->command = Command_TableInfo;
	req->tableId = tableId;
	request.setLen(sizeof(*req));
	packetLayer->sendData(&request);
	state = State_Request;
	return true;
}

bool ClientCommandLayer::getTableEntry(uint16_t tableId, uint32_t entryIndex) {
	if(state != State_Ready) {
		LOG_ERROR(LOG_ECP, "Wrong state " << state);
		return false;
	}
	TableEntryRequest *req = (TableEntryRequest*)request.getData();
	req->command = Command_TableEntry;
	req->tableId = tableId;
	req->entryIndex = entryIndex;
	request.setLen(sizeof(*req));
	packetLayer->sendData(&request);
	state = State_Request;
	return true;
}

bool ClientCommandLayer::getDateTime() {
	if(state != State_Ready) {
		LOG_ERROR(LOG_ECP, "Wrong state " << state);
		return false;
	}

	Request *req = (Request*)request.getData();
	req->command = Command_DateTime;
	request.setLen(sizeof(*req));
	packetLayer->sendData(&request);
	state = State_Request;
	return true;
}

bool ClientCommandLayer::resetConfig() {
	if(state != State_Ready) {
		LOG_ERROR(LOG_ECP, "Wrong state " << state);
		return false;
	}

	Request *req = (Request*)request.getData();
	req->command = Command_ConfigReset;
	request.setLen(sizeof(*req));
	packetLayer->sendData(&request);
	state = State_Request;
	return true;
}

void ClientCommandLayer::cancel() {
	switch(state) {
	case State_UploadStart:
	case State_UploadData:
	case State_UploadEnd: {
		state = State_Ready;
		return;
	}
	default:;
	}
}

void ClientCommandLayer::procConnect() {
	LOG_DEBUG(LOG_ECP, "procConnect");
	gotoStateSetup();
}

void ClientCommandLayer::procRecvData(const uint8_t *data, uint16_t dataLen) {
	switch(state) {
	case State_Setup: stateSetupRecv(data, dataLen); return;
	case State_UploadStart: stateUploadStart(data, dataLen); return;
	case State_UploadData: stateUploadDataRecv(data, dataLen); return;
	case State_UploadEnd: stateUploadEndRecv(data, dataLen); return;
	case State_DownloadStart: stateDownloadStartRecv(data, dataLen); return;
	case State_DownloadData: stateDownloadDataRecv(data, dataLen); return;
	case State_Request: stateRequestRecv(data, dataLen); return;
	default: LOG_ERROR(LOG_ECP, "Unwaited data " << state << "," << dataLen);
	}
}

void ClientCommandLayer::procRecvError(Error error) {
	LOG_DEBUG(LOG_ECP, "procRecvError " << error);
	switch(state) {
	case State_Connecting:
	case State_Setup: {
		state = State_Idle;
		Event event(Client::Event_ConnectError, (uint16_t)error);
		courier.deliver(&event);
		return;
	}
	case State_Request: {
		state = State_Ready;
		Event event(Client::Event_ResponseError, (uint16_t)error);
		courier.deliver(&event);
		return;
	}
	case State_UploadStart:
	case State_UploadData:
	case State_UploadEnd: {
		state = State_Ready;
		Event event(Client::Event_UploadError, (uint16_t)error);
		courier.deliver(&event);
		return;
	}
	default: {
		state = State_Idle;
		courier.deliver(Client::Event_Disconnect);
		return;
	}
	}
}

void ClientCommandLayer::procDisconnect() {
	LOG_DEBUG(LOG_ECP, "procDisconnect");
	state = State_Idle;
	courier.deliver(Client::Event_Disconnect);
}

//todo: продумать доставку событий наверх
void ClientCommandLayer::procError(uint8_t error) {
	LOG_DEBUG(LOG_ECP, "procError " << state << "," << error);
	switch(state) {
	case State_Connecting:
	case State_Setup: {
		state = State_Idle;
		Event event(Client::Event_ConnectError, (uint16_t)error);
		courier.deliver(&event);
		return;
	}
	case State_Request: {
		state = State_Ready;
		Event event(Client::Event_ResponseError, (uint16_t)error);
		courier.deliver(&event);
		return;
	}
	case State_UploadStart:
	case State_UploadData:
	case State_UploadEnd: {
		state = State_Ready;
		Event event(Client::Event_UploadError, (uint16_t)error);
		courier.deliver(&event);
		return;
	}
	case State_DownloadStart:
	case State_DownloadData: {
		state = State_Ready;
		Event event(Client::Event_DownloadError, (uint16_t)error);
		courier.deliver(&event);
		return;
	}
	default: {
		state = State_Idle;
		courier.deliver(Client::Event_Disconnect);
		return;
	}
	}
}

void ClientCommandLayer::procTimer() {
	LOG_DEBUG(LOG_ECP, "procTimer " << state);
	switch(state) {
		case State_UploadDataBusy: stateUploadDataBusyTimeout(); break;
		case State_UploadEndBusy: stateUploadEndBusyTimeout(); break;
		default: LOG_ERROR(LOG_ECP, "Unwaited timeout state=" << state);
	}
}

void ClientCommandLayer::gotoStateSetup() {
	LOG_DEBUG(LOG_ECP, "gotoStateSetup");
	Request *req = (Request*)request.getData();
	req->command = Command_Setup;
	request.setLen(sizeof(*req));
	packetLayer->sendData(&request);
	state = State_Setup;
}

void ClientCommandLayer::stateSetupRecv(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateSetupRecv");
	Response *resp = (Response*)data;
	if(sizeof(*resp) > dataLen) {
		procError(Error_WrongPacketSize);
		return;
	}

	if(resp->errorCode != Error_OK) {
		procError(resp->errorCode);
		return;
	}

	state = State_Ready;
	courier.deliver(Client::Event_ConnectOK);
}

void ClientCommandLayer::gotoStateUploadStart() {
	LOG_DEBUG(LOG_ECP, "gotoStateUploadStart");
	generator->reset();

	UploadStartRequest *req = (UploadStartRequest*)request.getData();
	req->command = Command_UploadStart;
	req->destination = destination;
	req->dataSize = generator->getSize();
	request.setLen(sizeof(*req));
	packetLayer->sendData(&request);

	state = State_UploadStart;
}

void ClientCommandLayer::stateUploadStart(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateUploadStart");
	Response *resp = (Response*)data;
	if(sizeof(*resp) > dataLen) {
		procError(Error_WrongPacketSize);
		return;
	}

	if(resp->errorCode != Error_OK) {
		procError(resp->errorCode);
		return;
	}

	gotoStateUploadData();
}

void ClientCommandLayer::gotoStateUploadData() {
	LOG_DEBUG(LOG_ECP, "gotoStateUploadData");
	Request *req = (Request*)request.getData();
	req->command = Command_UploadData;
	request.setLen(sizeof(*req));
	request.add(generator->getData(), generator->getLen());
	packetLayer->sendData(&request);
	state = State_UploadData;
}

void ClientCommandLayer::stateUploadDataRecv(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateUploadDataRecv");
	Response *resp = (Response*)data;
	if(sizeof(*resp) > dataLen) {
		procError(Error_WrongPacketSize);
		return;
	}

	if(resp->errorCode == Error_Busy) {
		gotoStateUploadDataBusy();
		return;
	} else if(resp->errorCode != Error_OK) {
		procError(resp->errorCode);
		return;
	}

	if(generator->isLast() == false) {
		generator->next();
		Request *req = (Request*)request.getData();
		req->command = Command_UploadData;
		request.setLen(sizeof(*req));
		request.add(generator->getData(), generator->getLen());
		packetLayer->sendData(&request);
		return;
	} else {
		gotoStateUploadEnd();
	}
}

void ClientCommandLayer::gotoStateUploadDataBusy() {
	LOG_DEBUG(LOG_ECP, "gotoStateUploadDataBusy");
	timer->start(ECP_BUSY_TIMEOUT);
	state = State_UploadDataBusy;
}

void ClientCommandLayer::stateUploadDataBusyTimeout() {
	LOG_DEBUG(LOG_ECP, "stateUploadDataBusyTimeout");
	Request *req = (Request*)request.getData();
	req->command = Command_UploadData;
	request.setLen(sizeof(*req));
	request.add(generator->getData(), generator->getLen());
	packetLayer->sendData(&request);
	state = State_UploadData;
}

void ClientCommandLayer::gotoStateUploadEnd() {
	LOG_DEBUG(LOG_ECP, "gotoStateUploadEnd");
	Request *req = (Request*)request.getData();
	req->command = Command_UploadEnd;
	request.setLen(sizeof(*req));
	packetLayer->sendData(&request);
	state = State_UploadEnd;
}

void ClientCommandLayer::stateUploadEndRecv(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateUploadEndRecv");
	Response *resp = (Response*)data;
	if(sizeof(*resp) > dataLen) {
		procError(Error_WrongPacketSize);
		return;
	}

	if(resp->errorCode == Error_Busy) {
		gotoStateUploadEndBusy();
		return;
	} else if(resp->errorCode != Error_OK) {
		procError(resp->errorCode);
		return;
	}

	state = State_Ready;
	courier.deliver(Client::Event_UploadOK);
}

void ClientCommandLayer::gotoStateUploadEndBusy() {
	LOG_DEBUG(LOG_ECP, "gotoStateUploadEndBusy");
	timer->start(ECP_BUSY_TIMEOUT);
	state = State_UploadEndBusy;
}

void ClientCommandLayer::stateUploadEndBusyTimeout() {
	LOG_DEBUG(LOG_ECP, "stateUploadEndBusyTimeout");
	Request *req = (Request*)request.getData();
	req->command = Command_UploadEnd;
	request.setLen(sizeof(*req));
	packetLayer->sendData(&request);
	state = State_UploadEnd;
}

void ClientCommandLayer::gotoStateDownloadStart() {
	LOG_DEBUG(LOG_ECP, "gotoStateDownloadStart");
	DownloadStartRequest *req = (DownloadStartRequest*)request.getData();
	req->command = Command_DownloadStart;
	req->source = source;
	request.setLen(sizeof(*req));
	packetLayer->sendData(&request);
	state = State_DownloadStart;
}

void ClientCommandLayer::stateDownloadStartRecv(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateDownloadStartRecv");
	DownloadStartResponse *resp = (DownloadStartResponse*)data;
	if(sizeof(*resp) > dataLen) {
		procError(Error_WrongPacketSize);
		return;
	}

	if(resp->errorCode != Error_OK) {
		procError(resp->errorCode);
		return;
	}

	parser->start(resp->dataSize);
	gotoStateDownloadData();
}

void ClientCommandLayer::gotoStateDownloadData() {
	LOG_DEBUG(LOG_ECP, "gotoStateDownloadData");
	Request *req = (Request*)request.getData();
	req->command = Command_DownloadData;
	request.setLen(sizeof(*req));
	packetLayer->sendData(&request);
	state = State_DownloadData;
}

void ClientCommandLayer::stateDownloadDataRecv(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateDownloadDataRecv");
	DownloadDataResponse *resp = (DownloadDataResponse*)data;
	if(sizeof(*resp) > dataLen) {
		procError(Error_WrongPacketSize);
		return;
	}

	parser->procData(resp->data, dataLen - sizeof(*resp));
	if(resp->errorCode == Error_EndOfFile) {
		parser->complete();
		state = State_Ready;
		courier.deliver(Client::Event_DownloadOK);
		return;
	} else if(resp->errorCode != Error_OK) {
		procError(resp->errorCode);
		return;
	}

	Request *req = (Request*)request.getData();
	req->command = Command_DownloadData;
	request.setLen(sizeof(*req));
	packetLayer->sendData(&request);
}

void ClientCommandLayer::stateRequestRecv(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateRequestRecv");
	TableEntryResponse *resp = (TableEntryResponse*)data;
	if(resp->errorCode != Error_OK) {
		procError(resp->errorCode);
		return;
	}

	state = State_Ready;
	eventResponse.data.clear();
	eventResponse.data.add(resp->data, dataLen - sizeof(*resp));
	courier.deliver(&eventResponse);
}

}
