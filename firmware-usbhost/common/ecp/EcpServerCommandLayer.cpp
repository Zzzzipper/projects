#include "EcpServerCommandLayer.h"
#include "EcpProtocol.h"
#include "logger/include/Logger.h"

namespace Ecp {

ServerCommandLayer::ServerCommandLayer(TimerEngine *timers, ServerPacketLayerInterface *packetLayer) :
	packetLayer(packetLayer),
	timers(timers),
	state(State_Idle),
	modemFirmwareParser(NULL),
	gsmFirmwareParser(NULL),
	screenFirmwareParser(NULL),
	configParser(NULL),
	parser(NULL),
	configGenerator(NULL),
	generator(NULL),
	processor(NULL),
	configEraser(NULL),
	packet(ECP_PACKET_MAX_SIZE)
{
	this->packetLayer->setObserver(this);
}

ServerCommandLayer::~ServerCommandLayer() {
}

void ServerCommandLayer::setObserver(EventObserver *observer) {
	courier.setRecipient(observer);
}

void ServerCommandLayer::setFirmwareParser(Dex::DataParser *parser) {
	this->modemFirmwareParser = parser;
	this->modemFirmwareParser->setObserver(this);
}

void ServerCommandLayer::setGsmParser(Dex::DataParser *parser) {
	this->gsmFirmwareParser = parser;
	this->gsmFirmwareParser->setObserver(this);
}

void ServerCommandLayer::setScreenParser(Dex::DataParser *parser) {
	this->screenFirmwareParser = parser;
	this->screenFirmwareParser->setObserver(this);
}

void ServerCommandLayer::setConfigParser(Dex::DataParser *parser) {
	this->configParser = parser;
	this->configParser->setObserver(this);
}

void ServerCommandLayer::setConfigGenerator(Dex::DataGenerator *generator) {
	this->configGenerator = generator;
}

void ServerCommandLayer::setConfigEraser(Dex::DataParser *eraser) {
	this->configEraser = eraser;
	this->configEraser->setObserver(this);
}

void ServerCommandLayer::setTableProcessor(TableProcessor *processor) {
	this->processor = processor;
}

void ServerCommandLayer::reset() {
	LOG_INFO(LOG_ECP, "reset");
	packetLayer->reset();
	state = State_Wait;
}

void ServerCommandLayer::shutdown() {
	LOG_INFO(LOG_ECP, "shutdown");
	packetLayer->shutdown();
	state = State_Idle;
}

void ServerCommandLayer::disconnect() {
	LOG_INFO(LOG_ECP, "disconnect");
	if(state == State_Idle) {
		courier.deliver(Server::Event_Disconnect);
		return;
	}
	packetLayer->disconnect();
	state = State_Disconnecting;
}

void ServerCommandLayer::procConnect() {
	LOG_DEBUG(LOG_ECP, "procConnect");
	state = State_Ready;
	courier.deliver(Server::Event_Connect);
}

void ServerCommandLayer::procRecvData(const uint8_t *data, uint16_t dataLen) {
	switch(state) {
	case State_Ready: stateReadyRecv(data, dataLen); return;
	case State_Recv: stateRecvRecv(data, dataLen); return;
	case State_Send: stateSendRecv(data, dataLen); return;
	default: LOG_ERROR(LOG_ECP, "Unwaited data " << state << "," << dataLen);
	}
}

void ServerCommandLayer::procError(uint8_t error) {
	LOG_INFO(LOG_ECP, "procError " << error);
	if(parser != NULL) { parser->error(); }
	state = State_Ready;
}

void ServerCommandLayer::procDisconnect() {
	LOG_INFO(LOG_ECP, "procDisconnect");
	if(parser != NULL) { parser->error(); }
	state = State_Wait;
	courier.deliver(Server::Event_Disconnect);
}

void ServerCommandLayer::proc(Event *event) {
	LOG_DEBUG(LOG_ECP, "proc");
	switch(state) {
	case State_RecvAsync: stateRecvAsyncEvent(event); break;
	case State_RecvEnd:  stateRecvEndEvent(event); break;
	default: LOG_ERROR(LOG_ECP, "Unwaited event " << state << "," << event->getType());
	}
}

void ServerCommandLayer::sendResponse(uint8_t command, Error errorCode) {
	Response *resp = (Response*)packet.getData();
	resp->command = command;
	resp->errorCode = errorCode;
	packet.setLen(sizeof(*resp));
	packetLayer->sendData(&packet);
}

void ServerCommandLayer::stateReadyRecv(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateReadyRecv");
	Request *req = (Request*)data;
	switch(req->command) {
	case Command_Setup: stateReadyCommandSetup(); return;
	case Command_UploadStart: stateReadyCommandUploadStart(data, dataLen); return;
	case Command_DownloadStart: stateReadyCommandDownloadStart(data, dataLen); return;
	case Command_TableInfo: stateReadyCommandTableInfo(data, dataLen); return;
	case Command_TableEntry: stateReadyCommandTableEntry(data, dataLen); return;
	case Command_DateTime: stateReadyCommandDateTime(data, dataLen); return;
	case Command_ConfigReset: stateReadyCommandConfigReset(data, dataLen); return;
	default: LOG_ERROR(LOG_ECP, "Unwaited command " << state << "," << req->command);
	}
}

void ServerCommandLayer::stateReadyCommandSetup() {
	LOG_DEBUG(LOG_ECP, "stateReadyCommandSetup");
	sendResponse(Command_Setup, Error_OK);
}

void ServerCommandLayer::stateReadyCommandUploadStart(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateReadyCommandUploadStart");
	UploadStartRequest *req = (UploadStartRequest*)data;
	if(sizeof(*req) != dataLen) {
		LOG_ERROR(LOG_ECP, "Wrong packet size " << (uint32_t)sizeof(*req) << "<>" << dataLen);
		sendResponse(Command_UploadStart, Error_WrongPacketSize);
		return;
	}

	if(req->destination == Destination_FirmwareModem) {
		if(modemFirmwareParser == NULL) {
			LOG_ERROR(LOG_ECP, "Modem firmware parser not defined");
			sendResponse(Command_UploadStart, Error_ServerError);
			return;
		}
		LOG_INFO(LOG_ECP, "Start upload modem firmware");
		parser = modemFirmwareParser;
	} else if(req->destination == Destination_FirmwareGsm) {
		if(gsmFirmwareParser == NULL) {
			LOG_ERROR(LOG_ECP, "GSM firmware parser not defined");
			sendResponse(Command_UploadStart, Error_ServerError);
			return;
		}
		LOG_INFO(LOG_ECP, "Start upload GSM firmware");
		parser = gsmFirmwareParser;
	} else if(req->destination == Destination_Config) {
		if(configParser == NULL) {
			LOG_ERROR(LOG_ECP, "Config parser not defined");
			sendResponse(Command_UploadStart, Error_ServerError);
			return;
		}
		LOG_INFO(LOG_ECP, "Start upload config");
		parser = configParser;
	} else if(req->destination == Destination_FirmwareScreen) {
		if(screenFirmwareParser == NULL) {
			LOG_ERROR(LOG_ECP, "GSM firmware parser not defined");
			sendResponse(Command_UploadStart, Error_ServerError);
			return;
		}
		LOG_INFO(LOG_ECP, "Start upload GSM firmware");
		parser = screenFirmwareParser;
	} else {
		LOG_ERROR(LOG_ECP, "Wrong destination value " << req->destination);
		sendResponse(Command_UploadStart, Error_WrongDestination);
		return;
	}

	Dex::DataParser::Result result = parser->start(req->dataSize);
	if(result == Dex::DataParser::Result_Ok) {
		sendResponse(Command_UploadStart, Error_OK);
		state = State_Recv;
	} else {
		sendResponse(Command_UploadStart, Error_ServerError);
		state = State_Ready;
	}
}

void ServerCommandLayer::stateReadyCommandDownloadStart(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateReadyCommandDownloadStart");
	DownloadStartRequest *req = (DownloadStartRequest*)data;
	if(sizeof(*req) != dataLen) {
		LOG_ERROR(LOG_ECP, "Wrong packet size " << (uint32_t)sizeof(*req) << "<>" << dataLen);
		sendResponse(req->command, Error_WrongPacketSize);
		return;
	}

	if(req->source == Source_Config) {
		if(configGenerator == NULL) {
			LOG_ERROR(LOG_ECP, "Config generator not defined");
			sendResponse(req->command, Error_ServerError);
			return;
		}
		LOG_INFO(LOG_ECP, "Start download config");
		generator = configGenerator;
	} else {
		LOG_ERROR(LOG_ECP, "Wrong source value " << req->source);
		sendResponse(req->command, Error_WrongSource);
		return;
	}

	generator->reset();
	DownloadStartResponse *resp = (DownloadStartResponse*)packet.getData();
	resp->command = req->command;
	resp->errorCode = Error_OK;
	resp->dataSize = generator->getSize();
	packet.setLen(sizeof(*resp));
	packetLayer->sendData(&packet);
	state = State_Send;
}

void ServerCommandLayer::stateReadyCommandTableInfo(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateReadyCommandTableInfo");
	TableInfoRequest *req = (TableInfoRequest*)data;
	if(processor == NULL || processor->isTableExist(req->tableId) == false) {
		sendResponse(req->command, Error_TableNotFound);
		return;
	}

	TableInfoResponse *resp = (TableInfoResponse*)packet.getData();
	resp->command = req->command;
	resp->errorCode = Error_OK;
	resp->size = processor->getTableSize(req->tableId);
	packet.setLen(sizeof(*resp));
	packetLayer->sendData(&packet);
}

void ServerCommandLayer::stateReadyCommandTableEntry(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateReadyCommandTableEntry");
	TableEntryRequest *req = (TableEntryRequest*)data;
	if(processor == NULL || processor->isTableExist(req->tableId) == false) {
		sendResponse(req->command, Error_TableNotFound);
		return;
	}

	TableEntryResponse *resp = (TableEntryResponse*)packet.getData();
	resp->command = req->command;
	resp->errorCode = Error_OK;
	uint16_t entryLen = processor->getTableEntry(req->tableId, req->entryIndex, resp->data, packet.getSize() - sizeof(*resp));
	if(entryLen == 0) {
		sendResponse(req->command, Error_EntryNotFound);
		return;
	}

	LOG_DEBUG_HEX(LOG_ECP, resp->data, entryLen);
	packet.setLen(sizeof(*resp) + entryLen);
	packetLayer->sendData(&packet);
}

void ServerCommandLayer::stateReadyCommandDateTime(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateReadyCommandDateTime");
	Request *req = (Request*)data;
	if(processor == NULL) {
		sendResponse(req->command, Error_TableNotFound);
		return;
	}

	TableEntryResponse *resp = (TableEntryResponse*)packet.getData();
	resp->command = req->command;
	resp->errorCode = Error_OK;
	uint16_t respLen = processor->getDateTime(resp->data, packet.getSize() - sizeof(*resp));
	if(respLen == 0) {
		sendResponse(req->command, Error_EntryNotFound);
		return;
	}

	LOG_DEBUG_HEX(LOG_ECP, resp->data, respLen);
	packet.setLen(sizeof(*resp) + respLen);
	packetLayer->sendData(&packet);
}

void ServerCommandLayer::stateReadyCommandConfigReset(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateReadyCommandConfigReset");
	Request *req = (Request*)data;
	if(configEraser == NULL) {
		sendResponse(req->command, Error_ServerError);
		return;
	}

	configEraser->start(0);
	sendResponse(Command_UploadStart, Error_OK);
	state = State_Ready;
}

void ServerCommandLayer::stateRecvRecv(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateRecvRecv");
	Request *req = (Request*)data;
	switch(req->command) {
	case Command_UploadData: stateRecvCommandUploadData(data, dataLen); return;
	case Command_UploadEnd: stateRecvCommandUploadEnd(); return;
	default: LOG_ERROR(LOG_ECP, "Unwaited command " << state << "," << req->command);
	}
}

void ServerCommandLayer::stateRecvCommandUploadData(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateRecvCommandUploadData");
	UploadDataRequest *req = (UploadDataRequest*)data;
	Dex::DataParser::Result result = parser->procData(req->data, dataLen - sizeof(*req));
	if(result == Dex::DataParser::Result_Ok) {
		sendResponse(Command_UploadData, Error_OK);
	} else if(result == Dex::DataParser::Result_Busy) {
		sendResponse(Command_UploadData, Error_Busy);
	} else if(result == Dex::DataParser::Result_Error) {
		sendResponse(Command_UploadData, Error_ServerError);
	} else if(result == Dex::DataParser::Result_Async) {
		gotoStateRecvAsync();
	}
}

void ServerCommandLayer::gotoStateRecvAsync() {
	LOG_DEBUG(LOG_ECP, "gotoStateRecvAsync");
	state = State_RecvAsync;
}

void ServerCommandLayer::stateRecvAsyncEvent(Event *event) {
	LOG_DEBUG(LOG_ECP, "stateRecvAsyncEvent");
	if(event->getType() == Dex::DataParser::Event_AsyncOk) {
		sendResponse(Command_UploadData, Error_OK);
		state = State_Recv;
	} else if(event->getType() == Dex::DataParser::Event_AsyncError) {
		sendResponse(Command_UploadData, Error_ServerError);
		state = State_Recv;
	} else {
		LOG_ERROR(LOG_ECP, "Unwaited event " << state << "," << event->getType());
	}
}

void ServerCommandLayer::stateRecvCommandUploadEnd() {
	LOG_DEBUG(LOG_ECP, "stateRecvCommandUploadEnd");
	Dex::DataParser::Result result = parser->complete();
	if(result == Dex::DataParser::Result_Ok) {
		sendResponse(Command_UploadEnd, Error_OK);
		parser = NULL;
		state = State_Ready;
	} else if(result == Dex::DataParser::Result_Busy) {
		sendResponse(Command_UploadEnd, Error_Busy);
	} else if(result == Dex::DataParser::Result_Error) {
		sendResponse(Command_UploadEnd, Error_ServerError);
		parser = NULL;
		state = State_Ready;
	} else if(result == Dex::DataParser::Result_Async) {
		parser = NULL;
		gotoStateRecvEnd();
	}
}

void ServerCommandLayer::gotoStateRecvEnd() {
	LOG_DEBUG(LOG_ECP, "gotoStateRecvEnd");
	state = State_RecvEnd;
}

void ServerCommandLayer::stateRecvEndEvent(Event *event) {
	LOG_DEBUG(LOG_ECP, "stateRecvEndEvent");
	if(event->getType() == Dex::DataParser::Event_AsyncOk) {
		sendResponse(Command_UploadEnd, Error_OK);
		state = State_Ready;
	} else if(event->getType() == Dex::DataParser::Event_AsyncError) {
		sendResponse(Command_UploadEnd, Error_ServerError);
		state = State_Ready;
	} else {
		LOG_ERROR(LOG_ECP, "Unwaited event " << state << "," << event->getType());
	}
}

void ServerCommandLayer::stateSendRecv(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateSendRecv");
	Request *req = (Request*)data;
	switch(req->command) {
	case Command_DownloadData: stateSendCommandDownloadData(data, dataLen); return;
	default: LOG_ERROR(LOG_ECP, "Unwaited command " << state << "," << req->command);
	}
}

void ServerCommandLayer::stateSendCommandDownloadData(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ECP, "stateSendCommandDownloadData");
	Request *req = (Request*)data;

	DownloadDataResponse *resp = (DownloadDataResponse*)packet.getData();
	resp->command = req->command;
	uint8_t *block = (uint8_t*)generator->getData();
	uint16_t blockSize = generator->getLen();
	for(uint16_t i = 0; i < blockSize; i++) {
		resp->data[i] = block[i];
	}

	if(generator->isLast() == false) {
		resp->errorCode = Error_OK;
		generator->next();
	} else {
		resp->errorCode = Error_EndOfFile;
		state = State_Ready;
	}

	LOG_DEBUG_HEX(LOG_ECP, block, blockSize);
	packet.setLen(sizeof(*resp) + blockSize);
	packetLayer->sendData(&packet);
}

}
