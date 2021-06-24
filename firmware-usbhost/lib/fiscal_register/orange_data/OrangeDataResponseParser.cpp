#include "lib/fiscal_register/orange_data/OrangeDataResponseParser.h"

#include "common/utils/include/Json.h"
#include "common/utils/include/Number.h"
#include "common/utils/include/CodePage.h"
#include "common/logger/include/Logger.h"

#define CHECK_JSON_NODE_MAX 20
#define POLL_JSON_NODE_MAX 80

namespace OrangeData {

CheckResponseParser::CheckResponseParser(uint16_t deviceId, StringBuilder *buf) :
	jsonParser(CHECK_JSON_NODE_MAX),
	error(deviceId)
{
	this->response.data = buf;
}

void CheckResponseParser::start() {
	parser.start(&response);
	result = Result_Wait;
}

void CheckResponseParser::parse(uint16_t dataLen) {
	LOG_DEBUG(LOG_FR, "parse " << dataLen);
	parser.parseData(dataLen);
	parser.parseData(0);
	if(parser.isComplete() == false) {
		procResponseBroken();
		return;
	}

	LOG_DEBUG_STR(LOG_FR, (char*)response.data->getData(), response.data->getLen());
	switch(response.statusCode) {
	case Http::Response::Status_Created: procResponse201(); return;
	case Http::Response::Status_BadRequest: procResponse400(); return;
	case Http::Response::Status_Unauthorized: procResponse401(); return;
	case Http::Response::Status_Conflict: procResponse409(); return;
	case Http::Response::Status_ServiceUnavailable: procResponse503(); return;
	default: procResponseOther(); return;
	}
}

uint8_t *CheckResponseParser::getBuf() {
	return parser.getBuf();
}

uint32_t CheckResponseParser::getBufSize() {
	return parser.getBufSize();
}

CheckResponseParser::Result CheckResponseParser::getResult() {
	return result;
}

Fiscal::EventError *CheckResponseParser::getError() {
	return &error;
}

void CheckResponseParser::procResponse201() {
	LOG_DEBUG(LOG_FR, "procResponse201");
	result = Result_Ok;
}

void CheckResponseParser::procResponse400() {
	LOG_DEBUG(LOG_FR, "procResponse400");
	LOG_DEBUG_STR(LOG_FR, (char*)response.data->getData(), response.data->getLen());
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
	JsonNode *nodeErrors = nodeRoot->getField("errors", JsonNode::Type_Array);
	if(nodeErrors == NULL) {
		LOG_ERROR(LOG_FR, "Wrong json format");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}
	JsonNode *nodeError = nodeErrors->getChildByIndex(0);
	if(nodeError == NULL) {
		LOG_ERROR(LOG_FR, "Wrong json format");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	error.code = ConfigEvent::Type_FiscalUnknownError;
	error.data.clear();
	error.data << response.statusCode << "*"; convertUtf8ToWin1251((uint8_t*)nodeError->getValue(), nodeError->getValueLen(), &error.data);
	result = Result_Error;
}

void CheckResponseParser::procResponse401() {
	error.code = ConfigEvent::Type_FiscalUnknownError;
	error.data.clear();
	error.data << response.statusCode;
	result = Result_Error;
}

void CheckResponseParser::procResponse409() {
	LOG_DEBUG(LOG_FR, "procResponse409");
	result = Result_Ok;
}

void CheckResponseParser::procResponse503() {
	error.code = ConfigEvent::Type_FiscalUnknownError;
	error.data.clear();
	error.data << response.statusCode;
	result = Result_Error;
}

void CheckResponseParser::procResponseOther() {
	error.code = ConfigEvent::Type_FiscalUnknownError;
	error.data.clear();
	error.data << response.statusCode;
	result = Result_Error;
}

void CheckResponseParser::procResponseBroken() {
	LOG_DEBUG(LOG_FR, "procResponseBroken");
	procError(ConfigEvent::Type_BrokenResponse);
}

void CheckResponseParser::procError(ConfigEvent::Code type) {
	error.code = type;
	error.data.clear();
	result = Result_Error;
}

PollResponseParser::PollResponseParser(uint16_t deviceId, StringBuilder *buf) :
	jsonParser(POLL_JSON_NODE_MAX),
	error(deviceId)
{
	this->response.data = buf;
}

void PollResponseParser::start() {
	parser.start(&response);
	fiscalRegister = 0;
	fiscalStorage = 0;
	fiscalDocument = 0;
	fiscalSign = 0;
	result = Result_Wait;
}

void PollResponseParser::parse(uint16_t dataLen) {
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
	case Http::Response::Status_Accepted: procResponse202(); return;
	case Http::Response::Status_BadRequest: procResponse400(); return;
	case Http::Response::Status_Unauthorized: procResponse401(); return;
	default: procResponseOther(); return;
	}
}

uint8_t *PollResponseParser::getBuf() {
	return parser.getBuf();
}

uint32_t PollResponseParser::getBufSize() {
	return parser.getBufSize();
}

PollResponseParser::Result PollResponseParser::getResult() {
	return result;
}

uint64_t PollResponseParser::getFiscalRegister() {
	return fiscalRegister;
}

uint64_t PollResponseParser::getFiscalStorage() {
	return fiscalStorage;
}

uint32_t PollResponseParser::getFiscalDocument() {
	return fiscalDocument;
}

uint32_t PollResponseParser::getFiscalSign() {
	return fiscalSign;
}

DateTime *PollResponseParser::getFiscalDatetime() {
	return &fiscalDatetime;
}

Fiscal::EventError *PollResponseParser::getError() {
	return &error;
}

void PollResponseParser::procResponse200() {
	LOG_DEBUG(LOG_FR, "procResponse200");
	LOG_DEBUG_STR(LOG_FR, (char*)response.data->getData(), response.data->getLen());
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

	if(nodeRoot->getNumberField("deviceRN", &fiscalRegister) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'deviceRN' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}
	if(nodeRoot->getNumberField("fsNumber", &fiscalStorage) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'fsNumber' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}
	if(nodeRoot->getNumberField("documentNumber", &fiscalDocument) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'documentNumber' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}
	if(nodeRoot->getNumberField("fp", &fiscalSign) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'fp' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}
	if(nodeRoot->getDateTimeField("processedAt", &fiscalDatetime) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'processedAt' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	result = Result_Ok;
}

void PollResponseParser::procResponse202() {
	LOG_DEBUG(LOG_FR, "procResponse202");
	fiscalRegister = 0;
	fiscalStorage = 0;
	fiscalDocument = 0;
	fiscalSign = 0;
	result = Result_Busy;
}

void PollResponseParser::procResponse400() {
	error.code = ConfigEvent::Type_FiscalUnknownError;
	error.data.clear();
	error.data << response.statusCode;
	result = Result_Error;
}

void PollResponseParser::procResponse401() {
	error.code = ConfigEvent::Type_FiscalUnknownError;
	error.data.clear();
	error.data << response.statusCode;
	result = Result_Error;
}

void PollResponseParser::procResponseOther() {
	error.code = ConfigEvent::Type_FiscalUnknownError;
	error.data.clear();
	error.data << response.statusCode;
	result = Result_Error;
}

void PollResponseParser::procResponseBroken() {
	procError(ConfigEvent::Type_BrokenResponse);
}

void PollResponseParser::procError(ConfigEvent::Code type) {
	error.code = type;
	error.data.clear();
	result = Result_Error;
}

}
