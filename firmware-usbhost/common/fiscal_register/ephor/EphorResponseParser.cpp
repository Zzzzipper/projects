#if 1
#include "fiscal_register/ephor/EphorResponseParser.h"

#include "utils/include/Json.h"
#include "utils/include/Number.h"
#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"

#define CHECK_JSON_NODE_MAX 20
#define POLL_JSON_NODE_MAX 80

namespace Ephor {

CheckResponseParser::CheckResponseParser(EventDeviceId deviceId, Http::Response *resp) :
	resp(resp),
	jsonParser(CHECK_JSON_NODE_MAX),
	error(deviceId)
{
}

void CheckResponseParser::parse() {
	LOG_DEBUG(LOG_FR, "parse " << resp->data->getLen());
	LOG_DEBUG_STR(LOG_FR, (char*)resp->data->getData(), resp->data->getLen());
	switch(resp->statusCode) {
	case Http::Response::Status_OK: procResponse200(); return;
	default: procResponseOther(); return;
	}
}

CheckResponseParser::Result CheckResponseParser::getResult() {
	return result;
}

Fiscal::EventError *CheckResponseParser::getError() {
	return &error;
}

uint32_t CheckResponseParser::getId() {
	return id;
}

DateTime *CheckResponseParser::getFiscalDatetime() {
	return &fiscalDatetime;
}

uint64_t CheckResponseParser::getFiscalRegister() {
	return fiscalRegister;
}

uint64_t CheckResponseParser::getFiscalStorage() {
	return fiscalStorage;
}

uint32_t CheckResponseParser::getFiscalDocument() {
	return fiscalDocument;
}

uint32_t CheckResponseParser::getFiscalSign() {
	return fiscalSign;
}

void CheckResponseParser::procResponse200() {
	LOG_DEBUG(LOG_FR, "procResponse200" << resp->data->getLen());
	LOG_DEBUG_STR(LOG_FR, (char*)resp->data->getData(), resp->data->getLen());
	if(jsonParser.parse((char*)resp->data->getData(), resp->data->getLen()) == false) {
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

	if(nodeRoot->getField("success", JsonNode::Type_TRUE) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'success' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(nodeRoot->getNumberField("id", &id) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'id' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(nodeRoot->getNumberField("status", &status) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'status' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(status == Fiscal::Status_Server) {
		LOG_INFO(LOG_FR, "Check not complete " << id);
		result = Result_Busy;
		return;
	}
	if(status != Fiscal::Status_Complete) {
		LOG_INFO(LOG_FR, "Wrong check status " << status);
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(nodeRoot->getDateTimeField("date", &fiscalDatetime) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'date' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	fiscalRegister = 0;

	if(nodeRoot->getNumberField("fn", &fiscalStorage) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'fn' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(nodeRoot->getNumberField("fd", &fiscalDocument) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'fd' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(nodeRoot->getNumberField("fp", &fiscalSign) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'fp' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	LOG_INFO(LOG_FR, ">>>>>>>>>>>>" << id << "," << status);
	result = Result_Ok;
}

void CheckResponseParser::procResponseOther() {
	error.code = ConfigEvent::Type_FiscalUnknownError;
	error.data.clear();
	error.data << resp->statusCode;
	result = Result_Error;
}

void CheckResponseParser::procResponseBroken() {
	procError(ConfigEvent::Type_BrokenResponse);
}

void CheckResponseParser::procError(ConfigEvent::Code type) {
	error.code = type;
	error.data.clear();
	result = Result_Error;
}

}
#else
#include "lib/fiscal_register/ephor/EphorResponseParser.h"

#include "common/utils/include/Json.h"
#include "common/utils/include/Number.h"
#include "common/utils/include/StringParser.h"
#include "common/logger/include/Logger.h"

#define CHECK_JSON_NODE_MAX 20
#define POLL_JSON_NODE_MAX 80

namespace Ephor {

CheckResponseParser::CheckResponseParser(EventDeviceId deviceId, StringBuilder *buf) :
	jsonParser(CHECK_JSON_NODE_MAX),
	error(deviceId)
{
	this->response.data = buf;
}

void CheckResponseParser::parse(Http::Response *resp) {
	LOG_DEBUG(LOG_FR, "parse " << dataLen);
	LOG_DEBUG_STR(LOG_FR, (char*)resp.data->getData(), resp.data->getLen());
	switch(resp.statusCode) {
	case Http::Response::Status_OK: procResponse200(); return;
	default: procResponseOther(); return;
	}
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
	case Http::Response::Status_OK: procResponse200(); return;
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

uint32_t CheckResponseParser::getId() {
	return id;
}

uint16_t CheckResponseParser::getStatus() {
	return status;
}

DateTime *CheckResponseParser::getFiscalDatetime() {
	return &fiscalDatetime;
}

uint64_t CheckResponseParser::getFiscalRegister() {
	return fiscalRegister;
}

uint64_t CheckResponseParser::getFiscalStorage() {
	return fiscalStorage;
}

uint32_t CheckResponseParser::getFiscalDocument() {
	return fiscalDocument;
}

uint32_t CheckResponseParser::getFiscalSign() {
	return fiscalSign;
}

void CheckResponseParser::procResponse200() {
	LOG_DEBUG(LOG_FR, "procResponse200" << response.data->getLen());
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

	if(nodeRoot->getField("success", JsonNode::Type_TRUE) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'success' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(nodeRoot->getNumberField("id", &id) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'id' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(nodeRoot->getNumberField("status", &status) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'status' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(status == Fiscal::Status_Server) {
		LOG_INFO(LOG_FR, "Check not complete " << id);
		result = Result_Busy;
		return;
	}
	if(status != Fiscal::Status_Complete) {
		LOG_INFO(LOG_FR, "Wrong check status " << status);
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(nodeRoot->getDateTimeField("date", &fiscalDatetime) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'id' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(nodeRoot->getNumberField("fn", &fiscalStorage) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'status' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(nodeRoot->getNumberField("fd", &fiscalDocument) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'status' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(nodeRoot->getNumberField("fp", &fiscalSign) == NULL) {
		LOG_ERROR(LOG_FR, "Field 'status' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	LOG_INFO(LOG_FR, ">>>>>>>>>>>>" << id << "," << status);
	result = Result_Ok;
}

void CheckResponseParser::procResponseOther() {
	error.code = ConfigEvent::Type_FiscalUnknownError;
	error.data.clear();
	error.data << response.statusCode;
	result = Result_Error;
}

void CheckResponseParser::procResponseBroken() {
	procError(ConfigEvent::Type_BrokenResponse);
}

void CheckResponseParser::procError(ConfigEvent::Code type) {
	error.code = type;
	error.data.clear();
	result = Result_Error;
}

}
#endif
