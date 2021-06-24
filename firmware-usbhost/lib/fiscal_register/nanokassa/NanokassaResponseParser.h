#ifndef COMMON_NANOKASSA_RESPONSEPARSER_H
#define COMMON_NANOKASSA_RESPONSEPARSER_H

#include "common/http/HttpResponseParser.h"
#include "common/utils/include/Json.h"
#include "common/fiscal_register/include/FiscalRegister.h"

namespace Nanokassa {

class CheckResponseParser {
public:
	enum Result {
		Result_Ok = 0,
		Result_Wait,
		Result_Error,
	};

	CheckResponseParser(EventDeviceId deviceId, StringBuilder *buf);
	void start();
	void parse(uint16_t dataLen);
	uint8_t *getBuf();
	uint32_t getBufSize();
	Result getResult();
	Fiscal::EventError *getError();

private:
	Result result;
	Http::Response response;
	Http::ResponseParser parser;
	JsonParser jsonParser;
	Fiscal::EventError error;

	void procResponse200();
	void procResponseOther();
	void procResponseBroken();
	void procError(ConfigEvent::Code type);
};

class PollResponseParser {
public:
	enum Result {
		Result_Ok = 0,
		Result_Busy,
		Result_Wait,
		Result_Error,
	};

	PollResponseParser(EventDeviceId deviceId, StringBuilder *buf);
	void start();
	void parse(uint16_t dataLen);
	uint8_t *getBuf();
	uint32_t getBufSize();
	Result getResult();
	DateTime *getFiscalDatetime();
	uint64_t getFiscalRegister();
	uint64_t getFiscalStorage();
	uint32_t getFiscalDocument();
	uint32_t getFiscalSign();
	Fiscal::EventError *getError();

private:
	Result result;
	Http::Response response;
	Http::ResponseParser parser;
	JsonParser jsonParser;
	DateTime fiscalDatetime; // время регистрации продажи
	uint64_t fiscalRegister; // ФР (номер фискального регистратора)
	uint64_t fiscalStorage;  // ФН (номер фискального накопителя)
	uint32_t fiscalDocument; // ФД (номер фискального документа)
	uint32_t fiscalSign;     // ФП или ФПД (фискальный признак документа)
	Fiscal::EventError error;

	void procResponse200();
	void parseQrCode(const char *qrCode, uint16_t qrCodeLen);
	void procResponseOther();
	void procResponseBroken();
	void procError(ConfigEvent::Code type);
};

}

#endif
