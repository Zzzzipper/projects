#include "EphorOnlineResponseParser.h"

#include "common/utils/include/Json.h"
#include "common/utils/include/Number.h"
#include "common/logger/include/Logger.h"

#define JSON_NODE_MAX 100

namespace EphorOnline {

ResponseParser::ResponseParser(EventDeviceId deviceId, StringBuilder *buf) :
	jsonParser(JSON_NODE_MAX),
	error(deviceId)
{
	this->response.data = buf;
}

void ResponseParser::start() {
	parser.start(&response);
	fiscalRegister = 0;
	fiscalStorage = 0;
	fiscalDocument = 0;
	fiscalSign = 0;
	result = Result_Wait;
}

void ResponseParser::parse(uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "parse " << dataLen);
	parser.parseData(dataLen);
	parser.parseData(0);
	if(parser.isComplete() == false) {
		procResponseBroken();
		return;
	}

	LOG_DEBUG_STR(LOG_FR, (char*)response.data->getData(), response.data->getLen());
	switch(response.statusCode) {
	case Http::Response::Status_OK: procResponse200(); return;
	case Http::Response::Status_ServerError: procResponse500(); return;
	default: procResponseOther(); return;
	}
}

uint8_t *ResponseParser::getBuf() {
	return parser.getBuf();
}

uint32_t ResponseParser::getBufSize() {
	return parser.getBufSize();
}

ResponseParser::Result ResponseParser::getResult() {
	return result;
}

uint64_t ResponseParser::getFiscalRegister() {
	return fiscalRegister;
}

uint64_t ResponseParser::getFiscalStorage() {
	return fiscalStorage;
}

uint32_t ResponseParser::getFiscalDocument() {
	return fiscalDocument;
}

uint32_t ResponseParser::getFiscalSign() {
	return fiscalSign;
}

Fiscal::EventError *ResponseParser::getError() {
	return &error;
}

void ResponseParser::procResponse200() {
	LOG_DEBUG(LOG_FR, "procResponse200");
	if(jsonParser.parse((char*)response.data->getData(), response.data->getLen()) == false) {
		LOG_ERROR(LOG_FR, "Json parse failed");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	JsonNode *nodeRoot = jsonParser.getRoot();
	if(nodeRoot == NULL || nodeRoot->getType() != JsonNode::Type_Object) {
		LOG_ERROR(LOG_FR, "Wrong json format");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	JsonNode *nodeResponse = nodeRoot->getField("Response", JsonNode::Type_Object);
	if(nodeResponse == NULL) {
		LOG_ERROR(LOG_FR, "Wrong json format");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	uint32_t errorCode;
	if(nodeResponse->getNumberField<uint32_t>("Error", &errorCode) == NULL) {
		LOG_ERROR(LOG_FR, "Wrong json format");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(errorCode != 0) {
		LOG_ERROR(LOG_FR, "Error result " << errorCode);
		error.code = ConfigEvent::Type_FiscalUnknownError;
		error.data.clear();
		error.data << "200*" << errorCode;
		result = Result_Error;
		return;
	}

	if(nodeRoot->getNumberField<uint64_t>("DeviceRegistrationNumber", &fiscalRegister) == NULL) {
		LOG_ERROR(LOG_FR, "Wrong json format");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(nodeRoot->getNumberField<uint64_t>("FNSerialNumber", &fiscalStorage) == NULL) {
		LOG_ERROR(LOG_FR, "Wrong json format");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(nodeRoot->getNumberField<uint32_t>("FiscalDocNumber", &fiscalDocument) == NULL) {
		LOG_ERROR(LOG_FR, "Wrong json format");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(nodeRoot->getNumberField<uint32_t>("FiscalSign", &fiscalSign) == NULL) {
		LOG_ERROR(LOG_FR, "Wrong json format");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	result = Result_Ok;
}

void ResponseParser::procResponse500() {
	LOG_DEBUG(LOG_FR, "procResponse500");
	if(jsonParser.parse((char*)response.data->getData(), response.data->getLen()) == false) {
		LOG_ERROR(LOG_FR, "Json parse failed");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	JsonNode *nodeRoot = jsonParser.getRoot();
	if(nodeRoot == NULL || nodeRoot->getType() != JsonNode::Type_Object) {
		LOG_ERROR(LOG_FR, "Wrong json format");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	uint32_t errorCode;
	if(nodeRoot->getNumberField<uint32_t>("FCEError", &errorCode) == NULL) {
		LOG_ERROR(LOG_FR, "Wrong json format");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	JsonNode *nodeDescription = nodeRoot->getField("ErrorDescription", JsonNode::Type_String);
	if(nodeDescription == NULL) {
		LOG_ERROR(LOG_FR, "Wrong json format");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	LOG_ERROR(LOG_FR, "Error result " << errorCode);
	LOG_ERROR_STR(LOG_FR, nodeDescription->getValue(), nodeDescription->getValueLen());
	error.code = ConfigEvent::Type_FiscalUnknownError;
	error.data.clear();
	error.data << "500*" << errorCode;
	result = Result_Error;
}

void ResponseParser::procResponseOther() {
	error.code = ConfigEvent::Type_FiscalUnknownError;
	error.data.clear();
	error.data << response.statusCode;
	result = Result_Error;
}

void ResponseParser::procResponseBroken() {
	procError(ConfigEvent::Type_BrokenResponse);
}

void ResponseParser::procError(ConfigEvent::Code type) {
	error.code = type;
	error.data.clear();
	result = Result_Error;
}

}
