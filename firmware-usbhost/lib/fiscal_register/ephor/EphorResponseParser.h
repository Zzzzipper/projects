#if 1
#ifndef COMMON_EPHOR_RESPONSEPARSER_H
#define COMMON_EPHOR_RESPONSEPARSER_H

#include "common/http/HttpResponseParser.h"
#include "common/utils/include/Json.h"
#include "common/fiscal_register/include/FiscalRegister.h"

namespace Ephor {

class CheckResponseParser {
public:
	enum Result {
		Result_Ok = 0,
		Result_Busy,
		Result_Wait,
		Result_Error,
	};

	CheckResponseParser(EventDeviceId deviceId, Http::Response *resp);
	void parse();
	Result getResult();
	Fiscal::EventError *getError();
	uint32_t getId();
	DateTime *getFiscalDatetime();
	uint64_t getFiscalRegister();
	uint64_t getFiscalStorage();
	uint32_t getFiscalDocument();
	uint32_t getFiscalSign();

private:
	Result result;
	Http::Response *resp;
	JsonParser jsonParser;
	uint32_t id;
	uint16_t status;
	DateTime fiscalDatetime; // время регистрации продажи
	uint64_t fiscalRegister; // ФР (номер фискального регистратора)
	uint64_t fiscalStorage;  // ФН (номер фискального накопителя)
	uint32_t fiscalDocument; // ФД (номер фискального документа)
	uint32_t fiscalSign;     // ФП или ФПД (фискальный признак документа)
	Fiscal::EventError error;

	void procResponse200();
	void procResponseOther();
	void procResponseBroken();
	void procError(ConfigEvent::Code type);
};

}

#endif
#else
#ifndef COMMON_EPHOR_RESPONSEPARSER_H
#define COMMON_EPHOR_RESPONSEPARSER_H

#include "common/http/HttpResponseParser.h"
#include "common/utils/include/Json.h"
#include "common/fiscal_register/include/FiscalRegister.h"

namespace Ephor {

class CheckResponseParser {
public:
	enum Result {
		Result_Ok = 0,
		Result_Busy,
		Result_Wait,
		Result_Error,
	};

	CheckResponseParser(EventDeviceId deviceId, StringBuilder *buf);
	void parse(Http::Response *resp);

	void start();
	void parse(uint16_t dataLen);
	uint8_t *getBuf();
	uint32_t getBufSize();
	Result getResult();
	Fiscal::EventError *getError();
	uint32_t getId();
	uint16_t getStatus();
	DateTime *getFiscalDatetime();
	uint64_t getFiscalRegister();
	uint64_t getFiscalStorage();
	uint32_t getFiscalDocument();
	uint32_t getFiscalSign();

private:
	Result result;
	Http::Response response;
	Http::ResponseParser parser;
	JsonParser jsonParser;
	uint32_t id;
	uint16_t status;
	DateTime fiscalDatetime; // время регистрации продажи
	uint64_t fiscalRegister; // ФР (номер фискального регистратора)
	uint64_t fiscalStorage;  // ФН (номер фискального накопителя)
	uint32_t fiscalDocument; // ФД (номер фискального документа)
	uint32_t fiscalSign;     // ФП или ФПД (фискальный признак документа)
	Fiscal::EventError error;

	void procResponse200();
	void procResponseOther();
	void procResponseBroken();
	void procError(ConfigEvent::Code type);
};

}

#endif
#endif
