#include "lib/fiscal_register/nanokassa/NanokassaResponseParser.h"

#include "common/utils/include/Json.h"
#include "common/utils/include/Number.h"
#include "common/utils/include/StringParser.h"
#include "common/logger/include/Logger.h"

#define CHECK_JSON_NODE_MAX 20
#define POLL_JSON_NODE_MAX 80

namespace Nanokassa {

CheckResponseParser::CheckResponseParser(EventDeviceId deviceId, StringBuilder *buf) :
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

void CheckResponseParser::procResponse200() {
	LOG_DEBUG(LOG_FR, "procResponse200");
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

PollResponseParser::PollResponseParser(EventDeviceId deviceId, StringBuilder *buf) :
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
#if 0
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

	JsonNode *fieldQr = nodeRoot->getChild("check_qr_code");
	if(fieldQr == NULL) {
		LOG_ERROR(LOG_FR, "Field 'check_qr_code' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	JsonNode *fieldQrValue = fieldQr->getChild();
	if(fieldQrValue == NULL) {
		LOG_ERROR(LOG_FR, "Field 'check_qr_code' wrong value");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(fieldQrValue->getType() == JsonNode::Type_NULL) {
		result = Result_Busy;
		return;
	}

	LOG_INFO(LOG_FR, ">>>>>>>>>>>>" << fieldQrValue->getValue());
	parseQrCode(fieldQrValue->getValue(), fieldQrValue->getValueLen());
	result = Result_Ok;
#else
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

	JsonNode *fieldQr = nodeRoot->getChild("qr");
	if(fieldQr == NULL) {
		LOG_ERROR(LOG_FR, "Field 'qr' not found");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	JsonNode *fieldQrValue = fieldQr->getChild();
	if(fieldQrValue == NULL) {
		LOG_ERROR(LOG_FR, "Field 'qr' wrong value");
		procError(ConfigEvent::Type_WrongResponse);
		return;
	}

	if(fieldQrValue->getType() == JsonNode::Type_NULL) {
		result = Result_Busy;
		return;
	}

	LOG_INFO(LOG_FR, ">>>>>>>>>>>>" << fieldQrValue->getValue());
	parseQrCode(fieldQrValue->getValue(), fieldQrValue->getValueLen());
	result = Result_Ok;
#endif
}

//t=20180720T1638&s=50.00&fn=9999078900005419&i=1866&fp=3326875305&n=2
void PollResponseParser::parseQrCode(const char *qrCode, uint16_t qrCodeLen) {
	fiscalRegister = 0;
	fiscalStorage = 9999999999999999;
	fiscalDocument = 12345;
	fiscalSign = 3333333333;

	StringParser parser(qrCode, qrCodeLen);
	if(parser.compareAndSkip("t=") == false) {
		LOG_ERROR(LOG_FR, "Wrong result format");
		return;
	}
	if(fiscal2datetime(parser.unparsed(), parser.unparsedLen(), &fiscalDatetime) <= 0) {
		LOG_ERROR(LOG_FR, "Wrong result format");
		return;
	}
	parser.skipNotEqual("&");

	if(parser.compareAndSkip("&s=") == false) {
		LOG_ERROR(LOG_FR, "Wrong result format");
		return;
	}
	parser.skipNotEqual("&");

	if(parser.compareAndSkip("&fn=") == false) {
		LOG_ERROR(LOG_FR, "Wrong result format");
		return;
	}
	if(parser.getNumber(&fiscalStorage) == false) {
		LOG_ERROR(LOG_FR, "Wrong result format");
		return;
	}

	if(parser.compareAndSkip("&i=") == false) {
		LOG_ERROR(LOG_FR, "Wrong result format");
		return;
	}
	if(parser.getNumber(&fiscalDocument) == false) {
		LOG_ERROR(LOG_FR, "Wrong result format");
		return;
	}

	if(parser.compareAndSkip("&fp=") == false) {
		LOG_ERROR(LOG_FR, "Wrong result format");
		return;
	}
	if(parser.getNumber(&fiscalSign) == false) {
		LOG_ERROR(LOG_FR, "Wrong result format");
		return;
	}
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
